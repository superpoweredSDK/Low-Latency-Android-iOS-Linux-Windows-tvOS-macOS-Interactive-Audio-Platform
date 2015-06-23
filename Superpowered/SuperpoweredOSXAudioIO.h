#import <Cocoa/Cocoa.h>

@protocol SuperpoweredOSXAudioIODelegate;

/**
 @brief You can have an audio processing callback in the delegate (Objective-C) or pure C. This is the pure C prototype.
 
 @return Return false when you did no audio processing (silence).

 @param clientData A custom pointer your callback receives.
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
 @brief Creates the audio output instance.
  
 @param delegate The object fully implementing the SuperpoweredOSXAudioIODelegate protocol. Not retained.
 @param preferredBufferSizeMs The initial value for preferredBufferSizeMs. 12 is good for every device (512 samples).
 @param numberOfChannels The number of channels you provide in the audio processing callback.
 @param enableInput Enable audio input.
 @param enableOutput Enable audio output.
 */
- (id)initWithDelegate:(id<SuperpoweredOSXAudioIODelegate>)delegate preferredBufferSizeMs:(unsigned int)preferredBufferSizeMs numberOfChannels:(int)numberOfChannels enableInput:(bool)enableInput enableOutput:(bool)enableOutput;

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
 @brief Set the audio processing callback to a C function, instead of the delegate's Objective-C method.
 
 99% of all audio apps work great with the Objective-C method, so you don't need to use this. Don't call this method after [start]!
 
 @param callback The callback function.
 @param clientdata Some custom pointer for the C processing callback. You can set it to NULL.
 */
- (void)setProcessingCallback_C:(audioProcessingCallback_C)callback clientdata:(void *)clientdata;

@end


/**
 @brief You must implement this protocol to make SuperpoweredOSXAudioIODelegate work.
 */
@protocol SuperpoweredOSXAudioIODelegate

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
- (bool)audioProcessingCallback:(float **)inputBuffers inputChannels:(unsigned int)inputChannels outputBuffers:(float **)outputBuffers outputChannels:(unsigned int)outputChannels numberOfSamples:(unsigned int)numberOfSamples samplerate:(unsigned int)samplerate hostTime:(UInt64)hostTime;

@end
