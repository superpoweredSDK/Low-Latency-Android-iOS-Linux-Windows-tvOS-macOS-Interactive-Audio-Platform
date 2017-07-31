#import "SuperpoweredIOSAudioIO.h"
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <MediaPlayer/MediaPlayer.h>
#import <pthread.h>

// Helpers
#define SILENCE_DEPRECATION(code)                                   \
{                                                                   \
_Pragma("clang diagnostic push")                                    \
_Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")   \
code;                                                               \
_Pragma("clang diagnostic pop")                                     \
}

#define USES_AUDIO_INPUT 1

typedef enum audioDeviceType {
    audioDeviceType_USB = 1, audioDeviceType_headphone = 2, audioDeviceType_HDMI = 3, audioDeviceType_other = 4
} audioDeviceType;

static audioDeviceType NSStringToAudioDeviceType(NSString *str) {
    if ([str isEqualToString:AVAudioSessionPortHeadphones]) return audioDeviceType_headphone;
    else if ([str isEqualToString:AVAudioSessionPortUSBAudio]) return audioDeviceType_USB;
    else if ([str isEqualToString:AVAudioSessionPortHDMI]) return audioDeviceType_HDMI;
    else return audioDeviceType_other;
}

// Initialization
@implementation SuperpoweredIOSAudioIO {
#if __has_feature(objc_arc)
    __weak
#endif
    id<SuperpoweredIOSAudioIODelegate>delegate;
    NSString *externalAudioDeviceName, *audioSessionCategory;
    NSTimer *stopTimer;
    NSMutableString *audioSystemInfo;
    audioProcessingCallback processingCallback;
    void *processingClientdata;
    AudioBufferList *inputBufferListForRecordingCategory;
    AudioComponentInstance audioUnit;
    multiOutputChannelMap outputChannelMap;
    multiInputChannelMap inputChannelMap;
    audioDeviceType RemoteIOOutputChannelMap[64];
    int numChannels, silenceFrames, samplerate, minimumNumberOfFrames, maximumNumberOfFrames, preferredMinimumSamplerate;
    bool audioUnitRunning, iOS6, background, inputEnabled;
}

@synthesize preferredBufferSizeMs, saveBatteryInBackground, started;

- (void)createAudioBuffersForRecordingCategory {
    inputBufferListForRecordingCategory = (AudioBufferList *)malloc(sizeof(AudioBufferList) + (sizeof(AudioBuffer) * numChannels));
    inputBufferListForRecordingCategory->mNumberBuffers = numChannels;
    for (int n = 0; n < numChannels; n++) {
        inputBufferListForRecordingCategory->mBuffers[n].mDataByteSize = 2048 * 4;
        inputBufferListForRecordingCategory->mBuffers[n].mNumberChannels = 1;
        inputBufferListForRecordingCategory->mBuffers[n].mData = malloc(inputBufferListForRecordingCategory->mBuffers[n].mDataByteSize);
    };
}

- (id)initWithDelegate:(NSObject<SuperpoweredIOSAudioIODelegate> *)d preferredBufferSize:(unsigned int)preferredBufferSize preferredMinimumSamplerate:(unsigned int)prefsamplerate audioSessionCategory:(NSString *)category channels:(int)channels audioProcessingCallback:(audioProcessingCallback)callback clientdata:(void *)clientdata {
    self = [super init];
    if (self) {
        iOS6 = ([[[UIDevice currentDevice] systemVersion] compare:@"6.0" options:NSNumericSearch] != NSOrderedAscending);
        numChannels = !iOS6 ? 2 : channels;
#if !__has_feature(objc_arc)
        audioSessionCategory = [category retain];
#else
        audioSessionCategory = category;
#endif
        saveBatteryInBackground = true;
        started = false;
        preferredBufferSizeMs = preferredBufferSize;
        preferredMinimumSamplerate = prefsamplerate;
#if (USES_AUDIO_INPUT == 1)
        bool recordOnly = [category isEqualToString:AVAudioSessionCategoryRecord];
        inputEnabled = recordOnly || [category isEqualToString:AVAudioSessionCategoryPlayAndRecord];
#else
        bool recordOnly = false;
        inputEnabled = false;
#endif
        processingCallback = callback;
        processingClientdata = clientdata;
        delegate = d;
        audioSystemInfo = [[NSMutableString alloc] initWithCapacity:256];
        silenceFrames = 0;
        background = audioUnitRunning = false;
        samplerate = minimumNumberOfFrames = maximumNumberOfFrames = 0;
        externalAudioDeviceName = nil;
        audioUnit = NULL;
        if (recordOnly) [self createAudioBuffersForRecordingCategory]; else inputBufferListForRecordingCategory = NULL;
        stopTimer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(everySecond) userInfo:nil repeats:YES];
        [self resetAudio];

        // Need to listen for a few app and audio session related events.
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onForeground) name:UIApplicationWillEnterForegroundNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onBackground) name:UIApplicationDidEnterBackgroundNotification object:nil];
        if (iOS6) {
            [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onMediaServerReset:) name:AVAudioSessionMediaServicesWereResetNotification object:[AVAudioSession sharedInstance]];
            [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onAudioSessionInterrupted:) name:AVAudioSessionInterruptionNotification object:[AVAudioSession sharedInstance]];
            [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onRouteChange:) name:AVAudioSessionRouteChangeNotification object:[AVAudioSession sharedInstance]];
        } else {
            AVAudioSession *s = [AVAudioSession sharedInstance];
            SILENCE_DEPRECATION(s.delegate = (id<AVAudioSessionDelegate>)self); // iOS 5 compatibility
        };
    };
    return self;
}

