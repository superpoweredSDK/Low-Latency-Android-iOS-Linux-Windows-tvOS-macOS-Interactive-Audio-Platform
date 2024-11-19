#!/bin/bash

cd ../example || exit
# Podspec does not support relative path
cp -r ../../../Superpowered/libSuperpoweredAudio.xcframework ../ios/

flutter run -d iPhone
