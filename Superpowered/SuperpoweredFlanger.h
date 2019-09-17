#ifndef Header_SuperpoweredFlanger
#define Header_SuperpoweredFlanger

#include "SuperpoweredFX.h"

namespace Superpowered {

struct flangerInternals;

/// @brief Flanger with aggressive sound ("jet").
/// One instance allocates around 80 kb memory.
class Flanger: public FX {
public:
    float wet;                ///< 0 to 1.
    float depth;              ///< 0 to 1 (0 is 0.3 ms, 1 is 8 ms).
    float lfoBeats;           ///< The length in beats between the "lowest" and the "highest" jet sound, >= 0.25 and <= 128.
    float bpm;                ///< The bpm of the current audio. Limited to >= 40 and <= 250.
    float clipperThresholdDb; ///< The flanger has a Clipper inside to prevent overdrive. This is the thresholdDb parameter.
    float clipperMaximumDb;   ///< The flanger has a Clipper inside to prevent overdrive. This is the maximumDb parameter.
    bool stereo;              ///< True: stereo, false: mono.
    
/// @brief Constructor. Enabled is false by default.
/// @param samplerate The initial sample rate in Hz.
    Flanger(unsigned int samplerate);
    ~Flanger();
    
/// @brief Returns with the current depth in milliseconds, 0.3f to 8.0f (0.3 ms to 8 ms).
    float getDepthMs();

/// @brief Processes the audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties and call getDepthMs() on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output. Can point to the same location with input (in-place processing).
/// @param numberOfFrames Number of frames to process. Recommendations for best performance: multiply of 4, minimum 64.
    bool process(float *input, float *output, unsigned int numberOfFrames);
    
private:
    flangerInternals *internals;
    Flanger(const Flanger&);
    Flanger& operator=(const Flanger&);
};

}

#endif
