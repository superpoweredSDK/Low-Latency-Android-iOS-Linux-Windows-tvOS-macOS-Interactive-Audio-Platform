#include <jni.h>
#include <string>
#include <android/log.h>
#include <AndroidIO/SuperpoweredAndroidAudioIO.h>
#include <SuperpoweredSimple.h>
#include <SuperpoweredRecorder.h>
#include <malloc.h>

#define log_write __android_log_write

static SuperpoweredAndroidAudioIO *audioIO;
static SuperpoweredRecorder *recorder;
float *floatBuffer;

// This is called periodically by the audio engine.
static bool audioProcessing (
        void * __unused clientdata, // custom pointer
        short int *audio,           // buffer of interleaved samples
        int numberOfFrames,         // number of frames to process
        int __unused samplerate     // sampling rate
) {
    SuperpoweredShortIntToFloat(audio, floatBuffer, (unsigned int)numberOfFrames);
    recorder->process(floatBuffer, (unsigned int)numberOfFrames);
    return false;
}

// This is called after the recorder closed the WAV file.
static void recorderStopped (void * __unused clientdata) {
    log_write(ANDROID_LOG_DEBUG, "RecorderExample", "Finished recording.");
    delete recorder;
}

// StartAudio - Start audio engine.
extern "C" JNIEXPORT void
Java_com_superpowered_recorder_MainActivity_StartAudio (
        JNIEnv *env,
        jobject  __unused obj,
        jint samplerate,
        jint buffersize,
        jstring tempPath,       // path to a temporary file
        jstring destPath        // path to the destination file
) {

    // Get path strings.
    const char *temp = env->GetStringUTFChars(tempPath, 0);
    const char *dest = env->GetStringUTFChars(destPath, 0);

    // Initialize the recorder with a temporary file path.
    recorder = new SuperpoweredRecorder (
            temp,               // The full filesystem path of a temporarily file.
            (unsigned int)samplerate,   // Sampling rate.
            1,                  // The minimum length of a recording (in seconds).
            2,                  // The number of channels.
            false,              // applyFade (fade in/out at the beginning / end of the recording)
            recorderStopped,    // Called when the recorder finishes writing after stop().
            NULL                // A custom pointer your callback receives (clientData).
    );

    // Start the recorder with the destination file path.
    recorder->start(dest);

    // Release path strings.
    env->ReleaseStringUTFChars(tempPath, temp);
    env->ReleaseStringUTFChars(destPath, dest);

    // Initialize float audio buffer.
    floatBuffer = (float *)malloc(sizeof(float) * 2 * buffersize);

    // Initialize audio engine with audio callback function.
    audioIO = new SuperpoweredAndroidAudioIO (
            samplerate,                     // sampling rate
            buffersize,                     // buffer size
            true,                           // enableInput
            false,                          // enableOutput
            audioProcessing,                // process callback function
            NULL                            // clientData
    );
}

// StopAudio - Stop audio engine and free audio buffer.
extern "C" JNIEXPORT void
Java_com_superpowered_recorder_MainActivity_StopAudio (
        JNIEnv * __unused env,
        jobject __unused obj
) {
    recorder->stop();
    delete audioIO;
    free(floatBuffer);
}

// onBackground - Put audio processing to sleep.
extern "C" JNIEXPORT void
Java_com_superpowered_recorder_MainActivity_onBackground (
        JNIEnv * __unused env,
        jobject __unused obj
) {
    audioIO->onBackground();
}

// onForeground - Resume audio processing.
extern "C" JNIEXPORT void
Java_com_superpowered_recorder_MainActivity_onForeground (
        JNIEnv * __unused env,
        jobject __unused obj
) {
    audioIO->onForeground();
}
