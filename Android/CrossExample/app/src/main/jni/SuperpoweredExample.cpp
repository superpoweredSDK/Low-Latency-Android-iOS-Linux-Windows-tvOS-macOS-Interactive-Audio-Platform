#include "SuperpoweredExample.h"
#include <jni.h>
#include <stdlib.h>
#include <stdio.h>
#include <android/log.h>

static void playerEventCallbackA(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event, void *value) {
    if (event == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) {
    	SuperpoweredAdvancedAudioPlayer *playerA = *((SuperpoweredAdvancedAudioPlayer **)clientData);
        playerA->setBpm(126.0f);
        playerA->setFirstBeatMs(353);
        playerA->setPosition(playerA->firstBeatMs, false, false);
    };
}

static void playerEventCallbackB(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event, void *value) {
    if (event == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) {
    	SuperpoweredAdvancedAudioPlayer *playerB = *((SuperpoweredAdvancedAudioPlayer **)clientData);
        playerB->setBpm(123.0f);
        playerB->setFirstBeatMs(40);
        playerB->setPosition(playerB->firstBeatMs, false, false);
    };
}

static void openSLESCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) {
	((SuperpoweredExample *)pContext)->process(caller);
}

static const SLboolean requireds[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

SuperpoweredExample::SuperpoweredExample(const char *path, int *params) : currentBuffer(0), buffersize(params[5]), activeFx(0), crossValue(0.0f), volB(0.0f), volA(1.0f * headroom) {
    pthread_mutex_init(&mutex, NULL); // This will keep our player volumes and playback states in sync.

    for (int n = 0; n < NUM_BUFFERS; n++) outputBuffer[n] = (float *)memalign(16, (buffersize + 16) * sizeof(float) * 2);

    unsigned int samplerate = params[4];

    playerA = new SuperpoweredAdvancedAudioPlayer(&playerA , playerEventCallbackA, samplerate, 0);
    playerA->open(path, params[0], params[1]);
    playerB = new SuperpoweredAdvancedAudioPlayer(&playerB, playerEventCallbackB, samplerate, 0);
    playerB->open(path, params[2], params[3]);

    playerA->syncMode = playerB->syncMode = SuperpoweredAdvancedAudioPlayerSyncMode_TempoAndBeat;

    roll = new SuperpoweredRoll(samplerate);
    filter = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Lowpass, samplerate);
    flanger = new SuperpoweredFlanger(samplerate);

    mixer = new SuperpoweredStereoMixer();

    // Create the OpenSL ES engine.
	slCreateEngine(&openSLEngine, 0, NULL, 0, NULL, NULL);
	(*openSLEngine)->Realize(openSLEngine, SL_BOOLEAN_FALSE);
	SLEngineItf openSLEngineInterface = NULL;
	(*openSLEngine)->GetInterface(openSLEngine, SL_IID_ENGINE, &openSLEngineInterface);
	// Create the output mix.
	(*openSLEngineInterface)->CreateOutputMix(openSLEngineInterface, &outputMix, 0, NULL, NULL);
	(*outputMix)->Realize(outputMix, SL_BOOLEAN_FALSE);
	SLDataLocator_OutputMix outputMixLocator = { SL_DATALOCATOR_OUTPUTMIX, outputMix };

	// Create the buffer queue player.
	SLDataLocator_AndroidSimpleBufferQueue bufferPlayerLocator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, NUM_BUFFERS };
	SLDataFormat_PCM bufferPlayerFormat = { SL_DATAFORMAT_PCM, 2, samplerate * 1000, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN };
	SLDataSource bufferPlayerSource = { &bufferPlayerLocator, &bufferPlayerFormat };
    const SLInterfaceID bufferPlayerInterfaces[1] = { SL_IID_BUFFERQUEUE };
    SLDataSink bufferPlayerOutput = { &outputMixLocator, NULL };
    (*openSLEngineInterface)->CreateAudioPlayer(openSLEngineInterface, &bufferPlayer, &bufferPlayerSource, &bufferPlayerOutput, 1, bufferPlayerInterfaces, requireds);
    (*bufferPlayer)->Realize(bufferPlayer, SL_BOOLEAN_FALSE);

    // Initialize and start the buffer queue.
    (*bufferPlayer)->GetInterface(bufferPlayer, SL_IID_BUFFERQUEUE, &bufferQueue);
    (*bufferQueue)->RegisterCallback(bufferQueue, openSLESCallback, this);
    memset(outputBuffer[0], 0, buffersize * 4);
    memset(outputBuffer[1], 0, buffersize * 4);
    (*bufferQueue)->Enqueue(bufferQueue, outputBuffer[0], buffersize * 4);
    (*bufferQueue)->Enqueue(bufferQueue, outputBuffer[1], buffersize * 4);
    SLPlayItf bufferPlayerPlayInterface;
    (*bufferPlayer)->GetInterface(bufferPlayer, SL_IID_PLAY, &bufferPlayerPlayInterface);
    (*bufferPlayerPlayInterface)->SetPlayState(bufferPlayerPlayInterface, SL_PLAYSTATE_PLAYING);
}

