#import "AppDelegate.h"
#include "SuperpoweredOSXAudioIO.h"
#include "Superpowered.h"
#include "SuperpoweredSpatializer.h"
#include "SuperpoweredAdvancedAudioPlayer.h"
#include "SuperpoweredSimple.h"

#define NUMSPEAKERS 14

static const float elevations[NUMSPEAKERS] = {
    0.0f, 0.0f, 0.0f, 0.0f, 90.0f, -90.0f, 45.0f, 45.0f, 45.0f, 45.0f, -45.0f, -45.0f, -45.0f, -45.0f
};

static const float azimuths[NUMSPEAKERS] = {
    0.0f, 180.0f, 90.0f, -90.0f, 0.0f, 0.0f, 45.0f, 135.0f, 225.0f, 315.0f, 45.0f, 135.0f, 225.0f, 315.0f
};

@interface AppDelegate () {
    SuperpoweredOSXAudioIO *io;
    Superpowered::Spatializer *sp[NUMSPEAKERS];
    Superpowered::AdvancedAudioPlayer *players[2];
    float *wx, *yz, *w, *x, *y, *z, *buf;
    float xmul[NUMSPEAKERS], ymul[NUMSPEAKERS], zmul[NUMSPEAKERS], azimuth, elevation, lastAzimuth, lastElevation;
    unsigned int loaded;
}

@property IBOutlet NSWindow *window;
@end

@implementation AppDelegate

static const float speakerVolume = 1.0f / float(NUMSPEAKERS);
static const float wmul = sqrtf(0.5f);

- (IBAction)onAzimuth:(id)sender {
    azimuth = [((NSSlider *)sender) floatValue];
}

- (IBAction)onElevation:(id)sender {
    elevation = [((NSSlider *)sender) floatValue];
}

- (void)updatePositions {
    lastAzimuth = azimuth;
    lastElevation = elevation;
    for (int n = 0; n < NUMSPEAKERS; n++) {
        float az = azimuths[n] - lastAzimuth;
        while (az > 360.0f) az -= 360.0f;
        while (az < 0.0f) az += 360.0f;
        float ev = elevations[n] + elevation;
        sp[n]->elevation = ev;
        sp[n]->azimuth = az;
    };
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    Superpowered::Initialize(
                             "ExampleLicenseKey-WillExpire-OnNextUpdate",
                             false, // enableAudioAnalysis (using SuperpoweredAnalyzer, SuperpoweredLiveAnalyzer, SuperpoweredWaveform or SuperpoweredBandpassFilterbank)
                             false, // enableFFTAndFrequencyDomain (using SuperpoweredFrequencyDomain, SuperpoweredFFTComplex, SuperpoweredFFTReal or SuperpoweredPolarFFT)
                             false, // enableAudioTimeStretching (using SuperpoweredTimeStretching)
                             true,  // enableAudioEffects (using any SuperpoweredFX class)
                             true,  // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
                             false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
                             false  // enableNetworking (using Superpowered::httpRequest)
                             );
    
    loaded = 0;
    azimuth = lastAzimuth = elevation = lastElevation = 0;

    wx = (float *)malloc(2048 * 8);
    yz = (float *)malloc(2048 * 8);
    buf = (float *)malloc(2048 * 4);
    w = (float *)malloc(2048 * 4);
    x = (float *)malloc(2048 * 4);
    y = (float *)malloc(2048 * 4);
    z = (float *)malloc(2048 * 4);

    for (int n = 0; n < NUMSPEAKERS; n++) {
        sp[n] = new Superpowered::Spatializer(44100);
        sp[n]->inputVolume = speakerVolume * 4.0f;
        sp[n]->sound2 = true;

        float az = float(M_PI) * (360.0f - azimuths[n]) / 180.0f, cosaz = cosf(az), sinaz = sinf(az), cosel = cosf(float(M_PI) * elevations[n] / 180.0f);
        xmul[n] = cosaz * cosel;
        ymul[n] = sinaz * cosel;
        zmul[n] = sinf(float(M_PI) * elevations[n] / 180.0f);
    };
    [self updatePositions];

    players[0] = new Superpowered::AdvancedAudioPlayer(44100, 0);
    players[1] = new Superpowered::AdvancedAudioPlayer(44100, 0);
    players[0]->loopOnEOF = players[1]->loopOnEOF = true;
    players[0]->open([[[NSBundle mainBundle] pathForResource:@"ambi_01" ofType:@"mp3"] fileSystemRepresentation]);
    players[1]->open([[[NSBundle mainBundle] pathForResource:@"ambi_23" ofType:@"mp3"] fileSystemRepresentation]);

    io = [[SuperpoweredOSXAudioIO alloc] initWithDelegate:(id<SuperpoweredOSXAudioIODelegate>)self preferredBufferSizeMs:11 numberOfChannels:2 enableInput:false enableOutput:true];
    [io start];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    [io dealloc];
    for (int n = 0; n < NUMSPEAKERS; n++) delete sp[n];
    delete players[0];
    delete players[1];
    free(wx);
    free(yz);
    free(buf);
    free(w);
    free(x);
    free(y);
    free(z);
}

- (bool)audioProcessingCallback:(float **)inputBuffers inputChannels:(unsigned int)inputChannels outputBuffers:(float **)outputBuffers outputChannels:(unsigned int)outputChannels numberOfFrames:(unsigned int)numberOfFrames samplerate:(unsigned int)samplerate hostTime:(UInt64)hostTime {
    if (players[0]->getLatestEvent() == Superpowered::PlayerEvent_Opened) loaded++;
    if (players[1]->getLatestEvent() == Superpowered::PlayerEvent_Opened) loaded++;
    if ((loaded != 2) || (outputChannels != 2)) return false;

    if (players[0]->outputSamplerate != samplerate) {
        for (int n = 0; n < NUMSPEAKERS; n++) sp[n]->samplerate = samplerate;
        players[0]->outputSamplerate = players[1]->outputSamplerate = samplerate;
    };
    
    if (!players[0]->isPlaying()) {
        players[0]->play();
        players[1]->play();
    };

    if ((azimuth != lastAzimuth) || (elevation != lastElevation)) [self updatePositions];

    if (!players[0]->processStereo(wx, false, numberOfFrames)) memset(wx, 0, numberOfFrames * 8);
    if (!players[1]->processStereo(yz, false, numberOfFrames)) memset(yz, 0, numberOfFrames * 8);

    Superpowered::DeInterleave(wx, w, x, numberOfFrames);
    Superpowered::DeInterleave(yz, y, z, numberOfFrames);

    for (int s = 0; s < NUMSPEAKERS; s++) {
        for (int n = 0; n < numberOfFrames; n++) buf[n] = w[n] * wmul + x[n] * xmul[s] + y[n] * ymul[s] + z[n] * zmul[s]; // This line is not Superpowered, not good for performance.
        sp[s]->process(buf, buf, outputBuffers[0], outputBuffers[1], numberOfFrames, s != 0);
    };

    return true;
}

@end
