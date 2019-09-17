#import <AVFoundation/AVFoundation.h>

/// @brief Output channel mapping for iOS audio I/O.
/// This structure maps the channels you provide in the audio processing callback to the appropriate output channels.
/// You can have multi-channel (more than a single stereo channel) output if a HDMI or USB audio device is connected. iOS does not provide multi-channel output for other audio devices, such as wireless audio accessories.
/// @em Example:
/// Let's say you have four output channels, and you'd like the first stereo pair on USB 3+4, and the other stereo pair on the iPad's headphone socket.
/// 1. Set deviceChannels[0] to 2, deviceChannels[1] to 3.
/// 2. Set USBChannels[2] to 0, USBChannels[3] to 1.
/// @em Explanation:
/// - Your four output channels are having the identifiers: 0, 1, 2, 3.
/// - Every iOS device has just one stereo built-in output. This is represented by deviceChannels, and (1.) sets your second stereo pair (2, 3) to these.
/// - You other stereo pair (0, 1) is mapped to USBChannels. USBChannels[2] represents the third USB channel.
/// @since Multi-channel output is available in iOS 6.0 and later.
typedef struct multiOutputChannelMap {
    int deviceChannels[2]; ///< The iOS device's built-in output channels. Write only.
    int HDMIChannels[8];   ///< HDMI output channels. Write only.
    int USBChannels[32];   ///< USB output channels. Write only.
    int numberOfHDMIChannelsAvailable; ///< Number of available HDMI output channels. Read only.
    int numberOfUSBChannelsAvailable;  ///< Number of available USB output channels. Read only.
    bool headphoneAvailable;           ///< Something is plugged into the iOS device's headphone socket or not. Read only.
} multiOutputChannelMap;

/// @brief Input channel mapping for iOS audio I/O.
/// Similar to the output channels, you can map the input channels to channels in the audio processing callback. This feature works with USB only.
/// Let's say you set the channel count to 4, so RemoteIO provides 4 input channel buffers. Using this struct, you can map which USB input channel appears on the specific buffer positions.
/// @since Available in iOS 6.0 and later.
/// @see @c multiOutputChannelMap
typedef struct multiInputChannelMap {
    int USBChannels[32];              ///< Example: set USBChannels[0] to 3, to receive the input of the third USB channel on the first buffer. Write only.
    int numberOfUSBChannelsAvailable; ///< Number of USB input channels. Read only.
} multiInputChannelMap;

@protocol SuperpoweredIOSAudioIODelegate;

/// @brief The audio processing callback prototype.
/// @return Return false for no audio output (silence).
/// @param clientData A custom pointer your callback receives.
/// @param inputBuffers Input buffers.
/// @param inputChannels The number of input channels.
/// @param outputBuffers Output buffers.
/// @param outputChannels The number of output channels.
/// @param numberOfFrames The number of frames requested.
/// @param samplerate The current sample rate in Hz.
/// @param hostTime A mach timestamp, indicates when this buffer of audio will be passed to the audio output.
typedef bool (*audioProcessingCallback) (void *clientData, float **inputBuffers, unsigned int inputChannels, float **outputBuffers, unsigned int outputChannels, unsigned int numberOfFrames, unsigned int samplerate, unsigned long long hostTime);


/// @brief Handles all audio session, audio lifecycle (interruptions), output, buffer size, samplerate and routing headaches.
/// @warning All methods and setters should be called on the main thread only!
@interface SuperpoweredIOSAudioIO: NSObject {
    int preferredBufferSizeMs;
    int preferredSamplerate;
    bool saveBatteryInBackground;
    bool started;
}

@property (nonatomic, assign) int preferredBufferSizeMs;    ///< The preferred buffer size in milliseconds. Recommended: 12.
@property (nonatomic, assign) int preferredSamplerate;      ///< The preferred sample rate in Hz.
@property (nonatomic, assign) bool saveBatteryInBackground; ///< Save battery if output is silence and the app runs in background mode. True by default.
@property (nonatomic, assign, readonly) bool started;       ///< Indicates if the instance has been started.

/// @brief Constructor.
/// @param delegate The object fully implementing the SuperpoweredIOSAudioIODelegate protocol. Not retained.
/// @param preferredBufferSize The initial value for preferredBufferSizeMs. 12 is good for every iOS device (512 frames).
/// @param preferredSamplerate The preferred sample rate. 44100 or 48000 are recommended for good sound quality.
/// @param audioSessionCategory The audio session category. Audio input is enabled for the appropriate categories only!
/// @param channels The number of output channels in the audio processing callback regardless the actual hardware capabilities. The number of input channels in the audio processing callback will reflect the actual hardware configuration.
/// @param callback The audio processing callback.
/// @param clientdata Custom data passed to the audio processing callback.
- (id)initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)delegate preferredBufferSize:(unsigned int)preferredBufferSize preferredSamplerate:(unsigned int)preferredSamplerate audioSessionCategory:(NSString *)audioSessionCategory channels:(int)channels audioProcessingCallback:(audioProcessingCallback)callback clientdata:(void *)clientdata;

/// @brief Starts audio I/O.
/// @return True if successful, false if failed.
- (bool)start;

/// @brief Stops audio I/O.
- (void)stop;

/// @brief Call this to re-configure the channel mapping.
- (void)mapChannels;

/// @brief Call this to re-configure the audio session category (such as enabling/disabling recording).
- (void)reconfigureWithAudioSessionCategory:(NSString *)audioSessionCategory;

@end


/// @brief You MUST implement this protocol to use SuperpoweredIOSAudioIO.
@protocol SuperpoweredIOSAudioIODelegate

/// @brief The audio session may be interrupted by a phone call, etc. This method is called on the main thread when the interrupt starts.
@optional
- (void)interruptionStarted;

/// @brief The audio session may be interrupted by a phone call, etc. This method is called on the main thread when audio resumes.
@optional
- (void)interruptionEnded;

/// @brief Called if the user did not grant a recording permission for the app.
@optional
- (void)recordPermissionRefused;

/// @brief This method is called on the main thread when a multi-channel audio device is connected or disconnected.
/// @param outputMap Map the output channels here.
/// @param inputMap Map the input channels here.
/// @param externalAudioDeviceName The name of the attached audio device, such as the model of the sound card.
/// @param outputsAndInputs A human readable description about the available outputs and inputs.
@optional
- (void)mapChannels:(multiOutputChannelMap *)outputMap inputMap:(multiInputChannelMap *)inputMap externalAudioDeviceName:(NSString *)externalAudioDeviceName outputsAndInputs:(NSString *)outputsAndInputs;

@end
