#import "SuperpoweredFrequencies-Bridging-Header.h"
#import "SuperpoweredIOSAudioOutput.h"
#include "SuperpoweredBandpassFilterbank.h"
#include "SuperpoweredMixer.h"
#include <pthread.h>

@implementation Superpowered {
    SuperpoweredIOSAudioOutput *audioIO;
    SuperpoweredBandpassFilterbank *filters;
    SuperpoweredStereoMixer *mixer;
    float bands[8];
    pthread_mutex_t mutex;
    unsigned int samplerate, samplesProcessedForOneDisplayFrame;
}

- (id)init {
    self = [super init];
    if (!self) return nil;
    samplerate = 44100;
    samplesProcessedForOneDisplayFrame = 0;
    memset(bands, 0, 8 * sizeof(float));

    // We use a mutex to prevent simultaneous reading/writing of bands.
    pthread_mutex_init(&mutex, NULL);

    mixer = new SuperpoweredStereoMixer();

    float frequencies[8] = { 55, 110, 220, 440, 880, 1760, 3520, 7040 };
    float widths[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    filters = new SuperpoweredBandpassFilterbank(8, frequencies, widths, samplerate);

    audioIO = [[SuperpoweredIOSAudioOutput alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:12 preferredMinimumSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryPlayAndRecord multiChannels:2 fixReceiver:2];
    audioIO.inputEnabled = true;
    [audioIO start];

    return self;
}

- (void)dealloc {
    delete filters;
    delete mixer;
    pthread_mutex_destroy(&mutex);
    audioIO = nil;
}

- (bool)audioProcessingCallback:(float **)buffers inputChannels:(unsigned int)inputChannels outputChannels:(unsigned int)outputChannels numberOfSamples:(unsigned int)numberOfSamples samplerate:(unsigned int)currentsamplerate hostTime:(UInt64)hostTime {
    if (samplerate != currentsamplerate) {
        samplerate = currentsamplerate;
        filters->setSamplerate(samplerate);
    };

    // Mix the non-interleaved input to interleaved.
    float interleaved[numberOfSamples * 2 + 16];
    float *inputs[4] = { buffers[0], buffers[1], NULL, NULL };
    float *outputs[2] = { interleaved, NULL };
    float inputLevels[8] = { 1, 1, 0, 0, 0, 0, 0, 0 };
    mixer->process(inputs, outputs, inputLevels, inputLevels, NULL, NULL, numberOfSamples);

    // Detect frequency magnitudes.
    float peak, sum;
    pthread_mutex_lock(&mutex);
    samplesProcessedForOneDisplayFrame += numberOfSamples;
    filters->process(interleaved, bands, &peak, &sum, numberOfSamples);
    pthread_mutex_unlock(&mutex);

    return false;
}

/*
 It's important to understand that the audio processing callback and the screen update (getFrequencies) are never in sync. 
 More than 1 audio processing turns can happen between two consecutive screen updates.
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
