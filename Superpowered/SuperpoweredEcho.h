#ifndef Header_SuperpoweredEcho
#define Header_SuperpoweredEcho

#include "SuperpoweredFX.h"
struct echoInternals;

/**
 @brief Simple echo.
 
 One instance allocates around 770 kb memory.
 
 @param dry >= 0.0f and <= 1.0f. Read only.
 @param wet >= 0.0f and <= 1.0f. Read only.
 @param bpm >= 60.0f and <= 240.0f. Read-write.
 @param beats Delay in beats, >= 0.125f and <= 2.0f. Read-write.
 @param decay >= 0.0f and <= 1.0f. Read-write.
 */
class SuperpoweredEcho: public SuperpoweredFX {
public:
// READ ONLY parameters, don't set them directly, use the methods below.
    float dry, wet;

// READ-WRITE PARAMETERS:
    float bpm;
    float beats;
    float decay;

/**
 @brief Wet is always == mix, but dry changes with a nice curve for a good echo/dry balance.
 
 @param mix >= 0.0f and <= 1.0f.
 */
    void setMix(float mix);
/**
 @brief Turns the effect on/off.
 */
    void enable(bool flag);
    
/**
 @brief Create an echo instance with the current sample rate value.
 
 Enabled is false by default, use enable(true) to enable.
 */
    SuperpoweredEcho(unsigned int samplerate);
    ~SuperpoweredEcho();
    
/**
 @brief Sets the sample rate.
 
 @param samplerate 44100, 48000, etc.
 */
    void setSamplerate(unsigned int samplerate); // 44100, 48000, etc.
/**
 @brief Reset all internals, sets the instance as good as new and turns it off.
 */
    void reset();

/**
 @brief Processes the audio.
     
 It's not locked when you call other methods from other threads, and they not interfere with process() at all.
 
 @return Put something into output or not.
 
 @param input 32-bit interleaved stereo input buffer. Can point to the same location with output (in-place processing). Special case: input can be NULL, echo will output the tail only this case.
 @param output 32-bit interleaved stereo output buffer. Can point to the same location with input (in-place processing).
 @param numberOfSamples Should be 16 minimum, and a multiply of 8.
*/
    bool process(float *input, float *output, unsigned int numberOfSamples);
    
private:
    echoInternals *internals;
    SuperpoweredEcho(const SuperpoweredEcho&);
    SuperpoweredEcho& operator=(const SuperpoweredEcho&);
};

#endif
