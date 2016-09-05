rm -r ../obj
rm -r ../libs
/android/ndk/ndk-build
cp ../libs/arm64-v8a/libAudioPluginSuperpoweredSpatializer.so ../../SuperpoweredUnity/Assets/Plugins/Android/arm64-v8a/
cp ../libs/armeabi-v7a/libAudioPluginSuperpoweredSpatializer.so ../../SuperpoweredUnity/Assets/Plugins/Android/armeabi-v7a/
cp ../libs/x86_64/libAudioPluginSuperpoweredSpatializer.so ../../SuperpoweredUnity/Assets/Plugins/Android/x86_64/
cp ../libs/x86/libAudioPluginSuperpoweredSpatializer.so ../../SuperpoweredUnity/Assets/Plugins/Android/x86/