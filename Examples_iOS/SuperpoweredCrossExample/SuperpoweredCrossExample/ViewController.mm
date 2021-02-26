#import "ViewController.h"
#import "Superpowered.h"
#import "SuperpoweredAdvancedAudioPlayer.h"
#import "SuperpoweredFilter.h"
#import "SuperpoweredRoll.h"
#import "SuperpoweredFlanger.h"
#import "SuperpoweredIOSAudioIO.h"
#import "SuperpoweredSimple.h"

#define HEADROOM_DECIBEL 3.0f
static const float headroom = powf(10.0f, -HEADROOM_DECIBEL * 0.05);

// This is a .mm file, meaning it's Objective-C++. You can perfectly mix it with Objective-C or Swift, until you keep the member variables and C++ related includes here.
@implementation ViewController {
    SuperpoweredIOSAudioIO *output;
    Superpowered::AdvancedAudioPlayer *playerA, *playerB;
    Superpowered::Roll *roll;
    Superpowered::Filter *filter;
    Superpowered::Flanger *flanger;
    float crossFaderPosition, volA, volB;
    unsigned int activeFx, numPlayersLoaded;
}

static bool audioProcessing(void *clientdata, float **inputBuffers, unsigned int inputChannels, float **outputBuffers, unsigned int outputChannels, unsigned int numberOfFrames, unsigned int samplerate, uint64_t hostTime) {
    __unsafe_unretained ViewController *self = (__bridge ViewController *)clientdata;
    return [self audioProcessing:outputBuffers[0] right:outputBuffers[1] numFrames:numberOfFrames samplerate:samplerate];
}

// This is where the Superpowered magic happens.
- (bool)audioProcessing:(float *)leftOutput right:(float *)rightOutput numFrames:(unsigned int)numberOfFrames samplerate:(unsigned int)samplerate {
    playerA->outputSamplerate = playerB->outputSamplerate = roll->samplerate = filter->samplerate = flanger->samplerate = samplerate;
    
    // Check player statuses. We're only interested in the Opened event in this example.
    if (playerA->getLatestEvent() == Superpowered::AdvancedAudioPlayer::PlayerEvent_Opened) numPlayersLoaded++;
    if (playerB->getLatestEvent() == Superpowered::AdvancedAudioPlayer::PlayerEvent_Opened) numPlayersLoaded++;
    
    // Both players opened? If yes, set the beatgrid information on the players.
    if (numPlayersLoaded == 2) {
        playerA->originalBPM = 126.0f;
        playerA->firstBeatMs = 353;
        playerB->originalBPM = 123.0f;
        playerB->firstBeatMs = 40;
        playerA->syncMode = playerB->syncMode = Superpowered::AdvancedAudioPlayer::SyncMode_TempoAndBeat;
        // Jump to the first beat.
        playerA->setPosition(playerA->firstBeatMs, false, false);
        playerB->setPosition(playerB->firstBeatMs, false, false);
        numPlayersLoaded++; // Make sure we don't get into this block again. Also enables the play button.
    }
    
    // Who is dictating the tempo, player A or player B?
    bool masterIsA = (crossFaderPosition <= 0.5f);
    
    // Everything will sync to the master player's tempo.
    playerA->syncToBpm = playerB->syncToBpm = masterIsA ? playerA->getCurrentBpm() : playerB->getCurrentBpm();
    roll->bpm = flanger->bpm = (float)playerA->syncToBpm;
    
    // Players will sync to opposite player's beat position.
    playerA->syncToMsElapsedSinceLastBeat = playerB->getMsElapsedSinceLastBeat();
    playerB->syncToMsElapsedSinceLastBeat = playerA->getMsElapsedSinceLastBeat();
    
    // Get audio from the players into a buffer on the stack.
    float outputBuffer[numberOfFrames * 2];
    bool silence = !playerA->processStereo(outputBuffer, false, numberOfFrames, volA);
    if (playerB->processStereo(outputBuffer, !silence, numberOfFrames, volB)) silence = false;
    
    // Add effects.
    if (roll->process(silence ? NULL : outputBuffer, outputBuffer, numberOfFrames) && silence) silence = false;
    if (!silence) {
        filter->process(outputBuffer, outputBuffer, numberOfFrames);
        flanger->process(outputBuffer, outputBuffer, numberOfFrames);
    };
    
    // The output buffer is ready now, let's put the finished audio into the left and right outputs.
    if (!silence) Superpowered::DeInterleave(outputBuffer, leftOutput, rightOutput, numberOfFrames);
    return !silence;
}

