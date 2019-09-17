#include "Superpowered.h"
#include "SuperpoweredBandpassFilterbank.h"
#include "SuperpoweredSimple.h"
#import "SuperpoweredBridge.h"
#import "SuperpoweredIOSAudioIO.h"

@implementation SuperpoweredBridge {
    SuperpoweredIOSAudioIO *audioIO;
    Superpowered::BandpassFilterbank *filterbank;
    float bands[128][8];
    unsigned int bandsWritePos, bandsReadPos, bandsPos, lastNumberOfFrames;
}

static bool audioProcessing(void *clientdata, float **inputBuffers, unsigned int inputChannels, float **outputBuffers, unsigned int outputChannels, unsigned int numberOfFrames, unsigned int samplerate, uint64_t hostTime) {
    __unsafe_unretained SuperpoweredBridge *self = (__bridge SuperpoweredBridge *)clientdata;
    self->filterbank->samplerate = samplerate;

    // Mix the stereo non-interleaved input to stereo interleaved and detect frequency magnitudes.
    float interleaved[numberOfFrames * 2];
    Superpowered::Interleave(inputBuffers[0], inputBuffers[1], interleaved, numberOfFrames);
    self->filterbank->processNoAdd(interleaved, numberOfFrames);
    
    // Write to the next position.
    unsigned int writePos = self->bandsWritePos++ & 127;
    memcpy(self->bands[writePos], self->filterbank->bands, 8 * sizeof(float));
    self->lastNumberOfFrames = numberOfFrames;
    __sync_fetch_and_add(&self->bandsPos, 1);
    return false;
}

- (id)init {
    self = [super init];
    if (!self) return nil;
    
    Superpowered::Initialize(
                             "ExampleLicenseKey-WillExpire-OnNextUpdate",
                             true, // enableAudioAnalysis (using SuperpoweredAnalyzer, SuperpoweredLiveAnalyzer, SuperpoweredWaveform or SuperpoweredBandpassFilterbank)
                             false, // enableFFTAndFrequencyDomain (using SuperpoweredFrequencyDomain, SuperpoweredFFTComplex, SuperpoweredFFTReal or SuperpoweredPolarFFT)
                             false, // enableAudioTimeStretching (using SuperpoweredTimeStretching)
                             false, // enableAudioEffects (using any SuperpoweredFX class)
                             false, // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
                             false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
                             false  // enableNetworking (using Superpowered::httpRequest)
                             );
    
    bandsWritePos = bandsReadPos = bandsPos = lastNumberOfFrames = 0;
    memset(bands, 0, 128 * 8 * sizeof(float));

    float frequencies[8] = { 55, 110, 220, 440, 880, 1760, 3520, 7040 };
    float widths[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    filterbank = new Superpowered::BandpassFilterbank(8, frequencies, widths, 44100);

    audioIO = [[SuperpoweredIOSAudioIO alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:12 preferredSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryRecord channels:2 audioProcessingCallback:audioProcessing clientdata:(__bridge void *)self];
    [audioIO start];
    return self;
}

- (void)dealloc {
    delete filterbank;
    audioIO = nil;
}

// It's important to understand that the audio processing callback and the screen update (getFrequencies) are never in sync.
// More than 1 audio processing calls may happen between two consecutive screen updates.

- (void)getFrequencies:(float *)freqs {
    memset(freqs, 0, 8 * sizeof(float));
    unsigned int currentPosition = __sync_fetch_and_add(&bandsPos, 0);
    if (currentPosition > bandsReadPos) {
        unsigned int positionsElapsed = currentPosition - bandsReadPos;
        float multiplier = 1.0f / float(positionsElapsed * lastNumberOfFrames);
        while (positionsElapsed--) {
            float *b = &bands[bandsReadPos++ & 127][0];
            for (int n = 0; n < 8; n++) freqs[n] += b[n] * multiplier;
        }
    }
}

@end
