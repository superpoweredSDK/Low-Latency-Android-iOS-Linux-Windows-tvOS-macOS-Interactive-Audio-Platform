#import "ViewController.h"
#import "SuperpoweredtvOSAudioIO.h"
#include "Superpowered.h"
#include "SuperpoweredWhoosh.h"
#include "SuperpoweredSimple.h"

@implementation ViewController {
    SuperpoweredtvOSAudioIO *audioIO;
    Superpowered::Whoosh *whoosh;
}

@synthesize marker;

- (void)viewDidLoad {
    [super viewDidLoad];
    
    Superpowered::Initialize(
                             "ExampleLicenseKey-WillExpire-OnNextUpdate",
                             false, // enableAudioAnalysis (using SuperpoweredAnalyzer, SuperpoweredLiveAnalyzer, SuperpoweredWaveform or SuperpoweredBandpassFilterbank)
                             false, // enableFFTAndFrequencyDomain (using SuperpoweredFrequencyDomain, SuperpoweredFFTComplex, SuperpoweredFFTReal or SuperpoweredPolarFFT)
                             false, // enableAudioTimeStretching (using SuperpoweredTimeStretching)
                             true, // enableAudioEffects (using any SuperpoweredFX class)
                             false, // enableAudioPlayerAndDecoder (using SuperpoweredAdvancedAudioPlayer or SuperpoweredDecoder)
                             false, // enableCryptographics (using Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher or Superpowered::AES)
                             false  // enableNetworking (using Superpowered::httpRequest)
                             );
    
    whoosh = new Superpowered::Whoosh(44100);
    whoosh->wet = 1.0f;
    whoosh->frequency = 2000.0f;
    
    audioIO = [[SuperpoweredtvOSAudioIO alloc] initWithDelegate:(id<SuperpoweredtvOSAudioIODelegate>)self preferredBufferSize:12 preferredMinimumSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryPlayback channels:2];
    [audioIO start];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    CGRect frame = marker.frame;
    frame.origin.x = 0;
    frame.origin.y = self.view.frame.size.height * 0.25;
    marker.frame = frame;
}

- (void)dealloc {
    audioIO = nil;
    whoosh = nil;
}

- (bool)audioProcessingCallback:(float **)buffers outputChannels:(unsigned int)outputChannels numberOfFrames:(unsigned int)numberOfFrames samplerate:(unsigned int)samplerate hostTime:(UInt64)hostTime {
    whoosh->samplerate = samplerate;
    float stereoOutput[numberOfFrames * 2];
    
    if (whoosh->process(NULL, stereoOutput, numberOfFrames)) {
        Superpowered::DeInterleave(stereoOutput, buffers[0], buffers[1], numberOfFrames);
        return true;
    } else return false;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    [super touchesBegan:touches withEvent:event];
    whoosh->enabled = true;
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    [super touchesMoved:touches withEvent:event];
    UITouch *touch = [touches anyObject];
    CGFloat movement = [touch locationInView:self.view].x - [touch previousLocationInView:self.view].x;

    CGRect frame = marker.frame;
    frame.origin.y = self.view.frame.size.height * 0.25;
    frame.origin.x = MAX(0, MIN(self.view.frame.size.width - marker.frame.size.width, marker.frame.origin.x + movement));
    marker.frame = frame;

    float percent = frame.origin.x / (self.view.frame.size.width - frame.size.width);
    whoosh->frequency = 400.0f + 8000.0f * percent;
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    [super touchesEnded:touches withEvent:event];
    whoosh->enabled = false;
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    [super touchesCancelled:touches withEvent:event];
    whoosh->enabled = false;
}

@end


