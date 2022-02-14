#include <jni.h>
#include <string>
#include <OpenSource/SuperpoweredAndroidAudioIO.h>
#include <Superpowered.h>
#include <SuperpoweredReverb.h>
#include <SuperpoweredSimple.h>
#include <malloc.h>

static SuperpoweredAndroidAudioIO *audioIO;
static Superpowered::Reverb *reverb;

// This is called periodically by the audio engine.
static bool audioProcessing (
        void * __unused clientdata, // custom pointer
        short int *audio,           // output buffer
        int numberOfFrames,         // number of frames to process
        int samplerate              // current sample rate in Hz
) {
    reverb->samplerate = (unsigned int)samplerate;
    float floatBuffer[numberOfFrames * 2];

    Superpowered::ShortIntToFloat(audio, floatBuffer, (unsigned int)numberOfFrames);
    reverb->process(floatBuffer, floatBuffer, (unsigned int)numberOfFrames);
    Superpowered::FloatToShortInt(floatBuffer, audio, (unsigned int)numberOfFrames);
    return true;
}

// StartAudio - Start audio engine.
extern "C" JNIEXPORT void
Java_com_superpowered_effect_MainActivity_StartAudio(JNIEnv * __unused env, jobject  __unused obj, jint samplerate, jint buffersize) {
    Superpowered::Initialize("ExampleLicenseKey-WillExpire-OnNextUpdate");

    // initialize reverb
    reverb = new Superpowered::Reverb((unsigned int)samplerate);
    reverb->enabled = true;

    // init audio with audio callback function
    audioIO = new SuperpoweredAndroidAudioIO (
            samplerate,                     // sampling rate
            buffersize,                     // buffer size
            true,                           // enableInput
            true,                           // enableOutput
            audioProcessing,                // process callback function
            NULL,                           // clientData
            -1,                             // inputStreamType (-1 = default)
            -1                              // outputStreamType (-1 = default)
    );
}

// StopAudio - Stop audio engine and free resources.
extern "C" JNIEXPORT void
Java_com_superpowered_effect_MainActivity_StopAudio(JNIEnv * __unused env, jobject __unused obj) {
    delete audioIO;
    delete reverb;
}

// onBackground - Put audio processing to sleep if no audio is playing.
extern "C" JNIEXPORT void
Java_com_superpowered_effect_MainActivity_onBackground(JNIEnv * __unused env, jobject __unused obj) {
    audioIO->onBackground();
}

// onForeground - Resume audio processing.
extern "C" JNIEXPORT void
Java_com_superpowered_effect_MainActivity_onForeground(JNIEnv * __unused env, jobject __unused obj) {
    audioIO->onForeground();
}