SuperpoweredExample::~SuperpoweredExample() {
	for (int n = 0; n < NUM_BUFFERS; n++) free(outputBuffer[n]);
    delete playerA;
    delete playerB;
    delete mixer;
    pthread_mutex_destroy(&mutex);
}

void SuperpoweredExample::onPlayPause(bool play) {
    pthread_mutex_lock(&mutex);
    if (!play) {
        playerA->pause();
        playerB->pause();
    } else {
        bool masterIsA = (crossValue <= 0.5f);
        playerA->play(!masterIsA);
        playerB->play(masterIsA);
    };
    pthread_mutex_unlock(&mutex);
}

void SuperpoweredExample::onCrossfader(int value) {
    pthread_mutex_lock(&mutex);
    crossValue = float(value) * 0.01f;
    if (crossValue < 0.01f) {
        volA = 1.0f * headroom;
        volB = 0.0f;
    } else if (crossValue > 0.99f) {
        volA = 0.0f;
        volB = 1.0f * headroom;
    } else { // constant power curve
        volA = cosf(M_PI_2 * crossValue) * headroom;
        volB = cosf(M_PI_2 * (1.0f - crossValue)) * headroom;
    };
    pthread_mutex_unlock(&mutex);
}

void SuperpoweredExample::onFxSelect(int value) {
	__android_log_print(ANDROID_LOG_VERBOSE, "SuperpoweredExample", "FXSEL %i", value);
	activeFx = value;
}

void SuperpoweredExample::onFxOff() {
    filter->enable(false);
    roll->enable(false);
    flanger->enable(false);
}

#define MINFREQ 60.0f
#define MAXFREQ 20000.0f

static inline float floatToFrequency(float value) {
    if (value > 0.97f) return MAXFREQ;
    if (value < 0.03f) return MINFREQ;
    value = powf(10.0f, (value + ((0.4f - fabsf(value - 0.4f)) * 0.3f)) * log10f(MAXFREQ - MINFREQ)) + MINFREQ;
    return value < MAXFREQ ? value : MAXFREQ;
}

void SuperpoweredExample::onFxValue(int ivalue) {
    float value = float(ivalue) * 0.01f;
    switch (activeFx) {
        case 1:
            filter->setResonantParameters(floatToFrequency(1.0f - value), 0.2f);
            filter->enable(true);
            flanger->enable(false);
            roll->enable(false);
            break;
        case 2:
            if (value > 0.8f) roll->beats = 0.0625f;
            else if (value > 0.6f) roll->beats = 0.125f;
            else if (value > 0.4f) roll->beats = 0.25f;
            else if (value > 0.2f) roll->beats = 0.5f;
            else roll->beats = 1.0f;
            roll->enable(true);
            filter->enable(false);
            flanger->enable(false);
            break;
        default:
            flanger->setWet(value);
            flanger->enable(true);
            filter->enable(false);
            roll->enable(false);
    };
}

