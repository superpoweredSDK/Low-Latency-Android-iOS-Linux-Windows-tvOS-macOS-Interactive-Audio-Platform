#include <jni.h>
#include <stdlib.h>
#include <SuperpoweredAdvancedAudioPlayer.h>
#include <AndroidIO/SuperpoweredAndroidAudioIO.h>
#include <SuperpoweredSimple.h>
#include <SuperpoweredCPU.h>
#include <android/log.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

static SuperpoweredAndroidAudioIO *audioIO;
static SuperpoweredAdvancedAudioPlayer *player;
static float *floatBuffer;

// Called by the player
static void playerEventCallback(void * __unused clientData, SuperpoweredAdvancedAudioPlayerEvent event, void *value) {
    switch (event) {
        case SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess: player->play(false); break;
        case SuperpoweredAdvancedAudioPlayerEvent_LoadError: __android_log_print(ANDROID_LOG_DEBUG, "HLSExample", "Open error: %s", (char *)value); break;
        case SuperpoweredAdvancedAudioPlayerEvent_EOF: player->seek(0); break;
        default:;
    };
}

// This is called periodically by the media server.
static bool audioProcessing(void * __unused clientdata, short int *audioInputOutput, int numberOfSamples, int __unused samplerate) {
    if (player->process(floatBuffer, false, (unsigned int)numberOfSamples)) {
        SuperpoweredFloatToShortInt(floatBuffer, audioInputOutput, (unsigned int)numberOfSamples);
        return true;
    } else return false;
}

extern "C" JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_SetTempFolder(JNIEnv *javaEnvironment, jobject __unused obj, jstring path) {
    const char *str = javaEnvironment->GetStringUTFChars(path, 0);
    SuperpoweredAdvancedAudioPlayer::setTempFolder(str);
    javaEnvironment->ReleaseStringUTFChars(path, str);
}

// Setup and start audio output
extern "C" JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_StartAudio(JNIEnv * __unused javaEnvironment, jobject __unused obj, jint samplerate, jint buffersize) {
    floatBuffer = (float *)malloc(sizeof(float) * 2 * buffersize + 128);
    player = new SuperpoweredAdvancedAudioPlayer(NULL, playerEventCallback, (unsigned int)samplerate, 0);
    audioIO = new SuperpoweredAndroidAudioIO(samplerate, buffersize, false, true, audioProcessing, NULL, -1, SL_ANDROID_STREAM_MEDIA, buffersize * 2);
}

extern "C" JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_onForeground(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    audioIO->onForeground();
}

extern "C" JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_onBackground(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    audioIO->onBackground();
}

extern "C" JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_Open(JNIEnv *javaEnvironment, jobject __unused obj, jstring url) {
    const char *str = javaEnvironment->GetStringUTFChars(url, 0);
    player->openHLS(str);
    javaEnvironment->ReleaseStringUTFChars(url, str);
}

extern "C" JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_Seek(JNIEnv * __unused javaEnvironment, jobject __unused obj, jfloat percent) {
    player->seek(percent);
}

extern "C" JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_SetDownloadStrategy(JNIEnv * __unused javaEnvironment, jobject __unused obj, jint optionIndex) {
    switch (optionIndex) {
        case 1: player->downloadSecondsAhead = 20; break; // Will not buffer more than 20 seconds ahead of the playback position.
        case 2: player->downloadSecondsAhead = 40; break; // Will not buffer more than 40 seconds ahead of the playback position.
        case 3: player->downloadSecondsAhead = HLS_DOWNLOAD_EVERYTHING; break; // Will buffer everything after and before the playback position.
        default: player->downloadSecondsAhead = HLS_DOWNLOAD_REMAINING; // Will buffer everything after the playback position.
    };
}

extern "C" JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_PlayPause(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    player->togglePlayback();
    SuperpoweredCPU::setSustainedPerformanceMode(player->playing);
}

extern "C" JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_SetSpeed(JNIEnv * __unused javaEnvironment, jobject __unused obj, jboolean fast) {
    player->setTempo(fast ? 2.0f : 1.0f, true);
}

// Helper functions to update some Java object instance's member variables
static inline void setFloatField(JNIEnv *javaEnvironment, jobject obj, jclass thisClass, const char *name, float value) {
    javaEnvironment->SetFloatField(obj, javaEnvironment->GetFieldID(thisClass, name, "F"), value);
}

static inline void setLongField(JNIEnv *javaEnvironment, jobject obj, jclass thisClass, const char *name, unsigned int value) {
    javaEnvironment->SetLongField(obj, javaEnvironment->GetFieldID(thisClass, name, "J"), value);
}

static inline void setBoolField(JNIEnv *javaEnvironment, jobject obj, jclass thisClass, const char *name, bool value) {
    javaEnvironment->SetBooleanField(obj, javaEnvironment->GetFieldID(thisClass, name, "Z"), (jboolean)value);
}

// Called periodically on every UI update
extern "C" JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_UpdateStatus(JNIEnv *javaEnvironment, jobject obj) {
    jclass thisClass = javaEnvironment->GetObjectClass(obj);
    setFloatField(javaEnvironment, obj, thisClass, "bufferEndPercent", player->bufferEndPercent);
    setLongField(javaEnvironment, obj, thisClass, "durationSeconds", player->durationSeconds);
    setLongField(javaEnvironment, obj, thisClass, "positionSeconds", player->positionSeconds);
    setFloatField(javaEnvironment, obj, thisClass, "positionPercent", player->positionPercent);
    setBoolField(javaEnvironment, obj, thisClass, "playing", player->playing);
}

extern "C" JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_Cleanup(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    delete audioIO;
    delete player;
    free(floatBuffer);
    SuperpoweredAdvancedAudioPlayer::clearTempFolder();
}
