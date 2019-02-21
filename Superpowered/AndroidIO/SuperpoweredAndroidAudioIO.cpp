#include "SuperpoweredAndroidAudioIO.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define NUM_CHANNELS 2

#if NUM_CHANNELS == 1
    #define CHANNELMASK SL_SPEAKER_FRONT_CENTER
#elif NUM_CHANNELS == 2
    #define CHANNELMASK (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT)
#endif

typedef struct fifo {
    short int *buffer;
    int readIndex, writeIndex;

    bool hasAudio() {
        return this->writeIndex != this->readIndex;
    }
    void incRead(int numBuffers) {
        if (this->readIndex < numBuffers - 1) this->readIndex++; else this->readIndex = 0;
    }
    void incWrite(int numBuffers) {
        if (this->writeIndex < numBuffers - 1) this->writeIndex++; else this->writeIndex = 0;
    }
} fifo;

typedef struct SuperpoweredAndroidAudioIOInternals {
    fifo inputFifo, outputFifo;
    void *clientdata;
    audioProcessingCallback callback;
    SLObjectItf openSLEngine, outputMix, outputBufferQueue, inputBufferQueue;
    SLAndroidSimpleBufferQueueItf outputBufferQueueInterface, inputBufferQueueInterface;
    short int *silence;
    size_t fifoBufferSizeBytes;
    int samplerate, buffersize, silenceSamples, numBuffers, bufferStep;
    bool hasOutput, hasInput, foreground, started;
} SuperpoweredAndroidAudioIOInternals;

// The entire operation is based on two Android Simple Buffer Queues, one for the audio input and one for the audio output.
static void startQueues(SuperpoweredAndroidAudioIOInternals *internals) {
    if (internals->started) return;
    internals->started = true;

    if (internals->inputBufferQueue) {
        memset(internals->inputFifo.buffer, 0, internals->fifoBufferSizeBytes);
        SLRecordItf recordInterface;
        (*internals->inputBufferQueue)->GetInterface(internals->inputBufferQueue, SL_IID_RECORD, &recordInterface);
        (*recordInterface)->SetRecordState(recordInterface, SL_RECORDSTATE_RECORDING);
    };

    if (internals->outputBufferQueue) {
        memset(internals->outputFifo.buffer, 0, internals->fifoBufferSizeBytes);
        SLPlayItf outputPlayInterface;
        (*internals->outputBufferQueue)->GetInterface(internals->outputBufferQueue, SL_IID_PLAY, &outputPlayInterface);
        (*outputPlayInterface)->SetPlayState(outputPlayInterface, SL_PLAYSTATE_PLAYING);
    };
}

// Stopping the Simple Buffer Queues.
static void stopQueues(SuperpoweredAndroidAudioIOInternals *internals) {
    if (!internals->started) return;
    internals->started = false;
    if (internals->outputBufferQueue) {
        SLPlayItf outputPlayInterface;
        (*internals->outputBufferQueue)->GetInterface(internals->outputBufferQueue, SL_IID_PLAY, &outputPlayInterface);
        (*outputPlayInterface)->SetPlayState(outputPlayInterface, SL_PLAYSTATE_STOPPED);
    };
    if (internals->inputBufferQueue) {
        SLRecordItf recordInterface;
        (*internals->inputBufferQueue)->GetInterface(internals->inputBufferQueue, SL_IID_RECORD, &recordInterface);
        (*recordInterface)->SetRecordState(recordInterface, SL_RECORDSTATE_STOPPED);
    };
}

// This is called periodically by the input audio queue. Audio input is received from the media server at this point.
static void SuperpoweredAndroidAudioIO_InputCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    SuperpoweredAndroidAudioIOInternals *internals = (SuperpoweredAndroidAudioIOInternals *)pContext;
    internals->inputFifo.incWrite(internals->numBuffers);

    if (!internals->hasOutput && internals->inputFifo.hasAudio()) { // When there is no audio output configured and we have enough audio input available.
        internals->callback(internals->clientdata, internals->inputFifo.buffer + internals->inputFifo.readIndex * internals->bufferStep, internals->buffersize, internals->samplerate);
        internals->inputFifo.incRead(internals->numBuffers);
    }

    (*caller)->Enqueue(caller, internals->inputFifo.buffer + internals->inputFifo.writeIndex * internals->bufferStep, (SLuint32)internals->buffersize * NUM_CHANNELS * 2);
}

