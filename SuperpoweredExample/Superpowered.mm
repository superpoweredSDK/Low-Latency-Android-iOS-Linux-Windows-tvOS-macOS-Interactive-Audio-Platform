#import "Superpowered.h"
#import "SuperpoweredAdvancedAudioPlayer.h"
#import "SuperpoweredReverb.h"
#import "SuperpoweredFilter.h"
#import "Superpowered3BandEQ.h"
#import "SuperpoweredEcho.h"
#import "SuperpoweredRoll.h"
#import "SuperpoweredFlanger.h"
#import "SuperpoweredDeinterleave.h"
#import "SuperpoweredIOSAudioOutput.h"
#import <mach/mach_time.h>

@implementation Superpowered {
    SuperpoweredAdvancedAudioPlayer *player;
    SuperpoweredFX *effects[NUMFXUNITS];
    SuperpoweredIOSAudioOutput *output;
    float *stereoBuffer;
    void *unaligned;
    bool started;
    uint64_t timeUnitsProcessed, maxTime;
    unsigned int lastPositionSeconds, lastSamplerate, samplesProcessed;
}

- (void)dealloc {
    delete player;
    for (int n = 2; n < NUMFXUNITS; n++) delete effects[n];
    free(unaligned);
#if !__has_feature(objc_arc)
    [output release];
    [super dealloc];
#endif
}

// Called periodically by ViewController to update the user interface.
- (void)updatePlayerLabel:(UILabel *)label slider:(UISlider *)slider button:(UIButton *)button {
    bool tracking = slider.tracking;
    unsigned int positionSeconds = tracking ? int(float(player->durationSeconds) * slider.value) : player->positionSeconds;
    
    if (positionSeconds != lastPositionSeconds) {
        lastPositionSeconds = positionSeconds;
        NSString *str = [[NSString alloc] initWithFormat:@"%02d:%02d %02d:%02d", player->durationSeconds / 60, player->durationSeconds % 60, positionSeconds / 60, positionSeconds % 60];
        label.text = str;
#if !__has_feature(objc_arc)
        [str release];
#endif
    };

    if (!button.tracking && (button.selected != player->playing)) button.selected = player->playing;
    if (!tracking && (slider.value != player->positionPercent)) slider.value = player->positionPercent;
}

- (bool)toggleFx:(int)index {
    if (index == TIMEPITCHINDEX) {
        bool enabled = (player->tempo != 1.0f);
        player->setTempo(enabled ? 1.0f : 1.1f, true);
        return !enabled;
    } else if (index == PITCHSHIFTINDEX) {
        bool enabled = (player->pitchShift != 0);
        player->setPitchShift(enabled ? 0 : 1);
        return !enabled;
    } else {
        bool enabled = effects[index]->enabled;
        effects[index]->enable(!enabled);
        return !enabled;
    };
}

- (void)togglePlayback { // Play/pause.
    player->togglePlayback();
}

- (void)seekTo:(float)percent {
    player->seek(percent);
}

- (void)toggle {
    if (started) [output stop]; else [output start];
    started = !started;
}

- (void)interruptionEnded {
    player->onMediaserverInterrupt(); // If the player plays Apple Lossless audio files, then we need this. Otherwise unnecessary.
}

