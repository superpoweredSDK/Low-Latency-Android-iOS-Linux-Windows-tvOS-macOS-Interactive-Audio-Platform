Superpowered Audio Engine for Games, Music and Interactive Audio Apps on Android, iOS and OSX.

Low Latency Audio. Cross Platform. Free.


# What is it?

The Superpowered Audio SDK is a software development kit based on Superpowered Inc�s digital signal processing (DSP) technology.

Superpowered technology allows developers to build computationally intensive audio apps and embedded applications that process more quickly and use less power than other comparable solutions.

Superpowered DSP is designed and optimized, from scratch, to run on low-power mobile processors. Specifically, any device running ARM with the NEON extension (which covers 99% of all mobile devices manufactured). Intel CPU is supported too.

PLEASE NOTE: all example codes working with the Superpowered features are cross-platform, can be used under iOS and Android.

# Folders

- /Superpowered
The SDK itself (static libraries and headers).

- /docs
The documentation. Start with index.html.

- /SuperpoweredCrossExample (iOS)
A fully-functional DJ app project example. Shows how to:

	* Set up two players.
	* Sync them together.
	* Apply some effects on the master mix.
	* Use Objective-C++.

- /SuperpoweredFrequencies (iOS)
Simple 8-band frequency analyzer. Shows how to:

	* Mix Swift and Objective-C++ in a project.
	* Use the SuperpoweredBandpassFilterbank.

- /SuperpoweredFrequencyDomain (iOS)
Simple time domain to frequency domain transformation with buffering and windowing. Shows how to:

	* Use the SuperpoweredFrequencyDomain class.
	* Process the magnitudes and phases of the audio input.

- /SuperpoweredHLSExample (iOS)
HTTP Live Streaming example project.

- /SuperpoweredOfflineProcessingExample (iOS)
Advanced example. Decodes an audio file, applies an effect or time stretching and saves the result in WAV. Shows how to:

	* Set up the SuperpoweredDecoder.
	* Apply a simple effect.
	* Use the time stretcher with an efficient dynamic memory pool.
	* Save the result in WAV.
	* Directly read from the iPod music library.- /SuperpoweredPerformance (iOS)
It compares several Superpowered features to Core Audio.

	* Shows the differences between Superpowered and Core Audio.
	* Syncs effects to the player�s bpm.
	* Shows how to use Objective-C++ in an Objective-C project.

	Swift note:
	We have also tried creating this project in Swift, but it's not complete for audio and several features were impossible to implement (such as proper performance measurement). Swift is not designed for real-time audio. Fortunately, Objective-C++ files work great in Swift projects.- /SuperpoweredSpatializer/SuperpoweredUnity (Unity)	* A Unity example project with one scene having a spatialized sound and a wall.	* Check SuperpoweredSpatializer.cs on how occlusion was implemented.- /SuperpoweredSpatializer/src (Unity, Android, OSX)	* The source code for the Superpowered Spatializer and Global Reverb plugins for Unity.	* Shows how to use Superpowered in the Unity Native Audio Plugin SDK.- /SuperpoweredSpatializer/Android/jni (Unity, Android)Builds the spatializer plugins for Unity->Android.- /SuperpoweredSpatializer/Xcode (Unity, OSX)Builds the spatializer plugins for Unity->OSX.- /SuperpoweredSpatializer/ambi (OSX)Simple ambisonics implementation based on the SuperpoweredSpatializer.
- /Android/CrossExample (Android)
	* Android NDK sample project, similar to SuperpoweredCrossExample.
	* Also shows how to use SuperpoweredAndroidAudioIO for output only.

- /Android/FrequencyDomain (Android)
	* Android NDK sample project, similar to SuperpoweredFrequencyDomain.
	* Shows how to use SuperpoweredAndroidAudioIO for audio input and output.

- /Android/HLSExample (Android)
HTTP Live Streaming example project.

- /Android/SuperpoweredUSBExample (Android)
This project comes with two example apps, a simple and a complex example app.

Simple Example App:

The simple example app demonstrates an easy audio setup, similar to how iOS handles USB audio devices. It also receives and displays MIDI. The audio functionality of the simple example app will cover most mobile audio use cases.

Complex Example App:

The complex example app demonstrates full USB audio discovery, for the use case of sophisticated DAW and recording apps. It displays the various input and output options, audio paths and hardware controls of a USB audio device. Hardware controls can be manipulated and advanced thru audio paths can be enabled.

If an audio output is selected without an audio input, the complex example sends a sine wave to every output channel. If both audio output and audio input are selected, then loopback or round-trip latency measurement are available.

This means that the complex example app is also a testing tool to discover a USB audio device’s advanced features, test audio quality, glitches and latency.

Remarks:

Superpowered USB Audio classes for Android are compatible from Android 19 (KitKat 4.4) onwards. That’s the first Android version where proper scheduling priorities were implemented for low latency audio. This covers more than 75% active Android devices today.


# Android Studio

Before running the example, please set up the paths in the local.properties file.


# How to create a Superpowered project with Android Studio

Prerequisites: latest Android SDK, Android NDK, Android Studio installed. Steps:

1. Create a new project in Android Studio.
2. Create the jni folder inside the project's folder: app/src/main/jni
3. Copy the contents of the following files from one of the example projects: gradle/wrapper/gradle-wrapper.properties, local.properties, build.gradle, app/build.gradle
4. Open build.gradle (Module: app), and change the applicationId (and moduleName if you like).


# The Latest Version

Details of the latest version can be found at http://superpowered.com/superpowered-audio-sdk/# Pricing and licensing

The Superpowered Audio SDK is free for software applications, except HLS playback. Please see the included license pdf.

Superpowered FFT benefits from ideas in Construction of a High-Performance FFT by Eric Postpischil (http://edp.org/resume.htm).

The Superpowered MP3 and AAC decoder benefits from optimizations by Ken Cooke.


# Contacts

If you want to be informed about new code releases, bug fixes, general news and information about Superpowered, please email hello@superpowered.com.
