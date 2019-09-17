#ifndef Header_SuperpoweredWhoosh
#define Header_SuperpoweredWhoosh

#include "SuperpoweredFX.h"

namespace Superpowered {

struct whooshInternals;

/// @brief White noise + filter.
/// One whoosh instance allocates around 4 kb memory.
class Whoosh: public FX {
public:
    float wet;       ///< Limited to >= 0 and <= 1.
    float frequency; ///< Limited to >= 20 and <= 20000.

/// @brief Constructor. Enabled is false by default.
/// @param samplerate The initial sample rate in Hz.
    Whoosh(unsigned int samplerate);
    ~Whoosh();
    
/// @brief Processes the audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input. The output will be mixed to this. Can be NULL.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo input. Can point to the same location with output (in-place processing).
/// @param numberOfFrames Number of frames to process. Recommendation for best performance: multiply of 4, minimum 64.
    bool process(float *input, float *output, unsigned int numberOfFrames);
    
private:
    whooshInternals *internals;
    Whoosh(const Whoosh&);
    Whoosh& operator=(const Whoosh&);
};

}

#endif
