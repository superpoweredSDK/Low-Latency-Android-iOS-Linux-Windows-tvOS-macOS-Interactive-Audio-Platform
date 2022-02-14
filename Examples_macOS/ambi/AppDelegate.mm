#import "AppDelegate.h"
#include "SuperpoweredOSXAudioIO.h"
#include "Superpowered.h"
#include "SuperpoweredSpatializer.h"
#include "SuperpoweredAdvancedAudioPlayer.h"
#include "SuperpoweredSimple.h"
#include "SuperpoweredMixer.h"

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
    Superpowered::MonoMixer *mixer[NUMSPEAKERS];
    Superpowered::AdvancedAudioPlayer *players[2];
    float azimuth, elevation, lastAzimuth, lastElevation;
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
    Superpowered::Initialize("ExampleLicenseKey-WillExpire-OnNextUpdate");
    
    loaded = 0;
    azimuth = lastAzimuth = elevation = lastElevation = 0;

    for (int n = 0; n < NUMSPEAKERS; n++) {
        sp[n] = new Superpowered::Spatializer(44100);
        sp[n]->inputVolume = speakerVolume * 4.0f;
        sp[n]->sound2 = true;
        mixer[n] = new Superpowered::MonoMixer();
        mixer[n]->inputGain[0] = wmul; // w
        
        float az = float(M_PI) * (360.0f - azimuths[n]) / 180.0f, cosaz = cosf(az), sinaz = sinf(az), cosel = cosf(float(M_PI) * elevations[n] / 180.0f);
        mixer[n]->inputGain[0] = cosaz * cosel; // x
        mixer[n]->inputGain[1] = sinaz * cosel; // y
        mixer[n]->inputGain[2] = sinf(float(M_PI) * elevations[n] / 180.0f); // z
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
    for (int n = 0; n < NUMSPEAKERS; n++) {
        delete sp[n];
        delete mixer[n];
    }
    delete players[0];
    delete players[1];
}

- (bool)audioProcessingCallback:(float *)inputBuffer outputBuffer:(float *)outputBuffer numberOfFrames:(unsigned int)numberOfFrames samplerate:(unsigned int)samplerate hostTime:(UInt64)hostTime {
    if (players[0]->getLatestEvent() == Superpowered::AdvancedAudioPlayer::PlayerEvent_Opened) loaded++;
    if (players[1]->getLatestEvent() == Superpowered::AdvancedAudioPlayer::PlayerEvent_Opened) loaded++;

    if (players[0]->outputSamplerate != samplerate) {
        for (int n = 0; n < NUMSPEAKERS; n++) sp[n]->samplerate = samplerate;
        players[0]->outputSamplerate = players[1]->outputSamplerate = samplerate;
    };
    
    if ((loaded == 2) && !players[0]->isPlaying()) {
        players[0]->play();
        players[1]->play();
    };

    if ((azimuth != lastAzimuth) || (elevation != lastElevation)) [self updatePositions];

    float wx[numberOfFrames * 2], yz[numberOfFrames * 2], w[numberOfFrames], x[numberOfFrames], y[numberOfFrames], z[numberOfFrames];
    bool _wx_ = true, _yz_ = true;
    if (!players[0]->processStereo(wx, false, numberOfFrames)) _wx_ = false; else Superpowered::DeInterleave(wx, w, x, numberOfFrames);
    if (!players[1]->processStereo(yz, false, numberOfFrames)) _yz_ = false; else Superpowered::DeInterleave(yz, y, z, numberOfFrames);
    
    for (int s = 0; s < NUMSPEAKERS; s++) {
        mixer[s]->process(_wx_ ? w : NULL, _wx_ ? x : NULL, _yz_ ? y : NULL, _yz_ ? z : NULL, outputBuffer, numberOfFrames);
        sp[s]->process(outputBuffer, outputBuffer, wx, yz, numberOfFrames, s != 0);
    };
    
    Superpowered::Interleave(wx, yz, outputBuffer, numberOfFrames);
    return true;
}

@end
