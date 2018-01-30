#ifndef Header_SuperpoweredGate
#define Header_SuperpoweredGate

#include "SuperpoweredFX.h"
struct gateInternals;

/**
 @brief Simple gate effect.
 
 It doesn't allocate any internal buffers and needs just a few bytes of memory.
 
 @param wet Limited to >= 0.0f and <= 1.0f.
 @param bpm Limited to >= 40.0f and <= 250.0f
 @param beats The rhythm in beats to open/close the "gate". From 1/64 beats to 4 beats. (>= 0.015625f and <= 4.0f)
 */
class SuperpoweredGate: public SuperpoweredFX {
public:
// READ-WRITE parameters, thread safe (change from any thread)
    float wet;
    float bpm;
    float beats;
    
/**
 @brief Turns the effect on/off.
 */
    void enable(bool flag);

/**
 @brief Create a gate instance with the current sample rate value.
 
 Enabled is false by default, use enable(true) to enable.
*/
    SuperpoweredGate(unsigned int samplerate);
    ~SuperpoweredGate();
    
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
 @param numberOfSamples Number of frames to process. Recommendations for best performance: minimum 64.
 */
    bool process(float *input, float *output, unsigned int numberOfSamples);
    
private:
    gateInternals *internals;
    SuperpoweredGate(const SuperpoweredGate&);
    SuperpoweredGate& operator=(const SuperpoweredGate&);
};

#endif
