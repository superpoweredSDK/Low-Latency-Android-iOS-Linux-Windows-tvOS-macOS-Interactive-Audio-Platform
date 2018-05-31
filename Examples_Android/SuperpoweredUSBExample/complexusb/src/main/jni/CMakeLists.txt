cmake_minimum_required(VERSION 3.4.1)

# convert SDK path to forward slashes on Windows
file(TO_CMAKE_PATH ${PATH_TO_SUPERPOWERED} PATH_TO_SUPERPOWERED)

include_directories(src/main/jni)
include_directories(${PATH_TO_SUPERPOWERED})

file(GLOB CPP_FILES "*.cpp")

# compile USB example to a native library
add_library(
	SuperpoweredExample
	SHARED
	${CPP_FILES}
	${PATH_TO_SUPERPOWERED}/AndroidIO/SuperpoweredAndroidAudioIO.cpp
)

# link the native library against the following libraries
target_link_libraries(
    SuperpoweredExample
    log
    android
    OpenSLES
    ${PATH_TO_SUPERPOWERED}/libSuperpoweredAndroid${ANDROID_ABI}.a
)