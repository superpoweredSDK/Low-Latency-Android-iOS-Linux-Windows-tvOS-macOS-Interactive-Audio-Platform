// Typical UWP stuff.
#include "pch.h"
#include "MainPage.xaml.h"
using namespace SuperpoweredPlayer;

MainPage::MainPage() {
	InitializeComponent();
}

// The interesting part begins here.
#include <helpers.h>
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
		char path[MAX_PATH + 1];
		stringToChar(Windows::ApplicationModel::Package::Current->InstalledLocation->Path, path);
		sprintf_s(path, MAX_PATH + 1, "%s\\track.mp3", path);

		player = new SuperpoweredAdvancedAudioPlayer(NULL, playerCallback, lastSamplerate, 0);
		player->open(path);

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
