#import "SuperpoweredIOSAudioIO.h"
#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <pthread.h>
#include <mach/mach_time.h>

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
    uint64_t lastCallbackTime;
    int numChannels, silenceFrames, samplerate, minimumNumberOfFrames, maximumNumberOfFrames;
    bool audioUnitRunning, background, inputEnabled;
}

@synthesize preferredBufferSizeMs, preferredSamplerate, saveBatteryInBackground, started;

- (void)createAudioBuffersForRecordingCategory {
    inputBufferListForRecordingCategory = (AudioBufferList *)malloc(sizeof(AudioBufferList) + (sizeof(AudioBuffer) * numChannels));
    inputBufferListForRecordingCategory->mNumberBuffers = numChannels;
    for (int n = 0; n < numChannels; n++) {
        inputBufferListForRecordingCategory->mBuffers[n].mDataByteSize = 2048 * 4;
        inputBufferListForRecordingCategory->mBuffers[n].mNumberChannels = 1;
        inputBufferListForRecordingCategory->mBuffers[n].mData = malloc(inputBufferListForRecordingCategory->mBuffers[n].mDataByteSize);
    };
}

- (id)initWithDelegate:(NSObject<SuperpoweredIOSAudioIODelegate> *)d preferredBufferSize:(unsigned int)preferredBufferSize preferredSamplerate:(unsigned int)prefsamplerate audioSessionCategory:(NSString *)category channels:(int)channels audioProcessingCallback:(audioProcessingCallback)callback clientdata:(void *)clientdata {
    self = [super init];
    if (self) {
        numChannels = channels;
#if !__has_feature(objc_arc)
        audioSessionCategory = [category retain];
#else
        audioSessionCategory = category;
#endif
        saveBatteryInBackground = true;
        started = false;
        preferredBufferSizeMs = preferredBufferSize;
        preferredSamplerate = prefsamplerate;
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
        inputBufferListForRecordingCategory = NULL;
#if (USES_AUDIO_INPUT == 1)
        if (recordOnly) [self createAudioBuffersForRecordingCategory];
#endif
        stopTimer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(everySecond) userInfo:nil repeats:YES];
#if !__has_feature(objc_arc)
        [self release]; // to prevent NSTimer retaining this
#endif
        [self resetAudio];

        // Need to listen for a few app and audio session related events.
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onForeground) name:UIApplicationDidBecomeActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onBackground) name:UIApplicationDidEnterBackgroundNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onMediaServerReset:) name:AVAudioSessionMediaServicesWereResetNotification object:[AVAudioSession sharedInstance]];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onAudioSessionInterrupted:) name:AVAudioSessionInterruptionNotification object:[AVAudioSession sharedInstance]];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onRouteChange:) name:AVAudioSessionRouteChangeNotification object:[AVAudioSession sharedInstance]];
        
#if (USES_AUDIO_INPUT == 1)
        if ((recordOnly || [category isEqualToString:AVAudioSessionCategoryPlayAndRecord]) && [[AVAudioSession sharedInstance] respondsToSelector:@selector(recordPermission)] && [[AVAudioSession sharedInstance] respondsToSelector:@selector(requestRecordPermission:)] && ([[AVAudioSession sharedInstance] recordPermission] != AVAudioSessionRecordPermissionGranted)) {
            [[AVAudioSession sharedInstance] requestRecordPermission:^(BOOL granted) {
                if (granted) [self onMediaServerReset:nil];
                else if ([(NSObject *)self->delegate respondsToSelector:@selector(recordPermissionRefused)]) [self->delegate recordPermissionRefused];
            }];
        };
#endif
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
    } else if (!background && audioUnitRunning && started) { // If it should run...
        mach_timebase_info_data_t timebase;
        mach_timebase_info(&timebase);
        uint64_t diff = mach_absolute_time() - lastCallbackTime;
        diff *= timebase.numer;
        diff /= timebase.denom;
        if (diff > 1000000000) { // But it didn't call the audio processing callback in the past second.
            audioUnitRunning = false;
            [[AVAudioSession sharedInstance] setActive:NO error:nil];
            [self resetAudio];
            [self start];
        }
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
        if ([(NSObject *)delegate respondsToSelector:@selector(interruptionEnded)]) [delegate interruptionEnded];
        [[AVAudioSession sharedInstance] setActive:NO error:nil];
        [self resetAudio];
        [self start];
    }
 }

- (void)endInterruptionWithFlags:(NSUInteger)flags {
    [self endInterruption];
}

- (void)startDelegateInterruption {
    if ([(NSObject *)delegate respondsToSelector:@selector(interruptionStarted)]) [delegate interruptionStarted];
}

