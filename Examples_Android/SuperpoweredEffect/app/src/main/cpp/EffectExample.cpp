#include <jni.h>
#include <string>
#include <AndroidIO/SuperpoweredAndroidAudioIO.h>
#include <Superpowered.h>
#include <SuperpoweredReverb.h>
#include <SuperpoweredSimple.h>
#include <malloc.h>

static SuperpoweredAndroidAudioIO *audioIO;
static SuperpoweredReverb *reverb;
static float *floatBuffer;

// This is called periodically by the audio engine.
static bool audioProcessing (
        void * __unused clientdata, // custom pointer
        short int *audio,           // buffer of interleaved samples
        int numberOfFrames,         // number of frames to process
        int __unused samplerate     // sampling rate
) {
    SuperpoweredShortIntToFloat(audio, floatBuffer, (unsigned int)numberOfFrames);
    reverb->process(floatBuffer, floatBuffer, (unsigned int)numberOfFrames);
    SuperpoweredFloatToShortInt(floatBuffer, audio, (unsigned int)numberOfFrames);
    //memset(audio, 0, numberOfFrames * 4);
    return true;
}

// StartAudio - Start audio engine.
extern "C" JNIEXPORT void
Java_com_superpowered_effect_MainActivity_StartAudio (
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
            true,  // enableAudioEffects (using any SuperpoweredFX class)
            false, // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
            false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
            false  // enableNetworking (using Superpowered::httpRequest)
    );

    // allocate audio buffer
    floatBuffer = (float *)malloc(sizeof(float) * 2 * buffersize);

    // initialize reverb
    reverb = new SuperpoweredReverb((unsigned int)samplerate);
    reverb->enable(true);

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
Java_com_superpowered_effect_MainActivity_StopAudio (
        JNIEnv * __unused env,
        jobject __unused obj
) {
    delete audioIO;
    delete reverb;
    free(floatBuffer);
}

// onBackground - Put audio processing to sleep.
extern "C" JNIEXPORT void
Java_com_superpowered_effect_MainActivity_onBackground (
        JNIEnv * __unused env,
        jobject __unused obj
) {
    audioIO->onBackground();
}

// onForeground - Resume audio processing.
extern "C" JNIEXPORT void
Java_com_superpowered_effect_MainActivity_onForeground (
        JNIEnv * __unused env,
        jobject __unused obj
) {
    audioIO->onForeground();
}
