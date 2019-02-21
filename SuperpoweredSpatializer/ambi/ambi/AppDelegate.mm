#import "AppDelegate.h"
#include "SuperpoweredOSXAudioIO.h"
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
    SuperpoweredSpatializer *sp[NUMSPEAKERS];
    SuperpoweredAdvancedAudioPlayer *players[2];
    float *wx, *yz, *w, *x, *y, *z, *buf;
    float xmul[NUMSPEAKERS], ymul[NUMSPEAKERS], zmul[NUMSPEAKERS], azimuth, elevation, lastAzimuth, lastElevation;
    unsigned int lastSamplerate, loaded;
}

@property IBOutlet NSWindow *window;
@end

@implementation AppDelegate

void playerCallback(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event, void *value) {
    if (event == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) *((int *)clientData) += 1;
}

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
    lastSamplerate = 44100;
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
        sp[n] = new SuperpoweredSpatializer(lastSamplerate);
        sp[n]->inputVolume = speakerVolume * 4.0f;
        sp[n]->sound2 = true;

        float az = float(M_PI) * (360.0f - azimuths[n]) / 180.0f, cosaz = cosf(az), sinaz = sinf(az), cosel = cosf(float(M_PI) * elevations[n] / 180.0f);
        xmul[n] = cosaz * cosel;
        ymul[n] = sinaz * cosel;
        zmul[n] = sinf(float(M_PI) * elevations[n] / 180.0f);
    };
    [self updatePositions];

    players[0] = new SuperpoweredAdvancedAudioPlayer(&loaded, playerCallback, lastSamplerate, 0);
    players[1] = new SuperpoweredAdvancedAudioPlayer(&loaded, playerCallback, lastSamplerate, 0);
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

- (bool)audioProcessingCallback:(float **)inputBuffers inputChannels:(unsigned int)inputChannels outputBuffers:(float **)outputBuffers outputChannels:(unsigned int)outputChannels numberOfSamples:(unsigned int)numberOfSamples samplerate:(unsigned int)samplerate hostTime:(UInt64)hostTime {
    if (!loaded || (outputChannels != 2)) return false;

    if (samplerate != lastSamplerate) {
        lastSamplerate = samplerate;
        for (int n = 0; n < NUMSPEAKERS; n++) sp[n]->setSamplerate(lastSamplerate);
        players[0]->setSamplerate(lastSamplerate);
        players[1]->setSamplerate(lastSamplerate);
    };

    if ((azimuth != lastAzimuth) || (elevation != lastElevation)) [self updatePositions];

    if (!players[0]->process(wx, false, numberOfSamples)) memset(wx, 0, numberOfSamples * 8);
    if (!players[1]->process(yz, false, numberOfSamples)) memset(yz, 0, numberOfSamples * 8);

    if (!players[0]->playing) {
        players[0]->play(false);
        players[1]->play(false);
    };

    SuperpoweredDeInterleave(wx, w, x, numberOfSamples);
    SuperpoweredDeInterleave(yz, y, z, numberOfSamples);

    for (int s = 0; s < NUMSPEAKERS; s++) {
        for (int n = 0; n < numberOfSamples; n++) buf[n] = w[n] * wmul + x[n] * xmul[s] + y[n] * ymul[s] + z[n] * zmul[s]; // This line is not Superpowered, not good for performance.
        sp[s]->process(buf, buf, outputBuffers[0], outputBuffers[1], numberOfSamples, s != 0);
    };

    return true;
}

@end
