#include "CrossExample.h"
#include <Superpowered.h>
#include <SuperpoweredSimple.h>
#include <SuperpoweredCPU.h>
#include <jni.h>
#include <android/log.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

#define log_print __android_log_print

// Audio callback function. Called by the audio engine.
static bool audioProcessing (
	void *clientdata,	// A custom pointer your callback receives.
	short int *audioIO,	// 16-bit stereo interleaved audio input and/or output.
	int numFrames,		// The number of frames received and/or requested.
	int samplerate	    // The current sample rate in Hz.
) {
	return ((CrossExample *)clientdata)->process(audioIO, (unsigned int)numFrames, (unsigned int)samplerate);
}

CrossExample::CrossExample (
		unsigned int samplerate, // device native sample rate
		unsigned int buffersize, // device native buffer size
        const char *path,        // path to APK package
		int fileAoffset,         // offset of file A in APK
		int fileAlength,         // length of file A
		int fileBoffset,         // offset of file B in APK
		int fileBlength          // length of file B
) : activeFx(0), numPlayersLoaded(0), crossFaderPosition(0.0f), volB(0.0f), volA(1.0f * headroom)
{
    Superpowered::Initialize(
            "ExampleLicenseKey-WillExpire-OnNextUpdate",
            false, // enableAudioAnalysis (using SuperpoweredAnalyzer, SuperpoweredLiveAnalyzer, SuperpoweredWaveform or SuperpoweredBandpassFilterbank)
            false, // enableFFTAndFrequencyDomain (using SuperpoweredFrequencyDomain, SuperpoweredFFTComplex, SuperpoweredFFTReal or SuperpoweredPolarFFT)
            false, // enableAudioTimeStretching (using SuperpoweredTimeStretching)
            true,  // enableAudioEffects (using any SuperpoweredFX class)
            true,  // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
            false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
            false  // enableNetworking (using Superpowered::httpRequest)
    );

    playerA = new Superpowered::AdvancedAudioPlayer(samplerate, 0);
    playerB = new Superpowered::AdvancedAudioPlayer(samplerate, 0);
    roll = new Superpowered::Roll(samplerate);
    filter = new Superpowered::Filter(Superpowered::Filter::Resonant_Lowpass, samplerate);
    flanger = new Superpowered::Flanger(samplerate);

    filter->resonance = 0.1f;
    playerA->open(path, fileAoffset, fileAlength);
    playerB->open(path, fileBoffset, fileBlength);

    // Initialize audio engine and pass callback function.
    outputIO = new SuperpoweredAndroidAudioIO (
			samplerate,                     // device native sample rate
			buffersize,                     // device native buffer size
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
    delete outputIO; // Always stop and delete audio I/O first.
    delete playerA;
    delete playerB;
    delete roll;
    delete filter;
    delete flanger;
}

// onPlayPause - Toggle playback state of players.
void CrossExample::onPlayPause(bool play) {
    if (numPlayersLoaded != 3) return;
    if (playerA->isPlaying()) {
        playerA->pause();
        playerB->pause();
    } else {
        if (crossFaderPosition <= 0.5f) { // playerA is master
            playerA->play();
            playerB->playSynchronized();
        } else { // playerB is master
            playerA->playSynchronized();
            playerB->play();
        }
    };
    Superpowered::CPU::setSustainedPerformanceMode(play); // <-- Important to prevent audio dropouts.
}

// onCrossfader - Handle crossfader and adjust volume levels accordingly.
void CrossExample::onCrossfader(int value) {
    crossFaderPosition = float(value) * 0.01f;
    if (crossFaderPosition < 0.01f) {
        volA = 1.0f * headroom;
        volB = 0.0f;
    } else if (crossFaderPosition > 0.99f) {
        volA = 0.0f;
        volB = 1.0f * headroom;
    } else { // constant power curve
        volA = cosf(float(M_PI_2) * crossFaderPosition) * headroom;
        volB = cosf(float(M_PI_2) * (1.0f - crossFaderPosition)) * headroom;
    };
}

// onFxSelect - Select effect type.
void CrossExample::onFxSelect(int value) {
	log_print(ANDROID_LOG_VERBOSE, "CrossExample", "FXSEL %i", value);
	activeFx = (unsigned char)value;
}

// onFxOff - Turn off all effects.
void CrossExample::onFxOff() {
    filter->enabled = roll->enabled = flanger->enabled = false;
}

static inline float floatToFrequency(float value) {
    static const float min = logf(20.0f) / logf(10.0f);
    static const float max = logf(20000.0f) / logf(10.0f);
    static const float range = max - min;
    return powf(10.0f, value * range + min);
}

// onFxValue - Adjust effect parameter.
void CrossExample::onFxValue(int ivalue) {
    float value = float(ivalue) * 0.01f;
    switch (activeFx) {
        case 1:
            filter->frequency = floatToFrequency(1.0f - value);
            filter->enabled = true;
            flanger->enabled = roll->enabled = false;
            break;
        case 2:
            roll->beats = 1.0f - floorf(value * 5.0f) * 0.2f;
            roll->enabled = true;
            filter->enabled = flanger->enabled = false;
            break;
        default:
            flanger->wet = value;
            flanger->enabled = true;
            filter->enabled = roll->enabled = false;
    };
}

// Main process function where audio is generated.
bool CrossExample::process(short int *output, unsigned int numberOfFrames, unsigned int samplerate) {
    playerA->outputSamplerate = playerB->outputSamplerate = roll->samplerate = filter->samplerate = flanger->samplerate = samplerate;

    // Check player statuses. We're only interested in the Opened event in this example.
    if (playerA->getLatestEvent() == Superpowered::AdvancedAudioPlayer::PlayerEvent_Opened) numPlayersLoaded++;
    if (playerB->getLatestEvent() == Superpowered::AdvancedAudioPlayer::PlayerEvent_Opened) numPlayersLoaded++;

    // Both players opened? If yes, set the beatgrid information on the players.
    if (numPlayersLoaded == 2) {
        playerA->originalBPM = 126.0f;
        playerA->firstBeatMs = 353;
        playerB->originalBPM = 123.0f;
        playerB->firstBeatMs = 40;
        playerA->syncMode = playerB->syncMode = Superpowered::AdvancedAudioPlayer::SyncMode_TempoAndBeat;
        // Jump to the first beat.
        playerA->setPosition(playerA->firstBeatMs, false, false);
        playerB->setPosition(playerB->firstBeatMs, false, false);
        numPlayersLoaded++; // Make sure we don't get into this block again. Also enables the play button.
    }

    // Who is dictating the tempo, player A or player B?
    bool masterIsA = (crossFaderPosition <= 0.5f);

    // Everything will sync to the master player's tempo.
    playerA->syncToBpm = playerB->syncToBpm = masterIsA ? playerA->getCurrentBpm() : playerB->getCurrentBpm();
    roll->bpm = flanger->bpm = (float)playerA->syncToBpm;

            // Players will sync to opposite player's beat position.
    playerA->syncToMsElapsedSinceLastBeat = playerB->getMsElapsedSinceLastBeat();
    playerB->syncToMsElapsedSinceLastBeat = playerA->getMsElapsedSinceLastBeat();

    // Get audio from the players into a buffer on the stack.
    float outputBuffer[numberOfFrames * 2];
    bool silence = !playerA->processStereo(outputBuffer, false, numberOfFrames, volA);
    if (playerB->processStereo(outputBuffer, !silence, numberOfFrames, volB)) silence = false;

    // Add effects.
    if (roll->process(silence ? NULL : outputBuffer, outputBuffer, numberOfFrames) && silence) silence = false;
    if (!silence) {
        filter->process(outputBuffer, outputBuffer, numberOfFrames);
        flanger->process(outputBuffer, outputBuffer, numberOfFrames);
    };

    // The output buffer is ready now, let's write the finished audio into the requested buffer.
    if (!silence) Superpowered::FloatToShortInt(outputBuffer, output, numberOfFrames);
    return !silence;
}

static CrossExample *example = NULL;

// CrossExample - Create the DJ app and initialize the players.
extern "C" JNIEXPORT void
Java_com_superpowered_crossexample_MainActivity_CrossExample(
		JNIEnv *env,
		jobject __unused obj,
		jint samplerate,  // device native sample rate
		jint buffersize,  // device native buffer size
		jstring apkPath,  // path to APK package
		jint fileAoffset, // offset of file A in APK
		jint fileAlength, // length of file A
		jint fileBoffset, // offset of file B in APK
		jint fileBlength  // length of file B
) {
    const char *path = env->GetStringUTFChars(apkPath, JNI_FALSE);
    example = new CrossExample((unsigned int)samplerate, (unsigned int)buffersize,
			path, fileAoffset, fileAlength, fileBoffset, fileBlength);
    env->ReleaseStringUTFChars(apkPath, path);
}

// onPlayPause - Toggle playback state of player.
extern "C" JNIEXPORT void
Java_com_superpowered_crossexample_MainActivity_onPlayPause(JNIEnv * __unused env, jobject __unused obj, jboolean play) {
	example->onPlayPause(play);
}

// onCrossfader - Handle crossfader events.
extern "C" JNIEXPORT void
Java_com_superpowered_crossexample_MainActivity_onCrossfader(JNIEnv * __unused env, jobject __unused obj, jint value) {
	example->onCrossfader(value);
}

// onFxSelect - Handle FX selection.
extern "C" JNIEXPORT void
Java_com_superpowered_crossexample_MainActivity_onFxSelect(JNIEnv * __unused env, jobject __unused obj, jint value) {
	example->onFxSelect(value);
}

// onFxOff - Turn of all effects.
extern "C" JNIEXPORT void
Java_com_superpowered_crossexample_MainActivity_onFxOff(JNIEnv * __unused env, jobject __unused obj) {
	example->onFxOff();
}

// onFxValue - Adjust FX value.
extern "C" JNIEXPORT void
Java_com_superpowered_crossexample_MainActivity_onFxValue(JNIEnv * __unused env, jobject __unused obj, jint value) {
	example->onFxValue(value);
}