- (void)dealloc {
    [stopTimer invalidate];
    if (audioUnit != NULL) {
        AudioUnitUninitialize(audioUnit);
        AudioComponentInstanceDispose(audioUnit);
    };
    if (inputBufferListForRecordingCategory) {
        for (int n = 0; n < numChannels; n++) free(inputBufferListForRecordingCategory->mBuffers[n].mData);
        free(inputBufferListForRecordingCategory);
    };
    [[AVAudioSession sharedInstance] setActive:NO error:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
#if !__has_feature(objc_arc)
    [audioSystemInfo release];
    [externalAudioDeviceName release];
    [super dealloc];
#endif
}

// App and audio session lifecycle
- (void)onMediaServerReset:(NSNotification *)notification { // The mediaserver daemon can die. Yes, it happens sometimes.
    if (![NSThread isMainThread]) [self performSelectorOnMainThread:@selector(onMediaServerReset:) withObject:nil waitUntilDone:NO];
    else {
        if (audioUnit) AudioOutputUnitStop(audioUnit);
        audioUnitRunning = false;
        [[AVAudioSession sharedInstance] setActive:NO error:nil];
        [self resetAudio];
        [self start];
    };
}

- (void)everySecond { // If we waited for more than 1 second with silence, stop RemoteIO to save battery.
    if (silenceFrames > samplerate) {
        [self beginInterruption];
        silenceFrames = 0;
    }
}

- (void)beginInterruption { // Phone call, etc.
    if (![NSThread isMainThread]) [self performSelectorOnMainThread:@selector(beginInterruption) withObject:nil waitUntilDone:NO];
    else {
        audioUnitRunning = false;
        if (audioUnit) AudioOutputUnitStop(audioUnit);
    };
}

- (void)endInterruption {
    if (audioUnitRunning || !started) return;
    if (![NSThread isMainThread]) [self performSelectorOnMainThread:@selector(endInterruption) withObject:nil waitUntilDone:NO];
    else {
        [delegate interruptionEnded];
        [[AVAudioSession sharedInstance] setActive:NO error:nil];
        [self resetAudio];
        [self start];
    }
 }

- (void)endInterruptionWithFlags:(NSUInteger)flags {
    [self endInterruption];
}

- (void)startDelegateInterruption {
    [delegate interruptionStarted];
}

- (void)onAudioSessionInterrupted:(NSNotification *)notification {
    NSNumber *interruption = [notification.userInfo objectForKey:AVAudioSessionInterruptionTypeKey];
    if (interruption) switch ([interruption intValue]) {
        case AVAudioSessionInterruptionTypeBegan: {
            NSNumber *wasSuspended = [notification.userInfo objectForKey:AVAudioSessionInterruptionWasSuspendedKey];
            if (!(wasSuspended && ([wasSuspended boolValue] == TRUE))) {
                if (audioUnitRunning) [self performSelectorOnMainThread:@selector(startDelegateInterruption) withObject:nil waitUntilDone:NO];
                [self beginInterruption];
            }
        } break;
        case AVAudioSessionInterruptionTypeEnded: {
            NSNumber *shouldResume = [notification.userInfo objectForKey:AVAudioSessionInterruptionOptionKey];
            if ((shouldResume == nil) || [shouldResume unsignedIntegerValue] == AVAudioSessionInterruptionOptionShouldResume) [self endInterruption];
        } break;
    };
}

- (void)onForeground { // App comes foreground.
    if (background) {
        background = false;
        [self endInterruption];
    };
}

- (void)onBackground { // App went to background.
    background = true;
}

// Audio Session
- (void)onRouteChange:(NSNotification *)notification {
    if (![NSThread isMainThread]) {
        [self performSelectorOnMainThread:@selector(onRouteChange:) withObject:notification waitUntilDone:NO];
        return;
    };
    
    bool receiverAvailable = false, usbOrHDMIAvailable = false;
    for (AVAudioSessionPortDescription *port in [[[AVAudioSession sharedInstance] currentRoute] outputs]) {
        if ([port.portType isEqualToString:AVAudioSessionPortUSBAudio] || [port.portType isEqualToString:AVAudioSessionPortHDMI]) {
            usbOrHDMIAvailable = true;
            break;
        } else if ([port.portType isEqualToString:AVAudioSessionPortBuiltInReceiver]) receiverAvailable = true;
    };

    if (receiverAvailable && !usbOrHDMIAvailable) { // Comment this out if you would like to use the receiver instead.
        [[AVAudioSession sharedInstance] overrideOutputAudioPort:AVAudioSessionPortOverrideSpeaker error:nil];
        [self performSelectorOnMainThread:@selector(onRouteChange:) withObject:nil waitUntilDone:NO];
        return;
    };

    memset(RemoteIOOutputChannelMap, 0, sizeof(RemoteIOOutputChannelMap));
    outputChannelMap.headphoneAvailable = false;
    outputChannelMap.numberOfHDMIChannelsAvailable = outputChannelMap.numberOfUSBChannelsAvailable = inputChannelMap.numberOfUSBChannelsAvailable = 0;
#if !__has_feature(objc_arc)
    [audioSystemInfo release];
    [externalAudioDeviceName release];
#endif
    audioSystemInfo = [[NSMutableString alloc] initWithCapacity:128];
    [audioSystemInfo appendString:@"Outputs: "];
    externalAudioDeviceName = nil;

    bool first = true; int n = 0;
    for (AVAudioSessionPortDescription *port in [[[AVAudioSession sharedInstance] currentRoute] outputs]) {
        int channels = (int)[port.channels count];
        audioDeviceType type = NSStringToAudioDeviceType(port.portType);
        [audioSystemInfo appendFormat:@"%s%@ (%i out)", first ? "" : ", ", [port.portName stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]], channels];
        first = false;
        
        if (type == audioDeviceType_headphone) outputChannelMap.headphoneAvailable = true;
        else if (type == audioDeviceType_HDMI) {
            outputChannelMap.numberOfHDMIChannelsAvailable = channels;
#if !__has_feature(objc_arc)
            [externalAudioDeviceName release];
            externalAudioDeviceName = [port.portName retain];
#else
            externalAudioDeviceName = port.portName;
#endif
        } else if (type == audioDeviceType_USB) { // iOS can handle one USB audio device only
            outputChannelMap.numberOfUSBChannelsAvailable = channels;
#if !__has_feature(objc_arc)
            [externalAudioDeviceName release];
            externalAudioDeviceName = [port.portName retain];
#else
            externalAudioDeviceName = port.portName;
#endif
        };
        
        while (channels > 0) {
            RemoteIOOutputChannelMap[n++] = type;
            channels--;
        };
    };

    if ([[AVAudioSession sharedInstance] isInputAvailable]) {
        [audioSystemInfo appendString:@", Inputs: "];
        first = true;
        
        for (AVAudioSessionPortDescription *port in [[[AVAudioSession sharedInstance] currentRoute] inputs]) {
            int channels = (int)[port.channels count];
            audioDeviceType type = NSStringToAudioDeviceType(port.portType);
            [audioSystemInfo appendFormat:@"%s%@ (%i in)", first ? "" : ", ", [port.portName stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]], channels];
            first = false;
            if (type == audioDeviceType_USB) inputChannelMap.numberOfUSBChannelsAvailable = channels;
        };
        
        if (first) [audioSystemInfo appendString:@"-"];
    };

    [self mapChannels];
    if (usbOrHDMIAvailable) SILENCE_DEPRECATION([[MPMusicPlayerController applicationMusicPlayer] setVolume:1.0f]); // iOS 5 and iOS 6 compatibility
}

