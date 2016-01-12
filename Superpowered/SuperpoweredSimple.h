#ifndef Header_SuperpoweredSimple
#define Header_SuperpoweredSimple

/**
 @file SuperpoweredSimple.h
 @brief Simple fast utility functions for transforming audio.
 */


/**
 @fn SuperpoweredVolume(float *input, float *output, float volumeStart, float volumeEnd, unsigned int numberOfSamples);
 @brief Applies volume on a single stereo interleaved buffer.

 @param input Input buffer.
 @param output Output buffer. Can be equal to input (in-place processing).
 @param volumeStart Volume for the first sample.
 @param volumeEnd Volume for the last sample. Volume will be smoothly calculated between start end end.
 @param numberOfSamples The number of samples to process.
 */
void SuperpoweredVolume(float *input, float *output, float volumeStart, float volumeEnd, unsigned int numberOfSamples);

/**
 @fn SuperpoweredChangeVolume(float *input, float *output, float volumeStart, float volumeChange, unsigned int numberOfSamples);
 @brief Applies volume on a single stereo interleaved buffer.

 @param input Input buffer.
 @param output Output buffer. Can be equal to input (in-place processing).
 @param volumeStart Voume for the first sample.
 @param volumeChange Change volume by this amount for every sample.
 @param numberOfSamples The number of samples to process.
 */
void SuperpoweredChangeVolume(float *input, float *output, float volumeStart, float volumeChange, unsigned int numberOfSamples);

/**
 @fn SuperpoweredVolumeAdd(float *input, float *output, float volumeStart, float volumeEnd, unsigned int numberOfSamples);
 @brief Applies volume on a single stereo interleaved buffer and adds it to the audio in the output buffer.

 @param input Input buffer.
 @param output Output buffer.
 @param volumeStart Volume for the first sample.
 @param volumeEnd Volume for the last sample. Volume will be smoothly calculated between start end end.
 @param numberOfSamples The number of samples to process.
 */
void SuperpoweredVolumeAdd(float *input, float *output, float volumeStart, float volumeEnd, unsigned int numberOfSamples);

/**
 @fn SuperpoweredChangeVolumeAdd(float *input, float *output, float volumeStart, float volumeChange, unsigned int numberOfSamples);
 @brief Applies volume on a single stereo interleaved buffer and adds it to the audio in the output buffer.

 @param input Input buffer.
 @param output Output buffer.
 @param volumeStart Volume for the first sample.
 @param volumeChange Change volume by this amount for every sample.
 @param numberOfSamples The number of samples to process.
 */
void SuperpoweredChangeVolumeAdd(float *input, float *output, float volumeStart, float volumeChange, unsigned int numberOfSamples);

/**
 @fn SuperpoweredPeak(float *input, unsigned int numberOfValues);
 @return Returns with the peak value.

 @param input An array of floating point values.
 @param numberOfValues The number of values to process. (2 * numberOfSamples for stereo input) Must be a multiply of 8.
 */
float SuperpoweredPeak(float *input, unsigned int numberOfValues);

/**
 @fn SuperpoweredFloatToShortInt(float *input, short int *output, unsigned int numberOfSamples);
 @brief Converts a stereo interleaved 32-bit float input to stereo interleaved 16-bit signed integer output.

 @param input Stereo interleaved 32-bit input. Should be numberOfSamples * 2 + 16 big minimum.
 @param output Stereo interleaved 16-bit output. Should be numberOfSamples * 2 + 16 big minimum.
 @param numberOfSamples The number of samples to process.
 */
void SuperpoweredFloatToShortInt(float *input, short int *output, unsigned int numberOfSamples);

/**
 @fn SuperpoweredFloatToShortInt(float *inputLeft, float *inputRight, short int *output, unsigned int numberOfSamples);
 @brief Converts two 32-bit float input channels to stereo interleaved 16-bit signed integer output.

 @param inputLeft 32-bit input for the left side. Should be numberOfSamples + 8 big minimum.
 @param inputRight 32-bit input for the right side. Should be numberOfSamples + 8 big minimum.
 @param output Stereo interleaved 16-bit output. Should be numberOfSamples * 2 + 16 big minimum.
 @param numberOfSamples The number of samples to process.
 */
void SuperpoweredFloatToShortInt(float *inputLeft, float *inputRight, short int *output, unsigned int numberOfSamples);

/**
 @fn SuperpoweredShortIntToFloat(short int *input, float *output, unsigned int numberOfSamples, float *peaks);
 @brief Converts a stereo interleaved 16-bit signed integer input to stereo interleaved 32-bit float output.

 @param input Stereo interleaved 16-bit input. Should be numberOfSamples + 8 big minimum.
 @param output Stereo interleaved 32-bit output. Should be numberOfSamples + 8 big minimum.
 @param numberOfSamples The number of samples to process.
 @param peaks Peak value result (2 floats: left peak, right peak). Can be NULL if you are not interested.
 */
