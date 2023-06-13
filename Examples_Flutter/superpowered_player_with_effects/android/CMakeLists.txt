# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause
# the plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 3.10)

project(superpowered_player_with_effects_library)
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

add_library(superpowered_player_with_effects SHARED
        "../src/superpowered_player_with_effects.cpp"
        "../src/SuperpoweredEngineExample.cpp"
        "cpp/SuperpoweredEngineExampleAndroid.cpp"
        ${PATH_TO_SUPERPOWERED}/OpenSource/SuperpoweredAndroidAudioIO.cpp
)

# link the native library against the following libraries
target_link_libraries (
        superpowered_player_with_effects
        log
        android
        OpenSLES
        ${PATH_TO_SUPERPOWERED}/libSuperpoweredAndroid${ANDROID_ABI}.a
)

set_target_properties(superpowered_player_with_effects PROPERTIES
  PUBLIC_HEADER superpowered_player_with_effects.h
  OUTPUT_NAME "superpowered_player_with_effects"
)

target_compile_definitions(superpowered_player_with_effects PUBLIC DART_SHARED_LIB)
