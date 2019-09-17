#ifndef Header_SuperpoweredEcho
#define Header_SuperpoweredEcho

#include "SuperpoweredFX.h"

namespace Superpowered {

struct echoInternals;

/// @brief Simple echo ("delay effect").
/// One instance allocates around 770 kb memory.
class Echo: public FX {
public:
    float dry;   ///< 0 to 1
    float wet;   ///< 0 to 1
    float bpm;   ///< 40 to 250
    float beats; ///< Delay in beats. 0.03125 to 2.
    float decay; ///< 0 to 0.99
    
/// @brief Constructor. Enabled is false by default.
/// @param samplerate The initial sample rate in Hz.
/// @param maximumSamplerate Maximum sample rate (affects memory usage, the lower the smaller).
    Echo(unsigned int samplerate, unsigned int maximumSamplerate = 96000);
    ~Echo();

/// @brief Sets dry and wet simultaneously with a good balance between them. Wet always equals to mix, but dry changes with a curve.
/// @param mix >= 0 and <= 1.
    void setMix(float mix);
    
/// @brief Processes the audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties and call setMix() on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, it indicates silence. The contents of output are not changed in this case (not overwritten with zeros).
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input. Can point to the same location with output (in-place processing). Special case: input can be NULL, the effect will output the tail only in this case.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output.
/// @param numberOfFrames Number of frames to process. Recommendation for best performance: multiply of 4, minimum 64.
    bool process(float *input, float *output, unsigned int numberOfFrames);

/// @brief Processes the audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties and call setMix() on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input. Can point to the same location with output (in-place processing). Special case: input can be NULL, the effect will output the tail only in this case.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output.
/// @param numberOfFrames Number of frames to process. Recommendation for best performance: multiply of 4, minimum 64.
/// @param fx fx->process() will be used to pass audio from input to the internal buffer. For example, a resonant Filter can provide a nice color for the echoes.
    bool process(float *input, float *output, unsigned int numberOfFrames, FX *fx);
    
private:
    echoInternals *internals;
    Echo(const Echo&);
    Echo& operator=(const Echo&);
};

}

#endif
