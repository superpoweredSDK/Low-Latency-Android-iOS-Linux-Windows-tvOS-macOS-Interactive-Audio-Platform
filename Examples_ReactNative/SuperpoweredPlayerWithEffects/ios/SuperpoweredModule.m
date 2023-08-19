//
//  Superpowered.m
//  SuperpoweredPlayerWithEffects
//
//  Created by Banto Balazs on 2023. 08. 19..
//

#import <React/RCTBridgeModule.h>
#import <Foundation/Foundation.h>
#import "UIKit/UIKit.h"
@interface RCT_EXTERN_MODULE(SuperpoweredModule, NSObject)
RCT_EXTERN_METHOD(initSuperpowered)
RCT_EXTERN_METHOD(togglePlayback)
RCT_EXTERN_METHOD(enableFlanger:(BOOL)enable)
@end
