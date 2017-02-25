#import "ViewController.h"
#import "CoreAudio.h"
#import "Superpowered.h"
#import <QuartzCore/QuartzCore.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <mach/mach_time.h>

@implementation ViewController {
    CADisplayLink *displayLink;
    UILabel *timeDisplay, *cpuLoad;
    UIButton *playPause;
    UISlider *seekSlider;
    UIColor *disabledColor, *enabledColor, *enabledBackgroundColor;
    CoreAudio *coreAudio;
    Superpowered *superpowered;
    bool SuperpoweredEnabled, fxEnabled[NUMFXUNITS], canCompare;
    uint64_t *superpoweredAvgUnits, *superpoweredMaxUnits, *coreaudioAvgUnits, *coreaudioMaxUnits;
    int frame, config;
    double ticksToCPUPercent;
}

@synthesize timeDisplay, cpuLoad, playPause, seekSlider, table;

- (void)dealloc {
    [displayLink invalidate];
    free(superpoweredAvgUnits);
    free(superpoweredMaxUnits);
    free(coreaudioAvgUnits);
    free(coreaudioMaxUnits);
#if !__has_feature(objc_arc)
    [superpowered release];
    [coreAudio release];
    [disabledColor release];
    [enabledColor release];
    [super dealloc];
#endif
}

- (void)viewDidLoad {
    [super viewDidLoad];
    cpuLoad.text = nil;
    
    canCompare = true;
    config = 0;
    for (int n = 0; n < NUMFXUNITS; n++) fxEnabled[n] = false;
    int bytes = sizeof(uint64_t) * pow(2, NUMFXUNITS);
    superpoweredAvgUnits = (uint64_t *)calloc(1, bytes);
    superpoweredMaxUnits = (uint64_t *)calloc(1, bytes);
    coreaudioAvgUnits = (uint64_t *)calloc(1, bytes);
    coreaudioMaxUnits = (uint64_t *)calloc(1, bytes);
    
    disabledColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1];
    enabledColor = [UIColor colorWithRed:0 green:0.4 blue:0.8 alpha:1];
#if !__has_feature(objc_arc)
    [disabledColor retain];
    [enabledColor retain];
#endif
    if ([table respondsToSelector:@selector(setSeparatorInset:)]) [table setSeparatorInset:UIEdgeInsetsZero];
    
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    double ticksToSeconds = 1e-9 * ((double)timebase.numer) / ((double)timebase.denom);
    ticksToCPUPercent = ticksToSeconds * 100.0;
    
    // Let's create two objects here, one to handle Superpowered, and the other to handle Core Audio.
    superpowered = [[Superpowered alloc] init];
    coreAudio = [[CoreAudio alloc] init];
    
    SuperpoweredEnabled = true;
    [superpowered toggle];
    
    // We'll update the screen at 60 Hz.
    frame = 0;
    displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(onDisplayLink)];
    displayLink.frameInterval = 1;
    [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
}

static NSString *fxNames[8] = { @"Time stretching", @"Pitch shifting", @"Roll", @"Filter", @"Equalizer", @"Flanger", @"Delay", @"Reverb" };
static const bool availableInCoreAudio[8] = { true, false, false, true, true, false, true, true };

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return 8;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"cell"];
    if (!cell) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"cell"];
#if !__has_feature(objc_arc)
        [cell autorelease];
#endif
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
        cell.detailTextLabel.textColor = disabledColor;
    };
    cell.textLabel.text = fxNames[indexPath.row];
    if (!SuperpoweredEnabled && !availableInCoreAudio[indexPath.row]) {
        cell.textLabel.textColor = disabledColor;
        cell.detailTextLabel.text = @"Not offered by Core Audio";
    } else {
        cell.textLabel.textColor = fxEnabled[indexPath.row] ? enabledColor : [UIColor blackColor];
        cell.detailTextLabel.text = nil;
    };
    cell.accessoryType = SuperpoweredEnabled && fxEnabled[indexPath.row] ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    return cell;
}

