// Typical UWP stuff.
#include "pch.h"
#include "MainPage.xaml.h"
using namespace SuperpoweredRecorderExample;

#include <helpers.h>
#include <Superpowered.h>
#include <OpenSource\SuperpoweredWindowsAudioIO.h>
#include <SuperpoweredRecorder.h>

enum State { Idle, Starting, Recording, Stopping };

static SuperpoweredWindowsAudioIO *audioIO = NULL;
static Superpowered::Recorder* recorder = NULL;
Windows::UI::Xaml::DispatcherTimer^ UIUpdateTimer;
static Windows::UI::Xaml::Controls::Button^ MyButton;
static int currentSamplerate = 0;
static State state = Idle;

// This is called periodically by the audio I/O.
static bool audioProcessing(void* clientdata, float* audio, int numberOfFrames, int samplerate) {
	if (!audio) {
        if (samplerate > 0) currentSamplerate = samplerate; // Audio I/O is about to begin.
		else if (samplerate == 0) recorder->stop();         // Audio I/O is about to stop.
		else Log("Audio I/O error %i.\n", samplerate);      // Audio I/O can not start.
    } else {
        if (state == Recording) recorder->recordInterleaved(audio, numberOfFrames);
    }
	return false;
}

// Runs periodically on the main/UI thread ("ui tick").
void MainPage::OnTick(Platform::Object^ sender, Platform::Object^ e) {
    switch (state) {
        case Starting:
            if (currentSamplerate > 0) {
                // Start a new recording with the destination file path.
                char path[MAX_PATH + 1];
                stringToChar(Windows::Storage::ApplicationData::Current->TemporaryFolder->Path, path);
                sprintf_s(path, MAX_PATH + 1, "%s\\recording", path);
                DeleteFileA(path);
                recorder->prepare(path, currentSamplerate, true, 1);
                
                Log("Will record to: %s\n", path);
				MyButton->Content = "Stop";
                state = Recording;
            }
            break;
        case Stopping:
            if (recorder->isFinished()) {
                // Delete the recorder, stop periodic UI updates and open the destination folder.
                delete recorder;
                UIUpdateTimer->Stop();
                Windows::System::Launcher::LaunchFolderAsync(Windows::Storage::ApplicationData::Current->TemporaryFolder);
				MyButton->Content = "Start";
                state = Idle;
            }
            break;
        default:; // Idle, Recording: do nothing
    }
}

// The button handler.
void MainPage::Toggle(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e) {
	MyButton = safe_cast<Windows::UI::Xaml::Controls::Button^>(sender);

    switch (state) {
        case Idle: {
            // Create a recorder with a temporary file path.
            char path[MAX_PATH + 1];
            stringToChar(Windows::Storage::ApplicationData::Current->TemporaryFolder->Path, path);
            sprintf_s(path, MAX_PATH + 1, "%s\\temp.wav", path);
            DeleteFileA(path);
            recorder = new Superpowered::Recorder(path);
            
            // Create and start audio input.
            audioIO = new SuperpoweredWindowsAudioIO(audioProcessing, NULL, true, false);
            audioIO->start();
            
            currentSamplerate = 0;
            UIUpdateTimer->Start(); // Start periodic UI updates (every frame).
            MyButton->Content = "Starting...";
            state = Starting;
        } break;
        case Recording: {
            // Stop and delete audio input.
            audioIO->stop();
            delete audioIO;
            
            MyButton->Content = "Stopping...";
            state = Stopping;
        } break;
        default:; // Starting, Stopping: do nothing
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
		false, // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
		false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
		false  // enableNetworking (using Superpowered::httpRequest)
	);
}
