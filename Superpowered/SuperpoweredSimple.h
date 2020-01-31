#ifndef Header_SuperpoweredSimple
#define Header_SuperpoweredSimple

#include <stdio.h>

/// @file SuperpoweredSimple.h
/// @brief Fast utility functions for transforming audio.

namespace Superpowered {

/// @fn Volume(float *input, float *output, float volumeStart, float volumeEnd, unsigned int numberOfFrames);
/// @brief Applies volume on a single stereo interleaved buffer: output = input * gain
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output. Can be equal to input (in-place processing).
/// @param volumeStart Volume for the first frame.
/// @param volumeEnd Volume for the last frame. Volume will be smoothly calculated between the first and last frames.
/// @param numberOfFrames The number of frames to process.
void Volume(float *input, float *output, float volumeStart, float volumeEnd, unsigned int numberOfFrames);

/// @fn ChangeVolume(float *input, float *output, float volumeStart, float volumeChange, unsigned int numberOfFrames);
/// @brief Applies volume on a single stereo interleaved buffer: output = input * gain
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output. Can be equal to input (in-place processing).
/// @param volumeStart Voume for the first frame.
/// @param volumeChange Change volume by this amount for every frame.
/// @param numberOfFrames The number of frames to process.
void ChangeVolume(float *input, float *output, float volumeStart, float volumeChange, unsigned int numberOfFrames);

/// @fn VolumeAdd(float *input, float *output, float volumeStart, float volumeEnd, unsigned int numberOfFrames);
/// @brief Applies volume on a single stereo interleaved buffer and adds it to the audio in the output buffer: output = output + input * gain
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output.
/// @param volumeStart Volume for the first frame.
/// @param volumeEnd Volume for the last frame. Volume will be smoothly calculated between the first and last frames.
/// @param numberOfFrames The number of frames to process.
void VolumeAdd(float *input, float *output, float volumeStart, float volumeEnd, unsigned int numberOfFrames);

/// @fn ChangeVolumeAdd(float *input, float *output, float volumeStart, float volumeChange, unsigned int numberOfFrames);
/// @brief Applies volume on a single stereo interleaved buffer and adds it to the audio in the output buffer: output = output + input * gain
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output.
/// @param volumeStart Volume for the first frame.
/// @param volumeChange Change volume by this amount for every frame.
/// @param numberOfFrames The number of frames to process.
void ChangeVolumeAdd(float *input, float *output, float volumeStart, float volumeChange, unsigned int numberOfFrames);

/// @fn Peak(float *input, unsigned int numberOfValues);
/// @return Returns the peak absolute value. Useful for metering.
/// @param input Pointer to floating point numbers.
/// @param numberOfValues The number of values to process. For a stereo input this value should be 2 * numberOfFrames. Must be a multiply of 8.
float Peak(float *input, unsigned int numberOfValues);

/// @fn CharToFloat(signed char *input, float *output, unsigned int numberOfFrames, unsigned int numChannels);
/// @brief Converts 8-bit audio to 32-bit floating point.
/// @param input Pointer to signed bytes. 8-bit input.
/// @param output Pointer to floating point numbers. 32-bit output.
/// @param numberOfFrames The number of frames to process.
/// @param numChannels The number of channels.
void CharToFloat(signed char *input, float *output, unsigned int numberOfFrames, unsigned int numChannels = 2);

/// @fn FloatToChar(float *input, signed char *output, unsigned int numberOfFrames, unsigned int numChannels);
/// @brief Converts 32-bit floating point audio 8-bit audio.
/// @param input Pointer to floating point numbers. 32-bit input.
/// @param output Pointer to signed bytes. 8-bit output.
/// @param numberOfFrames The number of frames to process.
/// @param numChannels The number of channels.
void FloatToChar(float *input, signed char *output, unsigned int numberOfFrames, unsigned int numChannels = 2);

/// @fn Bit24ToFloat(void *input, float *output, unsigned int numberOfFrames, unsigned int numChannels);
/// @brief Converts 24-bit audio to 32-bit floating point.
/// @param input Input buffer pointer.
/// @param output Pointer to floating point numbers. 32-bit output.
/// @param numberOfFrames The number of frames to process.
/// @param numChannels The number of channels.
void Bit24ToFloat(void *input, float *output, unsigned int numberOfFrames, unsigned int numChannels = 2);

/// @fn FloatTo24bit(float *input, void *output, unsigned int numberOfFrames, unsigned int numChannels);
/// @brief Converts 32-bit floating point audio to 24-bit.
/// @param input Pointer to floating point numbers. 32-bit input.
/// @param output Output buffer pointer.
/// @param numberOfFrames The number of frames to process.
/// @param numChannels The number of channels.
void FloatTo24bit(float *input, void *output, unsigned int numberOfFrames, unsigned int numChannels = 2);

/// @fn IntToFloat(int *input, float *output, unsigned int numberOfFrames, unsigned int numChannels);
/// @brief Converts 32-bit integer audio to 32-bit floating point.
/// @param input Pointer to integer numbers. 32-bit input.
/// @param output Pointer to floating point numbers. 32-bit output.
/// @param numberOfFrames The number of frames to process.
/// @param numChannels The number of channels.
void IntToFloat(int *input, float *output, unsigned int numberOfFrames, unsigned int numChannels = 2);

/// @fn FloatToInt(float *input, int *output, unsigned int numberOfFrames, unsigned int numChannels);
/// @brief Converts 32-bit floating point audio to 32-bit integer.
/// @param input Pointer to floating point numbers. 32-bit input.
/// @param output Pointer to integer numbers. 32-bit output.
/// @param numberOfFrames The number of frames to process.
/// @param numChannels The number of channels.
void FloatToInt(float *input, int *output, unsigned int numberOfFrames, unsigned int numChannels = 2);

/// @fn FloatToShortInt(float *input, short int *output, unsigned int numberOfFrames, unsigned int numChannels);
/// @brief Converts 32-bit float input to 16-bit signed integer output.
/// @param input Pointer to floating point numbers. 32-bit input.
/// @param output Pointer to short integer numbers. 16-bit output.
/// @param numberOfFrames The number of frames to process.
/// @param numChannels The number of channels.
void FloatToShortInt(float *input, short int *output, unsigned int numberOfFrames, unsigned int numChannels = 2);

/// @fn FloatToShortIntInterleave(float *inputLeft, float *inputRight, short int *output, unsigned int numberOfFrames);
/// @brief Converts two 32-bit mono float input channels to stereo interleaved 16-bit signed integer output.
/// @param inputLeft Pointer to floating point numbers. 32-bit input for the left side. Should be numberOfFrames + 8 big minimum.
/// @param inputRight Pointer to floating point numbers. 32-bit input for the right side. Should be numberOfFrames + 8 big minimum.
/// @param output Pointer to short integer numbers. Stereo interleaved 16-bit output. Should be numberOfFrames * 2 + 16 big minimum.
/// @param numberOfFrames The number of frames to process.
void FloatToShortIntInterleave(float *inputLeft, float *inputRight, short int *output, unsigned int numberOfFrames);

/// @fn ShortIntToFloat(short int *input, float *output, unsigned int numberOfFrames, float *peaks);
/// @brief Converts stereo interleaved 16-bit signed integer input to stereo interleaved 32-bit float output, and provides peak measurement.
/// @param input Pointer to short integer numbers. Stereo interleaved 16-bit input. Should be numberOfFrames + 8 big minimum.
/// @param output Pointer to floating point numbers. Stereo interleaved 32-bit output. Should be numberOfFrames + 8 big minimum.
/// @param numberOfFrames The number of frames to process.
/// @param peaks Pointer to two floating point numbers. Peak value result (left, right).
void ShortIntToFloat(short int *input, float *output, unsigned int numberOfFrames, float *peaks);

/// @fn ShortIntToFloat(short int *input, float *output, unsigned int numberOfFrames, unsigned int numChannels);
/// @brief Converts 16-bit signed integer input to 32-bit float output.
/// @param input Pointer to short integer numbers. Stereo interleaved 16-bit input.
/// @param output Pointer to floating point numbers. Stereo interleaved 32-bit output.
/// @param numberOfFrames The number of frames to process.
/// @param numChannels The number of channels.
void ShortIntToFloat(short int *input, float *output, unsigned int numberOfFrames, unsigned int numChannels = 2);

/// @fn Interleave(float *left, float *right, float *output, unsigned int numberOfFrames);
/// @brief Makes an interleaved stereo output from two mono input channels: output = [L, R, L, R, ...]
/// @param left Pointer to floating point numbers. Mono input for left channel.
/// @param right Pointer to floating point numbers. Mono input for right channel.
/// @param output Pointer to floating point numbers. Stereo interleaved output.
/// @param numberOfFrames The number of frames to process.
void Interleave(float *left, float *right, float *output, unsigned int numberOfFrames);

/// @fn Interleave(float *left, float *right, float *output, unsigned int numberOfFrames);
/// @brief Makes an interleaved stereo output from two mono input channels and adds the result to the audio in the output buffer: output = output + [L, R, L, R, ...]
/// @param left Pointer to floating point numbers. Mono input for left channel.
/// @param right Pointer to floating point numbers. Mono input for right channel.
/// @param output Pointer to floating point numbers. Stereo interleaved output.
/// @param numberOfFrames The number of frames to process.
void InterleaveAdd(float *left, float *right, float *output, unsigned int numberOfFrames);

/// @fn InterleaveAndGetPeaks(float *left, float *right, float *output, unsigned int numberOfFrames, float *peaks);
/// @brief Makes an interleaved output from two input channels and measures the volume: output = [L, R, L, R, ...]
/// @param left Pointer to floating point numbers. Mono input for left channel.
/// @param right Pointer to floating point numbers. Mono input for right channel.
/// @param output Pointer to floating point numbers. Stereo interleaved output.
/// @param numberOfFrames The number of frames to process.
/// @param peaks Pointer to two floating point numbers. Peak value result (left, right).
void InterleaveAndGetPeaks(float *left, float *right, float *output, unsigned int numberOfFrames, float *peaks);

/// @fn DeInterleave(float *input, float *left, float *right, unsigned int numberOfFrames);
/// @brief Deinterleaves an interleaved stereo input to two mono output channels: left = [L, L, L, L, ...], right = [R, R, R, R, ...]
/// @param input Pointer to floating point numbers. Stereo interleaved input.
/// @param left Pointer to floating point numbers. Mono output for left channel.
/// @param right Pointer to floating point numbers. Mono output for right channel.
/// @param numberOfFrames The number of frames to process.
void DeInterleave(float *input, float *left, float *right, unsigned int numberOfFrames);

/// @fn DeInterleaveMultiply(float *input, float *left, float *right, unsigned int numberOfFrames, float multiplier);
/// @brief Deinterleaves an interleaved stereo input to two mono output channels and multiplies the output (gain): left = [L, L, L, L, ...] * gain, right = [R, R, R, R, ...] * gain
 /// @param input Pointer to floating point numbers. Stereo interleaved input.
 /// @param left Pointer to floating point numbers. Mono output for left channel.
 /// @param right Pointer to floating point numbers. Mono output for right channel.
 /// @param numberOfFrames The number of frames to process.
 /// @param multiplier Multiply each output sample with this value.
void DeInterleaveMultiply(float *input, float *left, float *right, unsigned int numberOfFrames, float multiplier);

/// @fn DeInterleaveAdd(float *input, float *left, float *right, unsigned int numberOfFrames);
/// @brief Deinterleaves an interleaved stereo input and adds the results to the two mono output channels: left = left + [L, L, L, L, ...], right = right + [R, R, R, R, ...]
/// @param input Pointer to floating point numbers. Stereo interleaved input.
/// @param left Pointer to floating point numbers. Mono output for left channel.
/// @param right Pointer to floating point numbers. Mono output for right channel.
/// @param numberOfFrames The number of frames to process.
void DeInterleaveAdd(float *input, float *left, float *right, unsigned int numberOfFrames);

/// @fn DeInterleaveMultiplyAdd(float *input, float *left, float *right, unsigned int numberOfFrames, float multiplier);
/// @brief Deinterleaves an interleaved stereo input to two mono output channels, multiplies the result (gain) and and adds the results to the two mono output channels: left = left + [L, L, L, L, ...] * gain, right = right + [R, R, R, R, ...] * gain
 /// @param input Pointer to floating point numbers. Stereo interleaved input.
 /// @param left Pointer to floating point numbers. Mono output for left channel.
 /// @param right Pointer to floating point numbers. Mono output for right channel.
 /// @param numberOfFrames The number of frames to process.
 /// @param multiplier Multiply each output sample with this value.
void DeInterleaveMultiplyAdd(float *input, float *left, float *right, unsigned int numberOfFrames, float multiplier);

/// @fn HasNonFinite(float *buffer, unsigned int numberOfValues);
/// @brief Checks if the audio samples has non-valid values, such as infinity or NaN (not a number).
/// @param buffer Pointer to floating point numbers to check.
/// @param numberOfValues Number of values in the buffer. Please note, this is NOT numberOfFrames. You need to provide the number of numbers in the buffer.
bool HasNonFinite(float *buffer, unsigned int numberOfValues);

/// @fn StereoToMono(float *input, float *output, float leftGainStart, float leftGainEnd, float rightGainStart, float rightGainEnd, unsigned int numberOfFrames);
/// @brief Makes mono output from stereo interleaved input: output = [L + R], [L + R], [L + R], ...
/// @param input Pointer to floating point numbers. Stereo interleaved input.
/// @param output Pointer to floating point numbers. Mono output.
/// @param leftGainStart Gain of the first sample on the left channel.
/// @param leftGainEnd Gain for the last sample on the left channel. Gain will be smoothly calculated between start end end.
/// @param rightGainStart Gain of the first sample on the right channel.
/// @param rightGainEnd Gain for the last sample on the right channel. Gain will be smoothly calculated between start end end.
/// @param numberOfFrames The number of frames to process.
void StereoToMono(float *input, float *output, float leftGainStart, float leftGainEnd, float rightGainStart, float rightGainEnd, unsigned int numberOfFrames);

/// @fn CrossMono(float *inputA, float *inputB, float *output, float inputAGainStart, float inputAGainEnd, float inputBGainStart, float inputBGainEnd, unsigned int numberOfFrames);
/// @brief Crossfades two mono input channels into a mono output: output = inputA * gain + inputB + gain
/// @param inputA Pointer to floating point numbers. First mono input.
/// @param inputB Pointer to floating point numbers. Second mono input.
/// @param output Pointer to floating point numbers. Mono output. Can be equal with one of the inputs (in-place processing).
/// @param inputAGainStart Gain of the first sample on the first input.
/// @param inputAGainEnd Gain for the last sample on the first input. Gain will be smoothly calculated between start end end.
/// @param inputBGainStart Gain of the first sample on the second input.
/// @param inputBGainEnd Gain for the last sample on the second input. Gain will be smoothly calculated between start end end.
/// @param numberOfFrames The number of frames to process.
void CrossMono(float *inputA, float *inputB, float *output, float inputAGainStart, float inputAGainEnd, float inputBGainStart, float inputBGainEnd, unsigned int numberOfFrames);

/// @fn CrossStereo(float *inputA, float *inputB, float *output, float inputAGainStart, float inputAGainEnd, float inputBGainStart, float inputBGainEnd, unsigned int numberOfFrames);
/// @brief Crossfades two stereo inputs into a stereo output: output = inputA * gain + inputB + gain
/// @param inputA Pointer to floating point numbers. Interleaved stereo input (first).
/// @param inputB Pointer to floating point numbers. Interleaved stereo input (second).
/// @param output Pointer to floating point numbers. Interleaved stereo output. Can be equal with one of the inputs (in-place processing).
/// @param inputAGainStart Gain of the first sample on the first input.
/// @param inputAGainEnd Gain for the last sample on the first input. Gain will be smoothly calculated between start end end.
/// @param inputBGainStart Gain of the first sample on the second input.
/// @param inputBGainEnd Gain for the last sample on the second input. Gain will be smoothly calculated between start end end.
/// @param numberOfFrames The number of frames to process.
void CrossStereo(float *inputA, float *inputB, float *output, float inputAGainStart, float inputAGainEnd, float inputBGainStart, float inputBGainEnd, unsigned int numberOfFrames);

/// @fn Add1(float *input, float *output, unsigned int numberOfItems)
/// @brief Adds the values in input to the values in output: output[n] += input[n]
/// @param input Pointer to floating point numbers. Input data.
/// @param output Pointer to floating point numbers. Output data.
/// @param numberOfItems The length of input.
void Add1(float *input, float *output, unsigned int numberOfItems);

/// @fn Add2(float *inputA, float *inputB, float *output, unsigned int numberOfItems)
/// @brief Adds the values in two inputs to the values in output: output[n] += inputA[n] + inputB[n]
/// @param inputA Pointer to floating point numbers. Input data.
/// @param inputB Pointer to floating point numbers. Input data.
/// @param output Pointer to floating point numbers. Output data.
/// @param numberOfItems The length of input.
void Add2(float *inputA, float *inputB, float *output, unsigned int numberOfItems);

/// @fn Add4(float *inputA, float *inputB, float *inputC, float *inputD, float *output, unsigned int numberOfItems)
/// @brief Adds the values in four inputs to the values in output: output[n] += inputA[n] + inputB[n] + inputC[n] + inputD[n]
/// @param inputA Pointer to floating point numbers. Input data.
/// @param inputB Pointer to floating point numbers. Input data.
/// @param inputC Pointer to floating point numbers. Input data.
/// @param inputD Pointer to floating point numbers. Input data.
/// @param output Pointer to floating point numbers. Output data.
/// @param numberOfItems The length of input.
void Add4(float *inputA, float *inputB, float *inputC, float *inputD, float *output, unsigned int numberOfItems);

/// @fn StereoToMidSide(float *input, float *output, unsigned int numberOfFrames)
/// @brief Converts a stereo signal to mid-side.
/// @param input Pointer to floating point numbers. Interleaved stereo input.
/// @param output Pointer to floating point numbers. Mid-side interleaved output. Can be equal to input (in-place processing).
/// @param numberOfFrames The number of frames to process.
void StereoToMidSide(float *input, float *output, unsigned int numberOfFrames);

/// @fn MidSideToStereo(float *input, float *output, unsigned int numberOfFrames)
/// @brief Converts a mid-side signal to stereo.
/// @param input Pointer to floating point numbers. Mid-side interleaved input.
/// @param output Pointer to floating point numbers. Interleaved stereo output. Can be equal to input (in-place processing).
/// @param numberOfFrames The number of frames to process.
void MidSideToStereo(float *input, float *output, unsigned int numberOfFrames);

/// @fn DotProduct(float *inputA, float *inputB, unsigned int numValues)
/// @brief Calculates the dot product of two vectors.
/// @param inputA Pointer to floating point numbers. First input vector.
/// @param inputB Pointer to floating point numbers. Second input vector.
/// @param numValues Number of value pairs to process.
/// @return The dot product.
float DotProduct(float *inputA, float *inputB, unsigned int numValues);

/// @fn float frequencyOfNote(int note);
/// @return Returns the frequency of a specific note.
/// @param note The number of the note. Note 0 is the standard A note at 440 Hz.
float frequencyOfNote(int note);

/// @fn FILE *createWAV(const char *path, unsigned int samplerate, unsigned char numChannels);
/// @brief Creates a 16-bit WAV file.
/// After createWAV, write audio data using the writeWAV() function or fwrite(). Close the file with the closeWAV() function.
/// Never use direct disk writing in a real-time audio processing thread, use the Superpowered Recorder class in that case.
/// @return A file handle (success) or NULL (error).
/// @param path The full filesystem path of the file.
/// @param samplerate Sample rate of the file in Hz.
/// @param numChannels The number of channels.
FILE *createWAV(const char *path, unsigned int samplerate, unsigned char numChannels);

/// @fn FILE *createWAVfd(int fd, unsigned int samplerate, unsigned char numChannels);
/// @brief Creates a 16-bit WAV file.
/// After createWAVfd, write audio data using the writeWAV() function or fwrite(). Close the file with the closeWAV() function.
/// Never use direct disk writing in a real-time audio processing thread, use the Superpowered Recorder class in that case.
/// @return A file handle (success) or NULL (error).
/// @param fd Existing file descriptor. Superpowered will fdopen on this using "w" mode.
/// @param samplerate Sample rate of the file in Hz.
/// @param numChannels The number of channels.
FILE *createWAVfd(int fd, unsigned int samplerate, unsigned char numChannels);

/// @fn bool writeWAV(FILE *fd, short int *audio, unsigned int numberOfBytes);
/// @brief Writes audio into a WAV file.
/// @return Returns true for success and false for error.
/// @param fd The file handle to write into.
/// @param audio Pointer to signed short integer numbers. Audio to write.
/// @param numberOfBytes The number of bytes to write.
bool writeWAV(FILE *fd, short int *audio, unsigned int numberOfBytes);

/// @fn void closeWAV(FILE *fd);
/// @brief Closes a 16-bit stereo WAV file.
/// fclose() is not enough to create a valid a WAV file, use this function to close it.
/// @param fd The file handle to close.
void closeWAV(FILE *fd);

/// @fn Version()
/// @return Returns the current version of the Superpowered SDK.
/// The returned value is: major version * 10000 + minor version * 100 + revision
/// Example: 10402 means 1.4.2
unsigned int Version();

}

#endif
