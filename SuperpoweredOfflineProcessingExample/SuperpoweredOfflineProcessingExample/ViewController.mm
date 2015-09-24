#import "ViewController.h"
#include "SuperpoweredDecoder.h"
#include "SuperpoweredSimple.h"
#include "SuperpoweredRecorder.h"
#include "SuperpoweredTimeStretching.h"
#include "SuperpoweredAudioBuffers.h"
#include "SuperpoweredFilter.h"

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
    [self displayMediaPicker];
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

- (void)dealloc {
    [displayLink invalidate];
}

- (void)onDisplayLink {
    progressView.progress = progress;
}

// EXAMPLE 1: reading an audio file, applying a simple effect (filter) and saving the result to WAV
- (void)offlineFilter:(NSURL *)url {
    // Open the input file.
    SuperpoweredDecoder *decoder = new SuperpoweredDecoder();
    const char *openError = decoder->open([[url absoluteString] UTF8String], false, 0, 0);
    if (openError) {
        NSLog(@"open error: %s", openError);
        delete decoder;
        return;
    };

    // Create the output WAVE file. The destination is accessible in iTunes File Sharing.
    NSString *destinationPath = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0] stringByAppendingPathComponent:@"SuperpoweredOfflineTest.wav"];
    FILE *fd = createWAV([destinationPath fileSystemRepresentation], decoder->samplerate, 2);
    if (!fd) {
        NSLog(@"File creation error.");
        delete decoder;
        return;
    };

    // Creating the filter.
    SuperpoweredFilter *filter = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Lowpass, decoder->samplerate);
    filter->setResonantParameters(1000.0f, 0.1f);
    filter->enable(true);

    // Create a buffer for the 16-bit integer samples coming from the decoder.
    short int *intBuffer = (short int *)malloc(decoder->samplesPerFrame * 2 * sizeof(short int) + 16384);
    // Create a buffer for the 32-bit floating point samples required by the effect.
    float *floatBuffer = (float *)malloc(decoder->samplesPerFrame * 2 * sizeof(float) + 1024);

    // Processing.
    while (true) {
        // Decode one frame. samplesDecoded will be overwritten with the actual decoded number of samples.
        unsigned int samplesDecoded = decoder->samplesPerFrame;
        if (decoder->decode(intBuffer, &samplesDecoded) == SUPERPOWEREDDECODER_ERROR) break;
        if (samplesDecoded < 1) break;

        // Convert the decoded PCM samples from 16-bit integer to 32-bit floating point.
        SuperpoweredShortIntToFloat(intBuffer, floatBuffer, samplesDecoded);

        // Apply the effect.
        filter->process(floatBuffer, floatBuffer, samplesDecoded);

        // Convert the PCM samples from 32-bit floating point to 16-bit integer.
        SuperpoweredFloatToShortInt(floatBuffer, intBuffer, samplesDecoded);

        // Write the audio to disk.
        fwrite(intBuffer, 1, samplesDecoded * 4, fd);

        // Update the progress indicator.
        progress = (double)decoder->samplePosition / (double)decoder->durationSamples;
    };

    // iTunes File Sharing: https://support.apple.com/en-gb/HT201301
    NSLog(@"The file is available in iTunes File Sharing, and locally at %@.", destinationPath);

    // Cleanup.
    closeWAV(fd);
    delete decoder;
    delete filter;
    free(intBuffer);
    free(floatBuffer);
}

// EXAMPLE 2: reading an audio file, applying time stretching and saving the result to WAV
- (void)offlineTimeStretching:(NSURL *)url {
    // Open the input file.
    SuperpoweredDecoder *decoder = new SuperpoweredDecoder();
    const char *openError = decoder->open([[url absoluteString] UTF8String], false, 0, 0);
    if (openError) {
        NSLog(@"open error: %s", openError);
        delete decoder;
        return;
    };

    /*
     Creating the input-output buffer pool and the time stretcher.
     
     Due to it's nature, a time stretcher can not operate with fixed buffer sizes.
     This problem can be solved with variable size buffer chains (complex) or FIFO buffering (easier).

     Memory bandwidth on mobile devices is way lower than on desktop (laptop), so we need to use variable size buffer chains here.
     This solution provides almost 2x performance increase over FIFO buffering!
    */
    SuperpoweredAudiobufferPool *bufferPool = new SuperpoweredAudiobufferPool(4, 1024 * 1024); // Allow 1 MB max. memory for the buffer pool.
    SuperpoweredTimeStretching *timeStretch = new SuperpoweredTimeStretching(bufferPool, decoder->samplerate);
    timeStretch->setRateAndPitchShift(1.04f, 0); // Audio will be 4% faster.

    // This buffer list will receive the time-stretched samples.
    SuperpoweredAudiopointerList *outputBuffers = new SuperpoweredAudiopointerList(bufferPool);

    // Create the output WAVE file. The destination is accessible in iTunes File Sharing.
    NSString *destinationPath = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0] stringByAppendingPathComponent:@"SuperpoweredOfflineTest.wav"];
    FILE *fd = createWAV([destinationPath fileSystemRepresentation], decoder->samplerate, 2);
    if (!fd) {
        NSLog(@"File creation error.");
        delete decoder;
        return;
    };

    // Create a buffer for the 16-bit integer samples.
    short int *intBuffer = (short int *)malloc(decoder->samplesPerFrame * 2 * sizeof(short int) + 16384);

    // Processing.
    while (true) {
        // Decode one frame. samplesDecoded will be overwritten with the actual decoded number of samples.
        unsigned int samplesDecoded = decoder->samplesPerFrame;
        if (decoder->decode(intBuffer, &samplesDecoded) == SUPERPOWEREDDECODER_ERROR) break;
        if (samplesDecoded < 1) break;

        // Create an input buffer for the time stretcher.
        SuperpoweredAudiobufferlistElement inputBuffer;
        bufferPool->createSuperpoweredAudiobufferlistElement(&inputBuffer, decoder->samplePosition, samplesDecoded + 8);

        // Convert the decoded PCM samples from 16-bit integer to 32-bit floating point.
        SuperpoweredShortIntToFloat(intBuffer, bufferPool->floatAudio(&inputBuffer), samplesDecoded);
        inputBuffer.endSample = samplesDecoded; // <-- Important!

        // Time stretching.
        timeStretch->process(&inputBuffer, outputBuffers);

        // Do we have some output?
        if (outputBuffers->makeSlice(0, outputBuffers->sampleLength)) {

            while (true) { // Iterate on every output slice.
                float *timeStretchedAudio = NULL;
                int samples = 0;

                // Get pointer to the output samples.
                if (!outputBuffers->nextSliceItem(&timeStretchedAudio, &samples)) break;

                // Convert the time stretched PCM samples from 32-bit floating point to 16-bit integer.
                SuperpoweredFloatToShortInt(timeStretchedAudio, intBuffer, samples);

                // Write the audio to disk.
                fwrite(intBuffer, 1, samples * 4, fd);
            };
            
            // Clear the output buffer list.
            outputBuffers->clear();
        };

        // Update the progress indicator.
        progress = (double)decoder->samplePosition / (double)decoder->durationSamples;
    };

    // iTunes File Sharing: https://support.apple.com/en-gb/HT201301
    NSLog(@"The file is available in iTunes File Sharing, and locally at %@.", destinationPath);

    // Cleanup.
    closeWAV(fd);
    delete decoder;
    delete timeStretch;
    delete outputBuffers;
    delete bufferPool;
    free(intBuffer);
}

@end
