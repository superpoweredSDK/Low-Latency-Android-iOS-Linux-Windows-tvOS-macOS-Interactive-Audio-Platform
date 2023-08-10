#pragma once

#include <memory>

#ifdef __ANDROID__
#include "OpenSource/SuperpoweredAndroidAudioIO.h"
#include "SuperpoweredAdvancedAudioPlayer.h"
#include "SuperpoweredFlanger.h"
#endif

// The "Superpowered" directory cannot be included in the header search path, since podspec does not support relative paths
#ifdef __APPLE__
#include "../../../Superpowered/SuperpoweredAdvancedAudioPlayer.h"
#include "../../../Superpowered/SuperpoweredFlanger.h"
#endif


class SuperpoweredEngineExample {
public:
    SuperpoweredEngineExample();
    ~SuperpoweredEngineExample();
    
    void stop();

    void play();

    void togglePlayback();
    
    bool audioProcessing(float *interleaved, uint32_t frames, uint32_t samplerate);

    void enableFlanger(bool enable);

    void platformInit();
    
    void platformStartEngine();

    void platformStopEngine();

    void platformCleanup();
    
private:
#ifdef __ANDROID__
  SuperpoweredAndroidAudioIO* androidEngine;
#else
  void* iOSEngine;
#endif
  std::unique_ptr<Superpowered::AdvancedAudioPlayer> player;
  std::unique_ptr<Superpowered::Flanger> flanger;
};
