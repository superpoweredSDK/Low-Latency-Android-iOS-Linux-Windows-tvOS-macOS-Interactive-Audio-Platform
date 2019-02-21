#ifndef Header_SuperpoweredFX
#define Header_SuperpoweredFX

/**
 @brief This is the base class for Superpowered effects.
 
 @param enabled Indicates if the effect is enabled (processing audio).
*/
class SuperpoweredFX {
public:
    bool enabled;
    
/**
 @brief Turns the effect on/off.
 */
    virtual void enable(bool flag) = 0; // Use this to turn it on/off.
    
/**
 @brief Sets the sample rate.
 
 @param samplerate 44100, 48000, etc.
 */
    virtual void setSamplerate(unsigned int samplerate) = 0;

/**
 @brief Reset all internals, sets the instance as good as new and turns it off.
*/
    virtual void reset() = 0;

/**
 @brief Processes the audio.
 
 It's not locked when you call other methods from other threads, and they not interfere with process() at all.
 Check the process() documentation of each fx for the minimum number of samples and an optional vector size limitation. For maximum compatibility with all Superpowered effects, numberOfSamples should be minimum 32 and a multiply of 8.
 
 @return Put something into output or not.
 
 @param input 32-bit interleaved stereo input buffer.
 @param output 32-bit interleaved stereo output buffer.
 @param numberOfSamples Number of samples to process.
 */
    virtual bool process(float *input, float *output, unsigned int numberOfSamples) = 0;
    
    virtual ~SuperpoweredFX() {};
};

/**
 \mainpage Superpowered Audio SDK
 
 The Superpowered Audio SDK is a software development kit based on Superpowered Inc’s digital signal processing (DSP) technology.

 Superpowered technology allows developers to build computationally intensive audio apps and embedded applications that process more quickly and use less power than other comparable solutions.

 Superpowered DSP is designed and optimized, from scratch, to run on low-power mobile processors. Specifically, any device running ARM with the NEON extension or 64-bit ARM (which covers 99% of all mobile devices manufactured). Intel CPU is supported too.
 
 Details of the latest version can be found at http://superpowered.com/superpowered-audio-sdk/
 */

#endif
