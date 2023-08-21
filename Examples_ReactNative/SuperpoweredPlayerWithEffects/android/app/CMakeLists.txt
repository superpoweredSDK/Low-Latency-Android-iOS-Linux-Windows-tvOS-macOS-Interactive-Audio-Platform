cmake_minimum_required(VERSION 3.10)
project(SuperpoweredPlayerWithEffects)

set(CMAKE_VERBOSE_MAKEFILE on)
# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# now build app's shared lib
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -UNDEBUG -DDEBUG=1")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Os")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -UNDEBUG -DDEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Os")


file(TO_CMAKE_PATH ${PATH_TO_SUPERPOWERED} PATH_TO_SUPERPOWERED)
include_directories(${PATH_TO_SUPERPOWERED})

add_library(SuperpoweredPlayerWithEffects SHARED
        "../../cpp/JNI_Superpowered.cpp"
        "../../cpp/SuperpoweredEngineExample.cpp"
        "../../cpp/SuperpoweredEngineExampleAndroid.cpp"
        ${PATH_TO_SUPERPOWERED}/OpenSource/SuperpoweredAndroidAudioIO.cpp
)

# link the native library against the following libraries
target_link_libraries (
        SuperpoweredPlayerWithEffects
        log
        android
        OpenSLES
        ${PATH_TO_SUPERPOWERED}/libSuperpoweredAndroid${ANDROID_ABI}.a
)
