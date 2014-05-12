#import "SuperpoweredFX.h"
struct eqInternals;

/**
 @brief Classic three-band equalizer with unique characteristics and total kills.
 
 It doesn't allocate any internal buffers and needs just a few bytes of memory.
 
 @param lowDecibel The decibel value of the low band. Read only.
 @param midDecibel The decibel value of the mid band. Read only.
 @param highDecibel The decibel value of the high band. Read only.
 @param lowDecibelRounded The rounded decibel value of the low band. Read only.
 @param midDecibelRounded The rounded decibel value of the mid band. Read only.
 @param highDecibelRounded The rounded decibel value of the high band. Read only.
 @param lowGain Low band gain. Read-write. See the adjust() method.
 @param midGain Mid band gain. Read-write. See the adjust() method.
 @param highGain High band gain. Read-write. See the adjust() method.
 @param lowGainMod Low band gain modifier for advanced usage. Effective low gain is lowGain * lowGainMod. See the adjust() method.
 @param midGainMod Mid band gain modifier for advanced usage. Effective mid gain is midGain * midGainMod. See the adjust() method.
 @param highGainMod High band gain modifier for advanced usage. Effective high gain is highGain * highGainMod. See the adjust() method.
 @param lowKill If true, the low band will be killed regardless the other variables. See the adjust() method.
 @param midKill If true, the mid band will be killed regardless the other variables. See the adjust() method.
 @param highKill If true, the high band will be killed regardless the other variables. See the adjust() method.
 @param enabled True if the effect is enabled (processing audio). Read only. Use the enable() method to set.
 */
class Superpowered3BandEQ: public SuperpoweredFX {
public:
// READ ONLY parameters for nice displaying.
    float lowDecibel, midDecibel, highDecibel;
    float lowDecibelRounded, midDecibelRounded, highDecibelRounded;
    
// READ-WRITE parameters. After finished changing one or more of these, call adjust().
    float lowGain, midGain, highGain;
    float lowGainMod, midGainMod, highGainMod;
    bool lowKill, midKill, highKill;
    
/**
 @brief After setting any of the gain, gainMod or kill variables, call this to make the changes take to effect.
 */
    void adjust();
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
 @brief Reset all internals, sets the instance as good as new.
 */
    void reset();
    
/**
 @brief Processes the audio.
 
 It's not locked when you call other methods from other threads, and they not interfere with process() at all.
 
 @return Put something into output or not.
 
 @param input 32-bit interleaved stereo input buffer. Can point to the same location with output (in-place processing).
 @param output 32-bit interleaved stereo output buffer. Can point to the same location with input (in-place processing).
 @param numberOfSamples Should be 20 minimum.
*/
    bool process(float *input, float *output, unsigned int numberOfSamples);
    
private:
    eqInternals *internals;
    Superpowered3BandEQ(const Superpowered3BandEQ&);
    Superpowered3BandEQ& operator=(const Superpowered3BandEQ&);
};