// This is called periodically by the output audio queue. Audio for the user should be provided here.
static void SuperpoweredAndroidAudioIO_OutputCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    SuperpoweredAndroidAudioIOInternals *internals = (SuperpoweredAndroidAudioIOInternals *)pContext;
    short int *output = internals->outputFifo.buffer + internals->outputFifo.writeIndex * internals->bufferStep;
    internals->outputFifo.incWrite(internals->numBuffers);
    bool silence = false;

    if (internals->hasInput) { // If audio input is enabled.
        if (internals->inputFifo.hasAudio()) { // If we have enough audio input available.
            // Had to separate the input buffer from the output buffer, because it makes a feedback loop on some Android devices despite of memsetting everything to zero.
            memcpy(output, internals->inputFifo.buffer + internals->inputFifo.readIndex * internals->bufferStep, (size_t)internals->buffersize * NUM_CHANNELS * 2);
            internals->inputFifo.incRead(internals->numBuffers);

            if (!internals->callback(internals->clientdata, output, internals->buffersize, internals->samplerate)) {
                silence = true;
                internals->silenceSamples += internals->buffersize;
            } else internals->silenceSamples = 0;
        } else silence = true; // Dropout, not enough audio input.
    } else { // If audio input is not enabled.
        if (!internals->callback(internals->clientdata, output, internals->buffersize, internals->samplerate)) {
            silence = true;
            internals->silenceSamples += internals->buffersize;
        } else internals->silenceSamples = 0;
    };

    if (silence) memset(output, 0, (size_t)internals->buffersize * NUM_CHANNELS * 2);
    (*caller)->Enqueue(caller, output, (SLuint32)internals->buffersize * NUM_CHANNELS * 2);

    if (!internals->foreground && (internals->silenceSamples > internals->samplerate)) {
        internals->silenceSamples = 0;
        stopQueues(internals);
    };
}

