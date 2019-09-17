#ifndef Header_SuperpoweredBitcrusher
#define Header_SuperpoweredBitcrusher

#include "SuperpoweredFX.h"

namespace Superpowered {
    
struct bitcrusherInternals;
    
/// @brief Bit crusher with adjustable frequency and bit depth. Simulates an old-school digital sound card.
/// It doesn't allocate any internal buffers and needs just a few bytes of memory.
class Bitcrusher: public FX {
public:
    unsigned int frequency; ///< Frequency in Hz, from 20 Hz to the half of the samplerate.
    unsigned char bits;     ///< Bit depth, from 1 to 16.
    
/// @brief Constructor. Enabled is false by default.
/// @param samplerate The initial sample rate in Hz.
    Bitcrusher(unsigned int samplerate);
    ~Bitcrusher();
    
/// @brief Processes the audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output. Can point to the same location with input (in-place processing).
/// @param numberOfFrames Number of frames to process. Recommendations for best performance: multiply of 4.
    bool process(float *input, float *output, unsigned int numberOfFrames);
        
private:
    bitcrusherInternals *internals;
    Bitcrusher(const Bitcrusher&);
    Bitcrusher& operator=(const Bitcrusher&);
};
    
}

#endif
