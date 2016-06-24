#import "Superpowered.h"
#import "SuperpoweredAdvancedAudioPlayer.h"
#import "SuperpoweredReverb.h"
#import "SuperpoweredFilter.h"
#import "Superpowered3BandEQ.h"
#import "SuperpoweredEcho.h"
#import "SuperpoweredRoll.h"
#import "SuperpoweredFlanger.h"
#import "SuperpoweredSimple.h"
#import "SuperpoweredIOSAudioIO.h"
#import "fftTest.h"
#import <mach/mach_time.h>

/*
 This is a .mm file, meaning it's Objective-C++. 
 You can perfectly mix it with Objective-C or Swift, until you keep the member variables and C++ related includes here.
 Yes, the header file (.h) isn't the only place for member variables.
 */
@implementation Superpowered {
    SuperpoweredAdvancedAudioPlayer *player;
    SuperpoweredFX *effects[NUMFXUNITS];
    SuperpoweredIOSAudioIO *output;
    float *stereoBuffer;
    bool started;
    uint64_t timeUnitsProcessed, maxTime;
    unsigned int lastPositionSeconds, lastSamplerate, samplesProcessed;
}

- (void)dealloc {
    delete player;
    for (int n = 2; n < NUMFXUNITS; n++) delete effects[n];
    free(stereoBuffer);
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

- (void)interruptionStarted {}
- (void)recordPermissionRefused {}
- (void)mapChannels:(multiOutputChannelMap *)outputMap inputMap:(multiInputChannelMap *)inputMap externalAudioDeviceName:(NSString *)externalAudioDeviceName outputsAndInputs:(NSString *)outputsAndInputs {}

- (void)interruptionEnded {
    player->onMediaserverInterrupt(); // If the player plays Apple Lossless audio files, then we need this. Otherwise unnecessary.
}

// This is where the Superpowered magic happens.
static bool audioProcessing(void *clientdata, float **buffers, unsigned int inputChannels, unsigned int outputChannels, unsigned int numberOfSamples, unsigned int samplerate, uint64_t hostTime) {
    __unsafe_unretained Superpowered *self = (__bridge Superpowered *)clientdata;
    uint64_t startTime = mach_absolute_time();

    if (samplerate != self->lastSamplerate) { // Has samplerate changed?
        self->lastSamplerate = samplerate;
        self->player->setSamplerate(samplerate);
        for (int n = 2; n < NUMFXUNITS; n++) self->effects[n]->setSamplerate(samplerate);
    };

    // We're keeping our Superpowered time-based effects in sync with the player... with one line of code. Not bad, eh?
    ((SuperpoweredRoll *)self->effects[ROLLINDEX])->bpm = ((SuperpoweredFlanger *)self->effects[FLANGERINDEX])->bpm = ((SuperpoweredEcho *)self->effects[DELAYINDEX])->bpm = self->player->currentBpm;

    /*
     Let's process some audio.
     If you'd like to change connections or tap into something, no abstract connection handling and no callbacks required!
     */
    bool silence = !self->player->process(self->stereoBuffer, false, numberOfSamples, 1.0f, 0.0f, -1.0);
    if (self->effects[ROLLINDEX]->process(silence ? NULL : self->stereoBuffer, self->stereoBuffer, numberOfSamples)) silence = false;
    self->effects[FILTERINDEX]->process(self->stereoBuffer, self->stereoBuffer, numberOfSamples);
    self->effects[EQINDEX]->process(self->stereoBuffer, self->stereoBuffer, numberOfSamples);
    self->effects[FLANGERINDEX]->process(self->stereoBuffer, self->stereoBuffer, numberOfSamples);
    if (self->effects[DELAYINDEX]->process(silence ? NULL : self->stereoBuffer, self->stereoBuffer, numberOfSamples)) silence = false;
    if (self->effects[REVERBINDEX]->process(silence ? NULL : self->stereoBuffer, self->stereoBuffer, numberOfSamples)) silence = false;

    // CPU measurement code to show some nice numbers for the business guys.
    uint64_t elapsedUnits = mach_absolute_time() - startTime;
    if (elapsedUnits > self->maxTime) self->maxTime = elapsedUnits;
    self->timeUnitsProcessed += elapsedUnits;
    self->samplesProcessed += numberOfSamples;
    if (self->samplesProcessed >= samplerate) {
        self->avgUnitsPerSecond = self->timeUnitsProcessed;
        self->maxUnitsPerSecond = (double(samplerate) / double(numberOfSamples)) * self->maxTime;
        self->samplesProcessed = self->timeUnitsProcessed = self->maxTime = 0;
    };

    self->playing = self->player->playing;
    if (!silence) SuperpoweredDeInterleave(self->stereoBuffer, buffers[0], buffers[1], numberOfSamples); // The stereoBuffer is ready now, let's put the finished audio into the requested buffers.
    return !silence;
}

- (id)init {
    self = [super init];
    if (!self) return nil;
    SuperpoweredFFTTest();

    started = false;
    lastPositionSeconds = lastSamplerate = samplesProcessed = timeUnitsProcessed = maxTime = avgUnitsPerSecond = maxUnitsPerSecond = 0;
    if (posix_memalign((void **)&stereoBuffer, 16, 4096 + 128) != 0) abort(); // Allocating memory, aligned to 16.

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
    eq->bands[0] = 2.0f;
    eq->bands[1] = 0.5f;
    eq->bands[2] = 2.0f;
    effects[EQINDEX] = eq;

    output = [[SuperpoweredIOSAudioIO alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:12 preferredMinimumSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryPlayback channels:2 audioProcessingCallback:audioProcessing clientdata:(__bridge void *)self];
    return self;
}

@end
