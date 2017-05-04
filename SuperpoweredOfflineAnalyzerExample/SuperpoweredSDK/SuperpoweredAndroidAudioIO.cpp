#include "SuperpoweredAndroidAudioIO.h"
#include <android/log.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/resource.h>

typedef struct SuperpoweredAndroidAudioIOInternals {
    pthread_mutex_t audioProcessingThreadMutex;
    pthread_cond_t audioProcessingThreadCondition;
    void *clientdata;
    audioProcessingCallback callback;
    SLObjectItf openSLEngine, outputMix, outputBufferQueue, inputBufferQueue;
    SLAndroidSimpleBufferQueueItf outputBufferQueueInterface, inputBufferQueueInterface;
    short int *fifobuffer, *silence;
    int samplerate, buffersize, silenceSamples, latencySamples, numBuffers, bufferStep, readBufferIndex, writeBufferIndex;
    bool hasOutput, hasInput, foreground, started, separateAudioProcessingThread, stopAudioProcessingThread;
} SuperpoweredAndroidAudioIOInternals;

// The entire operation is based on two Android Simple Buffer Queues, one for the audio input and one for the audio output.
static void startQueues(SuperpoweredAndroidAudioIOInternals *internals) {
    if (internals->started) return;
    internals->started = true;
    if (internals->inputBufferQueue) {
        SLRecordItf recordInterface;
        (*internals->inputBufferQueue)->GetInterface(internals->inputBufferQueue, SL_IID_RECORD, &recordInterface);
        (*recordInterface)->SetRecordState(recordInterface, SL_RECORDSTATE_RECORDING);
    };
    if (internals->outputBufferQueue) {
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

static void cleanup(SuperpoweredAndroidAudioIOInternals *internals) {
    free(internals->fifobuffer);
    free(internals->silence);
    delete internals;
}

// Called periodically when audio input is enabled but audio output is not.
static void inputOnlyProcessing(SuperpoweredAndroidAudioIOInternals *internals) {
    int buffersAvailable = internals->writeBufferIndex - internals->readBufferIndex;
    if (buffersAvailable < 0) buffersAvailable = internals->numBuffers - (internals->readBufferIndex - internals->writeBufferIndex);
    if (buffersAvailable * internals->buffersize >= internals->latencySamples) { // if we have enough audio input available
        internals->callback(internals->clientdata, internals->fifobuffer + internals->readBufferIndex * internals->bufferStep, internals->buffersize, internals->samplerate);
        if (internals->readBufferIndex < internals->numBuffers - 1) internals->readBufferIndex++; else internals->readBufferIndex = 0;
    };
}

// Called periodically when audio output is enabled. Handles audio input as well.
static short int *normalProcessing(SuperpoweredAndroidAudioIOInternals *internals) {
    int buffersAvailable = internals->writeBufferIndex - internals->readBufferIndex;
    if (buffersAvailable < 0) buffersAvailable = internals->numBuffers - (internals->readBufferIndex - internals->writeBufferIndex);
    short int *audioGoesToOutput = internals->fifobuffer + internals->readBufferIndex * internals->bufferStep;

    if (internals->hasInput) { // If audio input is enabled.
        if (buffersAvailable * internals->buffersize >= internals->latencySamples) { // if we have enough audio input available
            if (!internals->callback(internals->clientdata, audioGoesToOutput, internals->buffersize, internals->samplerate)) {
                memset(audioGoesToOutput, 0, internals->buffersize * 4);
                internals->silenceSamples += internals->buffersize;
            } else internals->silenceSamples = 0;
        } else audioGoesToOutput = NULL; // dropout, not enough audio input
    } else { // If audio input is not enabled.
        short int *audioToGenerate = internals->fifobuffer + internals->writeBufferIndex * internals->bufferStep;

        if (!internals->callback(internals->clientdata, audioToGenerate, internals->buffersize, internals->samplerate)) {
            memset(audioToGenerate, 0, internals->buffersize * 4);
            internals->silenceSamples += internals->buffersize;
        } else internals->silenceSamples = 0;

        if (internals->writeBufferIndex < internals->numBuffers - 1) internals->writeBufferIndex++; else internals->writeBufferIndex = 0;
        if ((buffersAvailable + 1) * internals->buffersize < internals->latencySamples) audioGoesToOutput = NULL; // dropout, not enough audio generated
    };

    return audioGoesToOutput;
}

// Devices with badly configured/poorly written Android Audio HAL may need a separate processing thread - this one.
static void *audioProcessingThread(void *param) {
    SuperpoweredAndroidAudioIOInternals *internals = (SuperpoweredAndroidAudioIOInternals *)param;
    setpriority(PRIO_PROCESS, 0, -20);

    while (!internals->stopAudioProcessingThread) {
        pthread_mutex_lock(&internals->audioProcessingThreadMutex);
        pthread_cond_wait(&internals->audioProcessingThreadCondition, &internals->audioProcessingThreadMutex);
        pthread_mutex_unlock(&internals->audioProcessingThreadMutex);
        if (internals->stopAudioProcessingThread) break;
        if (!internals->hasOutput) inputOnlyProcessing(internals);
        else while (normalProcessing(internals) == NULL) { ; };
    };

    pthread_cond_destroy(&internals->audioProcessingThreadCondition);
    pthread_mutex_destroy(&internals->audioProcessingThreadMutex);
    cleanup(internals);

    pthread_detach(pthread_self());
    pthread_exit(NULL);
    return NULL;
}

// This is called periodically by the input audio queue. Audio input is received from the media server at this point.
static void SuperpoweredAndroidAudioIO_InputCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    SuperpoweredAndroidAudioIOInternals *internals = (SuperpoweredAndroidAudioIOInternals *)pContext;
    short int *buffer = internals->fifobuffer + internals->writeBufferIndex * internals->bufferStep;
    if (internals->writeBufferIndex < internals->numBuffers - 1) internals->writeBufferIndex++; else internals->writeBufferIndex = 0;

    if (!internals->hasOutput) {
        if (internals->separateAudioProcessingThread) pthread_cond_signal(&internals->audioProcessingThreadCondition); else inputOnlyProcessing(internals);
    };
    (*caller)->Enqueue(caller, buffer, internals->buffersize * 4);
}

// This is called periodically by the output audio queue. Audio for the user should be provided here.
static void SuperpoweredAndroidAudioIO_OutputCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    SuperpoweredAndroidAudioIOInternals *internals = (SuperpoweredAndroidAudioIOInternals *)pContext;
    short int *output = NULL;

    if (internals->separateAudioProcessingThread) {
        int buffersAvailable = internals->writeBufferIndex - internals->readBufferIndex;
        if (buffersAvailable < 0) buffersAvailable = internals->numBuffers - (internals->readBufferIndex - internals->writeBufferIndex);
        if (buffersAvailable * internals->buffersize > 0) {
            output = internals->fifobuffer + internals->readBufferIndex * internals->bufferStep;
            if (internals->readBufferIndex < internals->numBuffers - 1) internals->readBufferIndex++; else internals->readBufferIndex = 0;
        };
        pthread_cond_signal(&internals->audioProcessingThreadCondition);
    } else {
        output = normalProcessing(internals);
        if (output) {
            if (internals->readBufferIndex < internals->numBuffers - 1) internals->readBufferIndex++; else internals->readBufferIndex = 0;
        };
    };

    (*caller)->Enqueue(caller, output ? output : internals->silence, internals->buffersize * 4);

    if (!internals->foreground && (internals->silenceSamples > internals->samplerate)) {
        internals->silenceSamples = 0;
        stopQueues(internals);
    };
}

