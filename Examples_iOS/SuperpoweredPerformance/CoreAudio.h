// This object handles Apple's Core Audio.
// Check the source and you'll see how many lines of code you'll have to write. Lots.

@interface CoreAudio: NSObject {
@public
    bool playing;
    uint64_t avgUnitsPerSecond, maxUnitsPerSecond;
}

// Updates the user interface according to the file player's state.
- (void)updatePlayerLabel:(UILabel *)label slider:(UISlider *)slider button:(UIButton *)button;

- (void)togglePlayback; // Play/pause.
- (void)seekTo:(float)percent; // Jump to a specific position.

- (void)toggle; // Start/stop Core Audio.
- (bool)toggleFx:(int)index; // Enable/disable fx.

@end
