#import <UIKit/UIKit.h>
#import <MediaPlayer/MediaPlayer.h>

@interface ViewController: UIViewController<MPMediaPickerControllerDelegate>
    @property (nonatomic, retain) IBOutlet UIProgressView *progressView;
@end
