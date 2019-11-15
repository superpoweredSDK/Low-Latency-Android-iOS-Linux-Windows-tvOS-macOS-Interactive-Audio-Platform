// Typical UWP stuff.
#include "pch.h"
#include "MainPage.xaml.h"
using namespace SuperpoweredEffect;

#include <helpers.h>
#include <Superpowered.h>
#include <OpenSource/SuperpoweredWindowsAudioIO.h>
#include <SuperpoweredReverb.h>

static SuperpoweredWindowsAudioIO *audioIO = NULL;
static Superpowered::Reverb *reverb = NULL;

MainPage::MainPage() {
	InitializeComponent();
    Superpowered::Initialize(
		"ExampleLicenseKey-WillExpire-OnNextUpdate",
		false, // enableAudioAnalysis (using SuperpoweredAnalyzer, SuperpoweredLiveAnalyzer, SuperpoweredWaveform or SuperpoweredBandpassFilterbank)
		false, // enableFFTAndFrequencyDomain (using SuperpoweredFrequencyDomain, SuperpoweredFFTComplex, SuperpoweredFFTReal or SuperpoweredPolarFFT)
		false, // enableAudioTimeStretching (using SuperpoweredTimeStretching)
		true,  // enableAudioEffects (using any SuperpoweredFX class)
		false, // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
		false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
		false  // enableNetworking (using Superpowered::httpRequest)
	);
}

// Audio input/output. Runs periodically on the audio I/O thread.
static bool audioProcessing(void *clientdata, float *audio, int numberOfFrames, int samplerate) {
    if (!audio) {
        if (samplerate == 0) delete reverb; // Audio I/O is about to stop.
        else if (samplerate < 0) Log("Audio I/O error %i.\n", samplerate);
        return false;
    }
    
    reverb->samplerate = samplerate;
	reverb->process(audio, audio, numberOfFrames);
	return true;
}

// The button handler.
void MainPage::Toggle(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e) {
	Windows::UI::Xaml::Controls::Button^ button = safe_cast<Windows::UI::Xaml::Controls::Button^>(sender);

	if (!audioIO) {
        reverb = new Superpowered::Reverb(44100);
		reverb->enabled = true;

		audioIO = new SuperpoweredWindowsAudioIO(audioProcessing, NULL, true, true);
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
