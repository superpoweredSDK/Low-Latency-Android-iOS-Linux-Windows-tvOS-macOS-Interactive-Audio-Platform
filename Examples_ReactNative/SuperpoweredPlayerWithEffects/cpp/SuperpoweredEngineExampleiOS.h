#pragma once

#import "SuperpoweredEngineExample.h"
#import "OpenSource/SuperpoweredIOSAudioIO.h"

@interface SuperpoweredEngineExampleiOS : NSObject {
    SuperpoweredIOSAudioIO *audioIO;
}

- (instancetype)initWithBase:(SuperpoweredEngineExample *) example;

@end
