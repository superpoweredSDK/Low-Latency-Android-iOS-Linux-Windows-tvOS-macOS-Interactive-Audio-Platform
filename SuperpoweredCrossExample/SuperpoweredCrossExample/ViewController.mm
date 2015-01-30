#import "ViewController.h"
#import "SuperpoweredAdvancedAudioPlayer.h"
#import "SuperpoweredFilter.h"
#import "SuperpoweredRoll.h"
#import "SuperpoweredFlanger.h"
#import "SuperpoweredIOSAudioOutput.h"
#import "SuperpoweredMixer.h"
#import <stdlib.h>
#import <pthread.h>

#define HEADROOM_DECIBEL 3.0f
static const float headroom = powf(10.0f, -HEADROOM_DECIBEL * 0.025);

/*
 This is a .mm file, meaning it's Objective-C++.
 You can perfectly mix it with Objective-C or Swift, until you keep the member variables and C++ related includes here.
 Yes, the header file (.h) isn't the only place for member variables.
 */
@implementation ViewController {
    SuperpoweredAdvancedAudioPlayer *playerA, *playerB;
    SuperpoweredIOSAudioOutput *output;
    SuperpoweredRoll *roll;
    SuperpoweredFilter *filter;
    SuperpoweredFlanger *flanger;
    SuperpoweredStereoMixer *mixer;
    unsigned char activeFx;
    float *stereoBuffer, crossValue, volA, volB;
    unsigned int lastSamplerate;
    pthread_mutex_t mutex;
}

void playerEventCallbackA(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event, void *value) {
    if (event == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) {
        ViewController *self = (__bridge ViewController *)clientData;
        self->playerA->setBpm(126.0f);
        self->playerA->setFirstBeatMs(353);
        self->playerA->setPosition(self->playerA->firstBeatMs, false, false);
    };
}

void playerEventCallbackB(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event, void *value) {
    if (event == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) {
        ViewController *self = (__bridge ViewController *)clientData;
        self->playerB->setBpm(123.0f);
        self->playerB->setFirstBeatMs(40);
        self->playerB->setPosition(self->playerB->firstBeatMs, false, false);
    };
}

- (void)viewDidLoad {
    [super viewDidLoad];
    lastSamplerate = activeFx = 0;
    crossValue = volB = 0.0f;
    volA = 1.0f * headroom;
    pthread_mutex_init(&mutex, NULL); // This will keep our player volumes and playback states in sync.
    if (posix_memalign((void **)&stereoBuffer, 16, 4096 + 128) != 0) abort(); // Allocating memory, aligned to 16.

    playerA = new SuperpoweredAdvancedAudioPlayer((__bridge void *)self, playerEventCallbackA, 44100, 0);
    playerA->open([[[NSBundle mainBundle] pathForResource:@"lycka" ofType:@"mp3"] fileSystemRepresentation]);
    playerB = new SuperpoweredAdvancedAudioPlayer((__bridge void *)self, playerEventCallbackB, 44100, 0);
    playerB->open([[[NSBundle mainBundle] pathForResource:@"nuyorica" ofType:@"m4a"] fileSystemRepresentation]);

    playerA->syncMode = playerB->syncMode = SuperpoweredAdvancedAudioPlayerSyncMode_TempoAndBeat;
    
    roll = new SuperpoweredRoll(44100);
    filter = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Lowpass, 44100);
    flanger = new SuperpoweredFlanger(44100);
    
    mixer = new SuperpoweredStereoMixer();
    
    output = [[SuperpoweredIOSAudioOutput alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:12 preferredMinimumSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryPlayback multiChannels:2 fixReceiver:true];
    [output start];
}

- (void)dealloc {
    delete playerA;
    delete playerB;
    delete mixer;
    free(stereoBuffer);
    pthread_mutex_destroy(&mutex);
#if !__has_feature(objc_arc)
    [output release];
    [super dealloc];
#endif
}

- (void)interruptionEnded { // If a player plays Apple Lossless audio files, then we need this. Otherwise unnecessary.
    playerA->onMediaserverInterrupt();
    playerB->onMediaserverInterrupt();
}

