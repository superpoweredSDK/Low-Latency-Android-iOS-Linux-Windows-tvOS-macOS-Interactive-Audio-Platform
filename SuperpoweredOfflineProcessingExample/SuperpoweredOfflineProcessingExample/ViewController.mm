#import "ViewController.h"
#import "SuperpoweredDecoder.h"
#import "SuperpoweredMixer.h"
#import "SuperpoweredFilter.h"

@implementation ViewController {
    float progress;
    CADisplayLink *displayLink;
}

@synthesize progressView;

- (void)viewDidLoad {
    [super viewDidLoad];
    
    progress = 0.0f;
    displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(onDisplayLink)];
    displayLink.frameInterval = 1;
    [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
    
    [self performSelectorInBackground:@selector(processingThread) withObject:nil];
}

- (void)dealloc {
    [displayLink invalidate];
}

- (void)onDisplayLink {
    progressView.progress = progress;
}

- (void)processingThread {
    // Open the input file.
    SuperpoweredDecoder *decoder = new SuperpoweredDecoder(false);
    const char *openError = decoder->open([[[NSBundle mainBundle] pathForResource:@"track" ofType:@"mp3"] fileSystemRepresentation], false, 0, 0);
    if (openError) {
        NSLog(@"%s", openError);
        delete decoder;
        return;
    };
    
    // Create the output WAVE file. The destination is in iTunes File Sharing.
    NSString *destinationPath = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0] stringByAppendingPathComponent:@"SuperpoweredOfflineTest.wav"];
    FILE *fd = fopen([destinationPath fileSystemRepresentation], "w+");
    if (!fd) {
        NSLog(@"File creation error.");
        delete decoder;
        return;
    };
    writeWAVEHeader(fd, decoder->samplerate);
    
    // Create the low pass filter.
    SuperpoweredFilter *filter = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Lowpass, decoder->samplerate);
    filter->setResonantParameters(1000.0f, 0.1f);
    filter->enable(true);
    
    short int *intBuffer = (short int *)malloc(decoder->samplesPerFrame * 4 + 16384);
    float *floatBuffer = (float *)malloc(decoder->samplesPerFrame * 8 + 16384);
    unsigned int samples = decoder->samplesPerFrame, allSamples = 0;
    float durationMultiply = 1.0f / (float)decoder->durationSamples;
    
    // Processing.
    while (decoder->decode(intBuffer, &samples) == SUPERPOWEREDDECODER_OK) {
        SuperpoweredStereoMixer::shortIntToFloat(intBuffer, floatBuffer, samples);
        
        filter->process(floatBuffer, floatBuffer, samples);
        
        SuperpoweredStereoMixer::floatToShortInt(floatBuffer, intBuffer, samples);
        fwrite(intBuffer, 1, samples * 4, fd);
        
        allSamples += samples;
        samples = decoder->samplesPerFrame;
        progress = (float)decoder->samplePosition * durationMultiply;
    };
    
    // Cleanup.
    writeWAVESize(fd, allSamples);
    fclose(fd);
    free(intBuffer);
    free(floatBuffer);
    delete decoder;
    delete filter;
}

@end
