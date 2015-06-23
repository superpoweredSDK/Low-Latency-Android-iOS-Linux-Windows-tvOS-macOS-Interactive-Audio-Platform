#ifndef Header_SuperpoweredFlanger
#define Header_SuperpoweredFlanger

#include "SuperpoweredFX.h"
struct flangerInternals;

/**
 @brief Flanger with aggressive sound ("jet").
 
 One instance allocates around 80 kb memory.
 
 @param wet 0.0f to 1.0f. Read only.
 @param depthMs Depth in milliseconds, 0.3f to 8.0f (0.3 ms to 8 ms). Read only.
 @param depth 0.0f to 1.0f (0.0 is 0.3 ms, 1.0 is 8 ms). Read only.
 @param lfoBeats The length in beats between the "lowest" and the "highest" jet sound, >= 0.25f and <= 64.0f. Read only.
 @param bpm Set this right for a nice sounding lfo. Limited to >= 60.0f and <= 240.0f. Read-write.
 @param limitLevel The flanger has a hard-knee limiter inside to prevent overdrive. The default value of 1.0f means hard knee limiting to 0.0 db. Threshold is fixed at -3.0 db below the limit level. Valid range is 0.1f to 1.0f.
 @param stereo Stereo/mono switch. Read-write.
 */
class SuperpoweredFlanger: public SuperpoweredFX {
public:
// READ ONLY parameters, don't set them directly, use the methods below.
    float wet;
    float depthMs;
    float depth;
    float lfoBeats;
    
// READ-WRITE parameters, thread safe (change from any thread)
    float bpm;
    float limitLevel;
    bool stereo;
    
/**
 @brief Set wet.
 
 @param value 0.0f to 1.0f
 */
    void setWet(float value);
/**
 @brief Set depth.
 
 @param value 0.0f to 1.0f
 */
    void setDepth(float value);
/**
 @brief Set LFO, adjustable with beats.
 
 @param beats >= 0.25f and <= 64.0f
 */
    void setLFOBeats(float beats);
/**
 @brief Turns the effect on/off.
 */
    void enable(bool flag);
    
/**
 @brief Create a flanger instance with the current sample rate value.
 
 Enabled is false by default, use enable(true) to enable.
 */
    SuperpoweredFlanger(unsigned int samplerate);
    ~SuperpoweredFlanger();
    
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
 @brief Processes the audio.
 
 It's not locked when you call other methods from other threads, and they not interfere with process() at all.
 
 @return Put something into output or not.
 
 @param input 32-bit interleaved stereo input buffer. Can point to the same location with output (in-place processing).
 @param output 32-bit interleaved stereo output buffer. Can point to the same location with input (in-place processing).
 @param numberOfSamples Should be 16 minimum.
*/
    bool process(float *input, float *output, unsigned int numberOfSamples);
    
private:
    flangerInternals *internals;
    SuperpoweredFlanger(const SuperpoweredFlanger&);
    SuperpoweredFlanger& operator=(const SuperpoweredFlanger&);
};

#endif
