#ifndef Header_SuperpoweredFilter
#define Header_SuperpoweredFilter

#include "SuperpoweredFX.h"

namespace Superpowered {

struct filterInternals;

/// @brief Superpowered filter types and their effective parameters:
typedef enum FilterType {
    Resonant_Lowpass = 0,     ///< frequency, resonance
    Resonant_Highpass = 1,    ///< frequency, resonance
    Bandlimited_Bandpass = 2, ///< frequency, octave
    Bandlimited_Notch = 3,    ///< frequency, octave
    LowShelf = 4,             ///< frequency, slope, decibel
    HighShelf = 5,            ///< frequency, slope, decibel
    Parametric = 6,           ///< frequency, octave, decibel
    CustomCoefficients = 7
} FilterType;

/// @brief Filter is an IIR filter based on the typical direct form 1 formula:
/// y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2] - (a1/a0)*y[n-1] - (a2/a0)*y[n-2]
/// It doesn't allocate any internal buffers and needs just a few bytes of memory.
class Filter: public FX {
public:
    float frequency; ///< Frequency in Hz. From 1 Hz to the half of the current sample rate.
    float decibel;   ///< Decibel gain value for shelving and parametric filters. Limit: -96 to 24.
    float resonance; ///< Resonance value for resonant filters. Resonance = Q / 10. Limit: 0.01 to 1.
    float octave;    ///< Width in octave for bandlimited and parametric filters. Limit: 0.05 to 5.
    float slope;     ///< Slope value for shelving filters. Limit: 0.001 to 1.
    FilterType type; ///< Filter type. Changing the filter type often involves changing other parameters as well. Therefore in a real-time context change the parameters and the type in the same thread with the process() call.

/// @brief For advanced use. Set custom coefficients for the filter. Changes will be smoothly handled to prevent audio artifacts. Do not call this concurrently with process().
/// @param b0 b0/a0
/// @param b1 b1/a0
/// @param b2 b2/a0
/// @param a1 a1/a0
/// @param a2 a2/a0
    void setCustomCoefficients(float b0, float b1, float b2, float a1, float a2);
    
/// @brief Constructor. Enabled is false by default.
/// @param filterType The initial filter type.
/// @param samplerate The initial sample rate in Hz.
    Filter(FilterType filterType, unsigned int samplerate);
    ~Filter();
        
/// @brief Processes interleaved stereo audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process(). Do not call any method concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output. Can point to the same location with input (in-place processing).
/// @param numberOfFrames Number of frames to process. Recommendations for best performance: multiply of 4, minimum 64.
    bool process(float *input, float *output, unsigned int numberOfFrames);

/// @brief Processes mono audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process(). Do not call any method concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit mono input.
/// @param output Pointer to floating point numbers. 32-bit mono output. Can point to the same location with input (in-place processing).
/// @param numberOfFrames Number of frames to process. Recommendations for best performance: multiply of 4, minimum 64.
    bool processMono(float *input, float *output, unsigned int numberOfFrames);

protected:
    filterInternals *internals;
    Filter(const Filter&);
    Filter& operator=(const Filter&);
};

}

#endif
