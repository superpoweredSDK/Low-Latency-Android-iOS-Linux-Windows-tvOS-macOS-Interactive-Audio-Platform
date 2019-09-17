#ifndef Header_SuperpoweredAndroidAudioIO
#define Header_SuperpoweredAndroidAudioIO

struct SuperpoweredAndroidAudioIOInternals;

/// @brief This is the prototype of an audio processing callback function.
/// If the application requires both audio input and audio output, this callback is called once (there is no separate audio input and audio output callback). Audio input is available in audioIO, and the application should change it's contents for audio output.
/// @param clientdata A custom pointer your callback receives.
/// @param audioIO 16-bit stereo interleaved audio input and/or output.
/// @param numberOfFrames The number of frames received and/or requested.
/// @param samplerate The current sample rate in Hz.
typedef bool (*audioProcessingCallback) (void *clientdata, short int *audioIO, int numberOfFrames, int samplerate);

/// @brief Easy handling of OpenSL ES audio input and/or output.
class SuperpoweredAndroidAudioIO {
public:
/// @brief Creates an audio I/O instance. Audio input and/or output immediately starts after calling this.
/// @param samplerate The requested sample rate in Hz.
/// @param buffersize The requested buffer size (number of frames).
/// @param enableInput Enable audio input.
/// @param enableOutput Enable audio output.
/// @param callback The audio processing callback function to call periodically.
/// @param clientdata A custom pointer the callback receives.
/// @param inputStreamType OpenSL ES stream type, such as SL_ANDROID_RECORDING_PRESET_GENERIC. -1 means default. SLES/OpenSLES_AndroidConfiguration.h has them.
/// @param outputStreamType OpenSL ES stream type, such as SL_ANDROID_STREAM_MEDIA or SL_ANDROID_STREAM_VOICE. -1 means default. SLES/OpenSLES_AndroidConfiguration.h has them.
    SuperpoweredAndroidAudioIO(int samplerate, int buffersize, bool enableInput, bool enableOutput, audioProcessingCallback callback, void *clientdata, int inputStreamType = -1, int outputStreamType = -1);
    ~SuperpoweredAndroidAudioIO();

/// @brief Call this in the main activity's onResume() method.
/// Calling this is important if you'd like to save battery. When there is no audio playing and the app goes to the background, it will automatically stop audio input and/or output.
    void onForeground();
    
/// @brief Call this in the main activity's onPause() method.
/// Calling this is important if you'd like to save battery. When there is no audio playing and the app goes to the background, it will automatically stop audio input and/or output.
    void onBackground();
    
/// @brief Starts audio input and/or output.
    void start();
    
/// @brief Stops audio input and/or output.
    void stop();

private:
    SuperpoweredAndroidAudioIOInternals *internals;
    SuperpoweredAndroidAudioIO(const SuperpoweredAndroidAudioIO&);
    SuperpoweredAndroidAudioIO& operator=(const SuperpoweredAndroidAudioIO&);
};

#endif
