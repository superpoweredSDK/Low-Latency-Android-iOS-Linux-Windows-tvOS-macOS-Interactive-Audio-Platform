#import "ViewController.h"
#import "SuperpoweredIOSAudioIO.h"
#include "Superpowered.h"
#include "SuperpoweredAdvancedAudioPlayer.h"
#include "SuperpoweredSimple.h"

// some HLS stream url-title pairs
static const char *urls[8] = {
    "https://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/bipbop_16x9_variant.m3u8", "Apple Advanced Example Stream",
    "http://qthttp.apple.com.edgesuite.net/1010qwoeiuryfg/sl.m3u8", "Back to the Mac",
    "http://playertest.longtailvideo.com/adaptive/bbbfull/bbbfull.m3u8", "JW Player Test",
    "http://playertest.longtailvideo.com/adaptive/oceans_aes/oceans_aes.m3u8", "JW AES Encrypted",
};

@implementation ViewController {
    UIView *bufferIndicator;
    CADisplayLink *displayLink;
    SuperpoweredIOSAudioIO *audioIO;
    Superpowered::AdvancedAudioPlayer *player;
    CGFloat sliderThumbWidth;
    unsigned int lastPositionSeconds, durationMs;
    NSInteger selectedRow;
}

@synthesize seekSlider, currentTime, duration, playPause, sources;

- (void)viewDidLoad {
    [super viewDidLoad];
    #ifdef __IPHONE_13_0
        if (@available(iOS 13, *)) self.overrideUserInterfaceStyle = UIUserInterfaceStyleLight;
    #endif
    
    Superpowered::Initialize("ExampleLicenseKey-WillExpire-OnNextUpdate");
    
    Superpowered::AdvancedAudioPlayer::setTempFolder([NSTemporaryDirectory() fileSystemRepresentation]);
    player = new Superpowered::AdvancedAudioPlayer(44100, 0);
    
    lastPositionSeconds = 0;
    selectedRow = 0;
    sliderThumbWidth = [seekSlider thumbRectForBounds:seekSlider.bounds trackRect:[seekSlider trackRectForBounds:seekSlider.bounds] value:0].size.width;

    bufferIndicator = [[UIView alloc] initWithFrame:CGRectZero];
    bufferIndicator.backgroundColor = [UIColor lightGrayColor];
    [self.view insertSubview:bufferIndicator belowSubview:seekSlider];

    displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(onDisplayLink)];
    displayLink.preferredFramesPerSecond = 60;
    [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];

    audioIO = [[SuperpoweredIOSAudioIO alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:12 preferredSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryPlayback channels:2 audioProcessingCallback:audioProcessing clientdata:(__bridge void *)self];
    [audioIO start];

    [sources selectRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0] animated:NO scrollPosition:UITableViewScrollPositionTop];
    [self open:0];
}

- (void)dealloc {
    [displayLink invalidate];
    audioIO = nil;
    bufferIndicator = nil;
    delete player;
    Superpowered::AdvancedAudioPlayer::setTempFolder(NULL);
}

// Called periodically by the operating system's audio stack to provide audio output.
static bool audioProcessing(void *clientdata, float *input, float *output, unsigned int numberOfFrames, unsigned int samplerate, uint64_t hostTime) {
    __unsafe_unretained ViewController *self = (__bridge ViewController *)clientdata;
    self->player->outputSamplerate = samplerate;
    
    bool notSilence = self->player->processStereo(output, false, numberOfFrames);
    return notSilence;
}

// Called periodically on every screen refresh, 60 fps.
- (void)onDisplayLink {
    // Check player events.
    switch (player->getLatestEvent()) {
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_Opened:
            player->play();
            break;
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_OpenFailed:
            NSLog(@"Open error %i: %s", player->getOpenErrorCode(), Superpowered::AdvancedAudioPlayer::statusCodeToString(player->getOpenErrorCode()));
            break;
        default:;
    };
    
    // On end of file return to the beginning and stop.
    if (player->eofRecently()) player->setPosition(0, true, false);
    
    if (durationMs != player->getDurationMs()) {
        durationMs = player->getDurationMs();
        
        if (durationMs == UINT_MAX) {
            duration.text = @"LIVE";
            seekSlider.hidden = YES;
        } else {
            duration.text = [NSString stringWithFormat:@"%02d:%02d", player->getDurationSeconds() / 60, player->getDurationSeconds() % 60];
            seekSlider.hidden = NO;
        };
        
        currentTime.hidden = playPause.hidden = NO;
    }
    
    // Update the buffering indicator.
    CGRect frame = seekSlider.frame;
    frame.size.width -= sliderThumbWidth;
    frame.origin.x += (player->getBufferedStartPercent() * frame.size.width) + (sliderThumbWidth * 0.5f);
    frame.size.width = (player->getBufferedEndPercent() - player->getBufferedStartPercent()) * frame.size.width;
    bufferIndicator.frame = frame;

    // Update the seek slider.
    unsigned int positionSeconds;
    if (seekSlider.tracking) positionSeconds = seekSlider.value * (float)player->getDurationSeconds();
    else {
        positionSeconds = player->getDisplayPositionSeconds();
        seekSlider.value = player->getDisplayPositionPercent();
    };
    
    // Update the time display.
    if (lastPositionSeconds != positionSeconds) {
        lastPositionSeconds = positionSeconds;
        currentTime.text = [NSString stringWithFormat:@"%02d:%02d", positionSeconds / 60, positionSeconds % 60];
    };

    // Update the play/pause button.
    playPause.highlighted = player->isPlaying();
}

- (IBAction)onSeekSlider:(id)sender {
    player->seek(((UISlider *)sender).value);
}

- (IBAction)onDownloadStrategy:(id)sender {
    switch (((UISegmentedControl *)sender).selectedSegmentIndex) {
        case 1: player->HLSBufferingSeconds = 20; break; // Will not buffer more than 20 seconds ahead of the playback position.
        case 2: player->HLSBufferingSeconds = 40; break; // Will not buffer more than 40 seconds ahead of the playback position.
        case 3: player->HLSBufferingSeconds = Superpowered::AdvancedAudioPlayer::HLSDownloadEverything; break; // Will buffer everything after and before the playback position.
        default: player->HLSBufferingSeconds = Superpowered::AdvancedAudioPlayer::HLSDownloadRemaining;        // Will buffer everything after the playback position.
    };
}

- (IBAction)onPlayPause:(id)sender {
    player->togglePlayback();
}

- (IBAction)onSpeed:(id)sender {
    player->playbackRate = ((UISwitch *)sender).on ? 2 : 1;
}

- (void)open:(NSInteger)row {
    currentTime.hidden = playPause.hidden = seekSlider.hidden = YES;
    duration.text = @"Loading...";
    player->openHLS(urls[row]);
}

// The sources table is handled with these methods below:
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 1; }

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return sizeof(urls) / sizeof(urls[0]) / 2; }

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *cellID = @"cell";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:cellID];
    if (!cell) cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:cellID];
    cell.textLabel.text = [NSString stringWithUTF8String:urls[indexPath.row * 2 + 1]];
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.row == selectedRow) return;
    selectedRow = indexPath.row;
    [self open:indexPath.row * 2];
}

@end
