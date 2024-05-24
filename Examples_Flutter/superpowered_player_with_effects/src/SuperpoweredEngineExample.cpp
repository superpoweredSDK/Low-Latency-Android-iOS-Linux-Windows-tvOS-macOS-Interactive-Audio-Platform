#include <stdint.h>
#include <math.h>
#include "SuperpoweredEngineExample.h"

#ifdef __ANDROID__
#include "SuperpoweredSimple.h"
#include "SuperpoweredCPU.h"
#endif

// The "Superpowered" directory cannot be included in the header search path, since podspec does not support relative paths
#ifdef __APPLE__
#include "../../../Superpowered/SuperpoweredSimple.h"
#include "../../../Superpowered/SuperpoweredCPU.h"
#endif

SuperpoweredEngineExample::SuperpoweredEngineExample() {
  player = std::make_unique<Superpowered::AdvancedAudioPlayer>(48000, 0);
  player->loopOnEOF = true;

  flanger = std::make_unique<Superpowered::Flanger>(48000);
  flanger->enabled = true;

  platformInit();
  platformStartEngine();
}

SuperpoweredEngineExample::~SuperpoweredEngineExample() {
  platformCleanup();
}

void SuperpoweredEngineExample::play() {
  player->open("https://superpowered.s3.amazonaws.com/SO_PF_74_string_phrase_soaring_Gb.mp3");
  player->play();
}

void SuperpoweredEngineExample::stop() {
    platformStopEngine();
    player->pause();
}

void SuperpoweredEngineExample::togglePlayback() {
    player->togglePlayback();
}

void SuperpoweredEngineExample::enableFlanger(bool enable) {
      flanger->enabled = enable;
}

bool SuperpoweredEngineExample::audioProcessing(float *interleaved, uint32_t frames, uint32_t samplerate) {
  player->outputSamplerate = (unsigned int)samplerate;
  bool silence = !player->processStereo(interleaved, false, (unsigned int)frames);
  flanger->samplerate = samplerate;
  bool outputChanged = flanger->process(silence ? NULL : interleaved, interleaved, frames);
  return !silence;
}
