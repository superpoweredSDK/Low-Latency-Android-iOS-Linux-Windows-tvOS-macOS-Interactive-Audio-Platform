#include "SuperpoweredAndroidAudioIO.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <sys/system_properties.h>
#include <dlfcn.h>
#include <android/log.h>

#define USE_AAUDIO 0

#define AAUDIO_CALLBACK_RESULT_CONTINUE  0
#define AAUDIO_OK 0
#define AAUDIO_STREAM_STATE_DISCONNECTED 13
#define AAUDIO_DIRECTION_OUTPUT 0
#define AAUDIO_DIRECTION_INPUT 1
#define AAUDIO_FORMAT_PCM_I16 1
#define AAUDIO_SHARING_MODE_EXCLUSIVE 0
#define AAUDIO_PERFORMANCE_MODE_LOW_LATENCY 12
#define AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE 10

typedef struct AAudioStreamStruct AAudioStream;
typedef struct AAudioStreamBuilderStruct AAudioStreamBuilder;

typedef int32_t (* AAudioCreateStreamBuilderFunction)(AAudioStreamBuilder **builder);
AAudioCreateStreamBuilderFunction AAudio_createStreamBuilder = NULL;

typedef void (* AAudioStreamBuilder_SetIntFunction)(AAudioStreamBuilder *builder, int32_t parameter);
AAudioStreamBuilder_SetIntFunction AAudioStreamBuilder_setDirection = NULL;
AAudioStreamBuilder_SetIntFunction AAudioStreamBuilder_setSampleRate = NULL;
AAudioStreamBuilder_SetIntFunction AAudioStreamBuilder_setPerformanceMode = NULL;
AAudioStreamBuilder_SetIntFunction AAudioStreamBuilder_setChannelCount = NULL;
AAudioStreamBuilder_SetIntFunction AAudioStreamBuilder_setFormat = NULL;
AAudioStreamBuilder_SetIntFunction AAudioStreamBuilder_setSharingMode = NULL;
AAudioStreamBuilder_SetIntFunction AAudioStreamBuilder_setInputPreset = NULL;

typedef int32_t (* AAudioStream_dataCallback)(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames);
typedef void (* AAudioStreamBuilder_setDataCallbackFunction)(AAudioStreamBuilder *builder, AAudioStream_dataCallback callback, void *userData);
AAudioStreamBuilder_setDataCallbackFunction AAudioStreamBuilder_setDataCallback = NULL;

typedef void (* AAudioStream_errorCallback)(AAudioStream *stream, void *userData, int32_t error);
typedef void (* AAudioStreamBuilder_setErrorCallbackFunction)(AAudioStreamBuilder *builder, AAudioStream_errorCallback callback, void *userData);
AAudioStreamBuilder_setErrorCallbackFunction AAudioStreamBuilder_setErrorCallback = NULL;

typedef int32_t (* AAudioStreamBuilder_openStreamFunction)(AAudioStreamBuilder *builder, AAudioStream **stream);
AAudioStreamBuilder_openStreamFunction AAudioStreamBuilder_openStream = NULL;
typedef int32_t (* AAudioStreamBuilder_deleteFunction)(AAudioStreamBuilder *builder);
AAudioStreamBuilder_deleteFunction AAudioStreamBuilder_delete = NULL;

typedef int32_t (* AAudioStream_IntFunction)(AAudioStream *stream);
AAudioStream_IntFunction AAudioStream_requestStart = NULL;
AAudioStream_IntFunction AAudioStream_requestStop = NULL;
AAudioStream_IntFunction AAudioStream_getXRunCount = NULL;
AAudioStream_IntFunction AAudioStream_getFramesPerBurst = NULL;
AAudioStream_IntFunction AAudioStream_getSampleRate = NULL;
AAudioStream_IntFunction AAudioStream_getState = NULL;
AAudioStream_IntFunction AAudioStream_close = NULL;

typedef int32_t (* AAudioStream_setBufferSizeInFramesFunction)(AAudioStream *stream, int32_t numFrames);
AAudioStream_setBufferSizeInFramesFunction AAudioStream_setBufferSizeInFrames = NULL;
typedef int32_t (* AAudioStream_readFunction)(AAudioStream *stream, void *buffer, int32_t numFrames, int64_t timeoutNanoseconds);
AAudioStream_readFunction AAudioStream_read = NULL;

static bool aaudioInitialized = false;
static bool aaudioAvailable = false;

