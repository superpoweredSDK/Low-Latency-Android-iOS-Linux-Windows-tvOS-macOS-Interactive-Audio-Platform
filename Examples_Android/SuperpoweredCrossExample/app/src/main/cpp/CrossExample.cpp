#include "CrossExample.h"
#include <SuperpoweredSimple.h>
#include <SuperpoweredCPU.h>
#include <jni.h>
#include <stdio.h>
#include <android/log.h>
#include <stdlib.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#define log_print __android_log_print

// This is called by player A upon successful load.
static void playerEventCallbackA (
	void *clientData,   // &playerA
	SuperpoweredAdvancedAudioPlayerEvent event,
	void * __unused value
) {
    if (event == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) {
        // The pointer to the player is passed to the event callback via the custom clientData pointer.
    	SuperpoweredAdvancedAudioPlayer *playerA = *((SuperpoweredAdvancedAudioPlayer **)clientData);
        playerA->setBpm(126.0f);
        playerA->setFirstBeatMs(353);
        playerA->setPosition(playerA->firstBeatMs, false, false);
    };
}

// This is called by player B upon successful load.
static void playerEventCallbackB(
	void *clientData,   // &playerB
	SuperpoweredAdvancedAudioPlayerEvent event,
	void * __unused value
) {
    if (event == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) {
        // The pointer to the player is passed to the event callback via the custom clientData pointer.
    	SuperpoweredAdvancedAudioPlayer *playerB = *((SuperpoweredAdvancedAudioPlayer **)clientData);
        playerB->setBpm(123.0f);
        playerB->setFirstBeatMs(40);
        playerB->setPosition(playerB->firstBeatMs, false, false);
    };
}

// Audio callback function. Called by the audio engine.
static bool audioProcessing (
	void *clientdata,		    // A custom pointer your callback receives.
	short int *audioIO,		    // 16-bit stereo interleaved audio input and/or output.
	int numFrames,			    // The number of frames received and/or requested.
	int __unused samplerate	    // The current sample rate in Hz.
) {
	return ((CrossExample *)clientdata)->process(audioIO, (unsigned int)numFrames);
}

// Crossfader example - Initialize players and audio engine
CrossExample::CrossExample (
		unsigned int samplerate,    // sampling rate
		unsigned int buffersize,    // buffer size
        const char *path,           // path to APK package
		int fileAoffset,            // offset of file A in APK
		int fileAlength,            // length of file A
		int fileBoffset,            // offset of file B in APK
		int fileBlength             // length of file B
) : activeFx(0), crossValue(0.0f), volB(0.0f), volA(1.0f * headroom)
{
    // Allocate aligned memory for floating point buffer.
    stereoBuffer = (float *)memalign(16, buffersize * sizeof(float) * 2);

    // Initialize players and open audio files.
    playerA = new SuperpoweredAdvancedAudioPlayer(&playerA, playerEventCallbackA, samplerate, 0);
    playerA->open(path, fileAoffset, fileAlength);
    playerB = new SuperpoweredAdvancedAudioPlayer(&playerB, playerEventCallbackB, samplerate, 0);
    playerB->open(path, fileBoffset, fileBlength);

    playerA->syncMode = playerB->syncMode = SuperpoweredAdvancedAudioPlayerSyncMode_TempoAndBeat;

    // Setup effects.
    roll = new SuperpoweredRoll(samplerate);
    filter = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Lowpass, samplerate);
    flanger = new SuperpoweredFlanger(samplerate);

    // Initialize audio engine and pass callback function.
    audioSystem = new SuperpoweredAndroidAudioIO (
			samplerate,                     // sampling rate
			buffersize,                     // buffer size
			false,                          // enableInput
			true,                           // enableOutput
			audioProcessing,                // audio callback function
			this,                           // clientData
			-1,                             // inputStreamType (-1 = default)
			SL_ANDROID_STREAM_MEDIA         // outputStreamType (-1 = default)
	);
}

// Destructor. Free resources.
CrossExample::~CrossExample() {
    delete audioSystem;
    delete playerA;
    delete playerB;
    delete roll;
    delete filter;
    delete flanger;
    free(stereoBuffer);
}

// onPlayPause - Toggle playback state of players.
void CrossExample::onPlayPause(bool play) {
    if (!play) {
        playerA->pause();
        playerB->pause();
    } else {
        bool masterIsA = (crossValue <= 0.5f);
        playerA->play(!masterIsA);
        playerB->play(masterIsA);
    };
    SuperpoweredCPU::setSustainedPerformanceMode(play); // <-- Important to prevent audio dropouts.
}

// onCrossfader - Handle crossfader and adjust volume levels accordingly.
void CrossExample::onCrossfader(int value) {
    crossValue = float(value) * 0.01f;
    if (crossValue < 0.01f) {
        volA = 1.0f * headroom;
        volB = 0.0f;
    } else if (crossValue > 0.99f) {
        volA = 0.0f;
        volB = 1.0f * headroom;
    } else { // constant power curve
        volA = cosf(float(M_PI_2) * crossValue) * headroom;
        volB = cosf(float(M_PI_2) * (1.0f - crossValue)) * headroom;
    };
}

