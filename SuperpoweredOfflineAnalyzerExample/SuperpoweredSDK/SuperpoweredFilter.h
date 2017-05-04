#ifndef Header_SuperpoweredFilter
#define Header_SuperpoweredFilter

#include "SuperpoweredFX.h"
struct filterInternals;

typedef enum SuperpoweredFilterType {
    SuperpoweredFilter_Resonant_Lowpass,
    SuperpoweredFilter_Resonant_Highpass,
    SuperpoweredFilter_Bandlimited_Bandpass,
    SuperpoweredFilter_Bandlimited_Notch,
    SuperpoweredFilter_LowShelf,
    SuperpoweredFilter_HighShelf,
    SuperpoweredFilter_Parametric
} SuperpoweredFilterType;

/**
 @brief IIR filters.
 
 It doesn't allocate any internal buffers and needs just a few bytes of memory.
 
 @param frequency Current frequency value. Read only.
 @param decibel Current decibel value for shelving and parametric filters. Read only.
 @param resonance Current resonance value for resonant filters. Read only.
 @param octave Current octave value for bandlimited and parametric filters. Read only.
 @param slope Current slope value for shelving filters. Read only.
 @param type Filter type. Read only.
 */
class SuperpoweredFilter: public SuperpoweredFX {
public:
// READ ONLY parameters
    float frequency;
    float decibel;
    float resonance;
    float octave;
    float slope;
    SuperpoweredFilterType type;
    
/**
 @brief Change parameters for resonant filters.
 */
    void setResonantParameters(float frequency, float resonance);
/**
 @brief Change parameters for shelving filters.
 */
    void setShelfParameters(float frequency, float slope, float dbGain);
/**
 @brief Change parameters for bandlimited filters.
 */
    void setBandlimitedParameters(float frequency, float octaveWidth);
/**
 @brief Change parameters for parametric filters.
 */
    void setParametricParameters(float frequency, float octaveWidth, float dbGain);
    
/**
 @brief Set params and type at once for resonant filters.
 
 @param frequency The frequency in Hz.
 @param resonance Resonance value.
 @param type Must be lowpass or highpass.
 */
    void setResonantParametersAndType(float frequency, float resonance, SuperpoweredFilterType type);
/**
 @brief Set params and type at once for shelving filters.
 
 @param frequency The frequency in Hz.
 @param slope Slope.
 @param dbGain Gain in decibel.
 @param type Must be low shelf or high shelf.
 */
    void setShelfParametersAndType(float frequency, float slope, float dbGain, SuperpoweredFilterType type);
    
/**
 @brief Set params and type at once for bandlimited filters.
 
 @param frequency The frequency in Hz.
 @param octaveWidth Width in octave.
 @param type Must be bandpass or notch.
 */
    void setBandlimitedParametersAndType(float frequency, float octaveWidth, SuperpoweredFilterType type);
    
/**
 @brief Turns the effect on/off.
 */
    void enable(bool flag); // Use this to turn it on/off.
    
/**
 @brief Create an filter instance with the current sample rate value and filter type.
 
 Enabled is false by default, use enable(true) to enable.
 */
    SuperpoweredFilter(SuperpoweredFilterType filterType, unsigned int samplerate);
    ~SuperpoweredFilter();
    
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
 @brief Processes interleaved audio.
 
 @return Put something into output or not.
 
 @param input 32-bit interleaved stereo input buffer. Can point to the same location with output (in-place processing).
 @param output 32-bit interleaved stereo output buffer. Can point to the same location with input (in-place processing).
 @param numberOfSamples Should be 32 minimum.
*/
    bool process(float *input, float *output, unsigned int numberOfSamples);

protected:
    filterInternals *internals;
    SuperpoweredFilter(const SuperpoweredFilter&);
    SuperpoweredFilter& operator=(const SuperpoweredFilter&);
};

#endif
