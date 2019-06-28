// Typical UWP stuff.
#include "pch.h"
#include "MainPage.xaml.h"
using namespace SuperpoweredRecorderExample;

#include <helpers.h>
#include <Superpowered.h>
#include <SuperpoweredWindowsAudioIO.h>
#include <SuperpoweredRecorder.h>

static SuperpoweredWindowsAudioIO *audioIO = NULL;
static SuperpoweredRecorder *recorder = NULL;
static int lastSamplerate = 44100;

MainPage::MainPage() {
	InitializeComponent();
    SuperpoweredInitialize(
                           "ExampleLicenseKey-WillExpire-OnNextUpdate",
                           false, // enableAudioAnalysis (using SuperpoweredAnalyzer, SuperpoweredLiveAnalyzer, SuperpoweredWaveform or SuperpoweredBandpassFilterbank)
                           false, // enableFFTAndFrequencyDomain (using SuperpoweredFrequencyDomain, SuperpoweredFFTComplex, SuperpoweredFFTReal or SuperpoweredPolarFFT)
                           false, // enableAudioTimeStretching (using SuperpoweredTimeStretching)
                           false, // enableAudioEffects (using any SuperpoweredFX class)
                           false, // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
                           false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
                           false  // enableNetworking (using Superpowered::httpRequest)
                           );
}

// This is called after the recorder closed the wav file.
static void recorderStopped(void *clientdata) {
	Windows::System::Launcher::LaunchFolderAsync(Windows::Storage::ApplicationData::Current->TemporaryFolder);
	delete recorder;
}

// Audio input comes here.
static bool audioProcessing(void *clientdata, float *audio, int numberOfSamples, int samplerate) {
	if (!audio) {
		if (samplerate == 0) {
			Log("Audio I/O stopped.\n");
			recorder->stop();
		} else Log("Audio I/O error %i.\n", samplerate);
		return false;
	}

	if (lastSamplerate != samplerate) {
		lastSamplerate = samplerate;
		recorder->setSamplerate(samplerate);
	}

	recorder->process(audio, numberOfSamples);
	return false;
}

// The button handler.
void MainPage::Toggle(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e) {
	Windows::UI::Xaml::Controls::Button^ button = safe_cast<Windows::UI::Xaml::Controls::Button^>(sender);

	if (!audioIO) {
		// Initialize the recorder with a temporary file path.
		char path[MAX_PATH + 1];
		stringToChar(Windows::Storage::ApplicationData::Current->TemporaryFolder->Path, path);
		sprintf_s(path, MAX_PATH + 1, "%s\\temp.wav", path);
		DeleteFileA(path);
		recorder = new SuperpoweredRecorder(path, lastSamplerate, 1, 2, false, recorderStopped, NULL);

		// Start the recorder with the destination file path.
		stringToChar(Windows::Storage::ApplicationData::Current->TemporaryFolder->Path, path);
		sprintf_s(path, MAX_PATH + 1, "%s\\recording.wav", path);
		DeleteFileA(path);
		recorder->start(path);
		Log("Will record to: %s\n", path);

		// Start audio input, so the recorder starts doing its job.
		audioIO = new SuperpoweredWindowsAudioIO(audioProcessing, NULL, true, false);
		audioIO->start();

		button->Content = "Stop";
	} else {
		audioIO->stop();
		delete audioIO;
		audioIO = NULL;
		button->Content = "Start";
	}
}
