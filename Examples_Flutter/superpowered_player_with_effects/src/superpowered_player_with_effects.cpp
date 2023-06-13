#include "superpowered_player_with_effects.h"
#include "SuperpoweredEngineExample.h"

#ifdef __ANDROID__
#include "Superpowered.h"
#endif

// The "Superpowered" directory cannot be included in the header search path, since podspec does not support relative paths
#ifdef __APPLE__
#include "../../../Superpowered/Superpowered.h"
#endif

static SuperpoweredEngineExample *example;

EXTERNC void initialize(char* tempDir) {
    Superpowered::Initialize("ExampleLicenseKey-WillExpire-OnNextUpdate");
    Superpowered::AdvancedAudioPlayer::setTempFolder(tempDir);
    
    example = new SuperpoweredEngineExample();
    example->play();
}

EXTERNC void togglePlayback() {
    example->togglePlayback();
}

EXTERNC void enableFlanger(bool enable) {
  example->enableFlanger(enable);
}

EXTERNC void playerDispose() {
  example->stop();
  delete example; 
}