void SuperpoweredExample::process(SLAndroidSimpleBufferQueueItf caller) {
    pthread_mutex_lock(&mutex);
    float *stereoBuffer = outputBuffer[currentBuffer];

    bool masterIsA = (crossValue <= 0.5f);
    float masterBpm = masterIsA ? playerA->currentBpm : playerB->currentBpm;
    double msElapsedSinceLastBeatA = playerA->msElapsedSinceLastBeat; // When playerB needs it, playerA has already stepped this value, so save it now.

    bool silence = !playerA->process(stereoBuffer, false, buffersize, volA, masterBpm, playerB->msElapsedSinceLastBeat);
    if (playerB->process(stereoBuffer, !silence, buffersize, volB, masterBpm, msElapsedSinceLastBeatA)) silence = false;

    roll->bpm = flanger->bpm = masterBpm; // Syncing fx is one line.

    if (roll->process(silence ? NULL : stereoBuffer, stereoBuffer, buffersize) && silence) silence = false;
    if (!silence) {
        filter->process(stereoBuffer, stereoBuffer, buffersize);
        flanger->process(stereoBuffer, stereoBuffer, buffersize);
    };

    pthread_mutex_unlock(&mutex);

    // The stereoBuffer is ready now, let's put the finished audio into the requested buffers.
    if (silence) memset(stereoBuffer, 0, buffersize * 4); else SuperpoweredStereoMixer::floatToShortInt(stereoBuffer, (short int *)stereoBuffer, buffersize);

	(*caller)->Enqueue(caller, stereoBuffer, buffersize * 4);
	if (currentBuffer < NUM_BUFFERS - 1) currentBuffer++; else currentBuffer = 0;
}

extern "C" {
	JNIEXPORT void Java_com_superpowered_crossexample_MainActivity_SuperpoweredExample(JNIEnv *javaEnvironment, jobject self, jstring apkPath, jlongArray offsetAndLength);
	JNIEXPORT void Java_com_superpowered_crossexample_MainActivity_onPlayPause(JNIEnv *javaEnvironment, jobject self, jboolean play);
	JNIEXPORT void Java_com_superpowered_crossexample_MainActivity_onCrossfader(JNIEnv *javaEnvironment, jobject self, jint value);
	JNIEXPORT void Java_com_superpowered_crossexample_MainActivity_onFxSelect(JNIEnv *javaEnvironment, jobject self, jint value);
	JNIEXPORT void Java_com_superpowered_crossexample_MainActivity_onFxOff(JNIEnv *javaEnvironment, jobject self);
	JNIEXPORT void Java_com_superpowered_crossexample_MainActivity_onFxValue(JNIEnv *javaEnvironment, jobject self, jint value);
}

static SuperpoweredExample *example = NULL;

// Android is not passing more than 2 custom parameters, so we had to pack file offsets and lengths into an array.
JNIEXPORT void Java_com_superpowered_crossexample_MainActivity_SuperpoweredExample(JNIEnv *javaEnvironment, jobject self, jstring apkPath, jlongArray params) {
	// Convert the input jlong array to a regular int array.
    jlong *longParams = javaEnvironment->GetLongArrayElements(params, JNI_FALSE);
    int arr[6];
    for (int n = 0; n < 6; n++) arr[n] = longParams[n];
    javaEnvironment->ReleaseLongArrayElements(params, longParams, JNI_ABORT);

    const char *path = javaEnvironment->GetStringUTFChars(apkPath, JNI_FALSE);
    example = new SuperpoweredExample(path, arr);
    javaEnvironment->ReleaseStringUTFChars(apkPath, path);

}

JNIEXPORT void Java_com_superpowered_crossexample_MainActivity_onPlayPause(JNIEnv *javaEnvironment, jobject self, jboolean play) {
	example->onPlayPause(play);
}

JNIEXPORT void Java_com_superpowered_crossexample_MainActivity_onCrossfader(JNIEnv *javaEnvironment, jobject self, jint value) {
	example->onCrossfader(value);
}

JNIEXPORT void Java_com_superpowered_crossexample_MainActivity_onFxSelect(JNIEnv *javaEnvironment, jobject self, jint value) {
	example->onFxSelect(value);
}

JNIEXPORT void Java_com_superpowered_crossexample_MainActivity_onFxOff(JNIEnv *javaEnvironment, jobject self) {
	example->onFxOff();
}

JNIEXPORT void Java_com_superpowered_crossexample_MainActivity_onFxValue(JNIEnv *javaEnvironment, jobject self, jint value) {
	example->onFxValue(value);
}