- (void)setSamplerateAndBuffersize {
    if (samplerate > 0) {
        double sr = samplerate < preferredMinimumSamplerate ? preferredMinimumSamplerate : 0, current;
        if (!iOS6) {
            SILENCE_DEPRECATION(current = [[AVAudioSession sharedInstance] preferredHardwareSampleRate]); // iOS 5 compatibility
        } else current = [[AVAudioSession sharedInstance] preferredSampleRate];
        if (current != sr) {
            if (!iOS6) {
                SILENCE_DEPRECATION([[AVAudioSession sharedInstance] setPreferredHardwareSampleRate:sr error:NULL]); // iOS 5 compatibility
            } else [[AVAudioSession sharedInstance] setPreferredSampleRate:sr error:NULL];
        };
    };
    [[AVAudioSession sharedInstance] setPreferredIOBufferDuration:double(preferredBufferSizeMs) * 0.001 error:NULL];
}

- (void)resetAudio {
    if (audioUnit != NULL) {
        AudioUnitUninitialize(audioUnit);
        AudioComponentInstanceDispose(audioUnit);
        audioUnit = NULL;
    };

    bool multiRoute = false;
    if ((numChannels > 2) && [audioSessionCategory isEqualToString:AVAudioSessionCategoryPlayback]) {
        for (AVAudioSessionPortDescription *port in [[[AVAudioSession sharedInstance] currentRoute] outputs]) {
            if ([port.portType isEqualToString:AVAudioSessionPortUSBAudio]) {
                if ([port.channels count] == 2) multiRoute = true;
                break;
            };
        }
    };
#ifdef ALLOW_BLUETOOTH
    if (multiRoute)  [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryMultiRoute error:NULL];
    else [[AVAudioSession sharedInstance] setCategory:audioSessionCategory withOptions:AVAudioSessionCategoryOptionAllowBluetoothA2DP | AVAudioSessionCategoryOptionMixWithOthers error:NULL];
#else
    [[AVAudioSession sharedInstance] setCategory:multiRoute ? AVAudioSessionCategoryMultiRoute : audioSessionCategory error:NULL];
#endif
    [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeDefault error:NULL];
    [self setSamplerateAndBuffersize];
    [[AVAudioSession sharedInstance] setActive:YES error:NULL];

    audioUnit = [self createRemoteIO];
    if (!multiRoute) {
        if (iOS6) [self onRouteChange:nil]; else [self mapChannels];
    };
}

