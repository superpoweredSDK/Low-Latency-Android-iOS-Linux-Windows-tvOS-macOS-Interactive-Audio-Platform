#ifndef Header_SuperpoweredDelay
#define Header_SuperpoweredDelay

#include "SuperpoweredFX.h"

namespace Superpowered {

struct delayInternals;

/// @brief Simple delay with minimum memory operations.
class Delay {
public:
    float delayMs;           ///< Delay in milliseconds.
    unsigned int samplerate; ///< Sample rate in Hz.
    
/// @brief Constructor.
/// @param maximumDelayMs Maximum delay in milliseconds. Higher values increase memory usage.
/// @param maximumSamplerate Maximum sample rate to support. Higher values increase memory usage.
/// @param maximumNumberOfFramesToProcess Maximum number of frames for the process() call. Has minimum effect on memory usage.
/// @param samplerate The initial sample rate in Hz.
    Delay(unsigned int maximumDelayMs, unsigned int maximumSamplerate, unsigned int maximumNumberOfFramesToProcess, unsigned int samplerate);
    ~Delay();
    
/// @brief Processes the audio.
/// It's never blocking for real-time usage. You can change any properties concurrently with process().
/// @return Returns with a pointer to floating point numbers, which is the output with numberOfFrames audio available in it. It is valid until the next call to process().
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input. Special case: set to NULL to empty all buffered content.
/// @param numberOfFrames Number of frames to input and output.
/// @param fx Optional. If NULL, then simple memory copy will be used to pass audio from input to the internal buffer. If not NULL, fx->process() will be used to pass audio from input to the internal buffer.
    const float * const process(float *input, int numberOfFrames, FX *fx = 0);
   
private:
    delayInternals *internals;
    Delay(const Delay&);
    Delay& operator=(const Delay&);
};

}

#endif
