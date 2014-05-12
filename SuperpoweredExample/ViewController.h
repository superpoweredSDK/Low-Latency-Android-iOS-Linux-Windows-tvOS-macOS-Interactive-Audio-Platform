#import <UIKit/UIKit.h>

@interface ViewController: UIViewController<UITableViewDataSource, UITableViewDelegate>

@property (nonatomic, retain) IBOutlet UILabel *timeDisplay;
@property (nonatomic, retain) IBOutlet UILabel *cpuLoad;
@property (nonatomic, retain) IBOutlet UIButton *playPause;
@property (nonatomic, retain) IBOutlet UISlider *seekSlider;
@property (nonatomic, retain) IBOutlet UITableView *table;

- (IBAction)onSystem:(id)sender;
- (IBAction)onPlayPause:(id)sender;
- (IBAction)onSeek:(id)sender;

@end
