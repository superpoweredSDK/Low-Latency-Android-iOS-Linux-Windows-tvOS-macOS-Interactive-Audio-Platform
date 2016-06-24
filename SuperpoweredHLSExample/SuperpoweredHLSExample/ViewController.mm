#import "ViewController.h"
#import "SuperpoweredIOSAudioIO.h"
#include "SuperpoweredAdvancedAudioPlayer.h"
#include "SuperpoweredSimple.h"

// some HLS stream url-title pairs
static const char *urls[8] = {
    "https://devimages.apple.com.edgekey.net/streaming/examples/bipbop_16x9/bipbop_16x9_variant.m3u8", "Apple Advanced Example Stream",
    "http://vevoplaylist-live.hls.adaptive.level3.net/vevo/ch1/appleman.m3u8", "Vevo LIVE Channel 1",
    "http://playertest.longtailvideo.com/adaptive/bbbfull/bbbfull.m3u8", "JW Player Test",
    "http://playertest.longtailvideo.com/adaptive/oceans_aes/oceans_aes.m3u8", "JW AES Encrypted",
};

@implementation ViewController {
    UIView *bufferIndicator;
    CADisplayLink *displayLink;
    SuperpoweredIOSAudioIO *audioIO;
    SuperpoweredAdvancedAudioPlayer *player;
    float *interleavedBuffer;
    CGFloat sliderThumbWidth;
    unsigned int lastPositionSeconds, samplerate;
    NSInteger selectedRow;
}

@synthesize seekSlider, currentTime, duration, playPause, sources;

- (void)interruptionStarted {}
- (void)interruptionEnded {}
- (void)recordPermissionRefused {}
- (void)mapChannels:(multiOutputChannelMap *)outputMap inputMap:(multiInputChannelMap *)inputMap externalAudioDeviceName:(NSString *)externalAudioDeviceName outputsAndInputs:(NSString *)outputsAndInputs {}

- (void)updateDuration {
    if (player->durationMs == UINT_MAX) {
        duration.text = @"LIVE";
        seekSlider.hidden = YES;
    } else {
        duration.text = [NSString stringWithFormat:@"%02d:%02d", player->durationSeconds / 60, player->durationSeconds % 60];
        seekSlider.hidden = NO;
    };
    currentTime.hidden = playPause.hidden = NO;
}

static void playerEventCallback(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event, void *value) {
    ViewController *self = (__bridge ViewController *)clientData;
    switch (event) {
        case SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess:
            self->player->play(false);
            [self performSelectorOnMainThread:@selector(updateDuration) withObject:nil waitUntilDone:NO];
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_LoadError: NSLog(@"Open error: %s", (char *)value); break;
        case SuperpoweredAdvancedAudioPlayerEvent_EOF: self->player->seek(0); break;
        case SuperpoweredAdvancedAudioPlayerEvent_DurationChanged: [self performSelectorOnMainThread:@selector(updateDuration) withObject:nil waitUntilDone:NO]; break;
        default:;
    };
}

// Called periodically by the operating system's audio stack to provide audio output.
static bool audioProcessing(void *clientdata, float **buffers, unsigned int inputChannels, unsigned int outputChannels, unsigned int numberOfSamples, unsigned int samplerate, uint64_t hostTime) {
    __unsafe_unretained ViewController *self = (__bridge ViewController *)clientdata;
    if (self->samplerate != samplerate) {
        self->samplerate = samplerate;
        self->player->setSamplerate(self->samplerate);
    };
    bool hasAudio = self->player->process(self->interleavedBuffer, false, numberOfSamples);
    if (hasAudio) SuperpoweredDeInterleave(self->interleavedBuffer, buffers[0], buffers[1], numberOfSamples);
    return hasAudio;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    lastPositionSeconds = 0;
    selectedRow = 0;
    samplerate = 44100;
    sliderThumbWidth = [seekSlider thumbRectForBounds:seekSlider.bounds trackRect:[seekSlider trackRectForBounds:seekSlider.bounds] value:0].size.width;

    bufferIndicator = [[UIView alloc] initWithFrame:CGRectZero];
    bufferIndicator.backgroundColor = [UIColor lightGrayColor];
    [self.view insertSubview:bufferIndicator belowSubview:seekSlider];

    displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(onDisplayLink)];
    displayLink.frameInterval = 1;
    [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];

    SuperpoweredAdvancedAudioPlayer::setTempFolder([NSTemporaryDirectory() fileSystemRepresentation]);
    player = new SuperpoweredAdvancedAudioPlayer((__bridge void *)self, playerEventCallback, samplerate, 0);
    interleavedBuffer = (float *)malloc(8192);

    audioIO = [[SuperpoweredIOSAudioIO alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:12 preferredMinimumSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryPlayback channels:2 audioProcessingCallback:audioProcessing clientdata:(__bridge void *)self];
    [audioIO start];

    [sources selectRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0] animated:NO scrollPosition:UITableViewScrollPositionTop];
    [self open:0];
}

- (void)open:(NSInteger)row {
    currentTime.hidden = playPause.hidden = seekSlider.hidden = YES;
    duration.text = @"Loading...";
    player->open(urls[row]);
}

- (void)dealloc {
    audioIO = nil;
    [displayLink invalidate];
    bufferIndicator = nil;
    delete player;
    delete interleavedBuffer;
    SuperpoweredAdvancedAudioPlayer::clearTempFolder();
}

// Called periodically on every screen refresh, 60 fps.
- (void)onDisplayLink {
    // Update the buffer indicator.
    float start = player->bufferStartPercent;
    float end = player->bufferEndPercent;
    CGRect frame = seekSlider.frame;
    frame.size.width -= sliderThumbWidth;
    frame.origin.x += (start * frame.size.width) + (sliderThumbWidth * 0.5f);
    frame.size.width = (end - start) * frame.size.width;
    bufferIndicator.frame = frame;

    // Update the time display.
    unsigned int positionSeconds;
    if (seekSlider.tracking) positionSeconds = seekSlider.value * (float)player->durationSeconds;
    else {
        positionSeconds = player->positionSeconds;
        seekSlider.value = player->positionPercent;
    };
    if (lastPositionSeconds != positionSeconds) {
        lastPositionSeconds = positionSeconds;
        currentTime.text = [NSString stringWithFormat:@"%02d:%02d", positionSeconds / 60, positionSeconds % 60];
    };

    // Update the play/pause button.
    if (playPause.highlighted != player->playing) playPause.highlighted = player->playing;
}

- (IBAction)onSeekSlider:(id)sender {
    player->seek(((UISlider *)sender).value);
}

- (IBAction)onDownloadStrategy:(id)sender {
    switch (((UISegmentedControl *)sender).selectedSegmentIndex) {
        case 1: player->downloadSecondsAhead = 20; break; // Will not buffer more than 20 seconds ahead of the playback position.
        case 2: player->downloadSecondsAhead = 40; break; // Will not buffer more than 40 seconds ahead of the playback position.
        case 3: player->downloadSecondsAhead = HLS_DOWNLOAD_EVERYTHING; break; // Will buffer everything after and before the playback position.
        default: player->downloadSecondsAhead = HLS_DOWNLOAD_REMAINING; // Will buffer everything after the playback position.
    };
}

- (IBAction)onPlayPause:(id)sender {
    player->togglePlayback();
}

- (IBAction)onSpeed:(id)sender {
    player->setTempo(((UISwitch *)sender).on ? 2.0f : 1.0f, true);
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
