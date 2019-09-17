#ifndef Header_SuperpoweredThreeBandEQ
#define Header_SuperpoweredThreeBandEQ

#include "SuperpoweredFX.h"

namespace Superpowered {

struct eqInternals;

/// @brief Classic three-band equalizer with unique characteristics and total kills.
/// It doesn't allocate any internal buffers and needs just a few bytes of memory.
class ThreeBandEQ: public FX {
public:
    float low; ///< Low gain. Read-write. 1.0f is "flat", 2.0f is +6db. Kill is enabled under -40 db (0.01f). Default: 1.0f. Limits: 0.0f and 8.0f.
    float mid; ///< Mid gain. See low for details.
    float high; ///< High gain. See low for details.
    
/// @brief Constructor. Enabled is false by default.
/// @param samplerate The initial sample rate in Hz.
    ThreeBandEQ(unsigned int samplerate);
    ~ThreeBandEQ();
    
/// @brief Processes the audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output. Can point to the same location with input (in-place processing).
/// @param numberOfFrames Number of frames to process. Recommendation for best performance: multiply of 4, minimum 64.
    bool process(float *input, float *output, unsigned int numberOfFrames);
    
private:
    eqInternals *internals;
    ThreeBandEQ(const ThreeBandEQ&);
    ThreeBandEQ& operator=(const ThreeBandEQ&);
};

}

#endif
