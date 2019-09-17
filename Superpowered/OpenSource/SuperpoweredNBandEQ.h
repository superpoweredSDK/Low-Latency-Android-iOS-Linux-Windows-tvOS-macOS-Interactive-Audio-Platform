#ifndef Header_SuperpoweredNBandEQ
#define Header_SuperpoweredNBandEQ

#include "SuperpoweredFX.h"
struct nbeqInternals;

/// @brief N-band equalizer.
/// It doesn't allocate any internal buffers and needs just a few bytes of memory.
class SuperpoweredNBandEQ: public Superpowered::FX {
public:
/// @brief Sets the gain for a frequency band.
/// @param index The index of the frequency band.
/// @param gainDecibels The gain of the frequency band in decibels.
	void setGainDb(unsigned int index, float gainDecibels);
    
/// @return Returns with the gain for a frequency band in decibels.
    float getBandDb(unsigned int index);

/// @brief Constructor. Enabled is false by default.
/// @param samplerate The initial sample rate in Hz.
/// @param frequencies 0-terminated list of frequency bands.
	SuperpoweredNBandEQ(unsigned int samplerate, float *frequencies);
	~SuperpoweredNBandEQ();
    
/// @brief Processes the audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output. Can point to the same location with input (in-place processing).
/// @param numberOfFrames Number of frames to process. Recommendation for best performance: multiply of 4, minimum 64.
	bool process(float *input, float *output, unsigned int numberOfFrames);

private:
	nbeqInternals *internals;
	SuperpoweredNBandEQ(const SuperpoweredNBandEQ&);
	SuperpoweredNBandEQ& operator=(const SuperpoweredNBandEQ&);
};

#endif

