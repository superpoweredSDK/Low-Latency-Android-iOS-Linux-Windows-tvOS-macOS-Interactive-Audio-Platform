// Typical UWP stuff.
#include "pch.h"
#include "MainPage.xaml.h"
using namespace SuperpoweredHLS;

// The interesting part begins here.
#include <helpers.h>
#include <Superpowered.h>
#include <SuperpoweredWindowsAudioIO.h>
#include <SuperpoweredAdvancedAudioPlayer.h>

static SuperpoweredWindowsAudioIO *audioIO = NULL;
static SuperpoweredAdvancedAudioPlayer *player = NULL;
static int lastSamplerate = 44100;

// Audio output should be provided here.
static bool audioProcessing(void *clientdata, float *audio, int numberOfSamples, int samplerate) {
	if (!audio) {
		if (samplerate == 0) {
			Log("Audio I/O stopped.\n");
			delete player;
		}
		else Log("Audio I/O error %i.\n", samplerate);
		return false;
	}

	if (lastSamplerate != samplerate) {
		lastSamplerate = samplerate;
		player->setSamplerate(samplerate);
	}

	return player->process(audio, false, numberOfSamples);
}

MainPage::MainPage() {
	InitializeComponent();
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
// HLS playback needs a temporary folder.
	char path[MAX_PATH + 1];
	stringToChar(Windows::Storage::ApplicationData::Current->TemporaryFolder->Path, path);
	SuperpoweredAdvancedAudioPlayer::setTempFolder(path);
}

// Handle player events here.
static void playerCallback(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event, void *value) {
	switch (event) {
	case SuperpoweredAdvancedAudioPlayerEvent_EOF: break;
	case SuperpoweredAdvancedAudioPlayerEvent_ProgressiveDownloadError:
	case SuperpoweredAdvancedAudioPlayerEvent_HLSNetworkError:
	case SuperpoweredAdvancedAudioPlayerEvent_LoadError: Log("Load error: %s\n", (const char *)value); break;
	case SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess:
		Log("Load success.\n");
		player->play(false);
		break;
	default:;
	}
}

// The button handler.
void MainPage::Toggle(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e) {
	Windows::UI::Xaml::Controls::Button^ button = safe_cast<Windows::UI::Xaml::Controls::Button^>(sender);

	if (!audioIO) {
		player = new SuperpoweredAdvancedAudioPlayer(NULL, playerCallback, lastSamplerate, 0);
		player->openHLS("https://playertest.longtailvideo.com/adaptive/oceans_aes/oceans_aes.m3u8");

		audioIO = new SuperpoweredWindowsAudioIO(audioProcessing, NULL, false, true);
		audioIO->start();

		button->Content = "Stop";
	}
	else {
		audioIO->stop();
		delete audioIO;
		audioIO = NULL;
		button->Content = "Start";
	}
}
