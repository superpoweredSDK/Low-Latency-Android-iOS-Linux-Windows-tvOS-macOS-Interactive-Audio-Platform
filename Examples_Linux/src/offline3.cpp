#include <stdio.h>
#include <malloc.h>
#include "Superpowered.h"
#include "SuperpoweredDecoder.h"
#include "SuperpoweredSimple.h"
#include "SuperpoweredAnalyzer.h"

// EXAMPLE: reading an audio file, processing the audio and returning with some features detected
int main(int argc, char *argv[]) {
    Superpowered::Initialize("ExampleLicenseKey-WillExpire-OnNextUpdate");
    
    // Open the input file.
    Superpowered::Decoder *decoder = new Superpowered::Decoder();
    int openReturn = decoder->open("test.m4a");
    if (openReturn != Superpowered::Decoder::OpenSuccess) {
        printf("\rOpen error %i: %s\n", openReturn, Superpowered::Decoder::statusCodeToString(openReturn));
        delete decoder;
        return 0;
    };

    // Create the analyzer.
    Superpowered::Analyzer *analyzer = new Superpowered::Analyzer(decoder->getSamplerate(), decoder->getDurationSeconds());
    
    // Create a buffer for the 16-bit integer audio output of the decoder.
    short int *intBuffer = (short int *)malloc(decoder->getFramesPerChunk() * 2 * sizeof(short int) + 16384);
    // Create a buffer for the 32-bit floating point audio required by the effect.
    float *floatBuffer = (float *)malloc(decoder->getFramesPerChunk() * 2 * sizeof(float) + 16384);
    
    // Processing.
    while (true) {
        int framesDecoded = decoder->decodeAudio(intBuffer, decoder->getFramesPerChunk());
        if (framesDecoded < 1) break;
        
        // Submit the decoded audio to the analyzer.
        Superpowered::ShortIntToFloat(intBuffer, floatBuffer, framesDecoded);
        analyzer->process(floatBuffer, framesDecoded);
    };
    
    analyzer->makeResults(60, 200, 0, 0, false, false, false, false, false);
    printf("\rBpm is %f, average loudness is %f db, peak volume is %f db.\n", analyzer->bpm, analyzer->loudpartsAverageDb, analyzer->peakDb);
    
    // Cleanup.
    delete decoder;
    delete analyzer;
    free(intBuffer);
    free(floatBuffer);

    return 0;
}
