#import "SuperpoweredtvOSAudioIO.h"
#import <AVFoundation/AVAudioSession.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <UIKit/UIKit.h>

// Initialization
@implementation SuperpoweredtvOSAudioIO {
    id<SuperpoweredtvOSAudioIODelegate>delegate;
    NSString *audioSessionCategory;
    audioProcessingCallback_C processingCallback;
    void *processingClientdata;
    AudioComponentInstance audioUnit;
    int numChannels, samplerate, preferredMinimumSamplerate;
    bool audioUnitRunning, background;
}

@synthesize preferredBufferSizeMs;

- (id)initWithDelegate:(NSObject<SuperpoweredtvOSAudioIODelegate> *)d preferredBufferSize:(unsigned int)preferredBufferSize preferredMinimumSamplerate:(unsigned int)prefsamplerate audioSessionCategory:(NSString *)category channels:(int)channels {
    self = [super init];
    if (self) {
        numChannels = channels;
#if !__has_feature(objc_arc)
        audioSessionCategory = [category retain];
#else
        audioSessionCategory = category;
#endif
        preferredBufferSizeMs = preferredBufferSize;
        preferredMinimumSamplerate = prefsamplerate;
        processingCallback = NULL;
        processingClientdata = NULL;
        delegate = d;
        background = audioUnitRunning = false;
        samplerate = 0;
        audioUnit = NULL;

        [self resetAudio];
        
        // Need to listen for a few app and audio session related events.
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onForeground) name:UIApplicationWillEnterForegroundNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onBackground) name:UIApplicationDidEnterBackgroundNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onMediaServerReset:) name:AVAudioSessionMediaServicesWereResetNotification object:[AVAudioSession sharedInstance]];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onAudioSessionInterrupted:) name:AVAudioSessionInterruptionNotification object:[AVAudioSession sharedInstance]];
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

- (void)beginInterruption {
    if (![NSThread isMainThread]) [self performSelectorOnMainThread:@selector(beginInterruption) withObject:nil waitUntilDone:NO];
    else {
        audioUnitRunning = false;
        if (audioUnit) AudioOutputUnitStop(audioUnit);
    };
}

- (void)endInterruption {
    if (audioUnitRunning) return;
    if (![NSThread isMainThread]) [self performSelectorOnMainThread:@selector(endInterruption) withObject:nil waitUntilDone:NO];
    else for (int n = 0; n < 2; n++) if ([[AVAudioSession sharedInstance] setActive:YES error:nil] && [self start]) { // Need to try twice sometimes. Don't know why.
        [delegate interruptionEnded];
        audioUnitRunning = true;
        break;
    };
 }

- (void)startDelegateInterruption {
    [delegate interruptionStarted];
}

