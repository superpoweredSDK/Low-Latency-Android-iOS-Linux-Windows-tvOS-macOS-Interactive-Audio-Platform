#import <Cocoa/Cocoa.h>

typedef struct audioDevice {
    audioDevice *next;
    char *name;
    unsigned int deviceID, numInputChannels, numOutputChannels;
    
    void dealloc() {
        if (next) next->dealloc();
        if (name) free(name);
        free(this);
    }
} audioDevice;

@protocol SuperpoweredOSXAudioIODelegate;

/// @brief You can have an audio processing callback in Objective-C or pure C. This is the pure C prototype.
/// @return Return false when you did no audio processing (silence).
/// @param clientdata A custom pointer your callback receives.
/// @param inputBuffers Input buffers.
/// @param inputChannels The number of input channels.
/// @param outputBuffers Output buffers.
/// @param outputChannels The number of output channels.
/// @param numberOfFrames The number of frames requested.
/// @param samplerate The current sample rate in Hz.
/// @param hostTime A mach timestamp, indicates when this buffer of audio will be passed to the audio output.
typedef bool (*audioProcessingCallback_C) (void *clientdata, float **inputBuffers, unsigned int inputChannels, float **outputBuffers, unsigned int outputChannels, unsigned int numberOfFrames, unsigned int samplerate, uint64_t hostTime);

/// @brief A simple system audio input and/or output handler for macOS.
/// @warning All methods and setters should be called on the main thread only!
@interface SuperpoweredOSXAudioIO: NSObject {
    unsigned int audioDeviceID;
    int preferredBufferSizeMs;
    bool inputEnabled, outputEnabled;
}

@property (nonatomic, assign) int preferredBufferSizeMs; ///< The preferred buffer size in milliseconds. Recommended: 12.
@property (nonatomic, assign) bool inputEnabled;  ///< Set this to true to enable audio input.
@property (nonatomic, assign) bool outputEnabled; ///< Set this to true to enable audio output.

/// @brief Creates an audio IO instance using the default system audio input and/or output.
/// @param delegate The object fully implementing the SuperpoweredOSXAudioIODelegate protocol. Not retained.
/// @param preferredBufferSizeMs The initial value for preferredBufferSizeMs. 12 is good for every device (512 frames).
/// @param numberOfChannels The number of channels you provide in the audio processing callback.
/// @param enableInput Enable audio input.
/// @param enableOutput Enable audio output.
- (id)initWithDelegate:(id<SuperpoweredOSXAudioIODelegate>)delegate preferredBufferSizeMs:(unsigned int)preferredBufferSizeMs numberOfChannels:(int)numberOfChannels enableInput:(bool)enableInput enableOutput:(bool)enableOutput;

/// @brief Creates an audio output instance using a specific audio device.
/// @param delegate The object fully implementing the SuperpoweredOSXAudioIODelegate protocol. Not retained.
/// @param preferredBufferSizeMs The initial value for preferredBufferSizeMs. 12 is good for every device (512 frames).
/// @param numberOfChannels The number of channels you provide in the audio processing callback.
/// @param enableInput Enable audio input.
/// @param enableOutput Enable audio output.
/// @param audioDeviceID The device identifier of the audio device. Equals to AudioDeviceID of Core Audio.
- (id)initWithDelegate:(id<SuperpoweredOSXAudioIODelegate>)delegate preferredBufferSizeMs:(unsigned int)preferredBufferSizeMs numberOfChannels:(int)numberOfChannels enableInput:(bool)enableInput enableOutput:(bool)enableOutput audioDeviceID:(unsigned int)audioDeviceID;

/// @brief Changes the audio device.
/// @param audioDeviceID The device identifier of the audio device (equals to AudioDeviceID of Core Audio) or UINT_MAX to use the default system audio device.
- (void)setAudioDevice:(unsigned int)audioDeviceID;

/// @brief Starts audio I/O.
/// @return True if successful, false if failed.
- (bool)start;

/// @brief Stops audio I/O.
- (void)stop;

/// @brief Call this to re-configure the channel mapping.
- (void)mapChannels;

/// @brief Sets the audio processing callback to a C function, instead of the delegate's Objective-C method.
/// 99% of all audio apps work great with the Objective-C method, so you don't need to use this. Don't call this method after [start]!
/// @param callback The callback function.
/// @param clientdata Some custom pointer for the C processing callback. You can set it to NULL.
- (void)setProcessingCallback_C:(audioProcessingCallback_C)callback clientdata:(void *)clientdata;

/// @return Returns with a list of the current audio input and output devices. A linked list of audioDevice structs.
+ (audioDevice *)getAudioDevices;

@end


/// @brief You must implement this protocol to use SuperpoweredOSXAudioIO.
@protocol SuperpoweredOSXAudioIODelegate <NSObject>
@optional

/// @brief Process audio here.
/// @return Return false when you did no audio processing (silence).
/// @param inputBuffers Input buffers.
/// @param inputChannels The number of input channels.
/// @param outputBuffers Output buffers.
/// @param outputChannels The number of output channels.
/// @param numberOfFrames The number of frames requested.
/// @param samplerate The current sample rate in Hz.
/// @param hostTime A mach timestamp, indicates when this chunk of audio will be passed to the audio output.
/// @warning It's called on a high priority real-time audio thread, so please take care of blocking and processing time to prevent audio dropouts.
@optional
- (bool)audioProcessingCallback:(float **)inputBuffers inputChannels:(unsigned int)inputChannels outputBuffers:(float **)outputBuffers outputChannels:(unsigned int)outputChannels numberOfFrames:(unsigned int)numberOfFrames samplerate:(unsigned int)samplerate hostTime:(unsigned long long int)hostTime;

/// @brief This method is called on the main thread when an audio device is initialized.
/// @param outputDeviceName The name of the audio output device.
/// @param numOutputChannels The number of output channels available.
/// @param outputMap Map the output channels here.
/// @param inputDeviceName The name of the audio input device.
/// @param numInputChannels The number of input channels available.
/// @param inputMap Map the input channels here.
@optional
- (void)mapChannels:(NSString *)outputDeviceName numOutputChannels:(int)numOutputChannels outputMap:(int *)outputMap input:(NSString *)inputDeviceName numInputChannels:(int)numInputChannels inputMap:(int *)inputMap;

/// @brief This method is called when the an audio device is connected or disconnected.
/// @param devices A linked list of audioDevice structs.
@optional
- (void)audioDevicesChanged:(audioDevice *)devices;

@end
