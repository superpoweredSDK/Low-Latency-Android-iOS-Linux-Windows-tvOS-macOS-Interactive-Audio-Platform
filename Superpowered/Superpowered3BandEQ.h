#ifndef Header_Superpowered3BandEQ
#define Header_Superpowered3BandEQ

#include "SuperpoweredFX.h"
struct eqInternals;

/**
 @brief Classic three-band equalizer with unique characteristics and total kills.
 
 It doesn't allocate any internal buffers and needs just a few bytes of memory.
 
 @param bands Low/mid/high gain. Read-write. 1.0f is "flat", 2.0f is +6db. Kill is enabled under -40 db (0.01f).
 @param enabled True if the effect is enabled (processing audio). Read only. Use the enable() method to set.
 */
class Superpowered3BandEQ: public SuperpoweredFX {
public:
    float bands[3]; // READ-WRITE parameter.

/**
 @brief Turns the effect on/off.
 */
    void enable(bool flag);
    
/**
 @brief Create an eq instance with the current sample rate value.
 
 Enabled is false by default, use enable(true) to enable. Example: Superpowered3BandEQ eq = new Superpowered3BandEQ(44100);
*/
    Superpowered3BandEQ(unsigned int samplerate);
    ~Superpowered3BandEQ();
    
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
 @param numberOfSamples Number of frames to process. Recommendations for best performance: multiply of 4, minimum 64.
*/
    bool process(float *input, float *output, unsigned int numberOfSamples);
    
private:
    eqInternals *internals;
    Superpowered3BandEQ(const Superpowered3BandEQ&);
    Superpowered3BandEQ& operator=(const Superpowered3BandEQ&);
};

#endif