- (id)init {
    self = [super init];
    if (!self) return nil;
    started = false;
    lastPositionSeconds = lastSamplerate = samplesProcessed = timeUnitsProcessed = maxTime = avgUnitsPerSecond = maxUnitsPerSecond = 0;
    
    unaligned = malloc(4096 + 128 + 15);
    stereoBuffer = (float *)(((unsigned long)unaligned + 15) & (unsigned long)-16); // align to 16
    
// Create the Superpowered units we'll use.
    player = new SuperpoweredAdvancedAudioPlayer(NULL, NULL, 44100, 0);
    player->open([[[NSBundle mainBundle] pathForResource:@"track" ofType:@"mp3"] fileSystemRepresentation]);
    player->play(false);
    player->setBpm(124.0f);
    
    SuperpoweredFilter *filter = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Lowpass, 44100);
    filter->setResonantParameters(1000.0f, 0.1f);
    effects[FILTERINDEX] = filter;
    
    effects[ROLLINDEX] = new SuperpoweredRoll(44100);
    effects[FLANGERINDEX] = new SuperpoweredFlanger(44100);
    
    SuperpoweredEcho *delay = new SuperpoweredEcho(44100);
    delay->setMix(0.8f);
    effects[DELAYINDEX] = delay;
    
    SuperpoweredReverb *reverb = new SuperpoweredReverb(44100);
    reverb->setRoomSize(0.5f);
    reverb->setMix(0.3f);
    effects[REVERBINDEX] = reverb;
    
    Superpowered3BandEQ *eq = new Superpowered3BandEQ(44100);
    eq->lowGain = 2.0f;
    eq->midGain = 0.5f;
    eq->highGain = 2.0f;
    eq->adjust();
    effects[EQINDEX] = eq;
    
    output = [[SuperpoweredIOSAudioOutput alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:512 audioSessionCategory:AVAudioSessionCategoryPlayback multiRouteChannels:2 fixReceiver:true];
    return self;
}

// This is where the Superpowered magic happens.
- (bool)audioProcessingCallback:(float **)buffers inputChannels:(unsigned int)inputChannels outputChannels:(unsigned int)outputChannels numberOfSamples:(unsigned int)numberOfSamples samplerate:(unsigned int)samplerate hostTime:(UInt64)hostTime {
    uint64_t startTime = mach_absolute_time();
    
    if (samplerate != lastSamplerate) { // Has samplerate changed?
        lastSamplerate = samplerate;
        player->setSamplerate(samplerate);
        for (int n = 2; n < NUMFXUNITS; n++) effects[n]->setSamplerate(samplerate);
    };
    
// We're keeping our Superpowered time-based effects in sync with the player... with one line of code. Not bad, eh?
    ((SuperpoweredRoll *)effects[ROLLINDEX])->bpm = ((SuperpoweredFlanger *)effects[FLANGERINDEX])->bpm = ((SuperpoweredEcho *)effects[DELAYINDEX])->bpm = player->currentBpm;
    
/*
 Let's process some audio.
 If you'd like to change connections or tap into something, no abstract connection handling and no callbacks required!
*/
    bool silence = !player->process(stereoBuffer, false, numberOfSamples, 1.0f, 0.0f, -1.0);
    if (effects[ROLLINDEX]->process(silence ? NULL : stereoBuffer, stereoBuffer, numberOfSamples)) silence = false;
    effects[FILTERINDEX]->process(stereoBuffer, stereoBuffer, numberOfSamples);
    effects[EQINDEX]->process(stereoBuffer, stereoBuffer, numberOfSamples);
    effects[FLANGERINDEX]->process(stereoBuffer, stereoBuffer, numberOfSamples);
    if (effects[DELAYINDEX]->process(silence ? NULL : stereoBuffer, stereoBuffer, numberOfSamples)) silence = false;
    if (effects[REVERBINDEX]->process(silence ? NULL : stereoBuffer, stereoBuffer, numberOfSamples)) silence = false;
    
// The stereoBuffer is ready now, let's put the finished audio into the requested buffers.
    if (!silence) SuperpoweredDeinterleave(stereoBuffer, buffers[0], buffers[1], numberOfSamples);
    
// CPU measurement code to show some nice numbers for the business guys.
    uint64_t elapsedUnits = mach_absolute_time() - startTime;
    if (elapsedUnits > maxTime) maxTime = elapsedUnits;
    timeUnitsProcessed += elapsedUnits;
    samplesProcessed += numberOfSamples;
    if (samplesProcessed >= samplerate) {
        avgUnitsPerSecond = timeUnitsProcessed;
        maxUnitsPerSecond = (double(samplerate) / double(numberOfSamples)) * maxTime;
        samplesProcessed = timeUnitsProcessed = maxTime = 0;
    };
    
    playing = player->playing;
    return !silence;
}

@end