static bool initializeAAudio() {
    #if (USE_AAUDIO == 0)
    return false;
    #endif
    if (aaudioInitialized) return aaudioAvailable;
    aaudioInitialized = true;

    char sdkVersion[PROP_VALUE_MAX] = {0};
    if (__system_property_get("ro.build.version.sdk", sdkVersion) == 0) return false;
    if (atoi(sdkVersion) < __ANDROID_API_O_MR1__) return false;
    void *aaudioLib = dlopen("libaaudio.so", RTLD_NOW);
    if (aaudioLib == nullptr) return false;

#define GETFUNCTION(func, type) \
func = (type)dlsym(aaudioLib, #func); \
if (func == NULL) { dlclose(aaudioLib); __android_log_print(ANDROID_LOG_VERBOSE, "aaudio function not loaded", #func); return false; }

    GETFUNCTION(AAudio_createStreamBuilder, AAudioCreateStreamBuilderFunction)
    GETFUNCTION(AAudioStreamBuilder_setDirection, AAudioStreamBuilder_SetIntFunction)
    GETFUNCTION(AAudioStreamBuilder_setSampleRate, AAudioStreamBuilder_SetIntFunction)
    GETFUNCTION(AAudioStreamBuilder_setPerformanceMode, AAudioStreamBuilder_SetIntFunction)
    GETFUNCTION(AAudioStreamBuilder_setChannelCount, AAudioStreamBuilder_SetIntFunction)
    GETFUNCTION(AAudioStreamBuilder_setFormat, AAudioStreamBuilder_SetIntFunction)
    GETFUNCTION(AAudioStreamBuilder_setSharingMode, AAudioStreamBuilder_SetIntFunction)
    AAudioStreamBuilder_setInputPreset = (AAudioStreamBuilder_SetIntFunction)dlsym(aaudioLib, "AAudioStreamBuilder_setInputPreset"); // optional
    GETFUNCTION(AAudioStreamBuilder_setDataCallback, AAudioStreamBuilder_setDataCallbackFunction)
    GETFUNCTION(AAudioStreamBuilder_setErrorCallback, AAudioStreamBuilder_setErrorCallbackFunction)
    GETFUNCTION(AAudioStreamBuilder_openStream, AAudioStreamBuilder_openStreamFunction)
    GETFUNCTION(AAudioStreamBuilder_delete, AAudioStreamBuilder_deleteFunction)
    GETFUNCTION(AAudioStream_requestStart, AAudioStream_IntFunction)
    GETFUNCTION(AAudioStream_requestStop, AAudioStream_IntFunction)
    GETFUNCTION(AAudioStream_getXRunCount, AAudioStream_IntFunction)
    GETFUNCTION(AAudioStream_getFramesPerBurst, AAudioStream_IntFunction)
    GETFUNCTION(AAudioStream_getSampleRate, AAudioStream_IntFunction)
    GETFUNCTION(AAudioStream_getState, AAudioStream_IntFunction)
    GETFUNCTION(AAudioStream_close, AAudioStream_IntFunction)
    GETFUNCTION(AAudioStream_setBufferSizeInFrames, AAudioStream_setBufferSizeInFramesFunction)
    GETFUNCTION(AAudioStream_read, AAudioStream_readFunction)

    aaudioAvailable = true;
    return true;
}

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
    void clear() {
        this->readIndex = this->writeIndex = 0;
    }
} fifo;

typedef struct SuperpoweredAndroidAudioIOInternals {
    fifo inputFifo, outputFifo;
    AAudioStream *inputStream, *outputStream;
    void *clientdata;
    audioProcessingCallback callback;
    SLObjectItf openSLEngine, outputMix, outputBufferQueue, inputBufferQueue;
    SLAndroidSimpleBufferQueueItf outputBufferQueueInterface, inputBufferQueueInterface;
    size_t fifoBufferSizeBytes;
    int samplerate, buffersize, silenceFrames, numBuffers, bufferStep, aaudioBurstSize, aaudioFirstHalfSecond, aaudioXRunCount;
    bool hasOutput, hasInput, foreground, started, firstOutput, aaudio, firstAAudioInput, aaudioRestarting;
} SuperpoweredAndroidAudioIOInternals;

static void stopAAudio(SuperpoweredAndroidAudioIOInternals *internals) {
    if (!internals->started) return;
    internals->started = false;
    if (internals->outputStream) {
        AAudioStream_requestStop(internals->outputStream);
        AAudioStream_close(internals->outputStream);
    }
    if (internals->inputStream) {
        AAudioStream_requestStop(internals->inputStream);
        AAudioStream_close(internals->inputStream);
    }
    internals->outputStream = internals->inputStream = NULL;
}

static bool startAAudio(SuperpoweredAndroidAudioIOInternals *internals);

static void *restartAAudioThread(void *param) { // This is started by the error callback.
    SuperpoweredAndroidAudioIOInternals *internals = (SuperpoweredAndroidAudioIOInternals *)param;
    stopAAudio(internals);
    usleep(200000);
    startAAudio(internals);
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

static int32_t aaudioProcessingCallback(__unused AAudioStream *stream, void *userData, void *audioData, int32_t numFrames) {
    SuperpoweredAndroidAudioIOInternals *internals = (SuperpoweredAndroidAudioIOInternals *)userData;

    if (internals->inputStream) {
        if (internals->firstAAudioInput) {
            internals->firstAAudioInput = false;
            int32_t drainedFrames = 0;
            do {
                drainedFrames = AAudioStream_read(internals->inputStream, audioData, numFrames, 0);
            } while (drainedFrames > 0);
        }

        if (AAudioStream_read(internals->inputStream, audioData, numFrames, 0) != numFrames) {
            if (internals->outputStream) memset(audioData, 0, (size_t)numFrames * NUM_CHANNELS * 2);
            return AAUDIO_CALLBACK_RESULT_CONTINUE;
        }
    }

    bool makeSilence = false;
    if (!internals->callback(internals->clientdata, (short int *)audioData, numFrames, internals->samplerate)) {
        makeSilence = true;
        internals->silenceFrames += numFrames;
    } else internals->silenceFrames = 0;

    // Silence the output if it's not needed.
    if (!internals->hasOutput || makeSilence) memset(audioData, 0, (size_t)numFrames * NUM_CHANNELS * 2);

    if (!internals->foreground && (internals->silenceFrames > internals->samplerate)) {
        internals->silenceFrames = 0;
        stopAAudio(internals);
    };

    if (internals->aaudioFirstHalfSecond >= 0) internals->aaudioFirstHalfSecond -= numFrames;
    else {
        int xrunCount = internals->inputStream ? AAudioStream_getXRunCount(internals->inputStream) : 0;
        if (internals->outputStream) xrunCount += AAudioStream_getXRunCount(internals->outputStream);

        if (internals->aaudioXRunCount < xrunCount) {
            internals->aaudioXRunCount = xrunCount;
            if (internals->buffersize < 4096) internals->buffersize += internals->aaudioBurstSize;
            if (internals->inputStream) AAudioStream_setBufferSizeInFrames(internals->inputStream, internals->buffersize);
            if (internals->outputStream) AAudioStream_setBufferSizeInFrames(internals->outputStream, internals->buffersize);
        }
    }

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static void aaudioErrorCallback(AAudioStream *stream, void *userData, __unused int32_t error) {
    if (userData && (AAudioStream_getState(stream) == AAUDIO_STREAM_STATE_DISCONNECTED)) { // If the audio routing has been changed, restart audio I/O.
        SuperpoweredAndroidAudioIOInternals *internals = (SuperpoweredAndroidAudioIOInternals *)userData;
        if (!internals->aaudioRestarting) {
            internals->aaudioRestarting = true;
            pthread_t thread;
            pthread_create(&thread, NULL, restartAAudioThread, internals);
        }
    }
}

static bool startAAudio(SuperpoweredAndroidAudioIOInternals *internals) {
    if (internals->started) return true;
    internals->firstOutput = internals->started = true;
    internals->aaudioRestarting = false;
    AAudioStream *mainStream = NULL;

    // Theoretically AAudio should work with an input stream only.
    // However on many devices (such as the Samsung S10e) it doesn't return with audio if there is no output.
    // Therefore we set up an output stream even if not needed.
    //if (internals->hasOutput) {
        AAudioStreamBuilder *outputStreamBuilder;
        if (AAudio_createStreamBuilder(&outputStreamBuilder) != AAUDIO_OK) return false;

        AAudioStreamBuilder_setDirection(outputStreamBuilder, AAUDIO_DIRECTION_OUTPUT);
        AAudioStreamBuilder_setFormat(outputStreamBuilder, AAUDIO_FORMAT_PCM_I16);
        AAudioStreamBuilder_setChannelCount(outputStreamBuilder, NUM_CHANNELS);
        AAudioStreamBuilder_setSharingMode(outputStreamBuilder, AAUDIO_SHARING_MODE_EXCLUSIVE);
        AAudioStreamBuilder_setPerformanceMode(outputStreamBuilder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
        AAudioStreamBuilder_setErrorCallback(outputStreamBuilder, aaudioErrorCallback, internals);
        AAudioStreamBuilder_setDataCallback(outputStreamBuilder, aaudioProcessingCallback, internals);

        bool success = (AAudioStreamBuilder_openStream(outputStreamBuilder, &internals->outputStream) == AAUDIO_OK) && (internals->outputStream != NULL);
        AAudioStreamBuilder_delete(outputStreamBuilder);

        if (success) mainStream = internals->outputStream;
        else {
            internals->outputStream = NULL;
            return false;
        }
    //}

    if (internals->hasInput) {
        AAudioStreamBuilder *inputStreamBuilder;
        if (AAudio_createStreamBuilder(&inputStreamBuilder) != AAUDIO_OK) {
            if (internals->outputStream) {
                AAudioStream_close(internals->outputStream);
                internals->outputStream = NULL;
            }
            return false;
        }

        AAudioStreamBuilder_setDirection(inputStreamBuilder, AAUDIO_DIRECTION_INPUT);
        AAudioStreamBuilder_setFormat(inputStreamBuilder, AAUDIO_FORMAT_PCM_I16);
        AAudioStreamBuilder_setChannelCount(inputStreamBuilder, NUM_CHANNELS);
        //if (AAudioStreamBuilder_setInputPreset) AAudioStreamBuilder_setInputPreset(inputStreamBuilder, AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE);
        AAudioStreamBuilder_setSharingMode(inputStreamBuilder, AAUDIO_SHARING_MODE_EXCLUSIVE);
        AAudioStreamBuilder_setPerformanceMode(inputStreamBuilder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
        AAudioStreamBuilder_setErrorCallback(inputStreamBuilder, aaudioErrorCallback, NULL);

        if (!internals->outputStream) AAudioStreamBuilder_setDataCallback(inputStreamBuilder, aaudioProcessingCallback, internals);
        else AAudioStreamBuilder_setSampleRate(inputStreamBuilder, AAudioStream_getSampleRate(internals->outputStream));

        bool success = (AAudioStreamBuilder_openStream(inputStreamBuilder, &internals->inputStream) == AAUDIO_OK) && (internals->inputStream != NULL);
        AAudioStreamBuilder_delete(inputStreamBuilder);

        if (success) {
            if (!mainStream) mainStream = internals->inputStream;
        } else {
            if (internals->outputStream) AAudioStream_close(internals->outputStream);
            internals->inputStream = internals->outputStream = NULL;
            return false;
        }
    }

    internals->samplerate = AAudioStream_getSampleRate(mainStream);
    internals->aaudioBurstSize = AAudioStream_getFramesPerBurst(mainStream);
    internals->buffersize = internals->aaudioBurstSize * 2;
    internals->aaudioFirstHalfSecond = internals->samplerate / 2;
    internals->aaudioXRunCount = 0;

    if (internals->outputStream) {
        AAudioStream_setBufferSizeInFrames(internals->outputStream, internals->buffersize);

        if (AAudioStream_requestStart(internals->outputStream) != AAUDIO_OK) {
            AAudioStream_close(internals->outputStream);
            if (internals->inputStream) AAudioStream_close(internals->inputStream);
            internals->inputStream = internals->outputStream = NULL;
            return false;
        }
    }

    if (internals->inputStream) {
        AAudioStream_setBufferSizeInFrames(internals->inputStream, internals->buffersize);

        if (AAudioStream_requestStart(internals->inputStream) != AAUDIO_OK) {
            AAudioStream_close(internals->inputStream);
            if (internals->outputStream) AAudioStream_close(internals->outputStream);
            internals->inputStream = internals->outputStream = NULL;
            return false;
        }
    }

    return true;
}

// The entire operation is based on two Android Simple Buffer Queues, one for the audio input and one for the audio output.
static void startQueues(SuperpoweredAndroidAudioIOInternals *internals) {
    if (internals->started) return;
    internals->firstOutput = internals->started = true;

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
    internals->inputFifo.clear();
    internals->outputFifo.clear();
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
            if (internals->firstOutput) { // Check if at start there are too many input buffers received already and skip the excess ones.
                internals->firstOutput = false;
                internals->inputFifo.readIndex = internals->inputFifo.writeIndex - 1;
                if (internals->inputFifo.readIndex < 0) internals->inputFifo.readIndex = 0;
            }

            // Had to separate the input buffer from the output buffer, because it makes a feedback loop on some Android devices despite of memsetting everything to zero.
            memcpy(output, internals->inputFifo.buffer + internals->inputFifo.readIndex * internals->bufferStep, (size_t)internals->buffersize * NUM_CHANNELS * 2);
            internals->inputFifo.incRead(internals->numBuffers);

            if (!internals->callback(internals->clientdata, output, internals->buffersize, internals->samplerate)) {
                silence = true;
                internals->silenceFrames += internals->buffersize;
            } else internals->silenceFrames = 0;
        } else silence = true; // Dropout, not enough audio input.
    } else { // If audio input is not enabled.
        if (!internals->callback(internals->clientdata, output, internals->buffersize, internals->samplerate)) {
            silence = true;
            internals->silenceFrames += internals->buffersize;
        } else internals->silenceFrames = 0;
    };

    if (silence) memset(output, 0, (size_t)internals->buffersize * NUM_CHANNELS * 2);
    (*caller)->Enqueue(caller, output, (SLuint32)internals->buffersize * NUM_CHANNELS * 2);

    if (!internals->foreground && (internals->silenceFrames > internals->samplerate)) {
        internals->silenceFrames = 0;
        stopQueues(internals);
    };
}

SuperpoweredAndroidAudioIO::SuperpoweredAndroidAudioIO(int samplerate, int buffersize, bool enableInput, bool enableOutput, audioProcessingCallback callback, void *clientdata, int inputStreamType, int outputStreamType) {
    static const SLboolean requireds[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_FALSE };
    if (buffersize > 1024) buffersize = 1024;

    internals = new SuperpoweredAndroidAudioIOInternals;
    memset(internals, 0, sizeof(SuperpoweredAndroidAudioIOInternals));
    internals->aaudio = initializeAAudio();
    internals->samplerate = samplerate;
    internals->buffersize = buffersize;
    internals->clientdata = clientdata;
    internals->callback = callback;
    internals->hasInput = enableInput;
    internals->hasOutput = enableOutput;
    internals->foreground = true;
    internals->started = false;

    switch (inputStreamType) {
        case SL_ANDROID_RECORDING_PRESET_CAMCORDER:
        case SL_ANDROID_RECORDING_PRESET_VOICE_COMMUNICATION:
            internals->aaudio = false; break;
        default:;
    }

    switch (outputStreamType) {
        case SL_ANDROID_STREAM_MEDIA:
        case -1: break;
        default: internals->aaudio = false;
    }

    if (internals->aaudio) internals->aaudio = startAAudio(internals);
    if (!internals->aaudio) {
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

            if ((*internals->inputBufferQueue)->Realize(internals->inputBufferQueue, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) { // Record permission refused. You need to handle this case in Java!
                (*internals->inputBufferQueue)->Destroy(internals->inputBufferQueue);
                internals->inputBufferQueue = NULL;
                free(internals->inputFifo.buffer);
                internals->inputFifo.buffer = NULL;
                enableInput = internals->hasInput = false;
            }
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
}

void SuperpoweredAndroidAudioIO::onForeground() {
    internals->foreground = true;
    if (internals->aaudio) startAAudio(internals); else startQueues(internals);
}

void SuperpoweredAndroidAudioIO::onBackground() {
    internals->foreground = false;
}

void SuperpoweredAndroidAudioIO::start() {
    if (internals->aaudio) startAAudio(internals); else startQueues(internals);
}

void SuperpoweredAndroidAudioIO::stop() {
    if (internals->aaudio) stopAAudio(internals); else stopQueues(internals);
}

SuperpoweredAndroidAudioIO::~SuperpoweredAndroidAudioIO() {
    if (internals->aaudio) stopAAudio(internals);
    else {
        stopQueues(internals);
        usleep(200000);
        if (internals->outputBufferQueue) (*internals->outputBufferQueue)->Destroy(internals->outputBufferQueue);
        if (internals->inputBufferQueue) (*internals->inputBufferQueue)->Destroy(internals->inputBufferQueue);
        (*internals->outputMix)->Destroy(internals->outputMix);
        (*internals->openSLEngine)->Destroy(internals->openSLEngine);
        if (internals->inputFifo.buffer) free(internals->inputFifo.buffer);
        if (internals->outputFifo.buffer) free(internals->outputFifo.buffer);
    }
    delete internals;
}
