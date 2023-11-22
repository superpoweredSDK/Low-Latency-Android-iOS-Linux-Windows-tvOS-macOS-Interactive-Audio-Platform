//
//  SuperpoweredAudioEngine.m
//  SuperpoweredVisionOSExample
//
//  Created by Balazs Kiss on 06/10/2023.
//

#import "SuperpoweredAudioEngine.h"
#import "SuperpoweredVisionOSAudioIO.h"
#include "Superpowered.h"
#include "SuperpoweredSimple.h"
#include "SuperpoweredGenerator.h"

@implementation SuperpoweredAudioEngine {
    SuperpoweredVisionOSAudioIO *audioIO;
    Superpowered::Generator *generator;
}

- (instancetype)init {
    if (self = [super init]) {
        Superpowered::Initialize("ExampleLicenseKey-WillExpire-OnNextUpdate");

        generator = new Superpowered::Generator(44100, Superpowered::Generator::Sine);
        generator->frequency = 440;

        audioIO = [[SuperpoweredVisionOSAudioIO alloc] initWithDelegate:(id<SuperpoweredVisionOSAudioIODelegate>)self preferredBufferSize:12 preferredMinimumSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryPlayback channels:2];
    }
    return self;
}

- (void)start {
    [audioIO start];
}

- (void)stop {
    [audioIO stop];
}

- (bool)audioProcessingCallback:(float *)output numberOfFrames:(unsigned int)numberOfFrames samplerate:(unsigned int)samplerate hostTime:(UInt64)hostTime {
    float tempBuffer[numberOfFrames];
    generator->samplerate = samplerate;
    generator->generate(tempBuffer, numberOfFrames);
    Superpowered::Interleave(tempBuffer, tempBuffer, output, numberOfFrames);
    return true;
}

@end
