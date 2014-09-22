#import "SuperpoweredIOSAudioOutput.h"
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <MediaPlayer/MediaPlayer.h>

void apple824ToFloat(int **input, float **output, int numberOfBuffers, int numberOfFrames) {
    static const float conv_824_to_float = 1.0f / float(1 << 24);
    for (int n = 0; n < numberOfBuffers; n++) {
        int *src = input[n];
        float *dst = output[n];
        int f = numberOfFrames;
        while (f) {
            *dst++ = float(*src++) * conv_824_to_float;
            f--;
        };
    };
}

void stereoFloatToApple824(float *input, int *output[2], int numberOfFrames) {
    static const float conv_float_to_824 = float(1 << 24);
    while (numberOfFrames) {
        *output[0]++ = int(*input++ * conv_float_to_824);
        *output[1]++ = int(*input++ * conv_float_to_824);
        numberOfFrames--;
    };
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
    NSString *audioSessionCategory, *multiRouteDeviceName;
    NSMutableString *outputsAndInputs;
    
    AudioComponentInstance audioUnit;
    
    multiRouteOutputChannelMap outputChannelMap;
    multiRouteInputChannelMap inputChannelMap;
    
    audioDeviceType RemoteIOOutputChannelMap[64];
    SInt32 AUOutputChannelMap[32], AUInputChannelMap[32];
    
    int remoteIOChannels, silenceFrames, sampleRate, multiRouteChannels;
    bool audioUnitRunning, multiRouteDeviceConnected, samplerate48000, iOS6, background, fixReceiver, changingSamplerate, waitingForReset;
}

@synthesize preferredBufferSizeSamples, inputEnabled;

static OSStatus audioProcessingCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    SuperpoweredIOSAudioOutput *self = (__bridge SuperpoweredIOSAudioOutput *)inRefCon;
    if (self->changingSamplerate) return noErr;

// Superpowered works with a limited set of buffer sizes, from low to mid latency.
// There is no benefit to use higher sizes, as Superpowered is efficient with the battery and CPU.
    switch (inNumberFrames) {
        case 128: //  2.9 ms @ 44100 Hz,  2.7 ms @ 48000 Hz
        case 256: //  5.8 ms @ 44100 Hz,  5.3 ms @ 48000 Hz
        case 512: // 11.6 ms @ 44100 Hz, 10.7 ms @ 48000 Hz
            break;
        default:
            // Funky buffer sizes, because a new audio device or headphone is connected.
            // RemoteIO tries to resample us for a different sample rate.
            // But we don't want that, we can adapt our sample rate for best efficiency.
            self->samplerate48000 = !self->samplerate48000;
            self->sampleRate = self->samplerate48000 ? 48000 : 44100;
            self->changingSamplerate = true;
            [self performSelectorOnMainThread:@selector(changeSamplerate) withObject:nil waitUntilDone:NO];
            return noErr;
    };

    if (ioData->mNumberBuffers != self->remoteIOChannels) return kAudioUnitErr_InvalidParameter; // Should never happen. But what if... ?
    void *bufs[self->remoteIOChannels];
    for (int n = 0; n < self->remoteIOChannels; n++) bufs[n] = ioData->mBuffers[n].mData;
    
    unsigned int inputChannels = 0;
    if (self->inputEnabled) {
        if (!AudioUnitRender(self->audioUnit, ioActionFlags, inTimeStamp, 1, inNumberFrames, ioData)) inputChannels = self->remoteIOChannels;
    };
    
	if (![self->delegate audioProcessingCallback:bufs inputChannels:inputChannels outputChannels:self->remoteIOChannels numberOfSamples:inNumberFrames samplerate:self->samplerate48000 ? 48000 : 44100 hostTime:inTimeStamp->mHostTime]) {
        // Output silence.
        *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
        // Despite of ioActionFlags, it outputs garbage sometimes, so must zero the buffers:
        for (unsigned char n = 0; n < ioData->mNumberBuffers; n++) memset(ioData->mBuffers[n].mData, 0, ioData->mBuffers[n].mDataByteSize);
        
        if (self->background) { // If the app is in the background, check if we don't output anything.
            self->silenceFrames += inNumberFrames;
            if (self->silenceFrames > self->sampleRate) { // If we waited for more than 1 second with silence, stop RemoteIO to save battery.
                self->silenceFrames = 0;
                [self beginInterruption];
            };
        } else self->silenceFrames = 0;
    } else self->silenceFrames = 0;

	return noErr;
}

