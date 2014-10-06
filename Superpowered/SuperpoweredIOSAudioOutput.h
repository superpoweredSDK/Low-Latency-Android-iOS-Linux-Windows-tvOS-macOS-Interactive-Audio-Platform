#import <AVFoundation/AVAudioSession.h>

struct multiRouteOutputChannelMap;
struct multiRouteInputChannelMap;
@protocol SuperpoweredIOSAudioIODelegate;

/**
 @brief Handles all audio session, audio lifecycle (interruptions), output, buffer size, samplerate and routing headaches.
 
 @warning All methods and setters should be called on the main thread only!
 */
@interface SuperpoweredIOSAudioOutput: NSObject {
    int preferredBufferSizeSamples;
    bool inputEnabled;
}

/** @brief The preferred buffer size. Should be 128, 256 or 512. */
@property (nonatomic, assign) int preferredBufferSizeSamples;
/** @brief Set this to true to enable audio input. Disabled by default. */
@property (nonatomic, assign) bool inputEnabled;

/**
 @brief Creates the audio output instance.
  
 @param delegate The object fully implementing the SuperpoweredIOSAudioIODelegate protocol. Not retained.
 @param preferredBufferSize The initial value for preferredBufferSizeSamples. Should be 128, 256 or 512.
 @param audioSessionCategory The audio session category. If you want to use MultiRoute, set it to AVAudioSessionCategoryPlayback, and set multiRouteChannels to more than 2. You don't loose the ability of AirPlay this way.
 @param multiRouteChannels The number of channels you provide in the audio processing callback. Used in the MultiRoute category only.
 @param fixReceiver Sometimes the audio goes to the phone's receiver ("ear speaker"). Set this to true if you want the real speaker instead.
 */
- (id)initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)delegate preferredBufferSize:(unsigned int)preferredBufferSize audioSessionCategory:(NSString *)audioSessionCategory multiRouteChannels:(int)multiRouteChannels fixReceiver:(bool)fixReceiver;

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
 @brief Call this to re-configure the channel mapping for the MultiRoute category.
 */
- (void)multiRouteRemapChannels;

@end


/**
 @brief You must implement this protocol to make SuperpoweredIOSAudioOutput work.
 */
@protocol SuperpoweredIOSAudioIODelegate

/**
 @brief The audio session may be interrupted by a phone call, etc. This method is called on the main thread when audio resumes.
 */
- (void)interruptionEnded;

/**
 @brief This method is called on the main thread, when you use the MultiRoute audio session category and a new audio device is connected or disconnected.
 
 @param outputMap Map the output channels here.
 @param inputMap Map the input channels here.
 @param multiRouteDeviceName The name of the attached audio device, for example the model of the sound card.
 @param outputsAndInputs A human readable description about the available outputs and inputs.
 */
- (void)multiRouteMapChannels:(multiRouteOutputChannelMap *)outputMap inputMap:(multiRouteInputChannelMap *)inputMap multiRouteDeviceName:(NSString *)multiRouteDeviceName outputsAndInputs:(NSString *)outputsAndInputs;

/**
 @brief Process audio here.
 
 @return Return false when you did no audio processing (silence).
 
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


/**
 @brief Output channel mapping for the MultiRoute audio session category.
 
 This structure maps the channels you provide in the audio processing callback to the appropriate output channels.
 
 You can have more than a single stereo output with the MultiRoute audio session category, if a HDMI or USB audio device is connected (it doesn't work with other, such as wireless audio accessories).
 
 @em Example:
 
 
 Let's say you have four output channels, and you'd like the first stereo pair on USB 3+4, and the other stereo pair on the iPad's headphone socket.
 
 1. Set deviceChannels[0] to 2, deviceChannels[1] to 3.
 
 2. Set USBChannels[2] to 0, USBChannels[3] to 1.
 
 @em Explanation:
 
 
 - Your four output channels are having the identifiers: 0, 1, 2, 3.
 
 - The iPad (and all other iOS device) has just a stereo built-in channel pair. This is represented by deviceChannels, and (1.) sets your second stereo pair (2, 3) to these.
 
 - You other stereo pair (0, 1) is mapped to USBChannels. USBChannels[2] represents the third USB channel.
 
 @since The MultiRoute category is available in iOS 6.0 and later.
 
 @param deviceChannels The iOS device's built-in output channels.
 @param HDMIChannels HDMI output channels.
 @param USBChannels USB output channels.
 @param numberOfHDMIChannelsAvailable Number of available HDMI output channels.
 @param numberOfUSBChannelsAvailable Number of available USB output channels.
 @param headphoneAvailable Something is plugged into the iOS device's headphone socket or not.
 */
typedef struct multiRouteOutputChannelMap {
    int deviceChannels[2];
    int HDMIChannels[8];
    int USBChannels[32];
    
    // READ ONLY information:
    int numberOfHDMIChannelsAvailable, numberOfUSBChannelsAvailable;
    bool headphoneAvailable;
} multiRouteOutputChannelMap;

/**
 @brief Input channel mapping for the MultiRoute audio session category.
 
 Similar to the output channels, you can map the input channels for the multiroute category. It works with USB only.
 
 Let's say you set the channel count to 4, so RemoteIO is able to provide you 4 input channel buffers. Using this struct, you can map which USB input channel appears on the specific buffer positions.
 
 @since The MultiRoute category is available in iOS 6.0 and later.
 @see @c multiRouteOutputChannelMap
 
 @param USBChannels Example: set USBChannels[0] to 3, to receive the input of the third USB channel on the first buffer.
 @param numberOfUSBChannelsAvailable Number of USB input channels.
 */
typedef struct multiRouteInputChannelMap {
    int USBChannels[32];
    int numberOfUSBChannelsAvailable; // READ ONLY
} multiRouteInputChannelMap;
