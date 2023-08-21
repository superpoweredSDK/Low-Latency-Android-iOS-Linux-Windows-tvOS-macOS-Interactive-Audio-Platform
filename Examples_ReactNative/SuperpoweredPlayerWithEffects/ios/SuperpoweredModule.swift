//
//  Superpowered.swift
//  SuperpoweredPlayerWithEffects
//
//  Created by Banto Balazs on 2023. 08. 18..
//

import Foundation
@objc(SuperpoweredModule)
class SuperpoweredModule : NSObject {
  let superpowered = SuperpoweredAudio()
  
  @objc static func requiresMainQueueSetup() -> Bool {
    return false
  }
  
  @objc func initSuperpowered() -> Void {
    superpowered.initSuperpowered()
  }
  
  @objc func togglePlayback() -> Void {
    superpowered.togglePlayback()
  }
  
  @objc func enableFlanger(_ enable: Bool) -> Void {
    superpowered.enableFlanger(enable)
  }
}
