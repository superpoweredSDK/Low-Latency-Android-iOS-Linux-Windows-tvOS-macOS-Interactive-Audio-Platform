#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <SuperpoweredFrequencyDomain.h>
#include <AndroidIO/SuperpoweredAndroidAudioIO.h>
#include <SuperpoweredSimple.h>
#include <SuperpoweredCPU.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

static SuperpoweredAndroidAudioIO *audioIO;
static SuperpoweredFrequencyDomain *frequencyDomain;
static float *magnitudeLeft, *magnitudeRight, *phaseLeft, *phaseRight, *fifoOutput, *inputBufferFloat;
static int fifoOutputFirstSample, fifoOutputLastSample, stepSize, fifoCapacity;

#define FFT_LOG_SIZE 11         // 2^11 = 2048

// This is called periodically by the audio engine.
static bool audioProcessing (
        void * __unused clientdata,     // A custom pointer your callback receives.
        short int *audioInputOutput,    // 16-bit stereo interleaved audio input and/or output.
        int numberOfFrames,             // The number of frames received and/or requested.
        int __unused samplerate         // The current sampling rate in Hz.
) {
    // Convert 16-bit integer samples to 32-bit floating point.
    SuperpoweredShortIntToFloat(audioInputOutput, inputBufferFloat, (unsigned int)numberOfFrames);

    // Input goes to the frequency domain.
    frequencyDomain->addInput(inputBufferFloat, numberOfFrames);

    // When FFT size is 2048, we have 1024 magnitude and phase bins
    // in the frequency domain for every channel.
    while (frequencyDomain->timeDomainToFrequencyDomain(magnitudeLeft, magnitudeRight, phaseLeft, phaseRight)) {
        // You can work with frequency domain data from this point.

        // This is just a quick example: we remove the magnitude of the first 20 bins,
        // meaning total bass cut between 0-430 Hz.
        memset(magnitudeLeft, 0, 80);
        memset(magnitudeRight, 0, 80);

        // We are done working with frequency domain data. Let's go back to the time domain.

        // Check if we have enough room in the fifo buffer for the output.
        // If not, move the existing audio data back to the buffer's beginning.
        if (fifoOutputLastSample + stepSize >= fifoCapacity) {
            // This will be true for every 100th iteration only,
            // so we save precious memory bandwidth.
            int samplesInFifo = fifoOutputLastSample - fifoOutputFirstSample;
            if (samplesInFifo > 0) memmove(fifoOutput, fifoOutput + fifoOutputFirstSample * 2,
                                           samplesInFifo * sizeof(float) * 2);
            fifoOutputFirstSample = 0;
            fifoOutputLastSample = samplesInFifo;
        };

        // Transforming back to the time domain.
        frequencyDomain->frequencyDomainToTimeDomain(magnitudeLeft, magnitudeRight, phaseLeft, phaseRight,
                                                     fifoOutput + fifoOutputLastSample * 2);
        frequencyDomain->advance();
        fifoOutputLastSample += stepSize;
    };

    // If we have enough samples in the fifo output buffer, pass them to the audio output.
    if (fifoOutputLastSample - fifoOutputFirstSample >= numberOfFrames) {
        SuperpoweredFloatToShortInt(fifoOutput + fifoOutputFirstSample * 2,
                                    audioInputOutput, (unsigned int)numberOfFrames);
        fifoOutputFirstSample += numberOfFrames;
        return true;
    } else {
        return false;
    }
}

// FrequencyDomain - Initialize buffers and setup frequency domain processing.
extern "C" JNIEXPORT void
Java_com_superpowered_frequencydomain_MainActivity_FrequencyDomain (
        JNIEnv * __unused env,
        jobject __unused obj,
        jint samplerate,        // sampling rate
        jint buffersize         // buffer size
) {
    // This will do the main "magic".
    frequencyDomain = new SuperpoweredFrequencyDomain(FFT_LOG_SIZE);

    // The default overlap ratio is 4:1, so we will receive 1/4 of the
    // samples from the frequency domain in one step.
    stepSize = frequencyDomain->fftSize / 4;

    // Frequency domain data goes into these buffers.
    size_t fftBufferSize = frequencyDomain->fftSize * sizeof(float);
    magnitudeLeft  = (float *)malloc(fftBufferSize);
    magnitudeRight = (float *)malloc(fftBufferSize);
    phaseLeft      = (float *)malloc(fftBufferSize);
    phaseRight     = (float *)malloc(fftBufferSize);

    // Time domain result goes into a FIFO (first-in, first-out) buffer.
    fifoOutputFirstSample = fifoOutputLastSample = 0;

    // Let's make the FIFO's size 100 times the step size, so we save memory bandwidth.
    fifoCapacity = stepSize * 100;
    fifoOutput =       (float *)malloc(fifoCapacity * sizeof(float) * 2);
    inputBufferFloat = (float *)malloc(buffersize   * sizeof(float) * 2);

    // Prevent audio dropouts.
    SuperpoweredCPU::setSustainedPerformanceMode(true);

    // Start audio engine and processing.
    audioIO = new SuperpoweredAndroidAudioIO (
            samplerate,                 // sampling rate
            buffersize,                 // buffer size
            true,                       // enableInput
            true,                       // enableOutput
            audioProcessing,            // process callback function
            NULL,                       // clientData
            -1,                         // inputStreamType (-1 = default)
            SL_ANDROID_STREAM_MEDIA     // outputStreamType (-1 = default)
    );
}

// Cleanup - Stop audio processing and free resources.
extern "C" JNIEXPORT void
Java_com_superpowered_frequencydomain_MainActivity_Cleanup (
        JNIEnv * __unused env,
        jobject __unused obj
) {
    delete audioIO;
    free(magnitudeLeft);
    free(magnitudeRight);
    free(phaseLeft);
    free(phaseRight);
    free(fifoOutput);
    free(inputBufferFloat);
}
