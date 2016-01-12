#include <jni.h>
#include <stdlib.h>
#include "SuperpoweredAdvancedAudioPlayer.h"
#include "SuperpoweredAndroidAudioIO.h"
#include "SuperpoweredSimple.h"
#include <android/log.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

static SuperpoweredAndroidAudioIO *audioIO;
static SuperpoweredAdvancedAudioPlayer *player;
static float *floatBuffer;

// Called by the player
static void playerEventCallback(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event, void *value) {
    switch (event) {
        case SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess: player->play(false); break;
        case SuperpoweredAdvancedAudioPlayerEvent_LoadError: __android_log_print(ANDROID_LOG_DEBUG, "HLSExample", "Open error: %s", (char *)value); break;
        case SuperpoweredAdvancedAudioPlayerEvent_EOF: player->seek(0); break;
        default:;
    };
}

// This is called periodically by the media server.
static bool audioProcessing(void *clientdata, short int *audioInputOutput, int numberOfSamples, int samplerate) {
    if (player->process(floatBuffer, false, numberOfSamples)) {
        SuperpoweredFloatToShortInt(floatBuffer, audioInputOutput, numberOfSamples);
        return true;
    } else return false;
}

// Ugly Java-native bridges - JNI, that is.
extern "C" {
    JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_SetTempFolder(JNIEnv *javaEnvironment, jobject self, jstring path);
    JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_StartAudio(JNIEnv *javaEnvironment, jobject self, jlong samplerate, jlong buffersize);
    JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_onForeground(JNIEnv *javaEnvironment, jobject self);
    JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_onBackground(JNIEnv *javaEnvironment, jobject self);
    JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_Open(JNIEnv *javaEnvironment, jobject self, jstring url);
    JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_Seek(JNIEnv *javaEnvironment, jobject self, jfloat percent);
    JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_SetDownloadStrategy(JNIEnv *javaEnvironment, jobject self, jlong optionIndex);
    JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_PlayPause(JNIEnv *javaEnvironment, jobject self);
    JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_SetSpeed(JNIEnv *javaEnvironment, jobject self, jlong fast);
    JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_UpdateStatus(JNIEnv *javaEnvironment, jobject self);
    JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_Cleanup(JNIEnv *javaEnvironment, jobject self);
}

JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_SetTempFolder(JNIEnv *javaEnvironment, jobject self, jstring path) {
    const char *str = javaEnvironment->GetStringUTFChars(path, 0);
    SuperpoweredAdvancedAudioPlayer::setTempFolder(str);
    javaEnvironment->ReleaseStringUTFChars(path, str);
}

// Setup and start audio output
JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_StartAudio(JNIEnv *javaEnvironment, jobject self, jlong samplerate, jlong buffersize) {
    floatBuffer = (float *)malloc(sizeof(float) * 2 * buffersize + 128);
    player = new SuperpoweredAdvancedAudioPlayer(NULL, playerEventCallback, samplerate, 0);
    audioIO = new SuperpoweredAndroidAudioIO(samplerate, buffersize, false, true, audioProcessing, NULL, -1, SL_ANDROID_STREAM_MEDIA, buffersize * 2);
}

JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_onForeground(JNIEnv *javaEnvironment, jobject self) {
    audioIO->onForeground();
}

JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_onBackground(JNIEnv *javaEnvironment, jobject self) {
    audioIO->onBackground();
}

JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_Open(JNIEnv *javaEnvironment, jobject self, jstring url) {
    const char *str = javaEnvironment->GetStringUTFChars(url, 0);
    player->open(str);
    javaEnvironment->ReleaseStringUTFChars(url, str);
}

JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_Seek(JNIEnv *javaEnvironment, jobject self, jfloat percent) {
    player->seek(percent);
}

JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_SetDownloadStrategy(JNIEnv *javaEnvironment, jobject self, jlong optionIndex) {
    switch (optionIndex) {
        case 1: player->downloadSecondsAhead = 20; break; // Will not buffer more than 20 seconds ahead of the playback position.
        case 2: player->downloadSecondsAhead = 40; break; // Will not buffer more than 40 seconds ahead of the playback position.
        case 3: player->downloadSecondsAhead = HLS_DOWNLOAD_EVERYTHING; break; // Will buffer everything after and before the playback position.
        default: player->downloadSecondsAhead = HLS_DOWNLOAD_REMAINING; // Will buffer everything after the playback position.
    };
}

JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_PlayPause(JNIEnv *javaEnvironment, jobject self) {
    player->togglePlayback();
}

JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_SetSpeed(JNIEnv *javaEnvironment, jobject self, jlong fast) {
    player->setTempo(fast ? 2.0f : 1.0f, true);
}

// Helper functions to update some Java object instance's member variables
static inline void setFloatField(JNIEnv *javaEnvironment, jobject self, jclass thisClass, const char *name, float value) {
    javaEnvironment->SetFloatField(self, javaEnvironment->GetFieldID(thisClass, name, "F"), value);
}

static inline void setLongField(JNIEnv *javaEnvironment, jobject self, jclass thisClass, const char *name, unsigned int value) {
    javaEnvironment->SetLongField(self, javaEnvironment->GetFieldID(thisClass, name, "J"), value);
}

static inline void setBoolField(JNIEnv *javaEnvironment, jobject self, jclass thisClass, const char *name, bool value) {
    javaEnvironment->SetBooleanField(self, javaEnvironment->GetFieldID(thisClass, name, "Z"), value);
}

// Called periodically on every UI update
JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_UpdateStatus(JNIEnv *javaEnvironment, jobject self) {
    jclass thisClass = javaEnvironment->GetObjectClass(self);
    //setFloatField(javaEnvironment, self, thisClass, "bufferStartPercent", player->bufferStartPercent);
    setFloatField(javaEnvironment, self, thisClass, "bufferEndPercent", player->bufferEndPercent);
    setLongField(javaEnvironment, self, thisClass, "durationSeconds", player->durationSeconds);
    setLongField(javaEnvironment, self, thisClass, "positionSeconds", player->positionSeconds);
    setFloatField(javaEnvironment, self, thisClass, "positionPercent", player->positionPercent);
    setBoolField(javaEnvironment, self, thisClass, "playing", player->playing);
}

JNIEXPORT void Java_com_superpowered_hlsexample_MainActivity_Cleanup(JNIEnv *javaEnvironment, jobject self) {
    delete audioIO;
    delete player;
    free(floatBuffer);
    SuperpoweredAdvancedAudioPlayer::clearTempFolder();
}