- (void)onAudioSessionInterrupted:(NSNotification *)notification {
    NSNumber *interruption = [notification.userInfo objectForKey:AVAudioSessionInterruptionTypeKey];
    if (interruption != nil) switch ([interruption intValue]) {
        case AVAudioSessionInterruptionTypeBegan: {
            bool wasSuspended = false;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-pointer-compare"
            if (&AVAudioSessionInterruptionWasSuspendedKey != NULL) {
#pragma clang diagnostic pop
                NSNumber *obj = [notification.userInfo objectForKey:AVAudioSessionInterruptionWasSuspendedKey];
                if ((obj != nil) && ([obj boolValue] == TRUE)) wasSuspended = true;
            }
            if (!wasSuspended) {
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
}

- (void)applyBuffersize {
    [[AVAudioSession sharedInstance] setPreferredIOBufferDuration:double(preferredBufferSizeMs) * 0.001 error:NULL];
}

- (void)applySamplerate {
    [[AVAudioSession sharedInstance] setPreferredSampleRate:preferredSamplerate error:NULL];
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
    if (multiRoute) [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryMultiRoute error:NULL];
    else [[AVAudioSession sharedInstance] setCategory:audioSessionCategory withOptions:AVAudioSessionCategoryOptionAllowBluetoothA2DP | AVAudioSessionCategoryOptionMixWithOthers error:NULL];
#else
    [[AVAudioSession sharedInstance] setCategory:multiRoute ? AVAudioSessionCategoryMultiRoute : audioSessionCategory error:NULL];
#endif
    [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeDefault error:NULL];
    [self applyBuffersize];
    [self applySamplerate];
    [[AVAudioSession sharedInstance] setActive:YES error:NULL];

    audioUnit = [self createRemoteIO];
    if (!multiRoute) [self onRouteChange:nil];
}

/*
 RemoteIO scopes and elements:
 hardware -> element 1 input scope -> element 1 output scope -> app
 app -> element 0 input scope -> element 0 output scope -> hardware
 hardware input properties: element 1 input scope
 hardware output properties: element 0 output scope
 */

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
        int sr = (int)format.mSampleRate;
        __unsafe_unretained SuperpoweredIOSAudioIO *self = (__bridge SuperpoweredIOSAudioIO *)inRefCon;
        if (self->samplerate != sr) {
            self->samplerate = sr;
            int minimum = int(self->samplerate * 0.001f), maximum = int(self->samplerate * 0.025f);
            self->minimumNumberOfFrames = (minimum >> 3) << 3;
            self->maximumNumberOfFrames = (maximum >> 3) << 3;
            [self performSelectorOnMainThread:@selector(applyBuffersize) withObject:nil waitUntilDone:NO];
        }
    }
}

static OSStatus coreAudioProcessingCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    __unsafe_unretained SuperpoweredIOSAudioIO *self = (__bridge SuperpoweredIOSAudioIO *)inRefCon;
    self->lastCallbackTime = mach_absolute_time();
    
    if (!ioData) ioData = self->inputBufferListForRecordingCategory;
    div_t d = div(inNumberFrames, 8);
    if (d.rem != 0) {
        // Core Audio performs sample rate conversion, but received no streamFormatChangedCallback. Recreate audio I/O for perfect match with hardware.
        if (self->audioUnitRunning) {
            self->audioUnitRunning = false;
            [self performSelectorOnMainThread:@selector(onMediaServerReset:) withObject:nil waitUntilDone:NO];
        }
        return kAudioUnitErr_InvalidParameter;
    }

    if (((int)inNumberFrames < self->minimumNumberOfFrames) || ((int)inNumberFrames > self->maximumNumberOfFrames) || ((int)ioData->mNumberBuffers != self->numChannels)) {
        return kAudioUnitErr_InvalidParameter;
    };

    // Get audio input.
    unsigned int inputChannels = (self->inputEnabled && !AudioUnitRender(self->audioUnit, ioActionFlags, inTimeStamp, 1, inNumberFrames, ioData)) ? self->numChannels : 0;
    float *bufs[self->numChannels];
    for (int n = 0; n < self->numChannels; n++) bufs[n] = (float *)ioData->mBuffers[n].mData;
    bool silence = true;

    // Make audio output.
    silence = !self->processingCallback(self->processingClientdata, bufs, inputChannels, bufs, self->numChannels, inNumberFrames, self->samplerate, inTimeStamp->mHostTime);

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
        if (AudioUnitSetProperty(au, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, 0, &callbackStruct, sizeof(callbackStruct))) { AudioComponentInstanceDispose(au); return NULL; };
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
    if (ms == preferredBufferSizeMs) return;
    preferredBufferSizeMs = ms;
    [self applyBuffersize];
}

- (void)setPreferredSamplerate:(int)hz {
    if (hz == preferredSamplerate) return;
    preferredSamplerate = hz;
    [self applySamplerate];
}

- (void)mapChannels {
    outputChannelMap.deviceChannels[0] = outputChannelMap.deviceChannels[1] = -1;
    for (int n = 0; n < 8; n++) outputChannelMap.HDMIChannels[n] = -1;
    for (int n = 0; n < 32; n++) outputChannelMap.USBChannels[n] = inputChannelMap.USBChannels[n] = - 1;
    
    if ([(NSObject *)self->delegate respondsToSelector:@selector(mapChannels:inputMap:externalAudioDeviceName:outputsAndInputs:)]) [delegate mapChannels:&outputChannelMap inputMap:&inputChannelMap externalAudioDeviceName:externalAudioDeviceName outputsAndInputs:audioSystemInfo];
    if (!audioUnit || (numChannels <= 2)) return;

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
    AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Output, 0, outputmap, 128);
    AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Output, 1, inputmap, 128);
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
                if (granted) [self onMediaServerReset:nil];
                else if ([(NSObject *)self->delegate respondsToSelector:@selector(recordPermissionRefused)]) [self->delegate recordPermissionRefused];
            }];
        };
    } else [self onMediaServerReset:nil];
#else
    [self onMediaServerReset:nil];
#endif
}

@end
