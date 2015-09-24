#ifndef Header_SuperpoweredLimiter
#define Header_SuperpoweredLimiter

#include "SuperpoweredFX.h"
struct limiterInternals;

/**
 @brief Limiter with 32 samples latency.

 It doesn't allocate any internal buffers and needs less than 1 kb of memory. One limiter instance needs just 2.5x CPU compared to a SuperpoweredFilter.

 @param ceilingDb Ceiling in decibels, limited between 0.0f and -40.0f. Default: 0.
 @param thresholdDb Threshold in decibels, limited between 0.0f and -40.0f. Default: 0.
 @param releaseSec Release in seconds (not milliseconds!). Limited between 0.1f and 1.6f. Default: 0.05f (50 ms).
 */
class SuperpoweredLimiter: public SuperpoweredFX {
public:
    // READ-WRITE parameters
    float ceilingDb;
    float thresholdDb;
    float releaseSec;

/**
 @brief Turns the effect on/off.
*/
    void enable(bool flag); // Use this to turn it on/off.

/**
 @brief Create a limiter instance with the current sample rate value.

 Enabled is false by default, use enable(true) to enable.
*/
    SuperpoweredLimiter(unsigned int samplerate);
    ~SuperpoweredLimiter();

/**
 @brief Sets the sample rate.

 @param samplerate 44100, 48000, etc.
*/
    void setSamplerate(unsigned int samplerate);

/**
 @brief Reset all internals, sets the instance as good as new and turns it off.
*/
    void reset();

/**
 @return Returns with the maximum gain reduction in decibels since the last getGainReductionDb() call.

 This method uses the log10f function, which is CPU intensive. Call it when you must update your user interface.
*/
    float getGainReductionDb();

/**
 @brief Processes interleaved audio.

 @return Put something into output or not.

 @param input 32-bit interleaved stereo input buffer. Can point to the same location with output (in-place processing).
 @param output 32-bit interleaved stereo output buffer. Can point to the same location with input (in-place processing).
 @param numberOfSamples Should be 32 minimum.
*/
    bool process(float *input, float *output, unsigned int numberOfSamples);

private:
    limiterInternals *internals;
    SuperpoweredLimiter(const SuperpoweredLimiter&);
    SuperpoweredLimiter& operator=(const SuperpoweredLimiter&);
};

#endif