- (id)initWithDelegate:(NSObject<SuperpoweredIOSAudioIODelegate> *)d preferredBufferSize:(unsigned int)samples audioSessionCategory:(NSString *)category multiRouteChannels:(int)channels fixReceiver:(bool)fr {
    self = [super init];
    if (self) {
        iOS6 = ([[[UIDevice currentDevice] systemVersion] compare:@"6.0" options:NSNumericSearch] != NSOrderedAscending);
        multiRouteDeviceConnected = false;
        multiRouteChannels = !iOS6 ? 2 : channels;
#if !__has_feature(objc_arc)
        audioSessionCategory = [category retain];
#else
        audioSessionCategory = category;
#endif
        remoteIOChannels = 2;
        preferredBufferSizeSamples = samples;
        inputEnabled = changingSamplerate = waitingForReset = false;
        delegate = d;
        fixReceiver = fr;

        outputsAndInputs = [[NSMutableString alloc] initWithCapacity:256];
        silenceFrames = 0;
        samplerate48000 = background = audioUnitRunning = false;
        sampleRate = 44100;
        multiRouteDeviceName = nil;
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
            s.delegate = (id<AVAudioSessionDelegate>)self;
        };
    };
    return self;
}

- (void)dealloc {
    if (audioUnit != NULL) {
        AudioUnitUninitialize(audioUnit);
        AudioComponentInstanceDispose(audioUnit);
    };
    
    [[AVAudioSession sharedInstance] setActive:NO error:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
#if !__has_feature(objc_arc)
    [audioSessionCategory release];
    [outputsAndInputs release];
    [multiRouteDeviceName release];
    [super dealloc];
#endif
}

- (void)resetStart {
    [self resetAudioSession];
    [self performSelector:@selector(start) withObject:nil afterDelay:1.0]; // Give iOS 1 second for the new audio session, we may see some glitches otherwise with some USB devices.
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

- (void)onAudioSessionInterrupted:(NSNotification *)notification {
    NSNumber *interruption = [notification.userInfo objectForKey:AVAudioSessionInterruptionTypeKey];
    if (interruption) switch ([interruption intValue]) {
        case AVAudioSessionInterruptionTypeBegan: [self beginInterruption]; break;
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
    
    bool _multiRouteDeviceConnected = false, receiver = false;
    for (AVAudioSessionPortDescription *port in [[[AVAudioSession sharedInstance] currentRoute] outputs]) {
        if ([port.portType isEqualToString:AVAudioSessionPortUSBAudio] || [port.portType isEqualToString:AVAudioSessionPortHDMI]) _multiRouteDeviceConnected = true;
        else if ([port.portType isEqualToString:AVAudioSessionPortBuiltInReceiver]) receiver = true;
    };
    
    if (fixReceiver && receiver && !_multiRouteDeviceConnected) {
        [[AVAudioSession sharedInstance] overrideOutputAudioPort:AVAudioSessionPortOverrideSpeaker error:nil];
        [self performSelectorOnMainThread:@selector(onRouteChange:) withObject:nil waitUntilDone:NO];
        return;
    };
   
    // Switch to multiroute category.
    if ((multiRouteChannels > 2) && (_multiRouteDeviceConnected != multiRouteDeviceConnected)) {
        multiRouteDeviceConnected = _multiRouteDeviceConnected;
        
        if (audioUnit) {
            AudioOutputUnitStop(audioUnit);
            AudioUnitUninitialize(audioUnit);
            AudioComponentInstanceDispose(audioUnit);
            audioUnit = NULL;
        };
        audioUnitRunning = false;
        [[AVAudioSession sharedInstance] setActive:NO error:nil];
        
        // Something new is connected. Let's try 44100 Hz to save battery and CPU.
        // Unless you are a bat and require 48 kHz. Don't tell me you hear the difference in a scientifically correct A/B test.
        samplerate48000 = false;
        sampleRate = 44100;
        
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
    [multiRouteDeviceName release];
#endif
    outputsAndInputs = [[NSMutableString alloc] initWithCapacity:256];
    [outputsAndInputs appendString:@"Outputs: "];
    multiRouteDeviceName = nil;
    
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
            [multiRouteDeviceName release];
            multiRouteDeviceName = [port.portName retain];
#else
            multiRouteDeviceName = port.portName;
#endif
            
        } else if (type == audioDeviceType_USB) { // iOS can handle one USB audio device only
            outputChannelMap.numberOfUSBChannelsAvailable = channels;
#if !__has_feature(objc_arc)
            [multiRouteDeviceName release];
            multiRouteDeviceName = [port.portName retain];
#else
            multiRouteDeviceName = port.portName;
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
    
    [self multiRouteRemapChannels];
    // Maximize system volume for multiroute audio devices.
    if (_multiRouteDeviceConnected) [[MPMusicPlayerController applicationMusicPlayer] setVolume:1.0f];
}

- (void)resetAudioSession {
    // We don't check AVAudioSession errors here, because these are more like wishes than properties.
    remoteIOChannels = multiRouteDeviceConnected ? multiRouteChannels : 2;
    [[AVAudioSession sharedInstance] setCategory:multiRouteDeviceConnected ? AVAudioSessionCategoryMultiRoute : audioSessionCategory error:NULL];
    [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeDefault error:NULL];
    if (!iOS6) [[AVAudioSession sharedInstance] setPreferredHardwareSampleRate:44100.0 error:NULL];
    else [[AVAudioSession sharedInstance] setPreferredSampleRate:44100.0 error:NULL];
    [[AVAudioSession sharedInstance] setPreferredIOBufferDuration:double(preferredBufferSizeSamples) / 44100.0 error:NULL];
    [[AVAudioSession sharedInstance] setActive:YES error:NULL];
    [self recreateRemoteIO];
    waitingForReset = false;
    if (iOS6) [self onRouteChange:nil]; else [self multiRouteRemapChannels];
}

- (void)applyBufferSize {
    if (![NSThread isMainThread]) [self performSelectorOnMainThread:@selector(applyBufferSize) withObject:nil waitUntilDone:NO];
    else {
        if (!iOS6) {
            if ([[AVAudioSession sharedInstance] preferredHardwareSampleRate] != sampleRate) [[AVAudioSession sharedInstance] setPreferredHardwareSampleRate:sampleRate error:NULL];
        } else {
            if ([[AVAudioSession sharedInstance] preferredSampleRate] != sampleRate) [[AVAudioSession sharedInstance] setPreferredSampleRate:sampleRate error:nil];
        };
        
        int currentBufferSizeSamples = (int) round([[AVAudioSession sharedInstance] preferredIOBufferDuration] * NSTimeInterval(sampleRate));
        if (currentBufferSizeSamples != preferredBufferSizeSamples) [[AVAudioSession sharedInstance] setPreferredIOBufferDuration:double(preferredBufferSizeSamples) / double(sampleRate) error:nil];
    };
}

- (void)changeSamplerate { // Changing sample rate requires a new RemoteIO unit.
    if (audioUnit) AudioOutputUnitStop(audioUnit);
    changingSamplerate = false;
    [self recreateRemoteIO];
    [self start];
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
    
    AudioStreamBasicDescription format;
	format.mSampleRate = sampleRate;
	format.mFormatID = kAudioFormatLinearPCM;
	format.mFormatFlags = (kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved | kAudioFormatFlagsNativeEndian);
    format.mBitsPerChannel = 32;
	format.mFramesPerPacket = 1;
    format.mBytesPerFrame = 4;
    format.mBytesPerPacket = 4;
    format.mChannelsPerFrame = remoteIOChannels;
    if (AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, sizeof(format))) { AudioComponentInstanceDispose(au); return NULL; };
    if (AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &format, sizeof(format))) { AudioComponentInstanceDispose(au); return NULL; };
    
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
    [self applyBufferSize];
    if (AudioOutputUnitStart(audioUnit)) return false;
    audioUnitRunning = true;
    return true;
}

- (void)stop {
    if (audioUnit != NULL) AudioOutputUnitStop(audioUnit);
}

- (void)multiRouteRemapChannels {
    if (multiRouteChannels < 3) {
        if (!iOS6) [delegate multiRouteMapChannels:&outputChannelMap inputMap:&inputChannelMap multiRouteDeviceName:multiRouteDeviceName outputsAndInputs:outputsAndInputs];
        return;
    };
    for (int n = 0; n < 32; n++) AUOutputChannelMap[n] = -1;
    
    if (multiRouteChannels > 2) [delegate multiRouteMapChannels:&outputChannelMap inputMap:&inputChannelMap multiRouteDeviceName:multiRouteDeviceName outputsAndInputs:outputsAndInputs];

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

    if (audioUnit && iOS6) AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Output, 0, AUOutputChannelMap, sizeof(AUOutputChannelMap));
    
    for (int n = 0; n < 32; n++) AUInputChannelMap[n] = inputChannelMap.USBChannels[n];

    if (audioUnit && iOS6) AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 1, AUInputChannelMap, sizeof(AUInputChannelMap));
}

- (void)setInputEnabled:(bool)ie {
    if (inputEnabled != ie) {
        self->inputEnabled = ie;
        if (audioUnit) {
            AudioUnit oldUnit = audioUnit;
            
            audioUnit = [self createRemoteIO];
            if (multiRouteDeviceConnected && iOS6) {
                AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Output, 0, AUOutputChannelMap, sizeof(AUOutputChannelMap));
                AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 1, AUInputChannelMap, sizeof(AUInputChannelMap));
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

- (void)setPreferredBufferSizeSamples:(int)samples {
    preferredBufferSizeSamples = samples;
    if (audioUnitRunning) [self applyBufferSize];
}

@end
