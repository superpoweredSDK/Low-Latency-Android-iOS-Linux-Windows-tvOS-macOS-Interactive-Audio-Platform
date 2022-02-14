#include <jni.h>
#include <string>
#include <android/log.h>
#include <OpenSource/SuperpoweredAndroidAudioIO.h>
#include <Superpowered.h>
#include <SuperpoweredAdvancedAudioPlayer.h>
#include <SuperpoweredSimple.h>
#include <SuperpoweredCPU.h>
#include <malloc.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <SLES/OpenSLES.h>

#define log_print __android_log_print

static SuperpoweredAndroidAudioIO *audioIO;
static Superpowered::AdvancedAudioPlayer *player;

// This is called periodically by the audio engine.
static bool audioProcessing (
        void * __unused clientdata, // custom pointer
        short int *audio,           // output buffer
        int numberOfFrames,         // number of frames to process
        int samplerate              // current sample rate in Hz
) {
    player->outputSamplerate = (unsigned int)samplerate;
    float playerOutput[numberOfFrames * 2];

    if (player->processStereo(playerOutput, false, (unsigned int)numberOfFrames)) {
        Superpowered::FloatToShortInt(playerOutput, audio, (unsigned int)numberOfFrames);
        return true;
    } else return false;
}

// StartAudio - Start audio engine and initialize player.
extern "C" JNIEXPORT void
Java_com_superpowered_playerexample_MainActivity_NativeInit(JNIEnv *env, jobject __unused obj, jint samplerate, jint buffersize, jstring tempPath) {
    Superpowered::Initialize("ExampleLicenseKey-WillExpire-OnNextUpdate");

    // setting the temp folder for progressive downloads or HLS playback
    // not needed for local file playback
    const char *str = env->GetStringUTFChars(tempPath, 0);
    Superpowered::AdvancedAudioPlayer::setTempFolder(str);
    env->ReleaseStringUTFChars(tempPath, str);

    // creating the player
    player = new Superpowered::AdvancedAudioPlayer((unsigned int)samplerate, 0);

    audioIO = new SuperpoweredAndroidAudioIO (
            samplerate,                     // device native sampling rate
            buffersize,                     // device native buffer size
            false,                          // enableInput
            true,                           // enableOutput
            audioProcessing,                // process callback function
            NULL,                           // clientData
            -1,                             // inputStreamType (-1 = default)
            SL_ANDROID_STREAM_MEDIA         // outputStreamType (-1 = default)
    );
}

// OpenFile - Open file in player, specifying offset and length.
extern "C" JNIEXPORT void
Java_com_superpowered_playerexample_MainActivity_OpenFileFromAPK (
        JNIEnv *env,
        jobject __unused obj,
        jstring path,       // path to APK file
        jint offset,        // offset of audio file
        jint length         // length of audio file
) {
    const char *str = env->GetStringUTFChars(path, 0);
    player->open(str, offset, length);
    env->ReleaseStringUTFChars(path, str);

    // open file from any path: player->open("file system path to file");
    // open file from network (progressive download): player->open("http://example.com/music.mp3");
    // open HLS stream: player->openHLS("http://example.com/stream");
}

// onUserInterfaceUpdate - Called periodically. Check and react to player events. This can be done in any thread.
extern "C" JNIEXPORT jboolean
Java_com_superpowered_playerexample_MainActivity_onUserInterfaceUpdate(JNIEnv * __unused env, jobject __unused obj) {
    switch (player->getLatestEvent()) {
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_None:
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_Opening: break; // do nothing
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_Opened: player->play(); break;
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_OpenFailed:
        {
            int openError = player->getOpenErrorCode();
            log_print(ANDROID_LOG_ERROR, "PlayerExample", "Open error %i: %s", openError, Superpowered::AdvancedAudioPlayer::statusCodeToString(openError));
        }
            break;
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_ConnectionLost:
            log_print(ANDROID_LOG_ERROR, "PlayerExample", "Network download failed."); break;
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_ProgressiveDownloadFinished:
            log_print(ANDROID_LOG_ERROR, "PlayerExample", "Download finished. Path: %s", player->getFullyDownloadedFilePath()); break;
    }

    if (player->eofRecently()) player->setPosition(0, false, false);
    return (jboolean)player->isPlaying();
}

// TogglePlayback - Toggle Play/Pause state of the player.
extern "C" JNIEXPORT void
Java_com_superpowered_playerexample_MainActivity_TogglePlayback(JNIEnv * __unused env, jobject __unused obj) {
    player->togglePlayback();
    Superpowered::CPU::setSustainedPerformanceMode(player->isPlaying()); // prevent dropouts
}

// onBackground - Put audio processing to sleep if no audio is playing.
extern "C" JNIEXPORT void
Java_com_superpowered_playerexample_MainActivity_onBackground(JNIEnv * __unused env, jobject __unused obj) {
    audioIO->onBackground();
}

// onForeground - Resume audio processing.
extern "C" JNIEXPORT void
Java_com_superpowered_playerexample_MainActivity_onForeground(JNIEnv * __unused env, jobject __unused obj) {
    audioIO->onForeground();
}

// Cleanup - Free resources.
extern "C" JNIEXPORT void
Java_com_superpowered_playerexample_MainActivity_Cleanup(JNIEnv * __unused env, jobject __unused obj) {
    delete audioIO;
    delete player;
}
