#import "ViewController.h"
#include "Superpowered.h"
#include "SuperpoweredDecoder.h"
#include "SuperpoweredSimple.h"
#include "SuperpoweredTimeStretching.h"
#include "SuperpoweredFilter.h"
#include "SuperpoweredAnalyzer.h"

@implementation ViewController {
    float progress;
    CADisplayLink *displayLink;
}

@synthesize progressView;

- (void)viewDidLoad {
    [super viewDidLoad];
#ifdef __IPHONE_13_0
    if (@available(iOS 13, *)) self.overrideUserInterfaceStyle = UIUserInterfaceStyleLight;
#endif
    
    Superpowered::Initialize(
                             "ExampleLicenseKey-WillExpire-OnNextUpdate",
                             true, // enableAudioAnalysis (using SuperpoweredAnalyzer, SuperpoweredLiveAnalyzer, SuperpoweredWaveform or SuperpoweredBandpassFilterbank)
                             false, // enableFFTAndFrequencyDomain (using SuperpoweredFrequencyDomain, SuperpoweredFFTComplex, SuperpoweredFFTReal or SuperpoweredPolarFFT)
                             true, // enableAudioTimeStretching (using SuperpoweredTimeStretching)
                             true, // enableAudioEffects (using any SuperpoweredFX class)
                             true, // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
                             false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
                             false  // enableNetworking (using Superpowered::httpRequest)
                             );
    
    progress = 0.0f;
    displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(onDisplayLink)];
    displayLink.preferredFramesPerSecond = 60;
    [self displayMediaPicker];
}

- (void)dealloc {
    [displayLink invalidate];
}

- (void)onDisplayLink {
    progressView.progress = progress;
}

- (void)displayMediaPicker {
    MPMediaPickerController *picker = [[MPMediaPickerController alloc] initWithMediaTypes:MPMediaTypeMusic];
    [picker setDelegate:self];
    [picker setAllowsPickingMultipleItems:NO];
    picker.prompt = @"Pick a song to process";
    [self presentViewController:picker animated:YES completion:nil];
    picker = nil;
}

- (void)mediaPickerDidCancel:(MPMediaPickerController *)mediaPicker {
    [self dismissViewControllerAnimated:YES completion:nil];
    [self displayMediaPicker];
}

- (void)mediaPicker:(MPMediaPickerController *)mediaPicker didPickMediaItems:(MPMediaItemCollection *)collection {
    [self dismissViewControllerAnimated:YES completion:nil];
    NSURL *url = nil;
    MPMediaItem *item = [collection.items objectAtIndex:0];
    if (item) url = [item valueForProperty:@"assetURL"];
    if (url) {
        [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
        [self performSelectorInBackground:@selector(offlineFilter:) withObject:url];
    } else [self displayMediaPicker];
}

// Creates a Superpowered Decoder and tries to open a file.
static Superpowered::Decoder *openSourceFile(const char *path) {
    Superpowered::Decoder *decoder = new Superpowered::Decoder();
    
    while (true) {
        int openReturn = decoder->open(path);
    
        switch (openReturn) {
            case Superpowered::Decoder::OpenSuccess: return decoder;
            case Superpowered::Decoder::BufferingTryAgainLater: usleep(100000); break; // May happen for progressive downloads. Wait 100 ms for the network to load more data.
            default:
                delete decoder;
                NSLog(@"Open error %i: %s", openReturn, Superpowered::Decoder::statusCodeToString(openReturn));
                return NULL;
        }
    }
}

// Creates the output WAV file. The destination is accessible in iTunes File Sharing. https://support.apple.com/en-gb/HT201301
static FILE *createDestinationFile(const char *filename, unsigned int samplerate) {
    NSString *destinationPath = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0] stringByAppendingPathComponent:[NSString stringWithUTF8String:filename]];
    FILE *fd = Superpowered::createWAV([destinationPath fileSystemRepresentation], samplerate, 2);
    if (fd) NSLog(@"File created at %@.", destinationPath); else NSLog(@"File creation error.");
    return fd;
}

// EXAMPLE 1: reading an audio file, applying a simple effect (filter) and saving the result to WAV.
- (void)offlineFilter:(NSURL *)url {
    Superpowered::Decoder *decoder = openSourceFile([[url absoluteString] UTF8String]);
    if (!decoder) return;

    FILE *destinationFile = createDestinationFile("SuperpoweredOfflineTest.wav", decoder->getSamplerate());
    if (!destinationFile) {
        delete decoder;
        return;
    }

    // Create the low-pass filter.
    Superpowered::Filter *filter = new Superpowered::Filter(Superpowered::Filter::Resonant_Lowpass, decoder->getSamplerate());
    filter->frequency = 1000.0f;
    filter->resonance = 0.1f;
    filter->enabled = true;

    // Create a buffer for the 16-bit integer audio output of the decoder.
    short int *intBuffer = (short int *)malloc(decoder->getFramesPerChunk() * 2 * sizeof(short int) + 16384);
    // Create a buffer for the 32-bit floating point audio required by the effect.
    float *floatBuffer = (float *)malloc(decoder->getFramesPerChunk() * 2 * sizeof(float) + 16384);

    // Processing.
    while (true) {
        int framesDecoded = decoder->decodeAudio(intBuffer, decoder->getFramesPerChunk());
        if (framesDecoded == Superpowered::Decoder::BufferingTryAgainLater) { // May happen for progressive downloads.
            usleep(100000); // Wait 100 ms for the network to load more data.
            continue;
        } else if (framesDecoded < 1) break;

        // Apply the effect.
        Superpowered::ShortIntToFloat(intBuffer, floatBuffer, framesDecoded);
        filter->process(floatBuffer, floatBuffer, framesDecoded);
        Superpowered::FloatToShortInt(floatBuffer, intBuffer, framesDecoded);

        // Write the audio to disk and update the progress indicator.
        Superpowered::writeWAV(destinationFile, intBuffer, framesDecoded * 4);
        progress = (double)decoder->getPositionFrames() / (double)decoder->getDurationFrames();
    };
    
    // Close the file and clean up.
    Superpowered::closeWAV(destinationFile);
    delete decoder;
    delete filter;
    free(intBuffer);
    free(floatBuffer);
}

