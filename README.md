Superpowered Audio Engine for Games, Virtual Reality, Music and Interactive Audio Apps.

Cross Platform on Android, iOS, Mac OSX, tvOS and Linux. Real-time, low-Latency. Free.

![Superpowered!](http://superpowered.com/images/superpowered-github.gif)


# What is it?

Resource-hungry audio transformations and algorithms which can run unconstrained on high-power desktop CPUs do not function well on mobile devices, taking up disproportionate amounts of CPU processing time with unacceptable latencies that sap battery life. The resulting poor audio quality and high power consumption make for bad user experiences. To maintain developer sanity and a high quality user experience, rather than shoehorning desktop audio libraries onto mobile devices, audio technologies must be designed from scratch for the constrained mobile chipset environment, one that uses fewer CPU cycles, and significantly less power.

Superpowered is a set of audio infrastructure technologies that focuses on mobile audio. This means low-power, real-time latency and cross-platform.


# Supported Platforms

Superpowered is cross-platform: ALL SUPERPOWERED C++ CODE CAN BE COPY-PASTED BETWEEN ANDROID, iOS, MAC OSX, TVOS AND LINUX.


# Superpowered Technologies

Superpowered Inc develops the following mobile audio infrastructure:


### 1. Superpowered Audio SDK for Android, iOS, Mac OSX, tvOS and Linux.

A fully-featured audio engine with players, decoders, audio filters, effects and lower-level functions like FFT or mixing.

For the most recent and up-to-date feature list, see: http://superpowered.com/#comptable


### 2. Superpowered HLS (HTTP Live Streaming) audio for Android, iOS, Mac OSX, tvOS and Linux.

- VOD, live or event streams.
- AAC-LC or MP3 audio encoding.
- ADTS AAC, MP3 or MPEG-TS containers.
- Supports byte range requests and AES-128 encryption.
- Bandwidth measurement and selectable automatic stream switching.
- Selectable download strategies.

Background information: http://superpowered.com/http-live-streaming-for-android-superpowered-android-audio-io-and-audio-resampler


### 3. Superpowered USB Audio and USB MIDI SDK for Android

The Superpowered USB Audio and MIDI SDK for Android takes over MIDI device handling, providing low latency and low jitter (below 2 ms) access to MIDI devices for 1.2 billion Android devices, that is, 80+% of all Android devices on Google Play.
It’s the Android equivalent of iOS Core Audio and Core MIDI for USB devices.

For additional info, please see: http://superpowered.com/android-usb-audio-android-midi


### 4. Superpowered Media Server for Android

Superpowered has developed a system-space solution for Android&#39;s 10 ms Problem.

Interested parties should read:

- http://superpowered.com/android-audio-low-latency-primer
- http://superpowered.com/superpowered-android-media-server


## PLEASE NOTE 1:

Only the Superpowered Audio SDK is available for free use (when credit is given per license below).

The following audio technologies are not part of the Superpowered Audio SDK and must be licensed for a fee:

- Superpowered HLS (HTTP Live Streaming) audio for Android, iOS, Mac OSX, tvOS and Linux
- Superpowered USB Audio and USB MIDI SDK for Android
- Superpowered Media Server for Android


### NOTE 2
Superpowered HLS is integrated into the Superpowered Audio SDK SuperpoweredAdvancedAudioPlayer class for easy evaluation and deployment. However, outside of evaluation, Superpowered HLS must be licensed for a fee.


### NOTE 3
Superpowered USB Audio and USB MIDI SDK for Android is included as a download for evaluation but consists of separate classes, and also must be licensed for a fee.


### NOTE 4
To evaluate Superpowered Media Server for Android, please contact mediaserver@superpowered.com.


### NOTE 5

Linux builds are beta. Thank you for testing!


# Folders

- /Docs

	The documentation. Start with index.html.

- /Superpowered

	The Audio SDK itself (static libraries and headers).

- /License

	Contains the license document.

- /Examples_Android

	Example projects for Android.

- /Examples_iOS

	Example projects for iOS.

- /Examples_Linux

	Example projects for Linux.

- /Examples_tvOS

	Example projects for tvOS.

- /SuperpoweredSpatializer

	* Unity example project.
	* Unity Native Audio Plugin example project for iOS, Android and OSX.
	* Ambisonics (b-format) implementation based on the SuperpoweredSpatializer.


# Example Projects

#### /Examples_iOS/SuperpoweredCrossExample, /Examples_Android/CrossExample
A fully-functional DJ app project example. Shows how to:

- Set up audio I/O.
- Set up two players.
- Sync them together.
- Apply some effects on the master mix.
- Use Objective-C++.

#### /Examples_iOS/SuperpoweredFrequencyDomain, /Examples_Android/FrequencyDomain
Simple time domain to frequency domain transformation with buffering and windowing. Shows how to:

- Set up audio I/O.
- Use the SuperpoweredFrequencyDomain class.
- Process the magnitudes and phases of the audio input.

#### /Examples_iOS/uperpoweredHLSExample, /Examples_Android/HLSExample, /Examples_Linux/src/hls.cpp
HTTP Live Streaming example project.

#### /Examples_iOS/SuperpoweredOfflineProcessingExample, /Examples_Linux/src/offlineX.cpp
Advanced example. Decodes an audio file, applies an effect or time stretching and saves the result in WAV. Shows how to:

- Set up the SuperpoweredDecoder.
- Apply a simple effect.
- Use the time stretcher with an efficient dynamic memory pool.
- Save the result in WAV.
- Directly read from the iPod music library.
- Use the offline analyzer to detect bpm and other information.

#### /Examples_iOS/SuperpoweredFrequencies
Simple 8-band frequency analyzer. Shows how to:

- Mix Swift and Objective-C++ in a project.
- Use the SuperpoweredBandpassFilterbank.

#### /SuperpoweredPerformance (iOS)
It compares several Superpowered features to Core Audio.

- Shows the differences between Superpowered and Core Audio.
- Syncs effects to the player&#39;s bpm.
- Shows how to use Objective-C++ in an Objective-C project.

	Swift note:
	We have also tried creating this project in Swift, but it&#39;s not complete for audio and several features were impossible to implement (such as proper performance measurement). Swift is not designed for real-time audio. Fortunately, Objective-C++ files work great in Swift projects.

#### /SuperpoweredSpatializer/SuperpoweredUnity (Unity)

- A Unity example project with one scene having a spatialized sound and a wall.
- Check SuperpoweredSpatializer.cs on how occlusion was implemented.

#### /SuperpoweredSpatializer/src (Unity, Android, OSX)

- The source code for the Superpowered Spatializer and Global Reverb plugins for Unity.
- Shows how to use Superpowered in the Unity Native Audio Plugin SDK.

#### /SuperpoweredSpatializer/Android/jni (Unity, Android)
Builds the spatializer plugins for Unity->Android.

#### /SuperpoweredSpatializer/Xcode (Unity, OSX)
Builds the spatializer plugins for Unity->OSX.

#### /SuperpoweredSpatializer/ambi (OSX)
Simple ambisonics implementation based on the SuperpoweredSpatializer.

#### /Examples_Android/SuperpoweredUSBExample
This project comes with two example apps, a simple and a complex example app.

##### Simple USB Example App:

The simple example app demonstrates an easy audio setup, similar to how iOS handles USB audio devices. It also receives and displays MIDI. The audio functionality of the simple example app will cover most mobile audio use cases.

##### Complex USB Example App:

The complex example app demonstrates full USB audio discovery, for the use case of sophisticated DAW and recording apps. It displays the various input and output options, audio paths and hardware controls of a USB audio device. Hardware controls can be manipulated and advanced thru audio paths can be enabled.

If an audio output is selected without an audio input, the complex example sends a sine wave to every output channel. If both audio output and audio input are selected, then loopback or round-trip latency measurement are available.

This means that the complex example app is also a testing tool to discover a USB audio device’s advanced features, test audio quality, glitches and latency.

###### Remarks:

Superpowered USB Audio classes for Android are compatible from Android 19 (KitKat 4.4) onwards. That’s the first Android version where proper scheduling priorities were implemented for low latency audio. This covers more than 75% active Android devices today.


# Android Studio

Before running any Android example project, please set up the paths in the local.properties file.


# How to create a Superpowered project with Android Studio

Prerequisites: latest Android SDK, Android NDK, Android Studio installed. Steps:

1. Create a new project in Android Studio.
2. Create the jni folder inside the project&#39;s folder: app/src/main/jni
3. Copy the contents of the following files from one of the example projects: gradle/wrapper/gradle-wrapper.properties, local.properties, build.gradle, app/build.gradle, app/src/main/jni/CMakeLists.txt
4. Open build.gradle (Module: app), and change the applicationId


# The Latest Version

Details of the latest version can be found at http://superpowered.com/superpowered-audio-sdk/


# Pricing and licensing

The Superpowered Audio SDK is free for software applications, except HLS playback. Please see the included license pdf.

Superpowered FFT benefits from ideas in Construction of a High-Performance FFT by Eric Postpischil (http://edp.org/resume.htm).

The Superpowered MP3 and AAC decoder benefits from optimizations by Ken Cooke.


# Contacts

If you want to be informed about new code releases, bug fixes, general news and information about Superpowered, please email hello@superpowered.com.