// RemoteIO
static void streamFormatChangedCallback(void *inRefCon, AudioUnit inUnit, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement) {
    AudioStreamBasicDescription format;
    format.mSampleRate = 0;

    if ((inScope == kAudioUnitScope_Output) && (inElement == 0)) {
        UInt32 size = 0;
        AudioUnitGetPropertyInfo(inUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &size, NULL);
        AudioUnitGetProperty(inUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &format, &size);
    } else if ((inScope == kAudioUnitScope_Input) && (inElement == 1)) {
        UInt32 size = 0;
        AudioUnitGetPropertyInfo(inUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 1, &size, NULL);
        AudioUnitGetProperty(inUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 1, &format, &size);
    }

    if (format.mSampleRate != 0) {
        __unsafe_unretained SuperpoweredIOSAudioIO *self = (__bridge SuperpoweredIOSAudioIO *)inRefCon;
        self->samplerate = (int)format.mSampleRate;
        int minimum = int(self->samplerate * 0.001f), maximum = int(self->samplerate * 0.015f);
        self->minimumNumberOfFrames = (minimum >> 3) << 3;
        self->maximumNumberOfFrames = (maximum >> 3) << 3;
        [self performSelectorOnMainThread:@selector(setSamplerateAndBuffersize) withObject:nil waitUntilDone:NO];
    }
}

