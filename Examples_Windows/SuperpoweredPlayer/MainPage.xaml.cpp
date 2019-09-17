// Typical UWP stuff.
#include "pch.h"
#include "MainPage.xaml.h"
using namespace SuperpoweredPlayer;

#include <helpers.h>
#include <Superpowered.h>
#include <OpenSource/SuperpoweredWindowsAudioIO.h>
#include <SuperpoweredAdvancedAudioPlayer.h>

static SuperpoweredWindowsAudioIO *audioIO = NULL;
static Superpowered::AdvancedAudioPlayer *player = NULL;
Windows::UI::Xaml::DispatcherTimer^ UIUpdateTimer;

// Audio output should be provided here. Runs periodically on the audio I/O thread.
static bool audioProcessing(void *clientdata, float *audio, int numberOfFrames, int samplerate) {
	if (!audio) {
		if (samplerate == 0) delete player; // Audio I/O is about to stop.
		else if (samplerate < 0) Log("Audio I/O error %i.\n", samplerate);
		return false;
	}

	player->outputSamplerate = samplerate;
	return player->processStereo(audio, false, numberOfFrames);
}

// Runs periodically on the UI thread ("ui tick").
void MainPage::OnTick(Platform::Object^ sender, Platform::Object^ e) { 
    switch (player->getLatestEvent()) {
        case Superpowered::PlayerEvent_None:
        case Superpowered::PlayerEvent_Opening: break; // do nothing
		case Superpowered::PlayerEvent_Opened: player->play(); break;
        case Superpowered::PlayerEvent_OpenFailed: {
            int openError = player->getOpenErrorCode();
            Log("Open error %i: %s", openError, Superpowered::AdvancedAudioPlayer::statusCodeToString(openError));
        } break;
        case Superpowered::PlayerEvent_ConnectionLost: Log("Network download failed."); break;
        case Superpowered::PlayerEvent_ProgressiveDownloadFinished: Log("Download finished. Path: %s", player->getFullyDownloadedFilePath()); break;
    }
    
    if (player->eofRecently()) player->setPosition(0, false, false);
}

// The button handler.
void MainPage::Toggle(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e) {
	Windows::UI::Xaml::Controls::Button^ button = safe_cast<Windows::UI::Xaml::Controls::Button^>(sender);

	if (!audioIO) {
		char path[MAX_PATH + 1];
		stringToChar(Windows::ApplicationModel::Package::Current->InstalledLocation->Path, path);
		sprintf_s(path, MAX_PATH + 1, "%s\\track.mp3", path);

		player = new Superpowered::AdvancedAudioPlayer(44100, 0);
		player->open(path);

		audioIO = new SuperpoweredWindowsAudioIO(audioProcessing, NULL, false, true);
		audioIO->start();

		button->Content = "Stop";
		UIUpdateTimer->Start();
	} else {
		UIUpdateTimer->Stop();
		audioIO->stop();
		delete audioIO;
		audioIO = NULL;
		button->Content = "Start";
	}
}

MainPage::MainPage() {
	InitializeComponent();
	UIUpdateTimer = ref new Windows::UI::Xaml::DispatcherTimer();
	UIUpdateTimer->Tick += ref new Windows::Foundation::EventHandler<Platform::Object^>(this, &MainPage::OnTick);

	Superpowered::Initialize(
		"ExampleLicenseKey-WillExpire-OnNextUpdate",
		false, // enableAudioAnalysis (using SuperpoweredAnalyzer, SuperpoweredLiveAnalyzer, SuperpoweredWaveform or SuperpoweredBandpassFilterbank)
		false, // enableFFTAndFrequencyDomain (using SuperpoweredFrequencyDomain, SuperpoweredFFTComplex, SuperpoweredFFTReal or SuperpoweredPolarFFT)
		false, // enableAudioTimeStretching (using SuperpoweredTimeStretching)
		false, // enableAudioEffects (using any SuperpoweredFX class)
		true,  // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
		false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
		false  // enableNetworking (using Superpowered::httpRequest)
	);
}
