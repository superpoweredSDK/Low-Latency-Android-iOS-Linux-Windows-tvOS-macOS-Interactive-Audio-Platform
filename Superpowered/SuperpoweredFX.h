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
 \mainpage Audio, Networking, and Cryptographics for Android, iOS, macOS, tvOS, Linux and Windows
 
 Details of the latest features/versions can be found at:
 
 Audio: https://superpowered.com/audio-library-sdk
 
 Networking: https://superpowered.com/networking-library-sdk
 
 Cryptographics: https://superpowered.com/crypto-library-sdk
 */

#endif
