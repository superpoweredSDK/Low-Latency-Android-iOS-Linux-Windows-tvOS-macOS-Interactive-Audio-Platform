//
//  SuperpoweredAudio.h
//  SuperpoweredPlayerWithEffects
//
//  Created by Banto Balazs on 2023. 08. 18..
//

#ifndef SuperpoweredAudio_h
#define SuperpoweredAudio_h

@interface SuperpoweredAudio: NSObject {
}

- (void)initSuperpowered;
- (void)togglePlayback; 
- (void)enableFlanger:(bool)enable;

@end


#endif /* SuperpoweredAudio_h */
