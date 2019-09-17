#include <jni.h>
#include <string>
#include <android/log.h>
#include <OpenSource/SuperpoweredAndroidAudioIO.h>
#include <Superpowered.h>
#include <SuperpoweredSimple.h>
#include <SuperpoweredRecorder.h>
#include <unistd.h>

#define log_write __android_log_write

static SuperpoweredAndroidAudioIO *audioIO;
static Superpowered::Recorder *recorder;

// This is called periodically by the audio I/O.
static bool audioProcessing (
        void * __unused clientdata, // custom pointer
        short int *audio,           // buffer of interleaved samples
        int numberOfFrames,         // number of frames to process
        int __unused samplerate     // current sample rate in Hz
) {
    float floatBuffer[numberOfFrames * 2];
    Superpowered::ShortIntToFloat(audio, floatBuffer, (unsigned int)numberOfFrames);
    recorder->recordInterleaved(floatBuffer, (unsigned int)numberOfFrames);
    return false;
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
    Superpowered::Initialize(
            "ExampleLicenseKey-WillExpire-OnNextUpdate",
            false, // enableAudioAnalysis (using SuperpoweredAnalyzer, SuperpoweredLiveAnalyzer, SuperpoweredWaveform or SuperpoweredBandpassFilterbank)
            false, // enableFFTAndFrequencyDomain (using SuperpoweredFrequencyDomain, SuperpoweredFFTComplex, SuperpoweredFFTReal or SuperpoweredPolarFFT)
            false, // enableAudioTimeStretching (using SuperpoweredTimeStretching)
            false, // enableAudioEffects (using any SuperpoweredFX class)
            false, // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
            false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
            false  // enableNetworking (using Superpowered::httpRequest)
    );

    // Get path strings.
    const char *temp = env->GetStringUTFChars(tempPath, 0);
    const char *dest = env->GetStringUTFChars(destPath, 0);

    // Initialize the recorder with a temporary file path.
    recorder = new Superpowered::Recorder(temp);
    // Start a new recording.
    recorder->prepare(
            dest,                     // destination path
            (unsigned int)samplerate, // sample rate in Hz
            true,                     // apply fade in/fade out
            1                         // minimum length of the recording in seconds
            );

    // Release path strings.
    env->ReleaseStringUTFChars(tempPath, temp);
    env->ReleaseStringUTFChars(destPath, dest);

    // Initialize audio engine with audio callback function.
    audioIO = new SuperpoweredAndroidAudioIO (
            samplerate,      // native sampe rate
            buffersize,      // native buffer size
            true,            // enableInput
            false,           // enableOutput
            audioProcessing, // process callback function
            NULL             // clientData
    );
}

// StopAudio - Stop audio engine and free audio buffer.
extern "C" JNIEXPORT void
Java_com_superpowered_recorder_MainActivity_StopRecording(JNIEnv * __unused env, jobject __unused obj) {
    recorder->stop();
    delete audioIO;

    // Wait until the recorder finished writing everything to disk.
    // It's better to do this asynchronously, but we're just blocking (sleeping) now.
    while (!recorder->isFinished()) usleep(100000);

    log_write(ANDROID_LOG_DEBUG, "RecorderExample", "Finished recording.");
    delete recorder;
}

// onBackground - Put audio processing to sleep if no audio is playing.
extern "C" JNIEXPORT void
Java_com_superpowered_recorder_MainActivity_onBackground(JNIEnv * __unused env, jobject __unused obj) {
    audioIO->onBackground();
}

// onForeground - Resume audio processing.
extern "C" JNIEXPORT void
Java_com_superpowered_recorder_MainActivity_onForeground(JNIEnv * __unused env, jobject __unused obj) {
    audioIO->onForeground();
}