- (void)viewDidLoad {
    [super viewDidLoad];
#ifdef __IPHONE_13_0
    if (@available(iOS 13, *)) self.overrideUserInterfaceStyle = UIUserInterfaceStyleLight;
#endif
    activeFx = numPlayersLoaded = 0;
    crossFaderPosition = volB = 0.0f;
    volA = 1.0f * headroom;
    
    Superpowered::Initialize(
                             "ExampleLicenseKey-WillExpire-OnNextUpdate",
                             false, // enableAudioAnalysis (using SuperpoweredAnalyzer, SuperpoweredLiveAnalyzer, SuperpoweredWaveform or SuperpoweredBandpassFilterbank)
                             false, // enableFFTAndFrequencyDomain (using SuperpoweredFrequencyDomain, SuperpoweredFFTComplex, SuperpoweredFFTReal or SuperpoweredPolarFFT)
                             false, // enableAudioTimeStretching (using SuperpoweredTimeStretching)
                             true, // enableAudioEffects (using any SuperpoweredFX class)
                             true, // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
                             false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
                             false  // enableNetworking (using Superpowered::httpRequest)
                             );

    playerA = new Superpowered::AdvancedAudioPlayer(44100, 0);
    playerB = new Superpowered::AdvancedAudioPlayer(44100, 0);
    roll = new Superpowered::Roll(44100);
    filter = new Superpowered::Filter(Superpowered::Filter::Resonant_Lowpass, 44100);
    flanger = new Superpowered::Flanger(44100);
    
    filter->resonance = 0.1f;
    playerA->open([[[NSBundle mainBundle] pathForResource:@"lycka" ofType:@"mp3"] fileSystemRepresentation]);
    playerB->open([[[NSBundle mainBundle] pathForResource:@"nuyorica" ofType:@"m4a"] fileSystemRepresentation]);

    output = [[SuperpoweredIOSAudioIO alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:12 preferredSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryPlayback channels:2 audioProcessingCallback:audioProcessing clientdata:(__bridge void *)self];
    [output start];
}

- (void)dealloc {
    delete playerA;
    delete playerB;
#if !__has_feature(objc_arc)
    [output release];
    [super dealloc];
#endif
}

- (void)interruptionEnded { // If a player plays Apple Lossless audio files, then we need this. Otherwise unnecessary.
    playerA->onMediaserverInterrupt();
    playerB->onMediaserverInterrupt();
}

- (IBAction)onPlayPause:(id)sender {
    if (numPlayersLoaded != 3) return;
    if (playerA->isPlaying()) {
        playerA->pause();
        playerB->pause();
    } else {
        if (crossFaderPosition <= 0.5f) { // playerA is master
            playerA->play();
            playerB->playSynchronized();
        } else { // playerB is master
            playerA->playSynchronized();
            playerB->play();
        }
    };
    ((UIButton *)sender).selected = playerA->isPlaying();
}

- (IBAction)onCrossFader:(id)sender {
    crossFaderPosition = ((UISlider *)sender).value;
    if (crossFaderPosition < 0.01f) {
        volA = 1.0f * headroom;
        volB = 0.0f;
    } else if (crossFaderPosition > 0.99f) {
        volA = 0.0f;
        volB = 1.0f * headroom;
    } else { // constant power curve
        volA = cosf(M_PI_2 * crossFaderPosition) * headroom;
        volB = cosf(M_PI_2 * (1.0f - crossFaderPosition)) * headroom;
    };
}

static inline float floatToFrequency(float value) {
    static const float min = logf(20.0f) / logf(10.0f);
    static const float max = logf(20000.0f) / logf(10.0f);
    static const float range = max - min;
    return powf(10.0f, value * range + min);
}

- (IBAction)onFxSelect:(id)sender {
    activeFx = (unsigned int)((UISegmentedControl *)sender).selectedSegmentIndex;
}

- (IBAction)onFxValue:(id)sender {
    float value = ((UISlider *)sender).value;
    switch (activeFx) {
        case 1:
            filter->frequency = floatToFrequency(1.0f - value);
            filter->enabled = true;
            flanger->enabled = roll->enabled = false;
            break;
        case 2:
            roll->beats = 1.0f - floorf(value * 5.0f) * 0.2f;
            roll->enabled = true;
            filter->enabled = flanger->enabled = false;
            break;
        default:
            flanger->wet = value;
            flanger->enabled = true;
            filter->enabled = roll->enabled = false;
    };
}

- (IBAction)onFxOff:(id)sender {
    filter->enabled = roll->enabled = flanger->enabled = false;
}

@end
