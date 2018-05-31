cmake_minimum_required(VERSION 3.4.1)

# convert SDK path to forward slashes on Windows
file(TO_CMAKE_PATH ${PATH_TO_SUPERPOWERED} PATH_TO_SUPERPOWERED)

include_directories(src/main/cpp)
include_directories(${PATH_TO_SUPERPOWERED})

# compile frequency domain example to a native library
add_library(
	FrequencyDomainExample
	SHARED
	src/main/cpp/FrequencyDomain.cpp
	${PATH_TO_SUPERPOWERED}/AndroidIO/SuperpoweredAndroidAudioIO.cpp
)

# link the native library against the following libraries
target_link_libraries(
    FrequencyDomainExample
    log
    android
    OpenSLES
    ${PATH_TO_SUPERPOWERED}/libSuperpoweredAndroid${ANDROID_ABI}.a
)