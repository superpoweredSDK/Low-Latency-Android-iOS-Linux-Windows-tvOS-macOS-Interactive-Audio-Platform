#ifndef Header_SuperpoweredReverb
#define Header_SuperpoweredReverb

#include "SuperpoweredFX.h"

namespace Superpowered {

struct reverbInternals;

/// @brief CPU-friendly reverb.
/// One instance allocates around 120 kb memory.
class Reverb: public FX {
public:
    float dry;        ///< Loudness of the dry signal. Don't use the mix property when using the dry and wet properties. >= 0 and <= 1. Default: 0.987...
    float wet;        ///< Loudness of the wet signal. Don't use the mix property when using the dry and wet properties. >= 0 and <= 1. Default: 0.587...
    float mix;        ///< Sets dry and wet simultaneously with a nice balanced power curve. Don't use the dry and wet properties while using mix. >= 0 and <= 1. Default: 0.4.
    float width;      ///< Stereo width of the reverberation. >= 0 and <= 1. Default: 1.
    float damp;       ///< Used to control the absorption of high frequencies in the reverb. More absorption of high frequencies means higher damping values. The tail of the reverb will lose high frequencies as they bounce around softer surfaces like halls and result in warmer sounds. >= 0 and <= 1. Default: 0.5.
    float roomSize;   ///< Room size controls the scale of the decay time and reflections found in the physical characteristics of living spaces, and studios. These unique attributes will simulate the expected behavior of acoustic environments. A larger room size typically results in longer reverb time. >= 0 and <= 1. Default: 0.8.
    float predelayMs; ///< Pre-delay in milliseconds. The length of time it takes for a sound wave to leave its source and create its first reflection is determined by the pre-delay. This property controls the offset of reverb from the dry signal. An increase in pre-delay can result in a feeling of a bigger space. 0 to 500. Default: 0.
    float lowCutHz;   ///< Frequency of the low cut in Hz (-12 db point). Controls the low frequency build up generated from the reverb. Default: 0 (no low frequency cut).

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
