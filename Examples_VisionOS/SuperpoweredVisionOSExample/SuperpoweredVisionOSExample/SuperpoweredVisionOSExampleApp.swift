//
//  SuperpoweredVisionOSExampleApp.swift
//  SuperpoweredVisionOSExample
//
//  Created by Balazs Kiss on 06/10/2023.
//

import SwiftUI

@main
struct SuperpoweredVisionOSExampleApp: App {

    let audio = SuperpoweredAudioEngine()

    init() {
        audio.start()
    }

    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}
