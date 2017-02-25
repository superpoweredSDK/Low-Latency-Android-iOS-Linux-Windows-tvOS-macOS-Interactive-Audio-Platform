cmake_minimum_required(VERSION 3.4.1)

set(
	PATH_TO_SUPERPOWERED
	CACHE STRING ""
)

message(${ANDROID_ABI})

file(GLOB CPP_FILES "*.cpp")
file(GLOB USB_FILES "/Volumes/iMect/iphone/SuperpoweredSource/usb/*.cpp")

add_library(
	SuperpoweredExample
	SHARED
	${USB_FILES}
	${CPP_FILES}
	${PATH_TO_SUPERPOWERED}/AndroidIO/SuperpoweredAndroidAudioIO.cpp
)

include_directories(src/main/jni)
include_directories(${PATH_TO_SUPERPOWERED})
include_directories(/Volumes/iMect/iphone/SuperpoweredSource/usb)

target_link_libraries(
    SuperpoweredExample
    log
    android
    OpenSLES
    ${PATH_TO_SUPERPOWERED}/libSuperpoweredAndroid${ANDROID_ABI}.a
)