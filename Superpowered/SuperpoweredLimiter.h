#ifndef Header_SuperpoweredLimiter
#define Header_SuperpoweredLimiter

#include "SuperpoweredFX.h"

namespace Superpowered {

struct limiterInternals;

/// Limiter with 32 samples latency.
/// It doesn't allocate any internal buffers and needs less than 1 kb of memory.
class Limiter: public FX {
public:
    float ceilingDb;   ///< Ceiling in decibels, limited between 0 and -40. Default: 0.
    float thresholdDb; ///< Threshold in decibels, limited between 0 and -40. Default: 0.
    float releaseSec;  ///< Release in seconds (not milliseconds!). Limited between 0.1 and 1.6. Default: 0.05 (50 ms).

/// @brief Constructor. Enabled is false by default.
/// @param samplerate The initial sample rate in Hz.
    Limiter(unsigned int samplerate);
    ~Limiter();

/// @return Returns the maximum gain reduction in decibels since the last getGainReductionDb() call.
    float getGainReductionDb();

/// @brief Processes the audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties and call getGainReductionDb() on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output. Can point to the same location with input (in-place processing).
/// @param numberOfFrames Number of frames to process. Recommendations for best performance: multiply of 4, minimum 64.
    bool process(float *input, float *output, unsigned int numberOfFrames);

private:
    limiterInternals *internals;
    Limiter(const Limiter&);
    Limiter& operator=(const Limiter&);
};

}

#endif

