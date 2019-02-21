#import <UIKit/UIKit.h>

@interface ViewController: UIViewController<UITableViewDataSource, UITableViewDelegate>

@property (nonatomic, retain) IBOutlet UISlider *seekSlider;
@property (nonatomic, retain) IBOutlet UILabel *currentTime;
@property (nonatomic, retain) IBOutlet UILabel *duration;
@property (nonatomic, retain) IBOutlet UIButton *playPause;
@property (nonatomic, retain) IBOutlet UITableView *sources;

- (IBAction)onSeekSlider:(id)sender;
- (IBAction)onDownloadStrategy:(id)sender;
- (IBAction)onPlayPause:(id)sender;
- (IBAction)onSpeed:(id)sender;

@end
