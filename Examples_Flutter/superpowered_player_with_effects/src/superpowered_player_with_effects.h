#pragma once

#include <stdbool.h>

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC void initialize(char* tempDir) ;

EXTERNC void togglePlayback();

EXTERNC void enableFlanger(bool enable);

EXTERNC void playerDispose();

