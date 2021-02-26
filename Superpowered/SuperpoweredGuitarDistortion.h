#ifndef Header_SuperpoweredGuitarDistortion
#define Header_SuperpoweredGuitarDistortion

#include "SuperpoweredFX.h"

namespace Superpowered {

struct gdInternals;

/// @brief Guitar distortion effect including Marshall cabinet, ADA cabinet and V-Twin preamp simulation, 5-band equalizer, bass and treble tone controls and two distortion sounds.
/// One instance allocates around 32 kb memory.
class GuitarDistortion: public FX {
public:
    float gainDecibel;     ///< Gain value in decibel. Limit: -96 to 24. Default: 0.
    float drive;           ///< Drive percentage, from 0 to 1. Default: 0.
    float bassFrequency;   ///< High-pass filter frequency in Hz. From 1 Hz to 250 Hz. Default: 1.
    float trebleFrequency; ///< Low-pass filter frequency in Hz. From 6000 Hz to the half of the current sample rate. Default: constructor samplerate / 2 - 100.
    float eq80HzDecibel;   ///< EQ 80 Hz decibel gain. Limit: -96 to 24. Default: 0.
    float eq240HzDecibel;  ///< EQ 240 Hz decibel gain. Limit: -96 to 24. Default: 0.
    float eq750HzDecibel;  ///< EQ 750 Hz decibel gain. Limit: -96 to 24. Default: 0.
    float eq2200HzDecibel; ///< EQ 2200 Hz decibel gain. Limit: -96 to 24. Default: 0.
    float eq6600HzDecibel; ///< EQ 6600 Hz decibel gain. Limit: -96 to 24. Default: 0.
    bool distortion0;      ///< Enables the first distortion sound, that is similar to Boss DS-1. Default: false.
    bool distortion1;      ///< Enables the second distortion sound, that is similar to Tyrian. Default: false.
    bool marshall;         ///< Enables Marshall cabinet simulation. Default: false.
    bool ada;              ///< Enables ADA cabinet simulation. Adds a lot of bass and treble. Default: false.
    bool vtwin;            ///< Enables V-Twin preamp simulation. Recommended for blues/jazz. Default: false.
    
/// @brief Constructor. Enabled is false by default.
/// @param samplerate The initial sample rate in Hz.
    JSWASM GuitarDistortion(unsigned int samplerate);
    JSWASM ~GuitarDistortion();
    
/// @brief Processes the audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input. Can point to the same location with output (in-place processing).
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output.
/// @param numberOfFrames Number of frames to process. Recommendation for best performance: multiply of 4, minimum 64.
    JSWASM bool process(float *input, float *output, unsigned int numberOfFrames);
    
private:
    gdInternals *internals;
    GuitarDistortion(const GuitarDistortion&);
    GuitarDistortion& operator=(const GuitarDistortion&);
};

}

#endif