static OSStatus coreAudioProcessingCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    __unsafe_unretained SuperpoweredIOSAudioIO *self = (__bridge SuperpoweredIOSAudioIO *)inRefCon;
    if (!ioData) ioData = self->inputBufferListForRecordingCategory;
    div_t d = div(inNumberFrames, 8);
    if ((d.rem != 0) || (inNumberFrames < self->minimumNumberOfFrames) || (inNumberFrames > self->maximumNumberOfFrames) || (ioData->mNumberBuffers != self->numChannels)) {
        return kAudioUnitErr_InvalidParameter;
    };

    // Get audio input.
    unsigned int inputChannels = (self->inputEnabled && !AudioUnitRender(self->audioUnit, ioActionFlags, inTimeStamp, 1, inNumberFrames, ioData)) ? self->numChannels : 0;
    float *bufs[self->numChannels];
    for (int n = 0; n < self->numChannels; n++) bufs[n] = (float *)ioData->mBuffers[n].mData;
    bool silence = true;

    // Make audio output.
    silence = !self->processingCallback(self->processingClientdata, bufs, inputChannels, self->numChannels, inNumberFrames, self->samplerate, inTimeStamp->mHostTime);

    if (silence) { // Despite of ioActionFlags, it outputs garbage sometimes, so must zero the buffers:
        *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
        for (unsigned char n = 0; n < ioData->mNumberBuffers; n++) memset(ioData->mBuffers[n].mData, 0, inNumberFrames << 2);

        // If the app is in the background, check if we don't output anything.
        if (self->background && self->saveBatteryInBackground) self->silenceFrames += inNumberFrames; else self->silenceFrames = 0;
    } else self->silenceFrames = 0;
    
    return noErr;
}

- (AudioUnit)createRemoteIO {
    AudioUnit au;

    AudioComponentDescription desc;
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_RemoteIO;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	AudioComponent component = AudioComponentFindNext(NULL, &desc);
	if (AudioComponentInstanceNew(component, &au) != 0) return NULL;

    bool recordOnly = [audioSessionCategory isEqualToString:AVAudioSessionCategoryRecord];
    UInt32 value = recordOnly ? 0 : 1;
	if (AudioUnitSetProperty(au, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &value, sizeof(value))) { AudioComponentInstanceDispose(au); return NULL; };
    value = inputEnabled ? 1 : 0;
	if (AudioUnitSetProperty(au, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &value, sizeof(value))) { AudioComponentInstanceDispose(au); return NULL; };

    AudioUnitAddPropertyListener(au, kAudioUnitProperty_StreamFormat, streamFormatChangedCallback, (__bridge void *)self);

    UInt32 size = 0;
    AudioUnitGetPropertyInfo(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &size, NULL);
    AudioStreamBasicDescription format;
    AudioUnitGetProperty(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &format, &size);

    samplerate = (int)format.mSampleRate;
    [self setSamplerateAndBuffersize];

	format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved | kAudioFormatFlagsNativeEndian;
    format.mBitsPerChannel = 32;
	format.mFramesPerPacket = 1;
    format.mBytesPerFrame = 4;
    format.mBytesPerPacket = 4;
    format.mChannelsPerFrame = numChannels;
    if (AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, sizeof(format))) { AudioComponentInstanceDispose(au); return NULL; };
    if (AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &format, sizeof(format))) { AudioComponentInstanceDispose(au); return NULL; };

	AURenderCallbackStruct callbackStruct;
	callbackStruct.inputProc = coreAudioProcessingCallback;
	callbackStruct.inputProcRefCon = (__bridge void *)self;
    if (recordOnly) {
        if (AudioUnitSetProperty(au, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, 1, &callbackStruct, sizeof(callbackStruct))) { AudioComponentInstanceDispose(au); return NULL; };
    } else {
        if (AudioUnitSetProperty(au, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callbackStruct, sizeof(callbackStruct))) { AudioComponentInstanceDispose(au); return NULL; };
    };

	if (AudioUnitInitialize(au)) { AudioComponentInstanceDispose(au); return NULL; };
    return au;
}

