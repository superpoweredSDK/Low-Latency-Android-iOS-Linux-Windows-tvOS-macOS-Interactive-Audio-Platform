#ifndef SuperpoweredWindowsAudioIOHeader
#define SuperpoweredWindowsAudioIOHeader

struct SuperpoweredWindowsAudioIOInternals;

/// @brief This is the prototype of an audio processing callback function.
/// If the application requires both audio input and audio output, this callback is called once (there is no separate audio input and audio output callback). Audio input is available in audioIO, and the application should change it's contents for audio output.
/// @param clientdata A custom pointer your callback receives.
/// @param audioIO 32-bit floating point stereo interleaved audio input and/or output.
/// @param numberOfFrames The number of frames received and/or requested.
/// @param samplerate The current sample rate in Hz. If the audio processing callback is called with no audioIO == NULL, the value of samplerate can indicate 3 different things:
/// - samplerate > 0: audio I/O is about to begin with this sample rate. This is the first call of the callback and can be used to create additional audio features and allocate memory, as it doesn't run in a sensitive real-time context.
/// - samplerate == 0: audio I/O is about to stop, this is the last call of the callback.
/// - samplerate < 0: audio I/O can not start and this is an error code.
typedef bool (*audioProcessingCallback) (void *clientdata, float *audioIO, int numberOfFrames, int samplerate);

/// @brief Easy handling of WASAPI audio input and/or output.
class SuperpoweredWindowsAudioIO {
public:
/// @brief Creates an audio I/O instance and configures it for low latency.
/// @warning Don't forget to check the Capabilities section in the Package.appxmanifest file for UWP apps, for items like "Background Media Playback" or "Microphone".
/// @param callback The audio processing callback function to call periodically.
/// @param clientdata A custom pointer the callback receives.
/// @param enableInput Enable audio input.
/// @param enableOutput Enable audio output.
	SuperpoweredWindowsAudioIO(audioProcessingCallback callback, void *clientdata, bool enableInput, bool enableOutput);
	~SuperpoweredWindowsAudioIO();
    
/// @brief Starts audio input and/or output.
	void start();
    
/// @brief Stops audio input and/or output.
    void stop();

private:
	SuperpoweredWindowsAudioIOInternals *internals;
	SuperpoweredWindowsAudioIO(const SuperpoweredWindowsAudioIO&);
	SuperpoweredWindowsAudioIO& operator=(const SuperpoweredWindowsAudioIO&);
};

#endif
