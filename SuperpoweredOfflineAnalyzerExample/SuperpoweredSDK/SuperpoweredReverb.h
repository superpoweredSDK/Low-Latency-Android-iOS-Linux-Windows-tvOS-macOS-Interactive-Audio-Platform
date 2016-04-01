#ifndef Header_SuperpoweredReverb
#define Header_SuperpoweredReverb

#include "SuperpoweredFX.h"
struct reverbInternals;

/**
 @brief CPU-friendly reverb.
 
 One instance allocates around 120 kb memory.
 
 @param dry >= 0.0f and <= 1.0f. Read only.
 @param wet >= 0.0f and <= 1.0f. Read only.
 @param mix >= 0.0f and <= 1.0f. Read only.
 @param width >= 0.0f and <= 1.0f. Read only.
 @param damp >= 0.0f and <= 1.0f. Read only.
 @param roomSize >= 0.0f and <= 1.0f. Read only.
 */
class SuperpoweredReverb: public SuperpoweredFX {
public:
// READ ONLY parameters, don't set them directly, use the methods below.
    float dry, wet;
    float mix;
    float width;
    float damp;
    float roomSize;
    
/**
 @brief You can set dry and wet independently, but don't use setMix in this case.
 
 @param value Limited to >= 0.0f and <= 1.0f.
 */
    void setDry(float value);
/**
 @brief You can set dry and wet independently, but don't use setMix in this case.
 
 @param value Limited to >= 0.0f and <= 1.0f.
 */
    void setWet(float value);
/**
 @brief Mix has a nice dry/wet constant power curve. Don't use setDry() and setWet() with this.
 
 @param value Limited to >= 0.0f and <= 1.0f.
 */
    void setMix(float value);
/**
 @brief Sets stereo width.
 
 @param value Limited to >= 0.0f and <= 1.0f.
 */
    void setWidth(float value);
/**
 @brief Sets high frequency damping.
 
 @param value Limited to >= 0.0f and <= 1.0f.
 */
    void setDamp(float value);
/**
 @brief Adjust room size.
 
 @param value Limited to >= 0.0f and <= 1.0f.
 */
    void setRoomSize(float value);
/**
 @brief Turns the effect on/off.
 */
    void enable(bool flag);

/**
 @brief Create a reverb instance with the current sample rate value.
 
 Enabled is false by default, use enable(true) to enable.
 */
    SuperpoweredReverb(unsigned int samplerate);
    ~SuperpoweredReverb();
    
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
 
 @param input 32-bit interleaved stereo input buffer. Can point to the same location with output (in-place processing). Special case: input can be NULL, reverb will output the tail only this case.
 @param output 32-bit interleaved stereo output buffer. Can point to the same location with input (in-place processing).
 @param numberOfSamples Should be 16 minimum, and a multiply of 8.
 */
    bool process(float *input, float *output, unsigned int numberOfSamples);

private:
    reverbInternals *internals;    
    SuperpoweredReverb(const SuperpoweredReverb&);
    SuperpoweredReverb& operator=(const SuperpoweredReverb&);
};

#endif
