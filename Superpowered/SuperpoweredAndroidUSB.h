#ifndef SuperpoweredUSBAudio_Header
#define SuperpoweredUSBAudio_Header

namespace Superpowered {

/// @brief Called when a USB audio device is connected.
/// @param clientdata Some custom pointer you set at AndroidUSB::initialize().
/// @param deviceID Device identifier.
/// @param manufacturer Manufacturer name.
/// @param product Product name.
/// @param info Detailed, user-readable USB information about the device.
typedef void (*AndroidUSBAudioDeviceConnectedCallback)(void *clientdata, int deviceID, const char *manufacturer, const char *product, const char *info);

/// @brief Called when a USB audio device is disconnected.
/// @param clientdata Some custom pointer you set at AndroidUSB::initialize().
/// @param deviceID Device identifier.
typedef void (*AndroidUSBAudioDeviceDisconnectedCallback)(void *clientdata, int deviceID);

/// @brief Called when a USB MIDI device is connected.
/// @param clientdata Some custom pointer you set at AndroidUSB::initialize().
/// @param deviceID Device identifier.
/// @param manufacturer Manufacturer name.
/// @param product Product name.
typedef void (*AndroidUSBMIDIDeviceConnectedCallback)(void *clientdata, int deviceID, const char *manufacturer, const char *product);

/// @brief Called when a USB MIDI device is disconnected.
/// @param clientdata Some custom pointer you set at AndroidUSB::initialize().
/// @param deviceID Device identifier.
typedef void (*AndroidUSBMIDIDeviceDisconnectedCallback)(void *clientdata, int deviceID);

struct AndroidUSBInternals;

/// @brief Audio latency settings.
enum OnConnectReturn {
    AudioAndMIDISuccess, ///< Both audio and MIDI is recognized and connected.
    AudioSuccess,        ///< Audio is recognized and connected, MIDI not recognized.
    MIDISuccess,         ///< MIDI is recognized and connected, audio not recognized.
    NotRecognized        ///< Found no audio or MIDI features for this device.
};

/// @brief This class manages USB device connections.
class AndroidUSB {
public:
     /// @brief Initializes the Superpowered Android USB system and set up native connection management callbacks.
     /// It's very lightweight in memory and consumes no CPU. Call this only once in an app's lifecycle. Don't initialize/destroy AndroidUSB more than once.
    /// @param clientdata Some custom pointer you set for the callbacks.
    /// @param audioConnectedCallback Called when a USB audio device is connected. Can be NULL.
    /// @param audioDisconnectedCallback Called when a USB audio device is disconnected. Can be NULL.
    /// @param midiConnectedCallback Called when a MIDI audio device is connected. Can be NULL.
    /// @param midiDisconnectedCallback Called when a MIDI audio device is disconnected. Can be NULL.
    static void initialize(void *clientdata, AndroidUSBAudioDeviceConnectedCallback audioConnectedCallback, AndroidUSBAudioDeviceDisconnectedCallback audioDisconnectedCallback, AndroidUSBMIDIDeviceConnectedCallback midiConnectedCallback, AndroidUSBMIDIDeviceDisconnectedCallback midiDisconnectedCallback);

    /// @brief Destroys the Superpowered Android USB system. Call this only once in an app's lifecycle. Don't initialize/destroy AndroidUSB more than once.
    static void destroy();

    /// @brief Call this from Java when a USB device is connected.
    /// @param deviceID USB device identifier.
    /// @param fd File descriptor for communication.
    /// @param data Pointer to the raw USB descriptor.
    /// @param dataBytes Length of the raw USB descriptor.
    static OnConnectReturn onConnect(int deviceID, int fd, const unsigned char *data, int dataBytes);

    /// @brief Call this from Java when a USB device is disconnected.
    /// @param deviceID USB device identifier.
    static void onDisconnect(int deviceID);

