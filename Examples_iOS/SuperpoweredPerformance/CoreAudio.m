#import "CoreAudio.h"
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <mach/mach_time.h>

@implementation CoreAudio { 
    AUGraph graph;
    AudioUnit filePlayerAU, outputAU, fxUnits[NUMFXUNITS];
    AUNode filePlayerNode, outputNode, fxNodes[NUMFXUNITS];
    AudioFileID audioFile;
    ScheduledAudioFileRegion region;
    bool fxEnabled[NUMFXUNITS], started;
    int durationSeconds, durationSamples, lastPositionSeconds;
    uint64_t outputUnitProcessingStart, timeUnitsProcessed, maxTime;
    unsigned int samplerate, samplesProcessed;
}

- (void)dealloc {
    DisposeAUGraph(graph);
    AudioFileClose(audioFile);
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

// Called periodically by ViewController to update the user interface.
- (void)updatePlayerLabel:(UILabel *)label slider:(UISlider *)slider button:(UIButton *)button {
    bool tracking = slider.tracking;
    int positionSeconds = 0;
    float positionPercent = 0.0f;
    AudioTimeStamp timestamp;
    UInt32 size = sizeof(timestamp);
    
    if (!AudioUnitGetProperty(filePlayerAU, kAudioUnitProperty_CurrentPlayTime, kAudioUnitScope_Global, 0, &timestamp, &size)) {
        positionPercent = ((float)timestamp.mSampleTime + region.mStartFrame) / ((float)(durationSamples));
        positionSeconds = tracking ? (int)(((float)durationSeconds) * slider.value) : (int)(positionPercent * ((float)durationSeconds));
    };
    
    if (positionSeconds != lastPositionSeconds) {
        lastPositionSeconds = positionSeconds;
        NSString *str = [[NSString alloc] initWithFormat:@"%02d:%02d %02d:%02d", durationSeconds / 60, durationSeconds % 60, positionSeconds / 60, positionSeconds % 60];
        label.text = str;
#if !__has_feature(objc_arc)
        [str release];
#endif
    };
    
    if (!button.tracking && (button.selected != playing)) button.selected = playing;
    if (!tracking && (slider.value != positionPercent)) slider.value = positionPercent;
}

- (void)togglePlayback {
    playing = !playing;
    
    if (!playing) {
        AudioTimeStamp timestamp; UInt32 size = sizeof(timestamp);
        if (!AudioUnitGetProperty(filePlayerAU, kAudioUnitProperty_CurrentPlayTime, kAudioUnitScope_Global, 0, &timestamp, &size)) region.mStartFrame += timestamp.mSampleTime;
        AudioUnitReset(filePlayerAU, kAudioUnitScope_Global, 0);
    } else {
        AudioUnitSetProperty(filePlayerAU, kAudioUnitProperty_ScheduledFileRegion, kAudioUnitScope_Global, 0, &region, sizeof(region));
        
        AudioTimeStamp startTime;
        memset(&startTime, 0, sizeof(startTime));
        startTime.mFlags = kAudioTimeStampSampleTimeValid;
        startTime.mSampleTime = -1;
        AudioUnitSetProperty(filePlayerAU, kAudioUnitProperty_ScheduleStartTimeStamp, kAudioUnitScope_Global, 0, &startTime, sizeof(startTime));
    };
}

- (void)seekTo:(float)percent {
    region.mStartFrame = (SInt64) ((float)region.mFramesToPlay * percent);
    
    if (playing) {
        AudioUnitReset(filePlayerAU, kAudioUnitScope_Global, 0);
        AudioUnitSetProperty(filePlayerAU, kAudioUnitProperty_ScheduledFileRegion, kAudioUnitScope_Global, 0, &region, sizeof(region));
        
        AudioTimeStamp startTime;
        memset(&startTime, 0, sizeof(startTime));
        startTime.mFlags = kAudioTimeStampSampleTimeValid;
        startTime.mSampleTime = -1;
        AudioUnitSetProperty(filePlayerAU, kAudioUnitProperty_ScheduleStartTimeStamp, kAudioUnitScope_Global, 0, &startTime, sizeof(startTime));
    };
}

static OSStatus audioUnitRenderNotify(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
    CoreAudio *self = (__bridge CoreAudio *)inRefCon;
    if (*ioActionFlags & kAudioUnitRenderAction_PreRender) self->outputUnitProcessingStart = mach_absolute_time();
    else if (*ioActionFlags & kAudioUnitRenderAction_PostRender) {
        uint64_t elapsedUnits = mach_absolute_time() - self->outputUnitProcessingStart;
        if (elapsedUnits > self->maxTime) self->maxTime = elapsedUnits;
        self->timeUnitsProcessed += elapsedUnits;
        self->samplesProcessed += inNumberFrames;
        if (self->samplesProcessed >= self->samplerate) {
            self->avgUnitsPerSecond = self->timeUnitsProcessed;
            self->maxUnitsPerSecond = (((double)self->samplerate) / ((double)inNumberFrames)) * ((double)self->maxTime);
            self->samplesProcessed = self->timeUnitsProcessed = self->maxTime = 0;
        };
    };
    return noErr;
}

static void samplerateChanged(void *inRefCon, AudioUnit inUnit, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement) {
    CoreAudio *self = (__bridge CoreAudio *)inRefCon;
    Float64 sr;
    UInt32 size = sizeof(sr);
    AudioUnitGetProperty(self->outputAU, kAudioUnitProperty_SampleRate, kAudioUnitScope_Global, 0, &sr, &size);
    self->samplerate = sr;
}

static void CheckResult(const char *msg, OSStatus result) {
    if (result) {
        printf("%s %i %.4s\n", msg, (int)result, (char *)&result);
        exit(0);
    };
}

// Initializing all of Core Audio functionality needs quite a bit of code.
// Note: we are using AUGraph here, which makes our lives a lot easier.
// Imagine how much code this would be without AUGraph!
- (id)init {
    self = [super init];
    if (!self) return nil;
    started = false;
    lastPositionSeconds = -1;
    avgUnitsPerSecond = timeUnitsProcessed = samplerate = samplesProcessed = maxTime = maxUnitsPerSecond = 0;
    
    CheckResult("NewAUGraph", NewAUGraph(&graph)); // Let's start building the graph.
    
    AudioComponentDescription filePlayerDesc;
	filePlayerDesc.componentType = kAudioUnitType_Generator;
	filePlayerDesc.componentSubType = kAudioUnitSubType_AudioFilePlayer;
	filePlayerDesc.componentFlags = 0;
    filePlayerDesc.componentFlagsMask = 0;
	filePlayerDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    AudioComponentDescription timePitchDesc;
    timePitchDesc.componentType = kAudioUnitType_FormatConverter;
    timePitchDesc.componentSubType = kAudioUnitSubType_NewTimePitch;
    timePitchDesc.componentFlags = 0;
    timePitchDesc.componentFlagsMask = 0;
    timePitchDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    AudioComponentDescription filterDesc;
    filterDesc.componentType = kAudioUnitType_Effect;
    filterDesc.componentSubType = kAudioUnitSubType_LowPassFilter;
    filterDesc.componentFlags = 0;
    filterDesc.componentFlagsMask = 0;
    filterDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    AudioComponentDescription eqDesc;
    eqDesc.componentType = kAudioUnitType_Effect;
    eqDesc.componentSubType = kAudioUnitSubType_NBandEQ;
    eqDesc.componentFlags = 0;
    eqDesc.componentFlagsMask = 0;
    eqDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    AudioComponentDescription delayDesc;
    delayDesc.componentType = kAudioUnitType_Effect;
    delayDesc.componentSubType = kAudioUnitSubType_Delay;
    delayDesc.componentFlags = 0;
    delayDesc.componentFlagsMask = 0;
    delayDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    AudioComponentDescription reverbDesc;
    reverbDesc.componentType = kAudioUnitType_Effect;
    reverbDesc.componentSubType = kAudioUnitSubType_Reverb2;
    reverbDesc.componentFlags = 0;
    reverbDesc.componentFlagsMask = 0;
    reverbDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    AudioComponentDescription outputDesc;
	outputDesc.componentType = kAudioUnitType_Output;
	outputDesc.componentSubType = kAudioUnitSubType_RemoteIO;
	outputDesc.componentFlags = 0;
    outputDesc.componentFlagsMask = 0;
	outputDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    CheckResult("add fileplayer node ", AUGraphAddNode(graph, &filePlayerDesc, &filePlayerNode));
    CheckResult("add timepitch node ", AUGraphAddNode(graph, &timePitchDesc, fxNodes + TIMEPITCHINDEX));
    CheckResult("add filter node ", AUGraphAddNode(graph, &filterDesc, fxNodes + FILTERINDEX));
    CheckResult("add eq node ", AUGraphAddNode(graph, &eqDesc, fxNodes + EQINDEX));
    CheckResult("add delay node ", AUGraphAddNode(graph, &delayDesc, fxNodes + DELAYINDEX));
    CheckResult("add reverb node", AUGraphAddNode(graph, &reverbDesc, fxNodes + REVERBINDEX));
    CheckResult("add output node", AUGraphAddNode(graph, &outputDesc, &outputNode));
    
    CheckResult("AUGraphOpen", AUGraphOpen(graph));
    
    CheckResult("fileplayer node info", AUGraphNodeInfo(graph, filePlayerNode, NULL, &filePlayerAU));
    CheckResult("timepitch node info", AUGraphNodeInfo(graph, fxNodes[TIMEPITCHINDEX], NULL, fxUnits + TIMEPITCHINDEX));
    CheckResult("filter node info", AUGraphNodeInfo(graph, fxNodes[FILTERINDEX], NULL, fxUnits + FILTERINDEX));
    CheckResult("eq node info", AUGraphNodeInfo(graph, fxNodes[EQINDEX], NULL, fxUnits + EQINDEX));
    CheckResult("delay node info", AUGraphNodeInfo(graph, fxNodes[DELAYINDEX], NULL, fxUnits + DELAYINDEX));
    CheckResult("reverb node info", AUGraphNodeInfo(graph, fxNodes[REVERBINDEX], NULL, fxUnits + REVERBINDEX));
    CheckResult("output node info", AUGraphNodeInfo(graph, outputNode, NULL, &outputAU));
    CheckResult("output render callback", AudioUnitAddRenderNotify(outputAU, audioUnitRenderNotify, (__bridge void *)self));
    CheckResult("output samplerate callback", AudioUnitAddPropertyListener(outputAU, kAudioUnitProperty_SampleRate, samplerateChanged, (__bridge void *)self));
    
    // Core Audio doesn't offer pitch-shifting, roll or flanger. Superpowered does.
    fxUnits[PITCHSHIFTINDEX] = fxUnits[ROLLINDEX] = fxUnits[FLANGERINDEX] = NULL;
    
    CheckResult("fileplayer connect", AUGraphConnectNodeInput(graph, filePlayerNode, 0, fxNodes[TIMEPITCHINDEX], 0));
    CheckResult("timepitch connect", AUGraphConnectNodeInput(graph, fxNodes[TIMEPITCHINDEX], 0, fxNodes[FILTERINDEX], 0));
    CheckResult("filter connect", AUGraphConnectNodeInput(graph, fxNodes[FILTERINDEX], 0, fxNodes[EQINDEX], 0));
    CheckResult("eq connect", AUGraphConnectNodeInput(graph, fxNodes[EQINDEX], 0, fxNodes[DELAYINDEX], 0));
    CheckResult("delay connect", AUGraphConnectNodeInput(graph, fxNodes[DELAYINDEX], 0, fxNodes[REVERBINDEX], 0));
    CheckResult("reverb connect", AUGraphConnectNodeInput(graph, fxNodes[REVERBINDEX], 0, outputNode, 0));
    CheckResult("AUGraphUpdate", AUGraphUpdate(graph, NULL));
    CheckResult("AUGraphInitialize", AUGraphInitialize(graph));
    
    Float64 sr;
    UInt32 size = sizeof(sr);
    AudioUnitGetProperty(self->outputAU, kAudioUnitProperty_SampleRate, kAudioUnitScope_Global, 0, &sr, &size);
    samplerate = sr;
    // The graph is ready now.
    
    // Some nice initial values for the fx units:
    CheckResult("timepitch rate", AudioUnitSetParameter(fxUnits[TIMEPITCHINDEX], kNewTimePitchParam_Rate, kAudioUnitScope_Global, 0, 1.1, 0));
    CheckResult("timepitch overlap", AudioUnitSetParameter(fxUnits[TIMEPITCHINDEX], kNewTimePitchParam_Overlap, kAudioUnitScope_Global, 0, 4, 0));
    CheckResult("timepitch peak locking", AudioUnitSetParameter(fxUnits[TIMEPITCHINDEX], kNewTimePitchParam_EnablePeakLocking, kAudioUnitScope_Global, 0, 1.0, 0));
    CheckResult("timepitch pitch", AudioUnitSetParameter(fxUnits[TIMEPITCHINDEX], kNewTimePitchParam_Pitch, kAudioUnitScope_Global, 0, 1, 0));
    CheckResult("lowpass freq", AudioUnitSetParameter(fxUnits[FILTERINDEX], kLowPassParam_CutoffFrequency, kAudioUnitScope_Global, 0, 1000, 0));
    CheckResult("delay delaytime", AudioUnitSetParameter(fxUnits[DELAYINDEX], kDelayParam_DelayTime, kAudioUnitScope_Global, 0, 0.242, 0));
    CheckResult("eq type 0", AudioUnitSetParameter(fxUnits[EQINDEX], kAUNBandEQParam_FilterType + 0, kAudioUnitScope_Global, 0, kAUNBandEQFilterType_LowShelf, 0));
    CheckResult("eq type 1", AudioUnitSetParameter(fxUnits[EQINDEX], kAUNBandEQParam_FilterType + 1, kAudioUnitScope_Global, 0, kAUNBandEQFilterType_Parametric, 0));
    CheckResult("eq type 2", AudioUnitSetParameter(fxUnits[EQINDEX], kAUNBandEQParam_FilterType + 2, kAudioUnitScope_Global, 0, kAUNBandEQFilterType_HighShelf, 0));
    CheckResult("eq freq 0", AudioUnitSetParameter(fxUnits[EQINDEX], kAUNBandEQParam_Frequency + 0, kAudioUnitScope_Global, 0, 100, 0));
    CheckResult("eq freq 1", AudioUnitSetParameter(fxUnits[EQINDEX], kAUNBandEQParam_Frequency + 1, kAudioUnitScope_Global, 0, 1000, 0));
    CheckResult("eq freq 2", AudioUnitSetParameter(fxUnits[EQINDEX], kAUNBandEQParam_Frequency + 2, kAudioUnitScope_Global, 0, 10000, 0));
    CheckResult("eq gain 0", AudioUnitSetParameter(fxUnits[EQINDEX], kAUNBandEQParam_Gain + 0, kAudioUnitScope_Global, 0, 6, 0));
    CheckResult("eq gain 1", AudioUnitSetParameter(fxUnits[EQINDEX], kAUNBandEQParam_Gain + 1, kAudioUnitScope_Global, 0, -6, 0));
    CheckResult("eq gain 2", AudioUnitSetParameter(fxUnits[EQINDEX], kAUNBandEQParam_Gain + 2, kAudioUnitScope_Global, 0, 6, 0));
    CheckResult("eq band 0", AudioUnitSetParameter(fxUnits[EQINDEX], kAUNBandEQParam_BypassBand + 0, kAudioUnitScope_Global, 0, 0, 0));
    CheckResult("eq band 1", AudioUnitSetParameter(fxUnits[EQINDEX], kAUNBandEQParam_BypassBand + 1, kAudioUnitScope_Global, 0, 0, 0));
    CheckResult("eq band 2", AudioUnitSetParameter(fxUnits[EQINDEX], kAUNBandEQParam_BypassBand + 2, kAudioUnitScope_Global, 0, 0, 0));
    CheckResult("eq bandwidth", AudioUnitSetParameter(fxUnits[EQINDEX], kAUNBandEQParam_Bandwidth + 1, kAudioUnitScope_Global, 0, 3, 0));
    CheckResult("reverb drywetmix", AudioUnitSetParameter(fxUnits[REVERBINDEX], kReverb2Param_DryWetMix, kAudioUnitScope_Global, 0, 50, 0));
    CheckResult("reverb decay", AudioUnitSetParameter(fxUnits[REVERBINDEX], kReverb2Param_DecayTimeAt0Hz, kAudioUnitScope_Global, 0, 1, 0));
    
    // Turn off all fx units.
    for (int n = 0; n < NUMFXUNITS; n++) if (fxUnits[n]) {
        fxEnabled[n] = true;
        [self toggleFx:n];
    };
    
    // Let's prepare the fileplayer to play something.
    CheckResult("AudioFileOpenURL", AudioFileOpenURL((__bridge CFURLRef)[[NSBundle mainBundle] URLForResource:@"track" withExtension:@"mp3"], kAudioFileReadPermission, 0, &audioFile));
    
    AudioStreamBasicDescription fileFormat;
    UInt64 numPackets;
	size = sizeof(AudioStreamBasicDescription);
    CheckResult("file data format", AudioFileGetProperty(audioFile, kAudioFilePropertyDataFormat, &size, &fileFormat));
    size = sizeof(numPackets);
    CheckResult("file packet count", AudioFileGetProperty(audioFile, kAudioFilePropertyAudioDataPacketCount, &size, &numPackets));
    Float64 duration;
    size = sizeof(duration);
    CheckResult("file duration", AudioFileGetProperty(audioFile, kAudioFilePropertyEstimatedDuration, &size, &duration));
    durationSeconds = floor(duration);
    durationSamples = floor(duration * fileFormat.mSampleRate);
    
    memset(&region.mTimeStamp, 0, sizeof(region.mTimeStamp));
    region.mTimeStamp.mFlags = kAudioTimeStampSampleTimeValid;
    region.mTimeStamp.mSampleTime = 0;
    region.mCompletionProc = NULL;
    region.mCompletionProcUserData = NULL;
    region.mAudioFile = audioFile;
    region.mLoopCount = 10000;
    region.mStartFrame = 0;
    region.mFramesToPlay = (UInt32) (numPackets * fileFormat.mFramesPerPacket);
    CheckResult("schedule file", AudioUnitSetProperty(filePlayerAU, kAudioUnitProperty_ScheduledFileIDs, kAudioUnitScope_Global, 0, &audioFile, sizeof(audioFile)));
    CheckResult("file region", AudioUnitSetProperty(filePlayerAU, kAudioUnitProperty_ScheduledFileRegion, kAudioUnitScope_Global, 0, &region, sizeof(region)));
    
    UInt32 defaultValue = 0;
    CheckResult("file prime", AudioUnitSetProperty(filePlayerAU, kAudioUnitProperty_ScheduledFilePrime, kAudioUnitScope_Global, 0, &defaultValue, sizeof(defaultValue)));
    
    AudioTimeStamp startTime;
    memset(&startTime, 0, sizeof(startTime));
    startTime.mFlags = kAudioTimeStampSampleTimeValid;
    startTime.mSampleTime = -1;
    CheckResult("start file", AudioUnitSetProperty(filePlayerAU, kAudioUnitProperty_ScheduleStartTimeStamp, kAudioUnitScope_Global, 0, &startTime, sizeof(startTime)));
    playing = true;
    
    return self;
}

- (void)toggle {
    if (started) AudioOutputUnitStop(outputAU); else AudioOutputUnitStart(outputAU);
    started = !started;
    if (playing) {
        AudioUnitReset(filePlayerAU, kAudioUnitScope_Global, 0);
        AudioUnitSetProperty(filePlayerAU, kAudioUnitProperty_ScheduledFileRegion, kAudioUnitScope_Global, 0, &region, sizeof(region));
        
        AudioTimeStamp startTime;
        memset(&startTime, 0, sizeof(startTime));
        startTime.mFlags = kAudioTimeStampSampleTimeValid;
        startTime.mSampleTime = -1;
        AudioUnitSetProperty(filePlayerAU, kAudioUnitProperty_ScheduleStartTimeStamp, kAudioUnitScope_Global, 0, &startTime, sizeof(startTime));
    };
}

- (bool)toggleFx:(int)index {
    if (fxUnits[index] == NULL) {
        fxEnabled[index] = false;
        return false;
    };
    fxEnabled[index] = !fxEnabled[index];
    if (index == REVERBINDEX) { // kAudioUnitProperty_BypassEffect doesn't work for the reverb audio unit, don't ask us why. Ask Apple.
        AUGraphDisconnectNodeInput(graph, fxNodes[REVERBINDEX], 0);
        AUGraphDisconnectNodeInput(graph, outputNode, 0);
        
        if (fxEnabled[index]) {
            CheckResult("enable reverb 0", AUGraphConnectNodeInput(graph, fxNodes[DELAYINDEX], 0, fxNodes[REVERBINDEX], 0));
            CheckResult("enable reverb 1", AUGraphConnectNodeInput(graph, fxNodes[REVERBINDEX], 0, outputNode, 0));
        } else {
            CheckResult("disable reverb", AUGraphConnectNodeInput(graph, fxNodes[DELAYINDEX], 0, outputNode, 0));
        };
        
        CheckResult("bypass AUGraphUpdate", AUGraphUpdate(graph, NULL));
    } else {
        UInt32 bypass = fxEnabled[index] ? 0 : 1;
        CheckResult("bypass", AudioUnitSetProperty(fxUnits[index], kAudioUnitProperty_BypassEffect, kAudioUnitScope_Global, 0, &bypass, sizeof(bypass)));
    };
    return fxEnabled[index];
}

@end
