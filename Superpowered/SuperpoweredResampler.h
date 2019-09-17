#ifndef Header_SuperpoweredResampler
#define Header_SuperpoweredResampler

namespace Superpowered {

struct resamplerInternals;

/// @brief Linear or 6-point resampler, audio reverser and 16-bit to 32-bit audio converter.
/// It doesn't allocate any internal buffers and needs just a few bytes of memory.
class Resampler {
public:
    float rate; ///< Default: 1.0f. If rate = 1, process() is "transparent" without any effect on audio quality.
    
    Resampler();
    ~Resampler();
    
/// @brief Reset all internals. Doesn't change rate.
    void reset();
    
/// @brief Processes the audio.
/// @return The number of output frames.
/// @param input Pointer to short integer numbers, 16-bit stereo interleaved input. Should be numberOfFrames * 2 + 64 big.
/// @param output Pointer to floating point numbers, 32-bit stereo interleaved output. Should be big enough to store the expected number of output frames and some more.
/// @param numberOfFrames Number of frames to process.
/// @param reverse If true, the output will be backwards (reverse playback).
/// @param highQuality Enables more sophisticated processing to reduce interpolation noise. Good for scratching for example, but not recommended for continous music playback above 0.5 rate.
/// @param rateAdd Changes rate smoothly during process(). Useful for scratching or super smooth rate changes. After process() rate will be changed, but may or may not be precisely equal to the desired target value.
    int process(short int *input, float *output, int numberOfFrames, bool reverse = false, bool highQuality = false, float rateAdd = 0.0f);

/// @brief Processes the audio.
/// @return The number of output frames.
/// @param input Pointer to short integer numbers, 16-bit stereo interleaved input. Should be numberOfFrames * 2 + 64 big.
/// @param temp Pointer to floating point numbers. Should be numberOfFrames * 2 + 64 big.
/// @param output Pointer to short integer numbers, 16-bit stereo interleaved output. Should be big enough to store the expected number of output frames and some more.
/// @param numberOfFrames Number of frames to process.
/// @param reverse If true, the output will be backwards (reverse playback).
/// @param highQuality Enables more sophisticated processing to reduce interpolation noise. Good for scratching for example, but not recommended for continous music playback above 0.5 rate.
/// @param rateAdd Changes rate smoothly during process(). Useful for scratching or super smooth rate changes. After process() rate will be changed, but may or may not be precisely equal to the desired target value.
    int process(short int *input, float *temp, short int *output, int numberOfFrames, bool reverse = false, bool highQuality = false, float rateAdd = 0.0f);
    
private:
    resamplerInternals *internals;
    Resampler(const Resampler&);
    Resampler& operator=(const Resampler&);
};

}

#endif