void SuperpoweredShortIntToFloat(short int *input, float *output, unsigned int numberOfSamples, float *peaks = NULL);

/**
 @fn SuperpoweredInterleave(float *left, float *right, float *output, unsigned int numberOfSamples);
 @brief Makes an interleaved output from two input channels.
 
 @param left Input for left channel.
 @param right Input for right channel.
 @param output Interleaved output.
 @param numberOfSamples The number of samples to process. Should be 4 minimum, must be exactly divisible with 4.
 */
void SuperpoweredInterleave(float *left, float *right, float *output, unsigned int numberOfSamples);

/**
 @fn SuperpoweredInterleaveAndGetPeaks(float *left, float *right, float *output, unsigned int numberOfSamples, float *peaks);
 @brief Makes an interleaved output from two input channels, and measures the input volume.

 @param left Input for left channel.
 @param right Input for right channel.
 @param output Interleaved output.
 @param numberOfSamples The number of samples to process. Should be 4 minimum, must be exactly divisible with 4.
 @param peaks Peak value result (2 floats: left peak, right peak).
 */
void SuperpoweredInterleaveAndGetPeaks(float *left, float *right, float *output, unsigned int numberOfSamples, float *peaks);

/**
 @fn SuperpoweredDeInterleave(float *input, float *left, float *right, unsigned int numberOfSamples);
 @brief Deinterleaves an interleaved input.

 @param input Interleaved input.
 @param left Input for left channel.
 @param right Input for right channel.

 @param numberOfSamples The number of samples to process. Should be 4 minimum, must be exactly divisible with 4.
 */
void SuperpoweredDeInterleave(float *input, float *left, float *right, unsigned int numberOfSamples);

/**
 @fn SuperpoweredHasNonFinite(float *buffer, unsigned int numberOfValues);
 @brief Checks if the samples has non-valid samples, such as infinity or NaN (not a number).
 
 @param buffer The buffer to check.
 @param numberOfValues Number of values in buffer. For stereo buffers, multiply by two!
 */
bool SuperpoweredHasNonFinite(float *buffer, unsigned int numberOfValues);

/**
 @fn SuperpoweredStereoToMono(float *input, float *output, float leftGainStart, float leftGainEnd, float rightGainStart, float rightGainEnd, unsigned int numberOfSamples);
 @brief Makes mono output from stereo input.

 @param input Stereo interleaved input.
 @param output Output.
 @param leftGainStart Gain of the first sample on the left channel.
 @param leftGainEnd Gain for the last sample on the left channel. Gain will be smoothly calculated between start end end.
 @param rightGainStart Gain of the first sample on the right channel.
 @param rightGainEnd Gain for the last sample on the right channel. Gain will be smoothly calculated between start end end.
 @param numberOfSamples The number of samples to process.
 */
void SuperpoweredStereoToMono(float *input, float *output, float leftGainStart, float leftGainEnd, float rightGainStart, float rightGainEnd, unsigned int numberOfSamples);

/**
 @fn SuperpoweredCrossMono(float *left, float *right, float *output, float leftGainStart, float leftGainEnd, float rightGainStart, float rightGainEnd, unsigned int numberOfSamples);
 @brief Makes mono output from two separate input channels.
 
 @param left Input for left channel.
 @param right Input for right channel.
 @param output Output.
 @param leftGainStart Gain of the first sample on the left channel.
 @param leftGainEnd Gain for the last sample on the left channel. Gain will be smoothly calculated between start end end.
 @param rightGainStart Gain of the first sample on the right channel.
 @param rightGainEnd Gain for the last sample on the right channel. Gain will be smoothly calculated between start end end.
 @param numberOfSamples The number of samples to process.
 */
void SuperpoweredCrossMono(float *left, float *right, float *output, float leftGainStart, float leftGainEnd, float rightGainStart, float rightGainEnd, unsigned int numberOfSamples);

/**
 @fn SuperpoweredCrossStereo(float *inputA, float *inputB, float *output, float gainStart[4], float gainEnd[4], unsigned int numberOfSamples);
 @brief Crossfades two separate input channels.

 @param inputA Interleaved stereo input (first).
 @param inputB Interleaved stereo input (second).
 @param output Interleaved stereo output.
 @param gainStart Gain of the first sample (first left, first right, second left, second right).
 @param gainEnd Gain for the last sample (first left, first right, second left, second right). Gain will be smoothly calculated between start end end.
 @param numberOfSamples The number of samples to process.
 */
void SuperpoweredCrossStereo(float *inputA, float *inputB, float *output, float gainStart[4], float gainEnd[4], unsigned int numberOfSamples);

/**
 @fn SuperpoweredVersion()
 @return Returns with the current version of the Superpowered SDK.
 
 The returned value is 3 unsigned chars: major,minor,revision Example: 1,0,0 means 1.0.0
 */
const unsigned char *SuperpoweredVersion();

#endif
