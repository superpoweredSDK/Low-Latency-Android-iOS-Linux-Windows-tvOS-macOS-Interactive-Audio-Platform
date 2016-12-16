#ifndef Header_SuperpoweredNBandEQ
#define Header_SuperpoweredNBandEQ

#include "SuperpoweredFX.h"
struct nbeqInternals;

/**
 @brief N-band equalizer.

 It doesn't allocate any internal buffers and needs just a few bytes of memory.

 @param decibels Gain for each frequency band in decibels. Bandwidths are automatically calculated.
 @param enabled True if the effect is enabled (processing audio). Read only. Use the enable() method to set.
 */
class SuperpoweredNBandEQ: public SuperpoweredFX {
public:
	float *decibels; // READ-ONLY parameter.

/**
 @brief Sets the gain for a frequency band.

 @param index The index of the frequency band.
 @param gainDecibels The gain of the frequency band in decibels.
 */
	void setBand(unsigned int index, float gainDecibels);

/**
 @brief Turns the effect on/off.
 */
	void enable(bool flag);

/**
 @brief Create an eq instance.

 Enabled is false by default, use enable(true) to enable. Example: SuperpoweredNBandEQ eq = new SuperpoweredNBandEQ(44100);

 @param samplerate 44100, 48000, etc.
 @param frequencies 0-terminated list of frequency bands.
*/
	SuperpoweredNBandEQ(unsigned int samplerate, float *frequencies);
	~SuperpoweredNBandEQ();

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
 @param numberOfSamples Should be 32 minimum.
*/
	bool process(float *input, float *output, unsigned int numberOfSamples);

private:
	nbeqInternals *internals;
	SuperpoweredNBandEQ(const SuperpoweredNBandEQ&);
	SuperpoweredNBandEQ& operator=(const SuperpoweredNBandEQ&);
};

#endif

