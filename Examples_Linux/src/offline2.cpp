#include <stdio.h>
#include <malloc.h>
#include <sys/stat.h>
#include "Superpowered.h"
#include "SuperpoweredDecoder.h"
#include "SuperpoweredSimple.h"
#include "SuperpoweredRecorder.h"
#include "SuperpoweredTimeStretching.h"
#include "SuperpoweredAudioBuffers.h"

// EXAMPLE: reading an audio file, applying time stretching and saving the result to WAV
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
    FILE *destinationFile = Superpowered::createWAV("./results/offline2.wav", decoder->getSamplerate(), 2);
    if (!destinationFile) {
        printf("\rFile creation error.\n");
        delete decoder;
        return 0;
    };

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
        if (framesDecoded < 1) break;

        // Submit the decoded audio to the time stretcher.
        Superpowered::ShortIntToFloat(intBuffer, floatBuffer, framesDecoded);
        timeStretch->addInput(floatBuffer, framesDecoded);

        // The time stretcher may have 0 or more audio at this point. Write to disk if it has some.
        unsigned int outputFramesAvailable = timeStretch->getOutputLengthFrames();
        if ((outputFramesAvailable > 0) && timeStretch->getOutput(floatBuffer, outputFramesAvailable)) {
            Superpowered::FloatToShortInt(floatBuffer, intBuffer, outputFramesAvailable);
            Superpowered::writeWAV(destinationFile, intBuffer, outputFramesAvailable * 4);
        }
    };

    // Close the file and clean up.
    Superpowered::closeWAV(destinationFile);
    delete decoder;
    delete timeStretch;
    free(intBuffer);
    free(floatBuffer);

    printf("\rReady.\n");
    return 0;
}