// Public methods
- (bool)start {
    started = true;
    if (audioUnit == NULL) return false;
    if (AudioOutputUnitStart(audioUnit)) return false;
    audioUnitRunning = true;
    [[AVAudioSession sharedInstance] setActive:YES error:nil];
    return true;
}

- (void)stop {
    started = false;
    if (audioUnit != NULL) AudioOutputUnitStop(audioUnit);
}

- (void)setPreferredBufferSizeMs:(int)ms {
    preferredBufferSizeMs = ms;
    [self setSamplerateAndBuffersize];
}

- (void)mapChannels {
    outputChannelMap.deviceChannels[0] = outputChannelMap.deviceChannels[1] = -1;
    for (int n = 0; n < 8; n++) outputChannelMap.HDMIChannels[n] = -1;
    for (int n = 0; n < 32; n++) outputChannelMap.USBChannels[n] = inputChannelMap.USBChannels[n] = - 1;

    [delegate mapChannels:&outputChannelMap inputMap:&inputChannelMap externalAudioDeviceName:externalAudioDeviceName outputsAndInputs:audioSystemInfo];
    if (!audioUnit || !iOS6 || (numChannels <= 2)) return;

    SInt32 outputmap[32], inputmap[32];
    int devicePos = 0, hdmiPos = 0, usbPos = 0;

    for (int n = 0; n < 32; n++) {
        if (RemoteIOOutputChannelMap[n] != 0) switch (RemoteIOOutputChannelMap[n]) {
            case audioDeviceType_HDMI: if (hdmiPos < 8) outputmap[n] = outputChannelMap.HDMIChannels[hdmiPos++]; break;
            case audioDeviceType_USB: if (usbPos < 32) outputmap[n] = outputChannelMap.USBChannels[usbPos++]; break;
            default: if (devicePos < 2) outputmap[n] = outputChannelMap.deviceChannels[devicePos++];
        } else outputmap[n] = -1;
        inputmap[n] = inputChannelMap.USBChannels[n];
    };

#if !TARGET_IPHONE_SIMULATOR
    AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, outputmap, 128);
    AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 1, inputmap, 128);
#endif
}

- (void)reconfigureWithAudioSessionCategory:(NSString *)category {
    if (![NSThread isMainThread]) {
        [self performSelectorOnMainThread:@selector(reconfigureWithAudioSessionCategory:) withObject:category waitUntilDone:NO];
        return;
    };
#if !__has_feature(objc_arc)
    [audioSessionCategory release];
    audioSessionCategory = [category retain];
#else
    audioSessionCategory = category;
#endif

#if (USES_AUDIO_INPUT == 1)
    bool recordOnly = [category isEqualToString:AVAudioSessionCategoryRecord];
    if (recordOnly && !inputBufferListForRecordingCategory) [self createAudioBuffersForRecordingCategory];
    inputEnabled = recordOnly || [category isEqualToString:AVAudioSessionCategoryPlayAndRecord];

    if (inputEnabled && [[AVAudioSession sharedInstance] respondsToSelector:@selector(recordPermission)] && [[AVAudioSession sharedInstance] respondsToSelector:@selector(requestRecordPermission:)]) {
        if ([[AVAudioSession sharedInstance] recordPermission] == AVAudioSessionRecordPermissionGranted) [self onMediaServerReset:nil];
        else {
            [[AVAudioSession sharedInstance] requestRecordPermission:^(BOOL granted) {
                if (granted) [self onMediaServerReset:nil]; else [delegate recordPermissionRefused];
            }];
        };
    } else [self onMediaServerReset:nil];
#else
    [self onMediaServerReset:nil];
#endif
}

@end
