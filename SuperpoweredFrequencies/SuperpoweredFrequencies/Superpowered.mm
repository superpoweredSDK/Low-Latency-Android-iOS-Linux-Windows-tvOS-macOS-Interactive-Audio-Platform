#import "SuperpoweredFrequencies-Bridging-Header.h"
#import "SuperpoweredIOSAudioIO.h"
#include "SuperpoweredBandpassFilterbank.h"
#include "SuperpoweredSimple.h"
#include <pthread.h>

@implementation Superpowered {
    SuperpoweredIOSAudioIO *audioIO;
    SuperpoweredBandpassFilterbank *filters;
    float bands[8];
    pthread_mutex_t mutex;
    unsigned int samplerate, samplesProcessedForOneDisplayFrame;
}

static bool audioProcessing(void *clientdata, float **buffers, unsigned int inputChannels, unsigned int outputChannels, unsigned int numberOfSamples, unsigned int samplerate, uint64_t hostTime) {
    __unsafe_unretained Superpowered *self = (__bridge Superpowered *)clientdata;
    if (samplerate != self->samplerate) {
        self->samplerate = samplerate;
        self->filters->setSamplerate(samplerate);
    };

    // Mix the non-interleaved input to interleaved.
    float interleaved[numberOfSamples * 2 + 16];
    SuperpoweredInterleave(buffers[0], buffers[1], interleaved, numberOfSamples);

    // Detect frequency magnitudes.
    float peak, sum;
    pthread_mutex_lock(&self->mutex);
    self->samplesProcessedForOneDisplayFrame += numberOfSamples;
    self->filters->process(interleaved, self->bands, &peak, &sum, numberOfSamples);
    pthread_mutex_unlock(&self->mutex);

    return false;
}

- (id)init {
    self = [super init];
    if (!self) return nil;
    samplerate = 44100;
    samplesProcessedForOneDisplayFrame = 0;
    memset(bands, 0, 8 * sizeof(float));

    // We use a mutex to prevent simultaneous reading/writing of bands.
    pthread_mutex_init(&mutex, NULL);

    float frequencies[8] = { 55, 110, 220, 440, 880, 1760, 3520, 7040 };
    float widths[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    filters = new SuperpoweredBandpassFilterbank(8, frequencies, widths, samplerate);

    audioIO = [[SuperpoweredIOSAudioIO alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:12 preferredMinimumSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryRecord channels:2 audioProcessingCallback:audioProcessing clientdata:(__bridge void *)self];
    [audioIO start];

    return self;
}

- (void)dealloc {
    delete filters;
    pthread_mutex_destroy(&mutex);
    audioIO = nil;
}

- (void)interruptionStarted {}
- (void)interruptionEnded {}
- (void)recordPermissionRefused {}
- (void)mapChannels:(multiOutputChannelMap *)outputMap inputMap:(multiInputChannelMap *)inputMap externalAudioDeviceName:(NSString *)externalAudioDeviceName outputsAndInputs:(NSString *)outputsAndInputs {}

/*
 It's important to understand that the audio processing callback and the screen update (getFrequencies) are never in sync. 
 More than 1 audio processing turns may happen between two consecutive screen updates.
 */

- (void)getFrequencies:(float *)freqs {
    pthread_mutex_lock(&mutex);
    if (samplesProcessedForOneDisplayFrame > 0) {
        for (int n = 0; n < 8; n++) freqs[n] = bands[n] / float(samplesProcessedForOneDisplayFrame);
        memset(bands, 0, 8 * sizeof(float));
        samplesProcessedForOneDisplayFrame = 0;
    } else memset(freqs, 0, 8 * sizeof(float));
    pthread_mutex_unlock(&mutex);
}

@end