SuperpoweredAndroidAudioIO::SuperpoweredAndroidAudioIO(int samplerate, int buffersize, bool enableInput, bool enableOutput, audioProcessingCallback callback, void *clientdata, int inputStreamType, int outputStreamType, int latencySamples) {
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
    internals->silence = (short int *)malloc(buffersize * 4);
    memset(internals->silence, 0, buffersize * 4);

    if (latencySamples < 0) { // If latencySamples is negative, a separate processing thread is created for devices with badly configured/poorly written Android Audio HAL. It may help.
        internals->latencySamples = buffersize * 8;
        internals->separateAudioProcessingThread = true;
        internals->stopAudioProcessingThread = false;
    } else {
        internals->separateAudioProcessingThread = false;
        internals->latencySamples = latencySamples < buffersize ? buffersize : latencySamples;
    };

    internals->numBuffers = (internals->latencySamples / buffersize) * 2;
    if (internals->numBuffers < 16) internals->numBuffers = 16;
    internals->bufferStep = (buffersize + 64) * 2;
    int fifoBufferSizeBytes = internals->numBuffers * internals->bufferStep * sizeof(short int);
    internals->fifobuffer = (short int *)malloc(fifoBufferSizeBytes);
    memset(internals->fifobuffer, 0, fifoBufferSizeBytes);

    // The separate processing thread operates using a signal, the mutex is not really blocking anything otherwise, so don't worry!
    if (internals->separateAudioProcessingThread) {
        pthread_mutex_init(&internals->audioProcessingThreadMutex, NULL);
        pthread_cond_init(&internals->audioProcessingThreadCondition, NULL);
        pthread_t thread;
        pthread_create(&thread, NULL, audioProcessingThread, internals);
    };

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
        SLDataLocator_IODevice deviceInputLocator = { SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL };
        SLDataSource inputSource = { &deviceInputLocator, NULL };
        SLDataLocator_AndroidSimpleBufferQueue inputLocator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1 };
        SLDataFormat_PCM inputFormat = { SL_DATAFORMAT_PCM, 2, (SLuint32)samplerate * 1000, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN };
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
        SLDataLocator_AndroidSimpleBufferQueue outputLocator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1 };
        SLDataFormat_PCM outputFormat = { SL_DATAFORMAT_PCM, 2, (SLuint32)samplerate * 1000, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN };
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
        (*internals->inputBufferQueue)->GetInterface(internals->inputBufferQueue, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &internals->inputBufferQueueInterface);
        (*internals->inputBufferQueueInterface)->RegisterCallback(internals->inputBufferQueueInterface, SuperpoweredAndroidAudioIO_InputCallback, internals);
        (*internals->inputBufferQueueInterface)->Enqueue(internals->inputBufferQueueInterface, internals->fifobuffer, buffersize * 4);
    };

    if (enableOutput) { // Initialize the audio output buffer queue.
        (*internals->outputBufferQueue)->GetInterface(internals->outputBufferQueue, SL_IID_BUFFERQUEUE, &internals->outputBufferQueueInterface);
        (*internals->outputBufferQueueInterface)->RegisterCallback(internals->outputBufferQueueInterface, SuperpoweredAndroidAudioIO_OutputCallback, internals);
        (*internals->outputBufferQueueInterface)->Enqueue(internals->outputBufferQueueInterface, internals->fifobuffer, buffersize * 4);
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
    if (internals->separateAudioProcessingThread) {
        internals->stopAudioProcessingThread = true;
        pthread_cond_signal(&internals->audioProcessingThreadCondition);
    } else cleanup(internals);
}
