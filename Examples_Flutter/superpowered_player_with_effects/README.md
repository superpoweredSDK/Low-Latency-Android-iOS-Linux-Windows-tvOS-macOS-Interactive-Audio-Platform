# Superpowered Player with Effects Flutter Sample App

A [FFI plugin](https://docs.flutter.dev/platform-integration/android/c-interop) project integrating SuperpoweredSDK.
## How to run:
`cd scripts` 
* Android: build_android.sh 
* iOS: build_ios.sh 

## Project structure

This template uses the following structure:

* `src`: Contains the native source code shared by all platforms
* `android/cpp` : Android specific native code
* `ios/Classes` : iOS specific native code

* `lib`: Contains the Dart code that defines the API of the plugin, and which
  calls into the native code using `dart:ffi`.

* platform folders (`android`, `ios`, etc.): Contains the build files
  for building and bundling the native code library with the platform application.

## Building and bundling native code

The native build systems that are invoked by FFI (and method channel) plugins are:

* For Android: Gradle, which invokes the Android NDK for native builds.
  * See the documentation in android/build.gradle.
* For iOS and MacOS: Xcode, via CocoaPods.
  * See the documentation in ios/superpowered_player_with_effects.podspec.
  * See the documentation in macos/superpowered_player_with_effects.podspec.

## Binding to native code

To use the native code, bindings in Dart are needed.
To avoid writing these by hand, they are generated from the header file
(`src/superpowered_player_with_effects.h`) by `package:ffigen`.
Regenerate the bindings by running `flutter pub run ffigen --config ffigen.yaml`.


