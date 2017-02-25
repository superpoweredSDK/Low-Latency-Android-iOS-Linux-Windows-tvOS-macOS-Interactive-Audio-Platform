#import "SuperpoweredFrequencies-Bridging-Header.h"
#import "SuperpoweredIOSAudioIO.h"
#include "SuperpoweredBandpassFilterbank.h"
#include "SuperpoweredSimple.h"

@implementation Superpowered {
    SuperpoweredIOSAudioIO *audioIO;
    SuperpoweredBandpassFilterbank *filters;
    float bands[128][8];
    unsigned int samplerate, bandsWritePos, bandsReadPos, bandsPos, lastNumberOfSamples;
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

    // Get the next position to write.
    unsigned int writePos = self->bandsWritePos++ & 127;
    memset(&self->bands[writePos][0], 0, 8 * sizeof(float));

    // Detect frequency magnitudes.
    float peak, sum;
    self->filters->process(interleaved, &self->bands[writePos][0], &peak, &sum, numberOfSamples);

    // Update position.
    self->lastNumberOfSamples = numberOfSamples;
    __sync_synchronize();
    __sync_fetch_and_add(&self->bandsPos, 1);
    return false;
}

- (id)init {
    self = [super init];
    if (!self) return nil;
    samplerate = 44100;
    bandsWritePos = bandsReadPos = bandsPos = lastNumberOfSamples = 0;
    memset(bands, 0, 128 * 8 * sizeof(float));

    float frequencies[8] = { 55, 110, 220, 440, 880, 1760, 3520, 7040 };
    float widths[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    filters = new SuperpoweredBandpassFilterbank(8, frequencies, widths, samplerate);

    audioIO = [[SuperpoweredIOSAudioIO alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:12 preferredMinimumSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryRecord channels:2 audioProcessingCallback:audioProcessing clientdata:(__bridge void *)self];
    [audioIO start];

    return self;
}

- (void)dealloc {
    delete filters;
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
    memset(freqs, 0, 8 * sizeof(float));
    unsigned int currentPosition = __sync_fetch_and_add(&bandsPos, 0);
    if (currentPosition > bandsReadPos) {
        unsigned int positionsElapsed = currentPosition - bandsReadPos;
        float multiplier = 1.0f / float(positionsElapsed * lastNumberOfSamples);
        while (positionsElapsed--) {
            float *b = &bands[bandsReadPos++ & 127][0];
            for (int n = 0; n < 8; n++) freqs[n] += b[n] * multiplier;
        }
    }
}

@end
