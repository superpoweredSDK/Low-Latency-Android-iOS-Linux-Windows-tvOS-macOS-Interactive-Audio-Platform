#import "SuperpoweredIOSAudioOutput.h"
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <MediaPlayer/MediaPlayer.h>

#define SILENCE_DEPRECATION(code)                                   \
{                                                                   \
_Pragma("clang diagnostic push")                                    \
_Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")   \
code;                                                               \
_Pragma("clang diagnostic pop")                                     \
}

typedef enum audioDeviceType {
    audioDeviceType_USB = 1, audioDeviceType_headphone = 2, audioDeviceType_HDMI = 3, audioDeviceType_other = 4
} audioDeviceType;

static audioDeviceType NSStringToAudioDeviceType(NSString *str) {
    if ([str isEqualToString:AVAudioSessionPortHeadphones]) return audioDeviceType_headphone;
    else if ([str isEqualToString:AVAudioSessionPortUSBAudio]) return audioDeviceType_USB;
    else if ([str isEqualToString:AVAudioSessionPortHDMI]) return audioDeviceType_HDMI;
    else return audioDeviceType_other;
}


@implementation SuperpoweredIOSAudioOutput {
    id<SuperpoweredIOSAudioIODelegate>delegate;
    NSString *multiDeviceName;
    NSMutableString *outputsAndInputs;
    audioProcessingCallback_C processingCallback;
    void *processingClientdata;
    
    AudioComponentInstance audioUnit;
    
    multiOutputChannelMap outputChannelMap;
    multiInputChannelMap inputChannelMap;
    
    audioDeviceType RemoteIOOutputChannelMap[64];
    SInt32 AUOutputChannelMap[32], AUInputChannelMap[32];

    int remoteIOChannels, multiDeviceChannels, silenceFrames, samplerate, multiChannels, preferredMinimumSamplerate;
    bool audioUnitRunning, iOS6, background, fixReceiver, waitingForReset;
}

@synthesize preferredBufferSizeMs, inputEnabled, saveBatteryInBackground, audioSessionCategory;

static OSStatus audioProcessingCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    SuperpoweredIOSAudioOutput *self = (__bridge SuperpoweredIOSAudioOutput *)inRefCon;

    div_t d = div(inNumberFrames, 8);
    if ((d.rem != 0) || (inNumberFrames < 32) || (inNumberFrames > 512) || (ioData->mNumberBuffers != self->remoteIOChannels)) {
        return kAudioUnitErr_InvalidParameter;
    };

    float *bufs[self->remoteIOChannels];
    for (int n = 0; n < self->remoteIOChannels; n++) bufs[n] = (float *)ioData->mBuffers[n].mData;
    
    unsigned int inputChannels = 0;
    if (self->inputEnabled) {
        if (!AudioUnitRender(self->audioUnit, ioActionFlags, inTimeStamp, 1, inNumberFrames, ioData)) inputChannels = self->remoteIOChannels;
    };

    bool silence = true;
    if (self->processingCallback) silence = !self->processingCallback(self->processingClientdata, bufs, inputChannels, self->remoteIOChannels, inNumberFrames, self->samplerate, inTimeStamp->mHostTime);
    else silence = ![self->delegate audioProcessingCallback:bufs inputChannels:inputChannels outputChannels:self->remoteIOChannels numberOfSamples:inNumberFrames samplerate:self->samplerate hostTime:inTimeStamp->mHostTime];

	if (silence) {
        // Output silence.
        *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
        // Despite of ioActionFlags, it outputs garbage sometimes, so must zero the buffers:
        for (unsigned char n = 0; n < ioData->mNumberBuffers; n++) memset(ioData->mBuffers[n].mData, 0, ioData->mBuffers[n].mDataByteSize);
        
        if (self->background && self->saveBatteryInBackground) { // If the app is in the background, check if we don't output anything.
            self->silenceFrames += inNumberFrames;
            if (self->silenceFrames > self->samplerate) { // If we waited for more than 1 second with silence, stop RemoteIO to save battery.
                self->silenceFrames = 0;
                [self beginInterruption];
            };
        } else self->silenceFrames = 0;
    } else self->silenceFrames = 0;

	return noErr;
}