- (void)onAudioSessionInterrupted:(NSNotification *)notification {
    NSNumber *interruption = [notification.userInfo objectForKey:AVAudioSessionInterruptionTypeKey];
    if (interruption) switch ([interruption intValue]) {
        case AVAudioSessionInterruptionTypeBegan:
            if (audioUnitRunning) [self performSelectorOnMainThread:@selector(startDelegateInterruption) withObject:nil waitUntilDone:NO];
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

// Audio Session
- (void)setSamplerateAndBuffersize {
    if (samplerate > 0) {
        double sr = samplerate < preferredMinimumSamplerate ? preferredMinimumSamplerate : 0, current = [[AVAudioSession sharedInstance] preferredSampleRate];
        if (current != sr) [[AVAudioSession sharedInstance] setPreferredSampleRate:sr error:NULL];
    };
    [[AVAudioSession sharedInstance] setPreferredIOBufferDuration:double(preferredBufferSizeMs) * 0.001 error:NULL];
}

- (void)resetAudio {
    if (audioUnit != NULL) {
        AudioUnitUninitialize(audioUnit);
        AudioComponentInstanceDispose(audioUnit);
        audioUnit = NULL;
    };

    [[AVAudioSession sharedInstance] setCategory:audioSessionCategory error:NULL];
    [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeDefault error:NULL];
    [self setSamplerateAndBuffersize];
    [[AVAudioSession sharedInstance] setActive:YES error:NULL];

    audioUnit = [self createRemoteIO];
}

// RemoteIO
static void streamFormatChangedCallback(void *inRefCon, AudioUnit inUnit, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement) {
    if ((inScope == kAudioUnitScope_Output) && (inElement == 0)) {
        UInt32 size = 0;
        AudioUnitGetPropertyInfo(inUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &size, NULL);
        AudioStreamBasicDescription format;
        AudioUnitGetProperty(inUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &format, &size);
        SuperpoweredtvOSAudioIO *self = (__bridge SuperpoweredtvOSAudioIO *)inRefCon;
        self->samplerate = (int)format.mSampleRate;
        [self performSelectorOnMainThread:@selector(setSamplerateAndBuffersize) withObject:nil waitUntilDone:NO];
    };
}

static OSStatus audioProcessingCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    __unsafe_unretained SuperpoweredtvOSAudioIO *self = (__bridge SuperpoweredtvOSAudioIO *)inRefCon;

    div_t d = div(inNumberFrames, 8);
    if ((d.rem != 0) || (inNumberFrames < 32) || (inNumberFrames > 512) || (ioData->mNumberBuffers != self->numChannels)) {
        return kAudioUnitErr_InvalidParameter;
    };

    float *bufs[self->numChannels];
    for (int n = 0; n < self->numChannels; n++) bufs[n] = (float *)ioData->mBuffers[n].mData;
    bool silence = true;

    // Make audio output.
    if (self->processingCallback) silence = !self->processingCallback(self->processingClientdata, bufs, self->numChannels, inNumberFrames, self->samplerate, inTimeStamp->mHostTime);
    else silence = ![self->delegate audioProcessingCallback:bufs outputChannels:self->numChannels numberOfSamples:inNumberFrames samplerate:self->samplerate hostTime:inTimeStamp->mHostTime];

    if (silence) { // Despite of ioActionFlags, it outputs garbage sometimes, so must zero the buffers:
        *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
        for (unsigned char n = 0; n < ioData->mNumberBuffers; n++) memset(ioData->mBuffers[n].mData, 0, ioData->mBuffers[n].mDataByteSize);
    };
    
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

	UInt32 value = 1;
	if (AudioUnitSetProperty(au, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &value, sizeof(value))) { AudioComponentInstanceDispose(au); return NULL; };

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
	callbackStruct.inputProc = audioProcessingCallback;
	callbackStruct.inputProcRefCon = (__bridge void *)self;
	if (AudioUnitSetProperty(au, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callbackStruct, sizeof(callbackStruct))) { AudioComponentInstanceDispose(au); return NULL; };
    
	AudioUnitInitialize(au);
    return au;
}

// Public methods
- (bool)start {
    if (audioUnit == NULL) return false;
    if (AudioOutputUnitStart(audioUnit)) return false;
    audioUnitRunning = true;
    return true;
}

- (void)stop {
    if (audioUnit != NULL) AudioOutputUnitStop(audioUnit);
}

- (void)setPreferredBufferSizeMs:(int)ms {
    preferredBufferSizeMs = ms;
    [self setSamplerateAndBuffersize];
}

- (void)reconfigureWithAudioSessionCategory:(NSString *)category {
    if (![NSThread isMainThread]) {
        [self performSelectorOnMainThread:@selector(reconfigureWithAudioSessionCategory:) withObject:category waitUntilDone:NO];
        return;
    };
#if !__has_feature(objc_arc)
    [audioSessionCategory release], audioSessionCategory = [category retain];
#else
    audioSessionCategory = category;
#endif
    [self onMediaServerReset:nil];
}

@end
