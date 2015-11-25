#import "audioHandler.h"
#include "SuperpoweredWhoosh.h"
#include "SuperpoweredSimple.h"

@implementation audioHandler {
    SuperpoweredWhoosh *whoosh;
    float *stereoOutput;
    unsigned int lastSamplerate;
}

- (id)init {
    self = [super init];
    if (self) {
        lastSamplerate = 44100;
        whoosh = new SuperpoweredWhoosh(lastSamplerate);
        whoosh->wet = 1.0f;
        whoosh->setFrequency(2000.0f);
        stereoOutput = (float *)malloc(sizeof(float) * 2 * 2048);
    }
    return self;
}

- (void)dealloc {
    free(stereoOutput);
    delete whoosh;
}

- (void)interruptionStarted {}
- (void)interruptionEnded {}

- (void)enable:(bool)enabled {
    whoosh->enable(enabled);
}

- (void)adjust:(float)percent {
    whoosh->setFrequency(400.0f + 8000.0f * percent);
}

- (bool)audioProcessingCallback:(float **)buffers outputChannels:(unsigned int)outputChannels numberOfSamples:(unsigned int)numberOfSamples samplerate:(unsigned int)samplerate hostTime:(UInt64)hostTime {
    if (lastSamplerate != samplerate) {
        lastSamplerate = samplerate;
        whoosh->setSamplerate(samplerate);
    };

    if (whoosh->process(NULL, stereoOutput, numberOfSamples)) {
        SuperpoweredDeInterleave(stereoOutput, buffers[0], buffers[1], numberOfSamples);
        return true;
    } else return false;
}

@end
