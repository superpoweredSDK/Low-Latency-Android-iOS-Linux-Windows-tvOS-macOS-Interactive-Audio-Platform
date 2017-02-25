#import <Foundation/Foundation.h>

@interface audioHandler: NSObject
- (void)enable:(bool)enabled;
- (void)adjust:(float)percent;
- (void)interruptionStarted;
- (void)interruptionEnded;
- (bool)audioProcessingCallback:(float **)buffers outputChannels:(unsigned int)outputChannels numberOfSamples:(unsigned int)numberOfSamples samplerate:(unsigned int)samplerate hostTime:(UInt64)hostTime;
@end
