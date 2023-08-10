#include "SuperpoweredEngineExample.h"
#include "Superpowered.h"
#include <jni.h>

static SuperpoweredEngineExample *example;

extern "C" {
JNIEXPORT void JNICALL
Java_com_superpoweredplayerwitheffects_SuperpoweredModule_nativeInit(JNIEnv *env, jobject thiz,
                                                                     jstring temp_dir) {
    Superpowered::Initialize("ExampleLicenseKey-WillExpire-OnNextUpdate");
    const char *str = env->GetStringUTFChars(temp_dir, nullptr);
    Superpowered::AdvancedAudioPlayer::setTempFolder(str);
    env->ReleaseStringUTFChars(temp_dir, str);

    example = new SuperpoweredEngineExample();
    example->play();
}

JNIEXPORT void JNICALL
Java_com_superpoweredplayerwitheffects_SuperpoweredModule_nativeTogglePlayback(JNIEnv *env,
                                                                               jobject thiz) {
    example->togglePlayback();
}

JNIEXPORT void JNICALL
Java_com_superpoweredplayerwitheffects_SuperpoweredModule_nativeEnableFlanger(JNIEnv *env,
                                                                              jobject thiz,
                                                                              jboolean enable) {
    example->enableFlanger(enable);
}
}