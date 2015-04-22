#include "SuperpoweredAndroidAudioIO.h"
#include <android/log.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <pthread.h>

typedef struct SuperpoweredAndroidAudioIOInternals {
    pthread_mutex_t mutex;
    void *clientdata;
    audioProcessingCallback callback;
    SLObjectItf openSLEngine, outputMix, outputBufferQueue, inputBufferQueue;
    SLAndroidSimpleBufferQueueItf outputBufferQueueInterface, inputBufferQueueInterface;
    short int *fifobuffer, *silence;
    int samplerate, buffersize, fifoCapacity, fifoFirstSample, fifoLastSample, latencySamples;
    bool hasOutput, hasInput;
} SuperpoweredAndroidAudioIOInternals;

static inline void checkRoom(SuperpoweredAndroidAudioIOInternals *internals) {
    if (internals->fifoLastSample + internals->buffersize >= internals->fifoCapacity) { // If there is no room at the fifo buffer's end, move everything to the beginning.
        int samplesInFifo = internals->fifoLastSample - internals->fifoFirstSample;
        if (samplesInFifo > 0) memmove(internals->fifobuffer, internals->fifobuffer + internals->fifoFirstSample * 2, samplesInFifo * 8);
        internals->fifoFirstSample = 0;
        internals->fifoLastSample = samplesInFifo;
    };
}

static void SuperpoweredAndroidAudioIO_InputCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) { // Audio input comes here.
    SuperpoweredAndroidAudioIOInternals *internals = (SuperpoweredAndroidAudioIOInternals *)pContext;
    pthread_mutex_lock(&internals->mutex);
    checkRoom(internals);

    short int *input = internals->fifobuffer + internals->fifoLastSample * 2;
    internals->fifoLastSample += internals->buffersize;

    if (internals->hasOutput) {
        pthread_mutex_unlock(&internals->mutex);
        (*caller)->Enqueue(caller, input, internals->buffersize * 4);
    } else {
        short int *process = internals->fifobuffer + internals->fifoFirstSample * 2;
        internals->fifoFirstSample += internals->buffersize;
        pthread_mutex_unlock(&internals->mutex);

        (*caller)->Enqueue(caller, input, internals->buffersize * 4);
        internals->callback(internals->clientdata, process, internals->buffersize, internals->samplerate);
    };
}

static void SuperpoweredAndroidAudioIO_OutputCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    SuperpoweredAndroidAudioIOInternals *internals = (SuperpoweredAndroidAudioIOInternals *)pContext;
    pthread_mutex_lock(&internals->mutex);

    if (!internals->hasInput) {
        checkRoom(internals);
        short int *process = internals->fifobuffer + internals->fifoLastSample * 2;
        internals->fifoLastSample += internals->buffersize;
        short int *output = internals->fifobuffer + internals->fifoFirstSample * 2;
        internals->fifoFirstSample += internals->buffersize;
        pthread_mutex_unlock(&internals->mutex);

        if (!internals->callback(internals->clientdata, process, internals->buffersize, internals->samplerate)) memset(process, 0, internals->buffersize * 4);
        (*caller)->Enqueue(caller, output, internals->buffersize * 4);
    } else {
        if (internals->fifoLastSample - internals->fifoFirstSample >= internals->latencySamples) {
            short int *output = internals->fifobuffer + internals->fifoFirstSample * 2;
            internals->fifoFirstSample += internals->buffersize;
            pthread_mutex_unlock(&internals->mutex);

            if (!internals->callback(internals->clientdata, output, internals->buffersize, internals->samplerate)) memset(output, 0, internals->buffersize * 4);
            (*caller)->Enqueue(caller, output, internals->buffersize * 4);
        } else {
            pthread_mutex_unlock(&internals->mutex);
            (*caller)->Enqueue(caller, internals->silence, internals->buffersize * 4); // dropout, not enough audio input
        };
    };
}

