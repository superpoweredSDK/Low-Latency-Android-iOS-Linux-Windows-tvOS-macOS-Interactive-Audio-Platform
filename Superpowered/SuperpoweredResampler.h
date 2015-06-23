#ifndef Header_SuperpoweredResampler
#define Header_SuperpoweredResampler

struct resamplerInternals;

/**
 @brief Linear or 6-point resampler, audio reverser and 16-bit to 32-bit audio converter.
 
 It doesn't allocate any internal buffers and needs just a few bytes of memory.
 
 @param rate Read-write. Default: 1.0f.
 */
class SuperpoweredResampler {
public:
    float rate;
    
    SuperpoweredResampler();
    ~SuperpoweredResampler();
/**
 @brief Reset all internals. Doesn't change rate.
 */
    void reset();
    
/**
 @brief Processes the audio.

 @return The number of output frames (samples).
 
 @param input 16-bit stereo input. Should be numberOfSamples * 2 + 64 big.
 @param output 32-bit floating point stereo output. Should be numberOfSamples * 2 + 64 big.
 @param numberOfSamples Number of samples to process.
 @param reverse Plays backwards.
 @param highQuality Enables more sophisticated processing to reduce interpolation noise. Good for scratching for example, but not recommended for continous music playback above 0.5f rate.
 @param rateAdd Changes rate during process(), good for scratching or super smooth rate changes. After process(), rate will be near the desired value.
*/
    int process(short int *input, float *output, int numberOfSamples, bool reverse = false, bool highQuality = false, float rateAdd = 0.0f);

/**
 @brief Processes the audio.

 @return The number of output frames (samples).

 @param input 16-bit stereo input. Should be numberOfSamples * 2 + 64 big.
 @param temp Temporary buffer. Should be numberOfSamples * 2 + 64 big.
 @param output 16-bit stereo output. Should be numberOfSamples * 2 + 64 big.
 @param numberOfSamples Number of samples to process.
 @param reverse Plays backwards.
 @param highQuality Enables more sophisticated processing to reduce interpolation noise. Good for scratching for example, but not recommended for continous music playback above 0.5f rate.
 @param rateAdd Changes rate during process(), good for scratching or super smooth rate changes. After process(), rate will be near the desired value.
*/
    int process(short int *input, float *temp, short int *output, int numberOfSamples, bool reverse = false, bool highQuality = false, float rateAdd = 0.0f);
    
private:
    resamplerInternals *internals;
    SuperpoweredResampler(const SuperpoweredResampler&);
    SuperpoweredResampler& operator=(const SuperpoweredResampler&);
};

#endif