- (void)tableView:(UITableView *)tableView willDisplayCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath {
    cell.backgroundColor = fxEnabled[indexPath.row] && (SuperpoweredEnabled || availableInCoreAudio[indexPath.row]) ? disabledColor : [UIColor whiteColor];
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    if (!SuperpoweredEnabled && !availableInCoreAudio[indexPath.row]) return nil;
    
    fxEnabled[indexPath.row] = !fxEnabled[indexPath.row];
    int identifier = 1 << ((int)indexPath.row);
    if (!fxEnabled[indexPath.row]) config &= ~identifier; else config |= identifier;
    
    canCompare = true;
    for (int n = 0; n < NUMFXUNITS; n++) if (fxEnabled[n] && !availableInCoreAudio[n]) {
        canCompare = false;
        break;
    };
    
    [coreAudio toggleFx:(int)indexPath.row];
    [superpowered toggleFx:(int)indexPath.row];
    
    [table reloadData];
    return nil;
}

- (void)onDisplayLink { // Called continously on each screen frame.
    if (frame < 60) frame++;
    else {
        frame = 0;
        
        if (SuperpoweredEnabled) {
            if (superpowered->playing) {
                superpoweredAvgUnits[config] = superpowered->avgUnitsPerSecond;
                superpoweredMaxUnits[config] = superpowered->maxUnitsPerSecond;
            };
        } else {
            if (coreAudio->playing) {
                coreaudioAvgUnits[config] = coreAudio->avgUnitsPerSecond;
                coreaudioMaxUnits[config] = coreAudio->maxUnitsPerSecond;
            };
        };
        
        if (!canCompare) cpuLoad.text = @"Some selected features are not offered by Core Audio.";
        else {
            if (coreaudioAvgUnits[config] == 0) cpuLoad.text = @"Tap on Core Audio to compare.";
            else if (superpoweredAvgUnits[config] == 0) cpuLoad.text = @"Tap on Superpowered to compare.";
            else {
                NSString *str = nil;
                if (superpoweredAvgUnits[config] < coreaudioAvgUnits[config]) {
                    str = [[NSString alloc] initWithFormat:
                                     @"Superpowered processes %.1fx\nfaster than Core Audio:\n\n%.1f%% (avg), %.1f%% (peak)\nless CPU.",
                                     ((double)coreaudioAvgUnits[config]) / ((double)superpoweredAvgUnits[config]),
                                     (1.0 - (((double)superpoweredAvgUnits[config]) / ((double)coreaudioAvgUnits[config]))) * 100.0,
                                     (1.0 - (((double)superpoweredMaxUnits[config]) / ((double)coreaudioMaxUnits[config]))) * 100.0
                                     ];
                } else {
                    str = [[NSString alloc] initWithFormat:
                                     @"Core Audio processes %.1fx\nfaster than Superpowered:\n\n%.1f%% (avg), %.1f%% (peak)\nless CPU.",
                                     ((double)superpoweredAvgUnits[config]) / ((double)coreaudioAvgUnits[config]),
                                     (1.0 - (((double)coreaudioAvgUnits[config]) / ((double)superpoweredAvgUnits[config]))) * 100.0,
                                     (1.0 - (((double)coreaudioMaxUnits[config]) / ((double)superpoweredMaxUnits[config]))) * 100.0
                                     ];
                };
                cpuLoad.text = str;
#if !__has_feature(objc_arc)
                [str release];
#endif
            };
        };
    };
    
    if (SuperpoweredEnabled) [superpowered updatePlayerLabel:timeDisplay slider:seekSlider button:playPause];
    else [coreAudio updatePlayerLabel:timeDisplay slider:seekSlider button:playPause];
}

- (IBAction)onPlayPause:(id)sender {
    [coreAudio togglePlayback];
    [superpowered togglePlayback];
}

- (IBAction)onSeek:(id)sender {
    [coreAudio seekTo:seekSlider.value];
    [superpowered seekTo:seekSlider.value];
}

- (IBAction)onSystem:(id)sender {
    SuperpoweredEnabled = !SuperpoweredEnabled;
    [superpowered toggle];
    [coreAudio toggle];
    if (SuperpoweredEnabled) [superpowered seekTo:seekSlider.value]; else [coreAudio seekTo:seekSlider.value];
    [table reloadData];
    cpuLoad.text = @"Please wait...";
    frame = 0;
}

@end