SuperpoweredAndroidAudioIO::SuperpoweredAndroidAudioIO(int samplerate, int buffersize, bool enableInput, bool enableOutput, audioProcessingCallback callback, void *clientdata, int latencySamples) {
    static const SLboolean requireds[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

    internals = new SuperpoweredAndroidAudioIOInternals;
    memset(internals, 0, sizeof(SuperpoweredAndroidAudioIOInternals));
    pthread_mutex_init(&internals->mutex, NULL);
    internals->samplerate = samplerate;
    internals->buffersize = buffersize;
    internals->clientdata = clientdata;
    internals->callback = callback;
    internals->hasInput = enableInput;
    internals->hasOutput = enableOutput;
    internals->silence = (short int *)malloc(buffersize * 4);
    internals->latencySamples = latencySamples < buffersize ? buffersize : latencySamples;
    memset(internals->silence, 0, buffersize * 4);

    internals->fifoCapacity = buffersize * 100;
    int fifoBufferSizeBytes = internals->fifoCapacity * 4 + buffersize * 4;
    internals->fifobuffer = (short int *)malloc(fifoBufferSizeBytes);
    memset(internals->fifobuffer, 0, fifoBufferSizeBytes);

    // Create the OpenSL ES engine.
    slCreateEngine(&internals->openSLEngine, 0, NULL, 0, NULL, NULL);
    (*internals->openSLEngine)->Realize(internals->openSLEngine, SL_BOOLEAN_FALSE);
    SLEngineItf openSLEngineInterface = NULL;
    (*internals->openSLEngine)->GetInterface(internals->openSLEngine, SL_IID_ENGINE, &openSLEngineInterface);
    // Create the output mix.
    (*openSLEngineInterface)->CreateOutputMix(openSLEngineInterface, &internals->outputMix, 0, NULL, NULL);
    (*internals->outputMix)->Realize(internals->outputMix, SL_BOOLEAN_FALSE);
    SLDataLocator_OutputMix outputMixLocator = { SL_DATALOCATOR_OUTPUTMIX, internals->outputMix };

    if (enableInput) { // Create the input buffer queue.
        SLDataLocator_IODevice deviceInputLocator = { SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL };
        SLDataSource inputSource = { &deviceInputLocator, NULL };
        SLDataLocator_AndroidSimpleBufferQueue inputLocator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1 };
        SLDataFormat_PCM inputFormat = { SL_DATAFORMAT_PCM, 2, samplerate * 1000, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN };
        SLDataSink inputSink = { &inputLocator, &inputFormat };
        const SLInterfaceID inputInterfaces[1] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
        (*openSLEngineInterface)->CreateAudioRecorder(openSLEngineInterface, &internals->inputBufferQueue, &inputSource, &inputSink, 1, inputInterfaces, requireds);
        (*internals->inputBufferQueue)->Realize(internals->inputBufferQueue, SL_BOOLEAN_FALSE);
    };

    if (enableOutput) { // Create the output buffer queue.
        SLDataLocator_AndroidSimpleBufferQueue outputLocator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1 };
        SLDataFormat_PCM outputFormat = { SL_DATAFORMAT_PCM, 2, samplerate * 1000, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN };
        SLDataSource outputSource = { &outputLocator, &outputFormat };
        const SLInterfaceID outputInterfaces[1] = { SL_IID_BUFFERQUEUE };
        SLDataSink outputSink = { &outputMixLocator, NULL };
        (*openSLEngineInterface)->CreateAudioPlayer(openSLEngineInterface, &internals->outputBufferQueue, &outputSource, &outputSink, 1, outputInterfaces, requireds);
        (*internals->outputBufferQueue)->Realize(internals->outputBufferQueue, SL_BOOLEAN_FALSE);
    };

    if (enableInput) { // Initialize and start the input buffer queue.
        (*internals->inputBufferQueue)->GetInterface(internals->inputBufferQueue, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &internals->inputBufferQueueInterface);
        (*internals->inputBufferQueueInterface)->RegisterCallback(internals->inputBufferQueueInterface, SuperpoweredAndroidAudioIO_InputCallback, internals);
        SLRecordItf recordInterface;
        (*internals->inputBufferQueue)->GetInterface(internals->inputBufferQueue, SL_IID_RECORD, &recordInterface);
        (*internals->inputBufferQueueInterface)->Enqueue(internals->inputBufferQueueInterface, internals->fifobuffer, buffersize * 4);
        (*recordInterface)->SetRecordState(recordInterface, SL_RECORDSTATE_RECORDING);
    };

    if (enableOutput) { // Initialize and start the output buffer queue.
        (*internals->outputBufferQueue)->GetInterface(internals->outputBufferQueue, SL_IID_BUFFERQUEUE, &internals->outputBufferQueueInterface);
        (*internals->outputBufferQueueInterface)->RegisterCallback(internals->outputBufferQueueInterface, SuperpoweredAndroidAudioIO_OutputCallback, internals);
        (*internals->outputBufferQueueInterface)->Enqueue(internals->outputBufferQueueInterface, internals->fifobuffer, buffersize * 4);
        SLPlayItf outputPlayInterface;
        (*internals->outputBufferQueue)->GetInterface(internals->outputBufferQueue, SL_IID_PLAY, &outputPlayInterface);
        (*outputPlayInterface)->SetPlayState(outputPlayInterface, SL_PLAYSTATE_PLAYING);
    };
}

SuperpoweredAndroidAudioIO::~SuperpoweredAndroidAudioIO() {
    free(internals->fifobuffer);
    free(internals->silence);
    pthread_mutex_destroy(&internals->mutex);
    delete internals;
}