// This is where the Superpowered magic happens.
- (bool)audioProcessingCallback:(float **)buffers inputChannels:(unsigned int)inputChannels outputChannels:(unsigned int)outputChannels numberOfSamples:(unsigned int)numberOfSamples samplerate:(unsigned int)samplerate hostTime:(UInt64)hostTime {
    if (samplerate != lastSamplerate) { // Has samplerate changed?
        lastSamplerate = samplerate;
        playerA->setSamplerate(samplerate);
        playerB->setSamplerate(samplerate);
        roll->setSamplerate(samplerate);
        filter->setSamplerate(samplerate);
        flanger->setSamplerate(samplerate);
    };
    
    pthread_mutex_lock(&mutex);
    
    bool masterIsA = (crossValue <= 0.5f);
    float masterBpm = masterIsA ? playerA->currentBpm : playerB->currentBpm; // Players will sync to this tempo.
    double msElapsedSinceLastBeatA = playerA->msElapsedSinceLastBeat; // When playerB needs it, playerA has already stepped this value, so save it now.
    
    bool silence = !playerA->process(stereoBuffer, false, numberOfSamples, volA, masterBpm, playerB->msElapsedSinceLastBeat);
    if (playerB->process(stereoBuffer, !silence, numberOfSamples, volB, masterBpm, msElapsedSinceLastBeatA)) silence = false;

    roll->bpm = flanger->bpm = masterBpm; // Syncing fx is one line.

    if (roll->process(silence ? NULL : stereoBuffer, stereoBuffer, numberOfSamples) && silence) silence = false;
    if (!silence) {
        filter->process(stereoBuffer, stereoBuffer, numberOfSamples);
        flanger->process(stereoBuffer, stereoBuffer, numberOfSamples);
    };

    pthread_mutex_unlock(&mutex);

    // The stereoBuffer is ready now, let's put the finished audio into the requested buffers.
    float *mixerInputs[4] = { stereoBuffer, NULL, NULL, NULL };
    float mixerInputLevels[8] = { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float mixerOutputLevels[2] = { 1.0f, 1.0f };
    if (!silence) mixer->process(mixerInputs, buffers, mixerInputLevels, mixerOutputLevels, NULL, NULL, numberOfSamples);

    return !silence;
}

- (IBAction)onPlayPause:(id)sender {
    UIButton *button = (UIButton *)sender;
    pthread_mutex_lock(&mutex);
    if (playerA->playing) {
        playerA->pause();
        playerB->pause();
    } else {
        bool masterIsA = (crossValue <= 0.5f);
        playerA->play(!masterIsA);
        playerB->play(masterIsA);
    };
    pthread_mutex_unlock(&mutex);
    button.selected = playerA->playing;
}

- (IBAction)onCrossFader:(id)sender {
    pthread_mutex_lock(&mutex);
    crossValue = ((UISlider *)sender).value;
    if (crossValue < 0.01f) {
        volA = 1.0f * headroom;
        volB = 0.0f;
    } else if (crossValue > 0.99f) {
        volA = 0.0f;
        volB = 1.0f * headroom;
    } else { // constant power curve
        volA = cosf(M_PI_2 * crossValue) * headroom;
        volB = cosf(M_PI_2 * (1.0f - crossValue)) * headroom;
    };
    pthread_mutex_unlock(&mutex);
}

static inline float floatToFrequency(float value) {
    static const float min = logf(20.0f) / logf(10.0f);
    static const float max = logf(20000.0f) / logf(10.0f);
    static const float range = max - min;
    return powf(10.0f, value * range + min);
}

- (IBAction)onFxSelect:(id)sender {
    activeFx = ((UISegmentedControl *)sender).selectedSegmentIndex;
}

- (IBAction)onFxValue:(id)sender {
    float value = ((UISlider *)sender).value;
    switch (activeFx) {
        case 1:
            filter->setResonantParameters(floatToFrequency(1.0f - value), 0.1f);
            filter->enable(true);
            flanger->enable(false);
            roll->enable(false);
            break;
        case 2:
            if (value > 0.8f) roll->beats = 0.0625f;
            else if (value > 0.6f) roll->beats = 0.125f;
            else if (value > 0.4f) roll->beats = 0.25f;
            else if (value > 0.2f) roll->beats = 0.5f;
            else roll->beats = 1.0f;
            roll->enable(true);
            filter->enable(false);
            flanger->enable(false);
            break;
        default:
            flanger->setWet(value);
            flanger->enable(true);
            filter->enable(false);
            roll->enable(false);
    };
}

- (IBAction)onFxOff:(id)sender {
    filter->enable(false);
    roll->enable(false);
    flanger->enable(false);
}

@end
