//
//  RCTNativeSuperpoweredEngine.mm
//  SuperpoweredPlayerWithEffects
//
//  Created by Banto Balazs on 2023. 08. 19..
//

#import "RCTNativeSuperpoweredEngine.h"

#import "SuperpoweredAudio.h"

@interface RCTNativeSuperpoweredEngine()
@property (strong, nonatomic) SuperpoweredAudio *superpowered;
@end

@implementation RCTNativeSuperpoweredEngine

- (id)init {
  if (self = [super init]) {
    _superpowered = [[SuperpoweredAudio alloc] init];
  }
  return self;
}

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:(const facebook::react::ObjCTurboModule::InitParams &)params {
  return std::make_shared<facebook::react::NativeSuperpoweredEngineSpecJSI>(params);
}

- (void)initSuperpowered {
  [_superpowered initSuperpowered];
}

- (void)togglePlayback {
  [_superpowered togglePlayback];
}

- (void)enableFlanger:(BOOL)enable {
  [_superpowered enableFlanger:enable];
}

+ (NSString *)moduleName
{
  return @"NativeSuperpoweredEngine";
}

@end