// EXAMPLE 2: reading an audio file, applying time stretching and saving the result to WAV.
// For a real-time timestretching example check our JavaScript SDK.
- (void)offlineTimeStretching:(NSURL *)url {
    Superpowered::Decoder *decoder = openSourceFile([[url absoluteString] UTF8String]);
    if (!decoder) return;

    FILE *destinationFile = createDestinationFile("SuperpoweredOfflineTest.wav", decoder->getSamplerate());
    if (!destinationFile) {
        delete decoder;
        return;
    }

    // Create the time stretcher.
    Superpowered::TimeStretching *timeStretch = new Superpowered::TimeStretching(decoder->getSamplerate());
    timeStretch->rate = 1.04f; // 4% faster
    
    // Create a buffer to store 16-bit integer audio up to 1 seconds, which is a safe limit.
    short int *intBuffer = (short int *)malloc(decoder->getSamplerate() * 2 * sizeof(short int) + 16384);
    // Create a buffer to store 32-bit floating point audio up to 1 seconds, which is a safe limit.
    float *floatBuffer = (float *)malloc(decoder->getSamplerate() * 2 * sizeof(float));

    // Processing.
    while (true) {
        int framesDecoded = decoder->decodeAudio(intBuffer, decoder->getFramesPerChunk());
        if (framesDecoded == Superpowered::Decoder::BufferingTryAgainLater) { // May happen for progressive downloads.
            usleep(100000); // Wait 100 ms for the network to load more data.
            continue;
        } else if (framesDecoded < 1) break;
        
        // Submit the decoded audio to the time stretcher.
        Superpowered::ShortIntToFloat(intBuffer, floatBuffer, framesDecoded);
        timeStretch->addInput(floatBuffer, framesDecoded);

        // The time stretcher may have 0 or more audio at this point. Write to disk if it has some.
        unsigned int outputFramesAvailable = timeStretch->getOutputLengthFrames();
        if ((outputFramesAvailable > 0) && timeStretch->getOutput(floatBuffer, outputFramesAvailable)) {
            Superpowered::FloatToShortInt(floatBuffer, intBuffer, outputFramesAvailable);
            Superpowered::writeWAV(destinationFile, intBuffer, outputFramesAvailable * 4);
        }

        progress = (double)decoder->getPositionFrames() / (double)decoder->getDurationFrames();
    };

    // Close the file and clean up.
    Superpowered::closeWAV(destinationFile);
    delete decoder;
    delete timeStretch;
    free(intBuffer);
    free(floatBuffer);
}

// EXAMPLE 3: reading an audio file and analyzing it (for bpm, beatgrid, etc.)
- (void)offlineAnalyze:(NSURL *)url {
    Superpowered::Decoder *decoder = openSourceFile([[url absoluteString] UTF8String]);
    if (!decoder) return;

    // Create the analyzer.
    Superpowered::Analyzer *analyzer = new Superpowered::Analyzer(decoder->getSamplerate(), decoder->getDurationSeconds());

    // Create a buffer for the 16-bit integer audio output of the decoder.
    short int *intBuffer = (short int *)malloc(decoder->getFramesPerChunk() * 2 * sizeof(short int) + 16384);
    // Create a buffer for the 32-bit floating point audio required by the effect.
    float *floatBuffer = (float *)malloc(decoder->getFramesPerChunk() * 2 * sizeof(float) + 16384);

    // Processing.
    while (true) {
        int framesDecoded = decoder->decodeAudio(intBuffer, decoder->getFramesPerChunk());
        if (framesDecoded == Superpowered::Decoder::BufferingTryAgainLater) { // May happen for progressive downloads.
            usleep(100000); // Wait 100 ms for the network to load more data.
            continue;
        } else if (framesDecoded < 1) break;

        // Submit the decoded audio to the analyzer.
        Superpowered::ShortIntToFloat(intBuffer, floatBuffer, framesDecoded);
        analyzer->process(floatBuffer, framesDecoded);

        progress = (double)decoder->getPositionFrames() / (double)decoder->getDurationFrames();
    };
    
    analyzer->makeResults(60, 200, 0, 0, false, false, false, false, false);
    NSLog(@"Bpm is %f, average loudness is %f db, peak volume is %f db.", analyzer->bpm, analyzer->loudpartsAverageDb, analyzer->peakDb);

    // Cleanup.
    delete decoder;
    delete analyzer;
    free(intBuffer);
    free(floatBuffer);
}

@end
