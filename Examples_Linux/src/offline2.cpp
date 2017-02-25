#include <stdio.h>
#include <malloc.h>
#include "SuperpoweredDecoder.h"
#include "SuperpoweredSimple.h"
#include "SuperpoweredRecorder.h"
#include "SuperpoweredTimeStretching.h"
#include "SuperpoweredAudioBuffers.h"

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
    FILE *fd = createWAV("./results/offline2.wav", decoder->samplerate, 2);
    if (!fd) {
        printf("File creation error.\n");
        delete decoder;
        return 0;
    };

/*
 Due to it's nature, a time stretcher can not operate with fixed buffer sizes.
 This problem can be solved with variable size buffer chains (complex) or FIFO buffering (easier).

 Memory bandwidth on mobile devices is way lower than on desktop (laptop), so we need to use variable size buffer chains here.
 This solution provides almost 2x performance increase over FIFO buffering!
*/
    SuperpoweredTimeStretching *timeStretch = new SuperpoweredTimeStretching(decoder->samplerate);
    timeStretch->setRateAndPitchShift(1.04f, 0); // Audio will be 4% faster.
    // This buffer list will receive the time-stretched samples.
    SuperpoweredAudiopointerList *outputBuffers = new SuperpoweredAudiopointerList(8, 16);

    // Create a buffer for the 16-bit integer samples.
    short int *intBuffer = (short int *)malloc(decoder->samplesPerFrame * 2 * sizeof(short int) + 32768);

    // Processing.
    int progress = 0;
    while (true) {
        // Decode one frame. samplesDecoded will be overwritten with the actual decoded number of samples.
        unsigned int samplesDecoded = decoder->samplesPerFrame;
        if (decoder->decode(intBuffer, &samplesDecoded) == SUPERPOWEREDDECODER_ERROR) break;
        if (samplesDecoded < 1) break;

        // Create an input buffer for the time stretcher.
        SuperpoweredAudiobufferlistElement inputBuffer;
        inputBuffer.samplePosition = decoder->samplePosition;
        inputBuffer.startSample = 0;
        inputBuffer.samplesUsed = 0;
        inputBuffer.endSample = samplesDecoded; // <-- Important!
        inputBuffer.buffers[0] = SuperpoweredAudiobufferPool::getBuffer(samplesDecoded * 8 + 64);
        inputBuffer.buffers[1] = inputBuffer.buffers[2] = inputBuffer.buffers[3] = NULL;

        // Convert the decoded PCM samples from 16-bit integer to 32-bit floating point.
        SuperpoweredShortIntToFloat(intBuffer, (float *)inputBuffer.buffers[0], samplesDecoded);

        // Time stretching.
        timeStretch->process(&inputBuffer, outputBuffers);

        // Do we have some output?
        if (outputBuffers->makeSlice(0, outputBuffers->sampleLength)) {

            while (true) { // Iterate on every output slice.
                // Get pointer to the output samples.
                int numSamples = 0;
                float *timeStretchedAudio = (float *)outputBuffers->nextSliceItem(&numSamples);
                if (!timeStretchedAudio) break;

                // Convert the time stretched PCM samples from 32-bit floating point to 16-bit integer.
                SuperpoweredFloatToShortInt(timeStretchedAudio, intBuffer, numSamples);

                // Write the audio to disk.
                fwrite(intBuffer, 1, numSamples * 4, fd);
            };

            // Clear the output buffer list.
            outputBuffers->clear();
        };

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
    delete timeStretch;
    delete outputBuffers;
    free(intBuffer);

    printf("\rReady.\n");
    return 0;
}
