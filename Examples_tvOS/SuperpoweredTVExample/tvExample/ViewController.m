#import "ViewController.h"
#import "SuperpoweredtvOSAudioIO.h"
#import "audioHandler.h"

// This class handles the touch input and the audio output.

@implementation ViewController {
    SuperpoweredtvOSAudioIO *audioIO;
    audioHandler *superpowered;
}

@synthesize marker;

- (void)viewDidLoad {
    [super viewDidLoad];
    superpowered = [[audioHandler alloc] init];
    audioIO = [[SuperpoweredtvOSAudioIO alloc] initWithDelegate:(id<SuperpoweredtvOSAudioIODelegate>)superpowered preferredBufferSize:12 preferredMinimumSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryPlayback channels:2];
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
    superpowered = nil;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    [super touchesBegan:touches withEvent:event];
    [superpowered enable:true];
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
    [superpowered adjust:percent];

}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    [super touchesEnded:touches withEvent:event];
    [superpowered enable:false];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    [super touchesCancelled:touches withEvent:event];
    [superpowered enable:false];
}

@end


