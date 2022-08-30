#include <stdio.h>
#include <malloc.h>
#include <sys/stat.h>
#include "Superpowered.h"
#include "SuperpoweredDecoder.h"
#include "SuperpoweredSimple.h"
#include "SuperpoweredRecorder.h"
#include "SuperpoweredFilter.h"

// EXAMPLE: reading an audio file, applying a simple effect (filter) and saving the result to WAV
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

    // Create the output WAVE file.
    mkdir("./results", 0777);
    FILE *destinationFile = Superpowered::createWAV("./results/offline1.wav", decoder->getSamplerate(), 2);
    if (!destinationFile) {
        printf("\rFile creation error.\n");
        delete decoder;
        return 0;
    };

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
        if (framesDecoded < 1) break;

        // Apply the effect.
        Superpowered::ShortIntToFloat(intBuffer, floatBuffer, framesDecoded);
        filter->process(floatBuffer, floatBuffer, framesDecoded);
        Superpowered::FloatToShortInt(floatBuffer, intBuffer, framesDecoded);

        // Write the audio to disk.
        Superpowered::writeWAV(destinationFile, intBuffer, framesDecoded * 4);
    };

    // Close the file and clean up.
    Superpowered::closeWAV(destinationFile);
    delete decoder;
    delete filter;
    free(intBuffer);
    free(floatBuffer);

    printf("\rReady.\n");
    return 0;
}
