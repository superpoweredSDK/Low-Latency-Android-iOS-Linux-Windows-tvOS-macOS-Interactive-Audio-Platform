//
//  ContentView.swift
//  SuperpoweredVisionOSExample
//
//  Created by Balazs Kiss on 06/10/2023.
//

import SwiftUI
import RealityKit

struct ContentView: View {
    var body: some View {
        VStack {
            Model3D(named: "Scene")
                .padding(.bottom, 50)

            Text("Hello, world!")
        }
        .padding()
    }
}

#Preview(windowStyle: .automatic) {
    ContentView()
}
