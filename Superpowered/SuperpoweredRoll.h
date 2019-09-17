#ifndef Header_SuperpoweredRoll
#define Header_SuperpoweredRoll

#include "SuperpoweredFX.h"

namespace Superpowered {

struct rollInternals;

/// @brief Bpm/beat based loop roll effect.
/// One instance allocates around 1600 kb memory.
class Roll: public FX {
public:
    float wet;   ///< Limited to >= 0 and <= 1.
    float bpm;   ///< Limited to >= 40 and <= 250.
    float beats; ///< Limit: 1/64 beats to 4 beats. (>= 0.015625 and <= 4.0).
    
/// @brief Constructor. Enabled is false by default.
/// @param samplerate The initial sample rate in Hz.
/// @param maximumSamplerate The maximum sample rate in Hz to support. The higher the larger the memory usage.
    Roll(unsigned int samplerate, unsigned int maximumSamplerate = 96000);
    ~Roll();
    
/// @brief Processes the audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output. Can point to the same location with input (in-place processing).
/// @param numberOfFrames Number of frames to process. Recommendations for best performance: minimum 64.
    bool process(float *input, float *output, unsigned int numberOfFrames);
    
private:
    rollInternals *internals;
    Roll(const Roll&);
    Roll& operator=(const Roll&);
};

}

#endif
