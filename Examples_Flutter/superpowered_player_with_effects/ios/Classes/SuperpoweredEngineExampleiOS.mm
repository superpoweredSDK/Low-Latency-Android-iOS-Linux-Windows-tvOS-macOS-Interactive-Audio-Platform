#import <Foundation/Foundation.h>
#import "SuperpoweredEngineExampleiOS.h"

@implementation SuperpoweredEngineExampleiOS {
    SuperpoweredEngineExample *example;
    void* selfReference;
}

void SuperpoweredEngineExample::platformInit() {
    iOSEngine = (void*) CFBridgingRetain([[SuperpoweredEngineExampleiOS alloc] initWithBase:this]);
    printf("platformInit - iOSEngine %lld\n", (uint64_t)iOSEngine);
}

void SuperpoweredEngineExample::platformStartEngine() {
    NSLog(@"SuperpoweredEngineExampleiOS platformStartEngine");
    __unsafe_unretained SuperpoweredEngineExampleiOS *op = (__bridge SuperpoweredEngineExampleiOS *)iOSEngine;
    [op->audioIO start];
}

void SuperpoweredEngineExample::platformStopEngine() {
      __unsafe_unretained SuperpoweredEngineExampleiOS *op = (__bridge SuperpoweredEngineExampleiOS *)iOSEngine;
    [op->audioIO stop];
}

void SuperpoweredEngineExample::platformCleanup() {
      printf("destructor - iOSEngine %lld\n", (uint64_t)iOSEngine);
    __unsafe_unretained SuperpoweredEngineExampleiOS *op = (__bridge SuperpoweredEngineExampleiOS *)iOSEngine;
    [op clearSelfReference];
    CFBridgingRelease(iOSEngine);
}

- (void)dealloc {
    NSLog(@"SuperpoweredEngineExampleiOS dealloc %p", self);
}

- (void)clearSelfReference {
    CFBridgingRelease(selfReference);
}

- (instancetype)initWithBase:(SuperpoweredEngineExample *) engine {
    self = [super init];
    example = engine;
    selfReference = (void *) CFBridgingRetain(self);
    audioIO = [[SuperpoweredIOSAudioIO alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self
                                            preferredBufferSize:12
                                            preferredSamplerate:44100
                                            audioSessionCategory:AVAudioSessionCategoryPlayback
                                            channels:2
                                            audioProcessingCallback:audioProcessing
                                            clientdata:selfReference];
    NSLog(@"SuperpoweredEngineExampleiOS init %p", self);
    return self;
}

static bool audioProcessing(void *clientdata,
                            float *inputBuffers,
                            float *outputBuffers,
                            unsigned int numberOfFrames,
                            unsigned int samplerate,
                            uint64_t hostTime) {
    __unsafe_unretained SuperpoweredEngineExampleiOS *op = (__bridge SuperpoweredEngineExampleiOS *)clientdata;
    return op->example->audioProcessing(outputBuffers, numberOfFrames, samplerate);
}

@end
