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

/**
 @brief You can have an audio processing callback in the delegate (Objective-C) or pure C. This is the pure C prototype.
 
 @return Return false when you did no audio processing (silence).

 @param clientdata A custom pointer your callback receives.
 @param inputBuffers Input buffers.
 @param inputChannels The number of input channels.
 @param outputBuffers Output buffers.
 @param outputChannels The number of output channels.
 @param numberOfSamples The number of samples requested.
 @param samplerate The current sample rate in Hz.
 @param hostTime A mach timestamp, indicates when this chunk of audio will be passed to the audio output.
 */
typedef bool (*audioProcessingCallback_C) (void *clientdata, float **inputBuffers, unsigned int inputChannels, float **outputBuffers, unsigned int outputChannels, unsigned int numberOfSamples, unsigned int samplerate, uint64_t hostTime);

/**
 @brief A simple OSX system audio input and/or output handler.
 
 @warning All methods and setters should be called on the main thread only!
 */
@interface SuperpoweredOSXAudioIO: NSObject {
    unsigned int audioDeviceID;
    int preferredBufferSizeMs;
    bool inputEnabled, outputEnabled;
}

/** @brief The preferred buffer size. Recommended: 12. */
@property (nonatomic, assign) int preferredBufferSizeMs;
/** @brief Set this to true to enable audio input. */
@property (nonatomic, assign) bool inputEnabled;
/** @brief Set this to true to enable audio output. */
@property (nonatomic, assign) bool outputEnabled;
/**
 @brief Creates an audio IO instance using the default system audio input and/or output.
  
 @param delegate The object fully implementing the SuperpoweredOSXAudioIODelegate protocol. Not retained.
 @param preferredBufferSizeMs The initial value for preferredBufferSizeMs. 12 is good for every device (512 samples).
 @param numberOfChannels The number of channels you provide in the audio processing callback.
 @param enableInput Enable audio input.
 @param enableOutput Enable audio output.
 */
- (id)initWithDelegate:(id<SuperpoweredOSXAudioIODelegate>)delegate preferredBufferSizeMs:(unsigned int)preferredBufferSizeMs numberOfChannels:(int)numberOfChannels enableInput:(bool)enableInput enableOutput:(bool)enableOutput;

/**
 @brief Creates an audio output instance using a specific audio device.
 
 @param delegate The object fully implementing the SuperpoweredOSXAudioIODelegate protocol. Not retained.
 @param preferredBufferSizeMs The initial value for preferredBufferSizeMs. 12 is good for every device (512 samples).
 @param numberOfChannels The number of channels you provide in the audio processing callback.
 @param enableInput Enable audio input.
 @param enableOutput Enable audio output.
 @param audioDeviceID The device identifier of the audio device. Equals to AudioDeviceID of Core Audio.
 */
- (id)initWithDelegate:(id<SuperpoweredOSXAudioIODelegate>)delegate preferredBufferSizeMs:(unsigned int)preferredBufferSizeMs numberOfChannels:(int)numberOfChannels enableInput:(bool)enableInput enableOutput:(bool)enableOutput audioDeviceID:(unsigned int)audioDeviceID;

/**
 @brief Changes the audio device.
 
 @param audioDeviceID The device identifier of the audio device (equals to AudioDeviceID of Core Audio) or UINT_MAX to use the default system audio device.
 */
- (void)setAudioDevice:(unsigned int)audioDeviceID;

/**
 @brief Starts audio processing.
 
 @return True if successful, false if failed.
 */
- (bool)start;

/**
 @brief Stops audio processing.
 */
- (void)stop;

/**
 @brief Call this to re-configure the channel mapping.
 */
- (void)mapChannels;

/**
 @brief Set the audio processing callback to a C function, instead of the delegate's Objective-C method.
 
 99% of all audio apps work great with the Objective-C method, so you don't need to use this. Don't call this method after [start]!
 
 @param callback The callback function.
 @param clientdata Some custom pointer for the C processing callback. You can set it to NULL.
 */
- (void)setProcessingCallback_C:(audioProcessingCallback_C)callback clientdata:(void *)clientdata;

/**
 @brief Get a list of the current audio input and output devices.
 
 @return A linked list of audioDevice structs.
 */
+ (audioDevice *)getAudioDevices;

@end


/**
 @brief You must implement this protocol to make SuperpoweredOSXAudioIODelegate work.
 */
@protocol SuperpoweredOSXAudioIODelegate
@optional

/**
 @brief Process audio here.
 
 @return Return false when you did no audio processing (silence).
 
 @param inputBuffers Input buffers.
 @param inputChannels The number of input channels.
 @param outputBuffers Output buffers.
 @param outputChannels The number of output channels.
 @param numberOfSamples The number of samples requested.
 @param samplerate The current sample rate in Hz.
 @param hostTime A mach timestamp, indicates when this chunk of audio will be passed to the audio output.
 
 @warning It's called on a high priority real-time audio thread, so please take care of blocking and processing time to prevent audio dropouts.
 */
- (bool)audioProcessingCallback:(float **)inputBuffers inputChannels:(unsigned int)inputChannels outputBuffers:(float **)outputBuffers outputChannels:(unsigned int)outputChannels numberOfSamples:(unsigned int)numberOfSamples samplerate:(unsigned int)samplerate hostTime:(unsigned long long int)hostTime;

- (void)mapChannels:(NSString *)outputDeviceName numOutputChannels:(int)numOutputChannels outputMap:(int *)outputMap input:(NSString *)inputDeviceName numInputChannels:(int)numInputChannels inputMap:(int *)inputMap;

- (void)audioDevicesChanged:(audioDevice *)devices;

@end
