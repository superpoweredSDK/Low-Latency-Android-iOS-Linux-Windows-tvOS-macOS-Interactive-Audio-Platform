#ifndef Header_SuperpoweredReverb
#define Header_SuperpoweredReverb

#include "SuperpoweredFX.h"

namespace Superpowered {

struct reverbInternals;

/// @brief CPU-friendly reverb.
/// One instance allocates around 120 kb memory.
class Reverb: public FX {
public:
    float dry;        ///< Set dry independently from wet. Don't use the mix property in this case. >= 0 and <= 1. Default: 0.987...
    float wet;        ///< Set wet independently from dry. Don't use the mix property in this case. >= 0 and <= 1. Default: 0.587...
    float mix;        ///< Sets dry and wet simultaneously with a nice constant power curve. Don't change dry and wet in this case. >= 0 and <= 1. Default: 0.4.
    float width;      ///< Stereo width. >= 0 and <= 1. Default: 1.
    float damp;       ///< High frequency damping. >= 0 and <= 1. Default: 0.5.
    float roomSize;   ///< Room size. >= 0 and <= 1. Default: 0.8.
    float predelayMs; ///< Pre-delay in milliseconds. 0 to 500. Default: 0.
    float lowCutHz;   ///< Frequency of the low cut in Hz (-12 db point). Default: 0 (no low frequency cut). Default: 0.

/// @brief Constructor. Enabled is false by default.
/// @param samplerate The initial sample rate in Hz.
/// @param maximumSamplerate Maximum sample rate (affects memory usage, the lower the smaller).
    JSWASM Reverb(unsigned int samplerate, unsigned int maximumSamplerate = 96000);
    JSWASM ~Reverb();

/// @brief Processes the audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input. Can point to the same location with output (in-place processing). Special case: input can be NULL, the effect will output the tail only in this case.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output.
/// @param numberOfFrames Number of frames to process. Recommendation for best performance: multiply of 4, minimum 64.
    JSWASM bool process(float *input, float *output, unsigned int numberOfFrames);

private:
    reverbInternals *internals;    
    Reverb(const Reverb&);
    Reverb& operator=(const Reverb&);
};

}

#endif