// onFxSelect - Select effect type.
void CrossExample::onFxSelect(int value) {
	log_print(ANDROID_LOG_VERBOSE, "CrossExample", "FXSEL %i", value);
	activeFx = (unsigned char)value;
}

// onFxOff - Turn off effects.
void CrossExample::onFxOff() {
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

// onFxValue - Adjust effect parameter.
void CrossExample::onFxValue(int ivalue) {
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

// Main process function where audio is generated.
bool CrossExample::process (
        short int *output,         // buffer to receive output samples
        unsigned int numFrames     // number of frames requested
) {
    bool masterIsA = (crossValue <= 0.5f);
    double masterBpm = masterIsA ? playerA->currentBpm : playerB->currentBpm;
    // When playerB needs it, playerA has already stepped this value, so save it now.
    double msElapsedSinceLastBeatA = playerA->msElapsedSinceLastBeat;

    // Request audio from player A.
    bool silence = !playerA->process (
            stereoBuffer,  // 32-bit interleaved stereo output buffer.
            false,         // bufferAdd - true: add to buffer / false: overwrite buffer
            numFrames,     // The number of frames to provide.
            volA,          // volume - 0.0f is silence, 1.0f is "original volume"
            masterBpm,     // BPM value to sync with.
            playerB->msElapsedSinceLastBeat // ms elapsed since the last beat on the other track.
    );

    // Request audio from player B.
    if (playerB->process(
            stereoBuffer,  // 32-bit interleaved stereo output buffer.
            !silence,      // bufferAdd - true: add to buffer / false: overwrite buffer
            numFrames,     // The number of frames to provide.
            volB,          // volume - 0.0f is silence, 1.0f is "original volume"
            masterBpm,     // BPM value to sync with.
            msElapsedSinceLastBeatA   // ms elapsed since the last beat on the other track.
    )) silence = false;

    roll->bpm = flanger->bpm = (float)masterBpm; // Syncing fx is one line.

    if (roll->process(silence ? NULL : stereoBuffer, stereoBuffer, numFrames) && silence) silence = false;
    if (!silence) {
        filter->process(stereoBuffer, stereoBuffer, numFrames);
        flanger->process(stereoBuffer, stereoBuffer, numFrames);
    };

    // The stereoBuffer is ready now, let's write the finished audio into the requested buffers.
    if (!silence) SuperpoweredFloatToShortInt(stereoBuffer, output, numFrames);
    return !silence;
}

static CrossExample *example = NULL;

// CrossExample - Create the DJ app and initialize the players.
extern "C" JNIEXPORT void
Java_com_superpowered_crossexample_MainActivity_CrossExample ( 
		JNIEnv *env,
		jobject __unused obj,
		jint samplerate,        // sampling rate
		jint buffersize,        // buffer size
		jstring apkPath,        // path to APK package
		jint fileAoffset,       // offset of file A in APK
		jint fileAlength,       // length of file A
		jint fileBoffset,       // offset of file B in APK
		jint fileBlength        // length of file B
) {
    const char *path = env->GetStringUTFChars(apkPath, JNI_FALSE);
    example = new CrossExample((unsigned int)samplerate, (unsigned int)buffersize,
			path, fileAoffset, fileAlength, fileBoffset, fileBlength);
    env->ReleaseStringUTFChars(apkPath, path);
}

// onPlayPause - Toggle playback state of player.
extern "C" JNIEXPORT void
Java_com_superpowered_crossexample_MainActivity_onPlayPause (
        JNIEnv * __unused env,
        jobject __unused obj,
        jboolean play
) {
	example->onPlayPause(play);
}

// onCrossfader - Handle crossfader events.
extern "C" JNIEXPORT void
Java_com_superpowered_crossexample_MainActivity_onCrossfader (
        JNIEnv * __unused env,
        jobject __unused obj,
        jint value
) {
	example->onCrossfader(value);
}

// onFxSelect - Handle FX selection.
extern "C" JNIEXPORT void
Java_com_superpowered_crossexample_MainActivity_onFxSelect (
        JNIEnv * __unused env,
        jobject __unused obj,
        jint value
) {
	example->onFxSelect(value);
}

// onFxOff - Turn of effects.
extern "C" JNIEXPORT void
Java_com_superpowered_crossexample_MainActivity_onFxOff (
        JNIEnv * __unused env,
        jobject __unused obj
) {
	example->onFxOff();
}

// onFxValue - Adjust FX value.
extern "C" JNIEXPORT void
Java_com_superpowered_crossexample_MainActivity_onFxValue (
        JNIEnv * __unused env,
        jobject __unused obj,
        jint value
) {
	example->onFxValue(value);
}
