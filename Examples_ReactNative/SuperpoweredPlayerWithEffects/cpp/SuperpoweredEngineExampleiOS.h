#pragma once

#import "SuperpoweredEngineExample.h"
#import "SuperpoweredIOSAudioIO.h"

@interface SuperpoweredEngineExampleiOS : NSObject {
    SuperpoweredIOSAudioIO *audioIO;
}

- (instancetype)initWithBase:(SuperpoweredEngineExample *) example;

@end
