#include <jni.h>
#include <string>
#include <android/log.h>
#include <AndroidIO/SuperpoweredAndroidAudioIO.h>
#include <Superpowered.h>
#include <SuperpoweredAdvancedAudioPlayer.h>
#include <SuperpoweredSimple.h>
#include <SuperpoweredCPU.h>
#include <malloc.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <SLES/OpenSLES.h>

#define log_print __android_log_print
#define log_write __android_log_write

static SuperpoweredAndroidAudioIO *audioIO;
static SuperpoweredAdvancedAudioPlayer *player;
static float *floatBuffer;

// This is called periodically by the audio engine.
static bool audioProcessing (
        void * __unused clientdata, // custom pointer
        short int *audio,           // buffer of interleaved samples
        int numberOfFrames,         // number of frames to process
        int __unused samplerate     // sampling rate
) {
    if (player->process(floatBuffer, false, (unsigned int)numberOfFrames)) {
        SuperpoweredFloatToShortInt(floatBuffer, audio, (unsigned int)numberOfFrames);
        return true;
    } else {
        return false;
    }
}

// Handle player events here.
static void playerEventCallback(
        void __unused *clientData,
        SuperpoweredAdvancedAudioPlayerEvent event,
        void *value
) {
    switch (event) {
        case SuperpoweredAdvancedAudioPlayerEvent_EOF:
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_ProgressiveDownloadError:
        case SuperpoweredAdvancedAudioPlayerEvent_HLSNetworkError:
        case SuperpoweredAdvancedAudioPlayerEvent_LoadError:
            log_print(ANDROID_LOG_ERROR, "HLSPlayer", "Load error: %s", (char *)value);
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess:
            log_write(ANDROID_LOG_DEBUG, "HLSPlayer", "Load success.");
            break;
        default:;
    }
}

// StartAudio - Start audio engine and initialize player.
extern "C" JNIEXPORT void
Java_com_superpowered_hls_MainActivity_StartAudio (
        JNIEnv * __unused env,
        jobject  __unused obj,
        jint samplerate,
        jint buffersize
) {
    SuperpoweredInitialize(
            "ExampleLicenseKey-WillExpire-OnNextUpdate",
            false, // enableAudioAnalysis (using SuperpoweredAnalyzer, SuperpoweredLiveAnalyzer, SuperpoweredWaveform or SuperpoweredBandpassFilterbank)
            false, // enableFFTAndFrequencyDomain (using SuperpoweredFrequencyDomain, SuperpoweredFFTComplex, SuperpoweredFFTReal or SuperpoweredPolarFFT)
            false, // enableAudioTimeStretching (using SuperpoweredTimeStretching)
            false, // enableAudioEffects (using any SuperpoweredFX class)
            true, // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
            false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
            false  // enableNetworking (using Superpowered::httpRequest)
    );

    // Allocate audio buffer.
    floatBuffer = (float *)malloc(sizeof(float) * 2 * buffersize);

    // Initialize player and pass event callback function.
    player = new SuperpoweredAdvancedAudioPlayer (
            NULL,                           // clientData
            playerEventCallback,            // callback function
            (unsigned int)samplerate,       // sampling rate
            0                               // cachedPointCount
    );

    // Initialize audio engine with process callback function.
    audioIO = new SuperpoweredAndroidAudioIO (
            samplerate,                     // sampling rate
            buffersize,                     // buffer size
            false,                          // enableInput
            true,                           // enableOutput
            audioProcessing,                // process callback function
            NULL,                           // clientData
            -1,                             // inputStreamType (-1 = default)
            SL_ANDROID_STREAM_MEDIA         // outputStreamType (-1 = default)
    );
}

// SetTempFolder - Set temporary folder for the audio player.
extern "C" JNIEXPORT void
Java_com_superpowered_hls_MainActivity_SetTempFolder (
        JNIEnv *env,
        jobject __unused obj,
        jstring path
) {
    const char *str = env->GetStringUTFChars(path, 0);
    log_print(ANDROID_LOG_DEBUG, "HLSPlayer", "Temp folder: %s", str);
    SuperpoweredAdvancedAudioPlayer::setTempFolder(str);
    env->ReleaseStringUTFChars(path, str);
}

// OpenHLS - Open HTTP Live Stream from specified URL.
extern "C" JNIEXPORT void
Java_com_superpowered_hls_MainActivity_OpenHLS (
        JNIEnv *env,
        jobject __unused obj,
        jstring url
) {
    const char *str = env->GetStringUTFChars(url, 0);
    player->openHLS(str);
    env->ReleaseStringUTFChars(url, str);
}

// TogglePlayback - Toggle Play/Pause state of the player.
extern "C" JNIEXPORT void
Java_com_superpowered_hls_MainActivity_TogglePlayback (
        JNIEnv * __unused env,
        jobject __unused obj
) {
    player->togglePlayback();
    SuperpoweredCPU::setSustainedPerformanceMode(player->playing);  // prevent dropouts
}

// onBackground - Put audio processing to sleep.
extern "C" JNIEXPORT void
Java_com_superpowered_hls_MainActivity_onBackground (
        JNIEnv * __unused env,
        jobject __unused obj
) {
    audioIO->onBackground();
}

// onForeground - Resume audio processing.
extern "C" JNIEXPORT void
Java_com_superpowered_hls_MainActivity_onForeground (
        JNIEnv * __unused env,
        jobject __unused obj
) {
    audioIO->onForeground();
}

// Cleanup - Free resources.
extern "C" JNIEXPORT void
Java_com_superpowered_hls_MainActivity_Cleanup (
        JNIEnv * __unused env,
        jobject __unused obj
) {
    delete audioIO;
    delete player;
    free(floatBuffer);
    SuperpoweredAdvancedAudioPlayer::clearTempFolder();
}
