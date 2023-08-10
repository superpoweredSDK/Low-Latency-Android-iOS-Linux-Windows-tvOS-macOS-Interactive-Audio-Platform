#include <stdint.h>
#include <math.h>

#include "SuperpoweredEngineExample.h"
#include "SuperpoweredSimple.h"

#ifdef __ANDROID__

static bool audioProcessingAndroid(void *clientdata, short int *audioIO, int numberOfFrames, int samplerate) {
  auto example = static_cast<SuperpoweredEngineExample *>(clientdata);
  float playerOutput[numberOfFrames * 2];

  bool hasAudio = example->audioProcessing(playerOutput, (uint32_t) numberOfFrames, (uint32_t) samplerate);
  Superpowered::FloatToShortInt(playerOutput, audioIO, (unsigned int)numberOfFrames);
  return hasAudio;
}

void SuperpoweredEngineExample::platformCleanup() {
  delete androidEngine;
}
 
void SuperpoweredEngineExample::platformInit() {
  androidEngine = new SuperpoweredAndroidAudioIO(
          48000,
          512,
          false,
          true,
          audioProcessingAndroid,
          this
          );
}

void SuperpoweredEngineExample::platformStartEngine() {
  androidEngine->start();
}

void SuperpoweredEngineExample::platformStopEngine() {
  androidEngine->stop();
}

#endif
