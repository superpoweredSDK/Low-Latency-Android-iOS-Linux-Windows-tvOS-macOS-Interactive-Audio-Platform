#import "ViewController.h"
#import "SuperpoweredIOSAudioIO.h"
#include "SuperpoweredFrequencyDomain.h"
#include "SuperpoweredSimple.h"

@implementation ViewController {
    SuperpoweredIOSAudioIO *audioIO;
    SuperpoweredFrequencyDomain *frequencyDomain;
    float *magnitudeLeft, *magnitudeRight, *phaseLeft, *phaseRight, *fifoOutput;
    int fifoOutputFirstSample, fifoOutputLastSample, stepSize, fifoCapacity;
}

#define FFT_LOG_SIZE 11 // 2^11 = 2048

// This callback is called periodically by the audio system.
static bool audioProcessing(void *clientdata, float **buffers, unsigned int inputChannels, unsigned int outputChannels, unsigned int numberOfSamples, unsigned int samplerate, uint64_t hostTime) {
    __unsafe_unretained ViewController *self = (__bridge ViewController *)clientdata;
    // Input goes to the frequency domain.
    float interleaved[numberOfSamples * 2 + 16];
    SuperpoweredInterleave(buffers[0], buffers[1], interleaved, numberOfSamples);
    self->frequencyDomain->addInput(interleaved, numberOfSamples);

    // In the frequency domain we are working with 1024 magnitudes and phases for every channel (left, right), if the fft size is 2048.
    while (self->frequencyDomain->timeDomainToFrequencyDomain(self->magnitudeLeft, self->magnitudeRight, self->phaseLeft, self->phaseRight)) {
        // You can work with frequency domain data from this point.

        // This is just a quick example: we remove the magnitude of the first 20 bins, meaning total bass cut between 0-430 Hz.
        memset(self->magnitudeLeft, 0, 80);
        memset(self->magnitudeRight, 0, 80);

        // We are done working with frequency domain data. Let's go back to the time domain.

        // Check if we have enough room in the fifo buffer for the output. If not, move the existing audio data back to the buffer's beginning.
        if (self->fifoOutputLastSample + self->stepSize >= self->fifoCapacity) { // This will be true for every 100th iteration only, so we save precious memory bandwidth.
            int samplesInFifo = self->fifoOutputLastSample - self->fifoOutputFirstSample;
            if (samplesInFifo > 0) memmove(self->fifoOutput, self->fifoOutput + self->fifoOutputFirstSample * 2, samplesInFifo * sizeof(float) * 2);
            self->fifoOutputFirstSample = 0;
            self->fifoOutputLastSample = samplesInFifo;
        };

        // Transforming back to the time domain.
        self->frequencyDomain->frequencyDomainToTimeDomain(self->magnitudeLeft, self->magnitudeRight, self->phaseLeft, self->phaseRight, self->fifoOutput + self->fifoOutputLastSample * 2);
        self->frequencyDomain->advance();
        self->fifoOutputLastSample += self->stepSize;
    };

    // If we have enough samples in the fifo output buffer, pass them to the audio output.
    if (self->fifoOutputLastSample - self->fifoOutputFirstSample >= numberOfSamples) {
        SuperpoweredDeInterleave(self->fifoOutput + self->fifoOutputFirstSample * 2, buffers[0], buffers[1], numberOfSamples);
        // buffers[0] and buffer[1] now have time domain audio output (left and right channels)
        self->fifoOutputFirstSample += numberOfSamples;
        return true;
    } else return false;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    frequencyDomain = new SuperpoweredFrequencyDomain(FFT_LOG_SIZE); // This will do the main "magic".
    stepSize = frequencyDomain->fftSize / 4; // The default overlap ratio is 4:1, so we will receive this amount of samples from the frequency domain in one step.

    // Frequency domain data goes into these buffers:
    magnitudeLeft = (float *)malloc(frequencyDomain->fftSize * sizeof(float));
    magnitudeRight = (float *)malloc(frequencyDomain->fftSize * sizeof(float));
    phaseLeft = (float *)malloc(frequencyDomain->fftSize * sizeof(float));
    phaseRight = (float *)malloc(frequencyDomain->fftSize * sizeof(float));

    // Time domain result goes into a FIFO (first-in, first-out) buffer
    fifoOutputFirstSample = fifoOutputLastSample = 0;
    fifoCapacity = stepSize * 100; // Let's make the fifo's size 100 times more than the step size, so we save memory bandwidth.
    fifoOutput = (float *)malloc(fifoCapacity * sizeof(float) * 2 + 128);

    // Audio input/output handling.
    audioIO = [[SuperpoweredIOSAudioIO alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:12 preferredMinimumSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryPlayAndRecord channels:2 audioProcessingCallback:audioProcessing clientdata:(__bridge void *)self];
    [audioIO start];
}

- (void)dealloc {
    audioIO = nil;
    delete frequencyDomain;
    free(magnitudeLeft);
    free(magnitudeRight);
    free(phaseLeft);
    free(phaseRight);
    free(fifoOutput);
}

- (void)interruptionStarted {}
- (void)interruptionEnded {}
- (void)recordPermissionRefused {}
- (void)mapChannels:(multiOutputChannelMap *)outputMap inputMap:(multiInputChannelMap *)inputMap externalAudioDeviceName:(NSString *)externalAudioDeviceName outputsAndInputs:(NSString *)outputsAndInputs {}

@end
