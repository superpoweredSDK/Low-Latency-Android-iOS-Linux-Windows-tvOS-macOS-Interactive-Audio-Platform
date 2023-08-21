#include <stdint.h>
#include <math.h>
#include "SuperpoweredEngineExample.h"
#include "SuperpoweredSimple.h"
#include "SuperpoweredCPU.h"

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
  player->open("https://docs.superpowered.com/audio/samples/splice/SO_PF_74_string_phrase_soaring_Gb.mp3");
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
