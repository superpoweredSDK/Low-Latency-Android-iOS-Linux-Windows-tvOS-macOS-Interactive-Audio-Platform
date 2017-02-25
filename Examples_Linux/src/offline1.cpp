#include <stdio.h>
#include <malloc.h>
#include "SuperpoweredDecoder.h"
#include "SuperpoweredSimple.h"
#include "SuperpoweredRecorder.h"
#include "SuperpoweredFilter.h"

// EXAMPLE: reading an audio file, applying a simple effect (filter) and saving the result to WAV
int main(int argc, char *argv[]) {
    // Open the input file.
    SuperpoweredDecoder *decoder = new SuperpoweredDecoder();
    const char *openError = decoder->open("test.m4a", false, 0, 0);
    if (openError) {
        printf("Open error: %s\n", openError);
        delete decoder;
        return 0;
    };

    // Create the output WAVE file.
    FILE *fd = createWAV("./results/offline1.wav", decoder->samplerate, 2);
    if (!fd) {
        printf("File creation error.\n");
        delete decoder;
        return 0;
    };

    // Creating the filter.
    SuperpoweredFilter *filter = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Lowpass, decoder->samplerate);
    filter->setResonantParameters(1000.0f, 0.1f);
    filter->enable(true);

    // Create a buffer for the 16-bit integer samples coming from the decoder.
    short int *intBuffer = (short int *)malloc(decoder->samplesPerFrame * 2 * sizeof(short int) + 32768);
    // Create a buffer for the 32-bit floating point samples required by the effect.
    float *floatBuffer = (float *)malloc(decoder->samplesPerFrame * 2 * sizeof(float) + 32768);

    // Processing.
    int progress = 0;
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
        int p = int(((double)decoder->samplePosition / (double)decoder->durationSamples) * 100.0);
        if (progress != p) {
            progress = p;
            printf("\r%i%%", progress);
            fflush(stdout);
        }
    };

    // Cleanup.
    closeWAV(fd);
    delete decoder;
    delete filter;
    free(intBuffer);
    free(floatBuffer);

    printf("\rReady.\n");
    return 0;
}