- (id)initWithDelegate:(NSObject<SuperpoweredIOSAudioIODelegate> *)d preferredBufferSize:(unsigned int)preferredBufferSize preferredMinimumSamplerate:(unsigned int)prefsamplerate audioSessionCategory:(NSString *)category multiChannels:(int)channels fixReceiver:(bool)fr {
    self = [super init];
    if (self) {
        iOS6 = ([[[UIDevice currentDevice] systemVersion] compare:@"6.0" options:NSNumericSearch] != NSOrderedAscending);
        multiDeviceChannels = 0;
        multiChannels = !iOS6 ? 2 : channels;
#if !__has_feature(objc_arc)
        audioSessionCategory = [category retain];
#else
        audioSessionCategory = category;
#endif
        saveBatteryInBackground = true;
        remoteIOChannels = 2;
        preferredBufferSizeMs = preferredBufferSize;
        preferredMinimumSamplerate = prefsamplerate;
        waitingForReset = false;
        inputEnabled = [category isEqualToString:AVAudioSessionCategoryPlayAndRecord] || (iOS6 && [category isEqualToString:AVAudioSessionCategoryMultiRoute]);
        processingCallback = NULL;
        processingClientdata = NULL;
        delegate = d;
        fixReceiver = fr;

        outputsAndInputs = [[NSMutableString alloc] initWithCapacity:256];
        silenceFrames = 0;
        background = audioUnitRunning = false;
        samplerate = 0;
        multiDeviceName = nil;
        audioUnit = NULL;

        [self resetAudioSession];
        
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

- (void)setProcessingCallback_C:(audioProcessingCallback_C)callback clientdata:(void *)clientdata {
    processingCallback = callback;
    processingClientdata = clientdata;
}

- (void)dealloc {
    if (audioUnit != NULL) {
        AudioUnitUninitialize(audioUnit);
        AudioComponentInstanceDispose(audioUnit);
    };
    
    [[AVAudioSession sharedInstance] setActive:NO error:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
#if !__has_feature(objc_arc)
    [outputsAndInputs release];
    [multiDeviceName release];
    [super dealloc];
#endif
}

- (void)resetStart {
    [self resetAudioSession];
    [self performSelector:@selector((start)) withObject:nil afterDelay:1.0]; // Give iOS 1 second for the new audio session, we may see some glitches otherwise with some USB devices.
}

// App and audio session lifecycle.
- (void)onMediaServerReset:(NSNotification *)notification { // The mediaserver daemon can die. Yes, it happens sometimes.
    if (![NSThread isMainThread]) [self performSelectorOnMainThread:@selector(onMediaServerReset:) withObject:nil waitUntilDone:NO];
    else {
        if (audioUnit) AudioOutputUnitStop(audioUnit);
        audioUnitRunning = false;
        [[AVAudioSession sharedInstance] setActive:NO error:nil];
        waitingForReset = true;

        [self performSelector:@selector(resetStart) withObject:nil afterDelay:2.0]; // Let's wait 2 seconds before we resume.
    };
}

- (void)beginInterruption { // Phone call, etc.
    if (![NSThread isMainThread]) [self performSelectorOnMainThread:@selector(beginInterruption) withObject:nil waitUntilDone:NO];
    else {
        audioUnitRunning = false;
        if (audioUnit != NULL) AudioOutputUnitStop(audioUnit);
    };
}

- (void)endInterruption {
    if (audioUnitRunning) return;
    if (![NSThread isMainThread]) [self performSelectorOnMainThread:@selector(endInterruption) withObject:nil waitUntilDone:NO];
    else {
        if ([[AVAudioSession sharedInstance] setActive:YES error:nil] && [self start]) {
            [delegate interruptionEnded];
            audioUnitRunning = true;
        };
        
        if (!audioUnitRunning) { // Need to try twice sometimes. Don't know why.
            if ([[AVAudioSession sharedInstance] setActive:YES error:nil] && [self start]) {
                [delegate interruptionEnded];
                audioUnitRunning = true;
            };
        };
    };
}

- (void)endInterruptionWithFlags:(NSUInteger)flags {
    [self endInterruption];
}

- (void)startDelegateInterrupt {
    [delegate interruptionStarted];
}

- (void)onAudioSessionInterrupted:(NSNotification *)notification {
    NSNumber *interruption = [notification.userInfo objectForKey:AVAudioSessionInterruptionTypeKey];
    if (interruption) switch ([interruption intValue]) {
        case AVAudioSessionInterruptionTypeBegan:
            if (audioUnitRunning) [self performSelectorOnMainThread:@selector(startDelegateInterrupt) withObject:nil waitUntilDone:NO];
            [self beginInterruption];
            break;
        case AVAudioSessionInterruptionTypeEnded: [self endInterruption]; break;
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
// End of app and audio session lifecycle.

- (void)onRouteChange:(NSNotification *)notification {
    if (waitingForReset) return;
    if (![NSThread isMainThread]) {
        [self performSelectorOnMainThread:@selector(onRouteChange:) withObject:nil waitUntilDone:NO];
        return;
    };
    
    int _multiDeviceChannels = 0, receiver = false;
    for (AVAudioSessionPortDescription *port in [[[AVAudioSession sharedInstance] currentRoute] outputs]) {
        if ([port.portType isEqualToString:AVAudioSessionPortUSBAudio] || [port.portType isEqualToString:AVAudioSessionPortHDMI]) _multiDeviceChannels += [port.channels count];
        else if ([port.portType isEqualToString:AVAudioSessionPortBuiltInReceiver]) receiver = true;
    };
    
    if (fixReceiver && receiver && !_multiDeviceChannels) {
        [[AVAudioSession sharedInstance] overrideOutputAudioPort:AVAudioSessionPortOverrideSpeaker error:nil];
        [self performSelectorOnMainThread:@selector(onRouteChange:) withObject:nil waitUntilDone:NO];
        return;
    };
   
    // Switch to multiple channels.
    bool multiDeviceConnectedOrDisconnected = (_multiDeviceChannels > 0) != (multiDeviceChannels > 0);
    multiDeviceChannels = _multiDeviceChannels;
    if (multiDeviceConnectedOrDisconnected && (multiChannels > 2)) {
        if (audioUnit) {
            AudioOutputUnitStop(audioUnit);
            AudioUnitUninitialize(audioUnit);
            AudioComponentInstanceDispose(audioUnit);
            audioUnit = NULL;
        };
        audioUnitRunning = false;
        [[AVAudioSession sharedInstance] setActive:NO error:nil];

        waitingForReset = true;
        [self performSelector:@selector(resetStart) withObject:nil afterDelay:1.0]; // Give iOS 1 second to setup the audio system properly, we may see some glitches otherwise with some USB devices.
        return;
    };
    
    memset(RemoteIOOutputChannelMap, 0, sizeof(RemoteIOOutputChannelMap));
    outputChannelMap.headphoneAvailable = false;
    outputChannelMap.numberOfHDMIChannelsAvailable = outputChannelMap.numberOfUSBChannelsAvailable = inputChannelMap.numberOfUSBChannelsAvailable = 0;
    outputChannelMap.deviceChannels[0] = outputChannelMap.deviceChannels[1] = -1;
    for (int n = 0; n < 8; n++) outputChannelMap.HDMIChannels[n] = -1;
    for (int n = 0; n < 32; n++) outputChannelMap.USBChannels[n] = inputChannelMap.USBChannels[n] = - 1;
#if !__has_feature(objc_arc)
    [outputsAndInputs release];
    [multiDeviceName release];
#endif
    outputsAndInputs = [[NSMutableString alloc] initWithCapacity:256];
    [outputsAndInputs appendString:@"Outputs: "];
    multiDeviceName = nil;
    
    bool first = true; int n = 0;
    for (AVAudioSessionPortDescription *port in [[[AVAudioSession sharedInstance] currentRoute] outputs]) {
        int channels = (int)[port.channels count];
        audioDeviceType type = NSStringToAudioDeviceType(port.portType);
        [outputsAndInputs appendFormat:@"%s%@ (%i out)", first ? "" : ", ", [port.portName stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]], channels];
        first = false;
        
        if (type == audioDeviceType_headphone) outputChannelMap.headphoneAvailable = true;
        else if (type == audioDeviceType_HDMI) {
            outputChannelMap.numberOfHDMIChannelsAvailable = channels;
#if !__has_feature(objc_arc)
            [multiDeviceName release];
            multiDeviceName = [port.portName retain];
#else
            multiDeviceName = port.portName;
#endif
            
        } else if (type == audioDeviceType_USB) { // iOS can handle one USB audio device only
            outputChannelMap.numberOfUSBChannelsAvailable = channels;
#if !__has_feature(objc_arc)
            [multiDeviceName release];
            multiDeviceName = [port.portName retain];
#else
            multiDeviceName = port.portName;
#endif
        };
        
        while (channels > 0) {
            RemoteIOOutputChannelMap[n] = type;
            n++;
            channels--;
        };
    };
    
    if ([[AVAudioSession sharedInstance] isInputAvailable]) {
        [outputsAndInputs appendString:@", Inputs: "];
        first = true;
        
        for (AVAudioSessionPortDescription *port in [[[AVAudioSession sharedInstance] currentRoute] inputs]) {
            int channels = (int)[port.channels count];
            audioDeviceType type = NSStringToAudioDeviceType(port.portType);
            [outputsAndInputs appendFormat:@"%s%@ (%i in)", first ? "" : ", ", [port.portName stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]], channels];
            first = false;
            
            if (type == audioDeviceType_USB) inputChannelMap.numberOfUSBChannelsAvailable = channels;
        };
        
        if (first) [outputsAndInputs appendString:@"-"];
    };
    
    [self multiRemapChannels];
    // Maximize system volume for connected audio devices.
    if (_multiDeviceChannels) SILENCE_DEPRECATION([[MPMusicPlayerController applicationMusicPlayer] setVolume:1.0f]); // iOS 5 and iOS 6 compatibility
}

- (void)setSamplerateAndBuffersize {
    if (samplerate > 0) {
        double sr = samplerate < preferredMinimumSamplerate ? preferredMinimumSamplerate : 0;
        double current;
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

- (void)resetAudioSession {
    remoteIOChannels = multiDeviceChannels ? multiChannels : 2;

    // If an audio device is connected with only 2 channels and we provide more, or input is enabled but the category is playback, force multi-route category to combine the iOS device's output with the audio device.
    [[AVAudioSession sharedInstance] setCategory:((multiDeviceChannels == 2) && (multiChannels > 2)) ? AVAudioSessionCategoryMultiRoute : audioSessionCategory error:NULL];

    [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeDefault error:NULL];
    [self setSamplerateAndBuffersize];
    [[AVAudioSession sharedInstance] setActive:YES error:NULL];

    [self recreateRemoteIO];
    waitingForReset = false;
    if (iOS6) [self onRouteChange:nil]; else [self multiRemapChannels];
}

static void streamFormatChangedCallback(void *inRefCon, AudioUnit inUnit, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement) {
    if ((inScope == kAudioUnitScope_Output) && (inElement == 0)) {
        UInt32 size = 0;
        AudioUnitGetPropertyInfo(inUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &size, NULL);
        AudioStreamBasicDescription format;
        AudioUnitGetProperty(inUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &format, &size);

        SuperpoweredIOSAudioOutput *self = (__bridge SuperpoweredIOSAudioOutput *)inRefCon;
        self->samplerate = (int)format.mSampleRate;
        [self performSelectorOnMainThread:@selector(setSamplerateAndBuffersize) withObject:nil waitUntilDone:NO];
    };
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

	UInt32 value = 1;
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
    format.mChannelsPerFrame = remoteIOChannels;
    if (AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, sizeof(format))) { AudioComponentInstanceDispose(au); return NULL; };
    if (inputEnabled) {
        if (AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &format, sizeof(format))) { AudioComponentInstanceDispose(au); return NULL; };
    };
    
	AURenderCallbackStruct callbackStruct;
	callbackStruct.inputProc = audioProcessingCallback;
	callbackStruct.inputProcRefCon = (__bridge void *)self;
	if (AudioUnitSetProperty(au, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callbackStruct, sizeof(callbackStruct))) { AudioComponentInstanceDispose(au); return NULL; };
    
	AudioUnitInitialize(au);
    return au;
}

- (void)recreateRemoteIO {
	if (audioUnit != NULL) {
		AudioUnitUninitialize(audioUnit);
		AudioComponentInstanceDispose(audioUnit);
        audioUnit = NULL;
	};
    audioUnit = [self createRemoteIO];
}

// public methods
- (bool)start {
    if (audioUnit == NULL) return false;
    if (AudioOutputUnitStart(audioUnit)) return false;
    audioUnitRunning = true;
    return true;
}

- (void)stop {
    if (audioUnit != NULL) AudioOutputUnitStop(audioUnit);
}

- (void)multiRemapChannels {
    if (multiChannels < 3) {
        if (!iOS6) [delegate multiMapChannels:&outputChannelMap inputMap:&inputChannelMap multiDeviceName:multiDeviceName outputsAndInputs:outputsAndInputs];
        return;
    };
    for (int n = 0; n < 32; n++) AUOutputChannelMap[n] = -1;
    
    if (multiChannels > 2) [delegate multiMapChannels:&outputChannelMap inputMap:&inputChannelMap multiDeviceName:multiDeviceName outputsAndInputs:outputsAndInputs];

    int devicePos = 0, hdmiPos = 0, usbPos = 0;
    for (int n = 0; n < 32; n++) {
        if (RemoteIOOutputChannelMap[n] != 0) switch (RemoteIOOutputChannelMap[n]) {
            case audioDeviceType_HDMI:
                if (hdmiPos < 8) {
                    AUOutputChannelMap[n] = outputChannelMap.HDMIChannels[hdmiPos];
                    hdmiPos++;
                };
                break;
            case audioDeviceType_USB:
                if (usbPos < 32) {
                    AUOutputChannelMap[n] = outputChannelMap.USBChannels[usbPos];
                    usbPos++;
                };
                break;
            default:
                if (devicePos < 2) {
                    AUOutputChannelMap[n] = outputChannelMap.deviceChannels[devicePos];
                    devicePos++;
                };
        } else break;
    };

    if (audioUnit && iOS6) [self setOutputChannelMap:AUOutputChannelMap];
    for (int n = 0; n < 32; n++) AUInputChannelMap[n] = inputChannelMap.USBChannels[n];
    if (audioUnit && iOS6 && inputEnabled) [self setInputChannelMap:AUInputChannelMap];
}

- (void)setInputChannelMap:(SInt32 *)map {
    AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 1, map, sizeof(SInt32) * 32);
}

- (void)setOutputChannelMap:(SInt32 *)map {
    UInt32 size = 0;
    Boolean writable = false;
    AudioUnitGetPropertyInfo(audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, &size, &writable);
    if ((size > 0) && writable) {
        if (size > sizeof(AUOutputChannelMap)) size = sizeof(AUOutputChannelMap);
        AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, map, size);
    };
}

- (void)setInputEnabled:(bool)ie {
    if (inputEnabled != ie) {
        self->inputEnabled = ie;
        if (audioUnit) {
            AudioUnit oldUnit = audioUnit;
            
            audioUnit = [self createRemoteIO];
            if (multiDeviceChannels && iOS6) {
                [self setOutputChannelMap:AUOutputChannelMap];
                if (inputEnabled) [self setInputChannelMap:AUInputChannelMap];
            };
            
            if (audioUnitRunning) [self start];
            
            if (oldUnit != NULL) {
                AudioOutputUnitStop(oldUnit);
                AudioUnitUninitialize(oldUnit);
                AudioComponentInstanceDispose(oldUnit);
            };
        };
    };
}

- (void)setAudioSessionCategory:(NSString *)category {
    if ([category isEqualToString:self->audioSessionCategory]) return;
    if (!self->inputEnabled && [category isEqualToString:AVAudioSessionCategoryPlayAndRecord]) self->inputEnabled = true;
#if !__has_feature(objc_arc)
    audioSessionCategory = [category retain];
#else
    audioSessionCategory = category;
#endif
    [self performSelectorOnMainThread:@selector(onMediaServerReset:) withObject:nil waitUntilDone:NO];
}

- (void)setPreferredBufferSizeMs:(int)ms {
    preferredBufferSizeMs = ms;
    [self setSamplerateAndBuffersize];
}

@end
