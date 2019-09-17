#ifndef Header_SuperpoweredGate
#define Header_SuperpoweredGate

#include "SuperpoweredFX.h"

namespace Superpowered {

struct gateInternals;

/// @brief Simple gate effect.
/// It doesn't allocate any internal buffers and needs just a few bytes of memory.
class Gate: public FX {
public:
    float wet;   ///< Limited to >= 0 and <= 1.
    float bpm;   ///< Limited to >= 40 and <= 250.
    float beats; ///< The rhythm in beats to open and close the "gate". From 1/64 beats to 4 beats. (>= 0.015625 and <= 4)

/// @brief Constructor. Enabled is false by default.
/// @param samplerate The initial sample rate in Hz.
    Gate(unsigned int samplerate);
    ~Gate();

/// @brief Processes the audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output. Can point to the same location with input (in-place processing).
/// @param numberOfFrames Number of frames to process. Recommendations for best performance: minimum 64.
    bool process(float *input, float *output, unsigned int numberOfFrames);
    
private:
    gateInternals *internals;
    Gate(const Gate&);
    Gate& operator=(const Gate&);
};

}

#endif
