#import <AVFoundation/AVAudioSession.h>

/**
 @brief Output channel mapping.

 This structure maps the channels you provide in the audio processing callback to the appropriate output channels.

 You can have multi-channel (more than a single stereo channel) output if a HDMI or USB audio device is connected. iOS does not provide multi-channel output for other audio devices, such as wireless audio accessories.

 @em Example:


 Let's say you have four output channels, and you'd like the first stereo pair on USB 3+4, and the other stereo pair on the iPad's headphone socket.

 1. Set deviceChannels[0] to 2, deviceChannels[1] to 3.
 
 2. Set USBChannels[2] to 0, USBChannels[3] to 1.

 
 @em Explanation:


 - Your four output channels are having the identifiers: 0, 1, 2, 3.

 - Every iOS device has just one stereo built-in output. This is represented by deviceChannels, and (1.) sets your second stereo pair (2, 3) to these.

 - You other stereo pair (0, 1) is mapped to USBChannels. USBChannels[2] represents the third USB channel.

 @since Multi-channel output is available in iOS 6.0 and later.

 @param deviceChannels The iOS device's built-in output channels.
 @param HDMIChannels HDMI output channels.
 @param USBChannels USB output channels.
 @param numberOfHDMIChannelsAvailable Number of available HDMI output channels. Read only.
 @param numberOfUSBChannelsAvailable Number of available USB output channels. Read only.
 @param headphoneAvailable Something is plugged into the iOS device's headphone socket or not. Read only.
 */
typedef struct multiOutputChannelMap {
    // Information you provide:
    int deviceChannels[2];
    int HDMIChannels[8];
    int USBChannels[32];

    // READ ONLY information:
    int numberOfHDMIChannelsAvailable, numberOfUSBChannelsAvailable;
    bool headphoneAvailable;
} multiOutputChannelMap;

/**
 @brief Input channel mapping.

 Similar to the output channels, you can map the input channels too. It works with USB only.

 Let's say you set the channel count to 4, so RemoteIO provides 4 input channel buffers. Using this struct, you can map which USB input channel appears on the specific buffer positions.

 @since Available in iOS 6.0 and later.
 @see @c multiOutputChannelMap

 @param USBChannels Example: set USBChannels[0] to 3, to receive the input of the third USB channel on the first buffer.
 @param numberOfUSBChannelsAvailable Number of USB input channels.
 */
typedef struct multiInputChannelMap {
    int USBChannels[32]; // You provide this.
    int numberOfUSBChannelsAvailable; // READ ONLY
} multiInputChannelMap;

@protocol SuperpoweredIOSAudioIODelegate;

/**
 @brief You can have an audio processing callback with the delegate (Objective-C) or pure C. This is the pure C prototype.
 
 @return Return false for no audio output (silence).

 @param clientData A custom pointer your callback receives.
 @param buffers Input-output buffers.
 @param inputChannels The number of input channels.
 @param outputChannels The number of output channels.
 @param numberOfSamples The number of samples requested.
 @param samplerate The current sample rate in Hz.
 @param hostTime A mach timestamp, indicates when this chunk of audio will be passed to the audio output.
 */
typedef bool (*audioProcessingCallback_C) (void *clientdata, float **buffers, unsigned int inputChannels, unsigned int outputChannels, unsigned int numberOfSamples, unsigned int samplerate, uint64_t hostTime);

/**
 @brief Handles all audio session, audio lifecycle (interruptions), output, buffer size, samplerate and routing headaches.
 
 @warning All methods and setters should be called on the main thread only!
 */
@interface SuperpoweredIOSAudioIO: NSObject {
    int preferredBufferSizeMs;
    bool saveBatteryInBackground;
}

/** @brief The preferred buffer size. Recommended: 12. */
@property (nonatomic, assign) int preferredBufferSizeMs;
/** @brief Save battery if output is silence and the app runs in background mode. True by default. */
@property (nonatomic, assign) bool saveBatteryInBackground;

/**
 @brief Creates the audio output instance.
  
 @param delegate The object fully implementing the SuperpoweredIOSAudioIODelegate protocol. Not retained.
 @param preferredBufferSize The initial value for preferredBufferSizeMs. 12 is good for every iOS device (512 samples).
 @param preferredMinimumSamplerate The preferred minimum sample rate. 44100 or 48000 are recommended for good sound quality.
 @param audioSessionCategory The audio session category. Audio input is enabled for the appropriate categories only!
 @param channels The number of channels in the audio processing callback.
 */
- (id)initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)delegate preferredBufferSize:(unsigned int)preferredBufferSize preferredMinimumSamplerate:(unsigned int)preferredMinimumSamplerate audioSessionCategory:(NSString *)audioSessionCategory channels:(int)channels;

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

- (void)reconfigureWithAudioSessionCategory:(NSString *)audioSessionCategory;

@end


/**
 @brief You must implement this protocol to make SuperpoweredIOSAudioIO work.
 */
@protocol SuperpoweredIOSAudioIODelegate

/**
 @brief The audio session may be interrupted by a phone call, etc. This method is called on the main thread when this happens.
 */
- (void)interruptionStarted;

/**
 @brief The audio session may be interrupted by a phone call, etc. This method is called on the main thread when audio resumes.
 */
- (void)interruptionEnded;

/**
 @brief Called if the user did not grant a recording permission for the app.
 */
- (void)recordPermissionRefused;

/**
 @brief This method is called on the main thread, when a multi-channel audio device is connected or disconnected.
 
 @param outputMap Map the output channels here.
 @param inputMap Map the input channels here.
 @param externalAudioDeviceName The name of the attached audio device, such as the model of the sound card.
 @param outputsAndInputs A human readable description about the available outputs and inputs.
 */
- (void)mapChannels:(multiOutputChannelMap *)outputMap inputMap:(multiInputChannelMap *)inputMap externalAudioDeviceName:(NSString *)externalAudioDeviceName outputsAndInputs:(NSString *)outputsAndInputs;

/**
 @brief Process audio here.
 
 @return Return false for no audio output (silence).
 
 @param buffers Input-output buffers.
 @param inputChannels The number of input channels.
 @param outputChannels The number of output channels.
 @param numberOfSamples The number of samples requested.
 @param samplerate The current sample rate in Hz.
 @param hostTime A mach timestamp, indicates when this chunk of audio will be passed to the audio output.
 
 @warning It's called on a high priority real-time audio thread, so please take care of blocking and processing time to prevent audio dropouts.
 */
- (bool)audioProcessingCallback:(float **)buffers inputChannels:(unsigned int)inputChannels outputChannels:(unsigned int)outputChannels numberOfSamples:(unsigned int)numberOfSamples samplerate:(unsigned int)samplerate hostTime:(UInt64)hostTime;

@end