SuperpoweredAndroidAudioIO::SuperpoweredAndroidAudioIO(int samplerate, int buffersize, bool enableInput, bool enableOutput, audioProcessingCallback callback, void *clientdata, int inputStreamType, int outputStreamType) {
    static const SLboolean requireds[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_FALSE };

    internals = new SuperpoweredAndroidAudioIOInternals;
    memset(internals, 0, sizeof(SuperpoweredAndroidAudioIOInternals));
    internals->samplerate = samplerate;
    internals->buffersize = buffersize;
    internals->clientdata = clientdata;
    internals->callback = callback;
    internals->hasInput = enableInput;
    internals->hasOutput = enableOutput;
    internals->foreground = true;
    internals->started = false;
    internals->silence = (short int *)malloc((size_t)buffersize * NUM_CHANNELS * 2);
    memset(internals->silence, 0, (size_t)buffersize * NUM_CHANNELS * 2);

    internals->numBuffers = samplerate / buffersize;
    internals->bufferStep = (buffersize + 64) * NUM_CHANNELS;
    internals->fifoBufferSizeBytes = internals->numBuffers * internals->bufferStep * sizeof(short int);
    internals->inputFifo.buffer = internals->outputFifo.buffer = NULL;

    // Create the OpenSL ES engine.
    slCreateEngine(&internals->openSLEngine, 0, NULL, 0, NULL, NULL);
    (*internals->openSLEngine)->Realize(internals->openSLEngine, SL_BOOLEAN_FALSE);
    SLEngineItf openSLEngineInterface = NULL;
    (*internals->openSLEngine)->GetInterface(internals->openSLEngine, SL_IID_ENGINE, &openSLEngineInterface);
    // Create the output mix.
    (*openSLEngineInterface)->CreateOutputMix(openSLEngineInterface, &internals->outputMix, 0, NULL, NULL);
    (*internals->outputMix)->Realize(internals->outputMix, SL_BOOLEAN_FALSE);
    SLDataLocator_OutputMix outputMixLocator = { SL_DATALOCATOR_OUTPUTMIX, internals->outputMix };

    if (enableInput) { // Create the audio input buffer queue.
        internals->inputFifo.buffer = (short int *)malloc(internals->fifoBufferSizeBytes);

        SLDataLocator_IODevice deviceInputLocator = { SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL };
        SLDataSource inputSource = { &deviceInputLocator, NULL };
        SLDataLocator_AndroidSimpleBufferQueue inputLocator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1 };
        SLDataFormat_PCM inputFormat = { SL_DATAFORMAT_PCM, NUM_CHANNELS, (SLuint32)samplerate * 1000, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, CHANNELMASK, SL_BYTEORDER_LITTLEENDIAN };
        SLDataSink inputSink = { &inputLocator, &inputFormat };
        const SLInterfaceID inputInterfaces[2] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION };
        (*openSLEngineInterface)->CreateAudioRecorder(openSLEngineInterface, &internals->inputBufferQueue, &inputSource, &inputSink, 2, inputInterfaces, requireds);

        if (inputStreamType == -1) inputStreamType = (int)SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION; // Configure the voice recognition preset which has no signal processing for lower latency.
        if (inputStreamType > -1) {
            SLAndroidConfigurationItf inputConfiguration;
            if ((*internals->inputBufferQueue)->GetInterface(internals->inputBufferQueue, SL_IID_ANDROIDCONFIGURATION, &inputConfiguration) == SL_RESULT_SUCCESS) {
                SLuint32 st = (SLuint32)inputStreamType;
                (*inputConfiguration)->SetConfiguration(inputConfiguration, SL_ANDROID_KEY_RECORDING_PRESET, &st, sizeof(SLuint32));
            };
        };
        
        (*internals->inputBufferQueue)->Realize(internals->inputBufferQueue, SL_BOOLEAN_FALSE);
    };

    if (enableOutput) { // Create the audio output buffer queue.
        internals->outputFifo.buffer = (short int *)malloc(internals->fifoBufferSizeBytes);

        SLDataLocator_AndroidSimpleBufferQueue outputLocator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1 };
        SLDataFormat_PCM outputFormat = { SL_DATAFORMAT_PCM, NUM_CHANNELS, (SLuint32)samplerate * 1000, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, CHANNELMASK, SL_BYTEORDER_LITTLEENDIAN };
        SLDataSource outputSource = { &outputLocator, &outputFormat };
        const SLInterfaceID outputInterfaces[2] = { SL_IID_BUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION };
        SLDataSink outputSink = { &outputMixLocator, NULL };
        (*openSLEngineInterface)->CreateAudioPlayer(openSLEngineInterface, &internals->outputBufferQueue, &outputSource, &outputSink, 2, outputInterfaces, requireds);

        // Configure the stream type.
        if (outputStreamType > -1) {
            SLAndroidConfigurationItf outputConfiguration;
            if ((*internals->outputBufferQueue)->GetInterface(internals->outputBufferQueue, SL_IID_ANDROIDCONFIGURATION, &outputConfiguration) == SL_RESULT_SUCCESS) {
                SLint32 st = (SLint32)outputStreamType;
                (*outputConfiguration)->SetConfiguration(outputConfiguration, SL_ANDROID_KEY_STREAM_TYPE, &st, sizeof(SLint32));
            };
        };

        (*internals->outputBufferQueue)->Realize(internals->outputBufferQueue, SL_BOOLEAN_FALSE);
    };

    if (enableInput) { // Initialize the audio input buffer queue.
        memset(internals->inputFifo.buffer, 0, internals->fifoBufferSizeBytes);
        (*internals->inputBufferQueue)->GetInterface(internals->inputBufferQueue, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &internals->inputBufferQueueInterface);
        (*internals->inputBufferQueueInterface)->RegisterCallback(internals->inputBufferQueueInterface, SuperpoweredAndroidAudioIO_InputCallback, internals);
        (*internals->inputBufferQueueInterface)->Enqueue(internals->inputBufferQueueInterface, internals->inputFifo.buffer, (SLuint32)buffersize * NUM_CHANNELS * 2);
    };

    if (enableOutput) { // Initialize the audio output buffer queue.
        memset(internals->outputFifo.buffer, 0, internals->fifoBufferSizeBytes);
        (*internals->outputBufferQueue)->GetInterface(internals->outputBufferQueue, SL_IID_BUFFERQUEUE, &internals->outputBufferQueueInterface);
        (*internals->outputBufferQueueInterface)->RegisterCallback(internals->outputBufferQueueInterface, SuperpoweredAndroidAudioIO_OutputCallback, internals);
        (*internals->outputBufferQueueInterface)->Enqueue(internals->outputBufferQueueInterface, internals->outputFifo.buffer, (SLuint32)buffersize * NUM_CHANNELS * 2);
    };

    startQueues(internals);
}

void SuperpoweredAndroidAudioIO::onForeground() {
    internals->foreground = true;
    startQueues(internals);
}

void SuperpoweredAndroidAudioIO::onBackground() {
    internals->foreground = false;
}

void SuperpoweredAndroidAudioIO::start() {
    startQueues(internals);
}

void SuperpoweredAndroidAudioIO::stop() {
    stopQueues(internals);
}

SuperpoweredAndroidAudioIO::~SuperpoweredAndroidAudioIO() {
    stopQueues(internals);
    usleep(200000);
    if (internals->outputBufferQueue) (*internals->outputBufferQueue)->Destroy(internals->outputBufferQueue);
    if (internals->inputBufferQueue) (*internals->inputBufferQueue)->Destroy(internals->inputBufferQueue);
    (*internals->outputMix)->Destroy(internals->outputMix);
    (*internals->openSLEngine)->Destroy(internals->openSLEngine);
    if (internals->inputFifo.buffer) free(internals->inputFifo.buffer);
    if (internals->outputFifo.buffer) free(internals->outputFifo.buffer);
    free(internals->silence);
    delete internals;
}
