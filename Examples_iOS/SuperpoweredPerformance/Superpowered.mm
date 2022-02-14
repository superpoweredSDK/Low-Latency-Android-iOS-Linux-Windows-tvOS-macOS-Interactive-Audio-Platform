#import "SuperpoweredClass.h"
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

// This is a .mm file, meaning it's Objective-C++. You can perfectly mix it with Objective-C or Swift, until you keep the member variables and C++ related includes here.
@implementation SuperpoweredAudio {
    Superpowered::AdvancedAudioPlayer *player;
    Superpowered::FX *effects[NUMFXUNITS];
    SuperpoweredIOSAudioIO *output;
    bool started;
    uint64_t timeUnitsProcessed, maxTime;
    unsigned int lastPositionSeconds, lastSamplerate, framesProcessed;
}

- (void)dealloc {
    delete player;
    for (int n = 2; n < NUMFXUNITS; n++) delete effects[n];
#if !__has_feature(objc_arc)
    [output release];
    [super dealloc];
#endif
}

// Called periodically by ViewController to update the user interface.
- (void)updatePlayerLabel:(UILabel *)label slider:(UISlider *)slider button:(UIButton *)button {
    if (player->getLatestEvent() == Superpowered::AdvancedAudioPlayer::PlayerEvent_Opened) {
        player->play();
        player->originalBPM = 124.0f;
    }
    
    bool tracking = slider.tracking;
    unsigned int positionSeconds = tracking ? int(float(player->getDurationSeconds()) * slider.value) : player->getDisplayPositionSeconds();
    
    if (lastPositionSeconds != positionSeconds) {
        lastPositionSeconds = positionSeconds;
        NSString *str = [[NSString alloc] initWithFormat:@"%02d:%02d %02d:%02d", player->getDurationSeconds() / 60, player->getDurationSeconds() % 60, positionSeconds / 60, positionSeconds % 60];
        label.text = str;
#if !__has_feature(objc_arc)
        [str release];
#endif
    };

    if (!button.tracking && (button.selected != player->isPlaying())) button.selected = player->isPlaying();
    if (!tracking && (slider.value != player->getDisplayPositionPercent())) slider.value = player->getDisplayPositionPercent();
}

- (bool)toggleFx:(int)index {
    if (index == TIMEPITCHINDEX) {
        player->playbackRate = (player->playbackRate != 1.0f) ? 1.0f : 1.1f;
        return (player->playbackRate == 1.0f);
    } else if (index == PITCHSHIFTINDEX) {
        player->pitchShiftCents = (player->pitchShiftCents == 0) ? 100 : 0;
        return (player->pitchShiftCents == 0);
    } else {
        effects[index]->enabled = !effects[index]->enabled;
        return effects[index]->enabled;
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

// This is where the Superpowered magic happens.
static bool audioProcessing(void *clientdata, float *input, float *output, unsigned int numberOfFrames, unsigned int samplerate, uint64_t hostTime) {
    __unsafe_unretained SuperpoweredAudio *self = (__bridge SuperpoweredAudio *)clientdata;
    uint64_t startTime = mach_absolute_time();

    if (self->lastSamplerate != samplerate) { // Has samplerate changed?
        self->lastSamplerate = samplerate;
        self->player->outputSamplerate = samplerate;
        for (int n = 2; n < NUMFXUNITS; n++) self->effects[n]->samplerate = samplerate;
    };

    // We're keeping our Superpowered time-based effects in sync with the player... with one line of code. Not bad, eh?
    ((Superpowered::Roll *)self->effects[ROLLINDEX])->bpm = ((Superpowered::Flanger *)self->effects[FLANGERINDEX])->bpm = ((Superpowered::Echo *)self->effects[DELAYINDEX])->bpm = self->player->getCurrentBpm();

    // Let's process some audio. If you'd like to change connections or tap into something, no abstract connection handling and no callbacks required!
    bool silence = !self->player->processStereo(output, false, numberOfFrames);
    if (self->effects[ROLLINDEX]->process(silence ? NULL : output, output, numberOfFrames)) silence = false;
    self->effects[FILTERINDEX]->process(output, output, numberOfFrames);
    self->effects[EQINDEX]->process(output, output, numberOfFrames);
    self->effects[FLANGERINDEX]->process(output, output, numberOfFrames);
    if (self->effects[DELAYINDEX]->process(silence ? NULL : output, output, numberOfFrames)) silence = false;
    if (self->effects[REVERBINDEX]->process(silence ? NULL : output, output, numberOfFrames)) silence = false;

    // CPU measurement code to show some nice numbers for the business guys.
    uint64_t elapsedUnits = mach_absolute_time() - startTime;
    if (elapsedUnits > self->maxTime) self->maxTime = elapsedUnits;
    self->timeUnitsProcessed += elapsedUnits;
    self->framesProcessed += numberOfFrames;
    if (self->framesProcessed >= samplerate) {
        self->avgUnitsPerSecond = self->timeUnitsProcessed;
        self->maxUnitsPerSecond = (double(samplerate) / double(numberOfFrames)) * self->maxTime;
        self->framesProcessed = self->timeUnitsProcessed = self->maxTime = 0;
    };

    self->playing = self->player->isPlaying();
    return !silence;
}

- (id)init {
    self = [super init];
    if (!self) return nil;
    
    Superpowered::Initialize("ExampleLicenseKey-WillExpire-OnNextUpdate");
    SuperpoweredFFTTest();

    started = false;
    lastPositionSeconds = lastSamplerate = framesProcessed = timeUnitsProcessed = maxTime = avgUnitsPerSecond = maxUnitsPerSecond = 0;

    // Creating the Superpowered features we'll use.
    player = new Superpowered::AdvancedAudioPlayer(44100, 0);
    player->open([[[NSBundle mainBundle] pathForResource:@"track" ofType:@"mp3"] fileSystemRepresentation]);
    
    Superpowered::Filter *filter = new Superpowered::Filter(Superpowered::Filter::Resonant_Lowpass, 44100);
    filter->frequency = 1000.0f;
    filter->resonance = 0.1f;
    effects[FILTERINDEX] = filter;
    
    effects[ROLLINDEX] = new Superpowered::Roll(44100);
    effects[FLANGERINDEX] = new Superpowered::Flanger(44100);
    
    Superpowered::Echo *delay = new Superpowered::Echo(44100);
    delay->setMix(0.8f);
    effects[DELAYINDEX] = delay;
    
    Superpowered::Reverb *reverb = new Superpowered::Reverb(44100);
    reverb->roomSize = 0.5f;
    reverb->mix = 0.3f;
    effects[REVERBINDEX] = reverb;
    
    Superpowered::ThreeBandEQ *eq = new Superpowered::ThreeBandEQ(44100);
    eq->low = 2.0f;
    eq->mid = 0.5f;
    eq->high = 2.0f;
    effects[EQINDEX] = eq;

    output = [[SuperpoweredIOSAudioIO alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:12 preferredSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryPlayback channels:2 audioProcessingCallback:audioProcessing clientdata:(__bridge void *)self];
    return self;
}

@end
