#ifndef SuperpoweredWASAPIAudioIOHeader
#define SuperpoweredWASAPIAudioIOHeader

struct SuperpoweredWASAPIAudioIOInternals;

/// @brief This is the prototype of an audio processing callback function.
/// If the application requires both audio input and audio output, this callback is called once (there is no separate audio input and audio output callback). Audio input is available in audioIO, and the application should change it's contents for audio output.
/// @param clientdata A custom pointer your callback receives.
/// @param inputBuffer 32-bit floating point interleaved audio input. Never the same to outputBuffer.
/// @param outputBuffer 32-bit floating point interleaved audio output. Never the same to inputBuffer.
/// @param numberOfFrames The number of frames received and/or requested.
/// @param samplerate The current sample rate in Hz.
typedef bool (*audioProcessingCallback) (void *clientdata, float *inputBuffer, float *outputBuffer, int numberOfFrames, int samplerate);

/// @brief Easy handling of WASAPI audio input and/or output.
class SuperpoweredWASAPIAudioIO {
public:
/// @brief Creates an audio I/O instance and configures it for low latency.
/// @warning Don't forget to check the Capabilities section in the Package.appxmanifest file for UWP apps, for items like "Background Media Playback" or "Microphone".
/// @param callback The audio processing callback function to call periodically.
/// @param clientdata A custom pointer the callback receives.
/// @param preferredBufferSizeMs Preferred buffer size. 12 is good for every device (typically results in 512 frames). Set it to 0 for WASAPI exclusive mode with lowest buffer size.
/// @param numberOfChannels The number of channels you provide in the audio processing callback.
/// @param enableInput Enable audio input.
/// @param enableOutput Enable audio output.
	SuperpoweredWASAPIAudioIO(audioProcessingCallback callback, void *clientdata, unsigned int preferredBufferSizeMs, unsigned int numberOfChannels, bool enableInput, bool enableOutput);
	~SuperpoweredWASAPIAudioIO();
    
/// @brief Starts audio input and/or output.
/// @return Returns with an error message or NULL on success.
	const char *start();
    
/// @brief Stops audio input and/or output.
    void stop();

private:
	SuperpoweredWASAPIAudioIOInternals *internals;
	SuperpoweredWASAPIAudioIO(const SuperpoweredWASAPIAudioIO&);
	SuperpoweredWASAPIAudioIO& operator=(const SuperpoweredWASAPIAudioIO&);
};

#endif
