#import "ViewController.h"
#import "SuperpoweredIOSAudioIO.h"
#include "Superpowered.h"
#include "SuperpoweredFrequencyDomain.h"
#include "SuperpoweredSimple.h"

@implementation ViewController {
    SuperpoweredIOSAudioIO *audioIO;
    Superpowered::FrequencyDomain *frequencyDomain;
    float *magnitudeLeft, *magnitudeRight, *phaseLeft, *phaseRight, *fifoOutput;
    int fifoOutputFirstFrame, fifoOutputLastFrame, fftSize, fifoCapacity;
}

#define FFT_LOG_SIZE 11 // 2^11 = 2048
#define OVERLAP_RATIO 4 // The default overlap ratio is 4:1, so we will receive this amount of frames from the frequency domain in one step.

// This callback is called periodically by the audio system.
static bool audioProcessing(void *clientdata, float *input, float *output, unsigned int numberOfFrames, unsigned int samplerate, uint64_t hostTime) {
    __unsafe_unretained ViewController *self = (__bridge ViewController *)clientdata;
    // Input goes to the frequency domain.
    self->frequencyDomain->addInput(input, numberOfFrames);

    // In the frequency domain we are working with 1024 magnitudes and phases for every channel (left, right), if the fft size is 2048.
    while (self->frequencyDomain->timeDomainToFrequencyDomain(self->magnitudeLeft, self->magnitudeRight, self->phaseLeft, self->phaseRight)) {
        // You can work with frequency domain data from this point.

        // This is just a quick example: we remove the magnitude of the first 20 bins, meaning total bass cut between 0-430 Hz.
        memset(self->magnitudeLeft, 0, 80);
        memset(self->magnitudeRight, 0, 80);

        // We are done working with frequency domain data. Let's go back to the time domain.

        // Check if we have enough room in the fifo buffer for the output. If not, move the existing audio data back to the buffer's beginning.
        if (self->fifoOutputLastFrame + self->fftSize / OVERLAP_RATIO >= self->fifoCapacity) { // This will be true for every 100th iteration only, so we save precious memory bandwidth.
            int samplesInFifo = self->fifoOutputLastFrame - self->fifoOutputFirstFrame;
            if (samplesInFifo > 0) memmove(self->fifoOutput, self->fifoOutput + self->fifoOutputFirstFrame * 2, samplesInFifo * sizeof(float) * 2);
            self->fifoOutputFirstFrame = 0;
            self->fifoOutputLastFrame = samplesInFifo;
        };

        // Transforming back to the time domain.
        self->frequencyDomain->frequencyDomainToTimeDomain(self->magnitudeLeft, self->magnitudeRight, self->phaseLeft, self->phaseRight, self->fifoOutput + self->fifoOutputLastFrame * 2);
        self->frequencyDomain->advance();
        self->fifoOutputLastFrame += self->fftSize / OVERLAP_RATIO;
    };

    // If we have enough samples in the fifo output buffer, pass them to the audio output.
    if (self->fifoOutputLastFrame - self->fifoOutputFirstFrame >= numberOfFrames) {
        memcpy(output, self->fifoOutput + self->fifoOutputFirstFrame * 2, numberOfFrames * sizeof(float) * 2);
        self->fifoOutputFirstFrame += numberOfFrames;
        return true;
    } else return false;
}

- (void)viewDidLoad {
    [super viewDidLoad];
#ifdef __IPHONE_13_0
    if (@available(iOS 13, *)) self.overrideUserInterfaceStyle = UIUserInterfaceStyleLight;
#endif
    
    Superpowered::Initialize("ExampleLicenseKey-WillExpire-OnNextUpdate");

    frequencyDomain = new Superpowered::FrequencyDomain(FFT_LOG_SIZE); // This will do the main "magic".
    fftSize = 1 << FFT_LOG_SIZE;

    // Frequency domain data goes into these buffers:
    magnitudeLeft = (float *)malloc(fftSize * sizeof(float));
    magnitudeRight = (float *)malloc(fftSize * sizeof(float));
    phaseLeft = (float *)malloc(fftSize * sizeof(float));
    phaseRight = (float *)malloc(fftSize * sizeof(float));

    // Time domain result goes into a FIFO (first-in, first-out) buffer
    fifoOutputFirstFrame = fifoOutputLastFrame = 0;
    fifoCapacity = fftSize / OVERLAP_RATIO * 100; // Let's make the fifo's size 100 times more than the step size, so we save memory bandwidth.
    fifoOutput = (float *)malloc(fifoCapacity * sizeof(float) * 2 + 128);

    // Audio input/output handling.
    audioIO = [[SuperpoweredIOSAudioIO alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:12 preferredSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryPlayAndRecord channels:2 audioProcessingCallback:audioProcessing clientdata:(__bridge void *)self];
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

@end
