Superpowered Audio Engine for Games, Music and Interactive Audio Apps on Android, iOS & OSX.

Low Latency Audio. Cross Platform. Free.
What is it?-----------The Superpowered Audio SDK is a software development kit based on Superpowered Inc’s digital signal processing (DSP) technology. Superpowered technology allows developers to build computationally intensive audio apps and embedded applications that process more quickly and use less power than other comparable solutions. Superpowered DSP is designed and optimized, from scratch, to run on low-power mobile processors. Specifically, any device running ARM with the NEON extension (which covers 99% of all mobile devices manufactured as of Spring 2014). Intel CPU is supported too.PLEASE NOTE: all example codes working with the Superpowered features are cross-platform, can be used under iOS and Android.
Folders-------
/SuperpoweredThe SDK itself (static library and headers).

docs
The documentation. Start with index.html.


/SuperpoweredCrossExample (iOS)A fully-functional DJ app project example. Shows how to:

- Set up two players.
- Sync them together.
- Apply some effects on the master mix.
- Use Objective-C++.


/SuperpoweredFrequencies (iOS)
Simple 8-band frequency analyzer. Show how to:

- Mix Swift and Objective-C++ in a project.
- Use the SuperpoweredBandpassFilterbank./SuperpoweredPerformance (iOS)It compares several Superpowered features to Core Audio.

- Shows the differences between Superpowered and Core Audio.
- Syncs effects to the player’s bpm.
- Shows how to use Objective-C++ in an Objective-C project.

Swift note:
We have also tried creating this project in Swift, but it’s not complete for audio and several features were impossible to implement (such as proper performance measurement). Swift is not designed for real-time audio. Fortunately, Objective-C++ files work great in Swift projects.

/SuperpoweredOfflineProcessingExample
Advanced example. Decodes an audio file, applies time stretching and saves the result in WAV. Shows how to:

- Set up the SuperpoweredDecoder.
- Use the time stretcher with an efficient dynamic memory pool.
- Save the result in WAV.

/Android/SuperpoweredExample (Android)
The Android NDK sample project, similar to SuperpoweredCrossExample.


Android Studio
--------------

NDK integration with static libraries is still incomplete in Android Studio. Before running the example, please set up the following:

- Open local.properties. Set ndk.dir to your ndk folder.
- Open build.gradle for the app module. Check for ndk-build, and set the appropriate path.- Before building the project, uncomment the jni.srcDirs line. This makes the C folder disappear from the view, but don’t worry. After a successful build, comment the line again.The Latest Version 
------------------Details of the latest version can be found at http://superpowered.com/superpowered-audio-sdk/Pricing and licensing
---------
The Superpowered Audio SDK is free for software applications. Please see the file called license.pdf.


Contacts
--------

If you want to be informed about new code releases, bug fixes, general news and information about Superpowered, please email hello@superpowered.com.