    static AndroidUSBInternals *internals;
};

/// @brief This is the prototype of an audio processing callback function.
/// Audio input is available in audioIO, and the application should change it's contents for audio output.
/// @return Return with false for no audio output (silence), true otherwise.
/// @param clientdata A custom pointer your callback receives.
/// @param deviceID USB device identifier.
/// @param audioIO 32-bit interleaved audio input and/or output.
/// @param numberOfFrames The number of frames.
/// @param samplerate The current sample rate in Hz.
/// @param numberOfInputChannels Number of available input channels.
/// @param numberOfOutputChannels Number of available output channels.
typedef bool (*AndroidUSBAudioProcessingCallback)(void *clientdata, int deviceID, float *audioIO, int numberOfFrames, int samplerate, int numberOfInputChannels, int numberOfOutputChannels);

/// @brief Information about an audio input or output.
typedef struct AndroidUSBAudioIOInfo {
    char *name;        ///< Human readable name.
    int numChannels;   ///< Number of channels.
    int bitsPerSample; ///< Bits per sample.
    int samplerate;    ///< Sample rate in Hz.
    int interface;     ///< Number of the interface the input or output belongs to.
    bool isInput;      ///< Input or output.
} AndroidUSBAudioIOInfo;

/// @brief Audio latency settings.
enum AndroidUSBAudioBufferSize {
    AndroidUSBAudioBufferSize_VeryLow = 128,
    AndroidUSBAudioBufferSize_Low = 256,
    AndroidUSBAudioBufferSize_Mid = 512
};

/// @brief This class implements a custom low latency USB audio stack for Android, fully bypassing the Android system audio stack.
/// It simplifies the unlimited possibilities of the USB Audio Class to a hierarchy with 3 levels:
/// 1. configuration
/// 2. inputs and outputs
/// 3. paths
class AndroidUSBAudio {
public:
    /// @brief Get basic information about a USB audio device.
    /// @param deviceID Device identifier.
    /// @param manufacturer Manufacturer name.
    /// @param product Product name.
    /// @param info Detailed USB information about the device.
    static void getInfo(int deviceID, const char **manufacturer, const char **product, const char **info);
    
    /// @brief Get information about a USB audio device's configurations.
    /// @param deviceID Device identifier.
    /// @param numConfigurations Number of configurations.
    /// @param configurationNames Name of each configuration.
    static void getConfigurationInfo(int deviceID, int *numConfigurations, char ***configurationNames);
    
    /// @brief Set the active configuration of USB audio device.
    /// @param deviceID Device identifier.
    /// @param configurationIndex Index of the configuration, starting from 0.
    static void setConfiguration(int deviceID, int configurationIndex);
    
    /// @return Get information about audio inputs. Returns the number of inputs.
    /// @param deviceID Device identifier.
    /// @param info Array of AndroidUSBAudioIOInfo for each input.
    static int getInputs(int deviceID, AndroidUSBAudioIOInfo **info);
    
    /// @return Get information about device outputs. Returns the number of outputs.
    /// @param deviceID Device identifier.
    /// @param info Array of AndroidUSBAudioIOInfo for each output.
    static int getOutputs(int deviceID, AndroidUSBAudioIOInfo **info);
    
    /// @brief Returns with the best input and output index for some audio parameters.
    /// @param deviceID Device identifier.
    /// @param inputIOindex Returns the best input index.
    /// @param outputIOindex Returns the best output index.
    /// @param samplerate Sample rate in Hz.
    /// @param bitsPerSample Bit depth.
    /// @param numInputChannels Number of input channels. Set to 0 if you are not interested in input.
    /// @param numOutputChannels Number of output channels. Set to 0 if you are not interested in output.
    /// @param strict Set to true to return with indexes fully matching the parameters. Set to false to return with indexes matching the parameters as close as possible.
    static void getBestIO(int deviceID, int *inputIOindex, int *outputIOindex, int samplerate, int bitsPerSample, int numInputChannels, int numOutputChannels, bool strict);
    
    /// @brief Returns with information about an input or output.
    /// @param deviceID Device identifier.
    /// @param input True for input, false for output.
    /// @param IOindex The index of the input or output.
    /// @param paths Returns the path indexes.
    /// @param pathNames Returns the path names.
    /// @param numPaths Returns the number of paths.
    /// @param thruPaths Returns the audio-thru path indexes (input only).
    /// @param thruPathNames Returns the audio-thru path names (input only).
    /// @param numThruPaths Returns the number of audio-thru paths (input only).
    static void getIOOptions(int deviceID, bool input, int IOindex, int **paths, char ***pathNames, int *numPaths, int **thruPaths, char ***thruPathNames, int *numThruPaths);
    
