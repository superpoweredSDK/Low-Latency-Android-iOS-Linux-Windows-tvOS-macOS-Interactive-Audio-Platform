#ifndef Header_SuperpoweredFX
#define Header_SuperpoweredFX

namespace Superpowered {

/// @brief This is the base class for Superpowered effects.
class FX {
public:
    bool enabled = false;        ///< Turns the effect on/off. The actual switch will happen on the next process() call for smooth, audio-artifact free operation.
    unsigned int samplerate = 0; ///< Sample rate in Hz.

/// @brief Processes the audio.
/// Check the process() documentation of each derived class for the minimum number of samples and an optional vector size limitation. For maximum compatibility with all Superpowered effects, numberOfSamples should be minimum 32 and a multiply of 8.
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, it indicates silence. The contents of output are not changed in this case (not overwritten with zeros).
/// @param input 32-bit input buffer.
/// @param output 32-bit output buffer.
/// @param numberOfFrames Number of frames to process.
    virtual bool process(float *input, float *output, unsigned int numberOfFrames) = 0;
    
    virtual ~FX() {};
};

}

#endif
