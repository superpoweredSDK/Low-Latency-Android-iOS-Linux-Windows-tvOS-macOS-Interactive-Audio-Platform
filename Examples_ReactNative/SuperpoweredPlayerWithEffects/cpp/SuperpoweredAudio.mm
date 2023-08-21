//
//  Superpowered.m
//  SuperpoweredPlayerWithEffects
//
//  Created by Banto Balazs on 2023. 08. 18..
//

#import <Foundation/Foundation.h>
#import "SuperpoweredAudio.h"
#import "SuperpoweredEngineExample.h"
#import "Superpowered.h"
#include <string>

@implementation SuperpoweredAudio {
  SuperpoweredEngineExample *example;
}

- (void)dealloc {
  delete example;
}

- (void)initSuperpowered { // Play/pause.
  Superpowered::Initialize("ExampleLicenseKey-WillExpire-OnNextUpdate");
  std::string tempDir = std::string([NSTemporaryDirectory() UTF8String]);
  Superpowered::AdvancedAudioPlayer::setTempFolder(tempDir.c_str());
  example = new SuperpoweredEngineExample();
  example->play();
}

- (void)togglePlayback { // Play/pause.
  example->togglePlayback();
}

- (void)enableFlanger:(bool)enable {
  example->enableFlanger(enable);
}
@end