    /// @brief Returns with information about an audio path.
    /// @param deviceID Device identifier.
    /// @param pathIndex Path index.
    /// @param numFeatures Returns the number of features (the size of the minVolumes, maxVolumes, curVolumes and mutes arrays).
    /// @param minVolumes Minimum volume values in db.
    /// @param maxVolumes Maximum volume values in db.
    /// @param curVolumes Current volume values in db.
    /// @param mutes Current mute values (1 muted, 0 not muted).
    static void getPathInfo(int deviceID, int pathIndex, int *numFeatures, float **minVolumes, float **maxVolumes, float **curVolumes, char **mutes);
    
    /// @return Sets volume and returns with the current volume in db.
    /// @param deviceID Device identifier.
    /// @param pathIndex Path index.
    /// @param channel Channel number (starting from 0).
    /// @param db Volume in decibels.
    static float setVolume(int deviceID, int pathIndex, int channel, float db);
    
    /// @return Sets mute and returns with the current state.
    /// @param deviceID Device identifier.
    /// @param pathIndex Path index.
    /// @param channel Channel number (starting from 0).
    /// @param mute Mute.
    static bool setMute(int deviceID, int pathIndex, int channel, bool mute);
    
    /// @return Starts audio input/output and returns with success (true) or failure (false).
    /// @param deviceID Device identifier.
    /// @param inputIOindex Input index.
    /// @param outputIOindex Output index.
    /// @param bufferSize Buffer size.
    /// @param clientdata Custom pointer for the callback.
    /// @param callback The callback to be called periodically.
    static bool startIO(int deviceID, int inputIOindex, int outputIOindex, AndroidUSBAudioBufferSize bufferSize, void *clientdata, AndroidUSBAudioProcessingCallback callback);
    
    /// @return Starts audio input/output and returns with success (true) or failure (false).
    /// @param deviceID Device identifier.
    /// @param samplerate Preferred sample rate in Hz.
    /// @param bitsPerSample Preferred bit depth.
    /// @param numInputChannels Preferred number of input channels. Set to 0 if you are not interested in audio input.
    /// @param numOutputChannels Preferred number of output channels. Set to 0 if you are not interested in audio output.
    /// @param bufferSize Buffer size
    /// @param clientdata Custom pointer for the callback.
    /// @param callback The callback to be called periodically.
    static bool easyIO(int deviceID, int samplerate, int bitsPerSample, int numInputChannels, int numOutputChannels, AndroidUSBAudioBufferSize bufferSize, void *clientdata, AndroidUSBAudioProcessingCallback callback);
    
    /// @brief Stops audio input/output.
    /// @param deviceID Device identifier.
    static void stopIO(int deviceID);
};

/// @brief This is the prototype of a MIDI callback function.
/// @param clientdata A custom pointer your callback receives.
/// @param deviceID USB device identifier.
/// @param data Raw MIDI data.
/// @param bytes Number of bytes.
typedef void (*AndroidUSBMIDIReceivedCallback)(void *clientdata, int deviceID, unsigned char *data, int bytes);

/// @brief This class implements a custom low latency USB MIDI stack for Android, fully bypassing the Android system MIDI stack.
class AndroidUSBMIDI {
public:
    /// @return Starts MIDI input/output and returns with success (true) or failure (false).
    /// @param deviceID Device identifier.
    /// @param clientdata Custom pointer for the callback.
    /// @param callback The callback to be called when MIDI input is available.
    static bool startIO(int deviceID, void *clientdata, AndroidUSBMIDIReceivedCallback callback);
    
    /// @brief Stops MIDI input/output.
    /// @param deviceID Device identifier.
    static void stopIO(int deviceID);
    
    /// @brief Sends MIDI to a USB MIDI device.
    /// @param deviceID Device identifier.
    /// @param data Raw MIDI data.
    /// @param bytes Number of bytes.
    static void send(int deviceID, unsigned char *data, int bytes);
};

}

#endif
