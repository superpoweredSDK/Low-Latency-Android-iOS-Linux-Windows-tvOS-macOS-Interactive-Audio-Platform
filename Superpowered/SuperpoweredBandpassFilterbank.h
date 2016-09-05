#ifndef Header_SuperpoweredBandpassFilterBank
#define Header_SuperpoweredBandpassFilterBank

struct bandpassFilterbankInternals;

/**
 @brief Efficient bandpass filter bank for real-time, zero latency frequency analysation.
*/
class SuperpoweredBandpassFilterbank {
public:

/**
 Create a filterbank instance.
 
 @param numBands The number of bands. Must be a multiply of 8.
 @param frequencies The center frequencies of the bands.
 @param widths The width of the bands. 1.0f is one octave, 1.0f / 12.0f is one halfnote.
 @param samplerate The initial sample rate.
 */
    SuperpoweredBandpassFilterbank(int numBands, float *frequencies, float *widths, unsigned int samplerate);
/**
 Create a filterbank instance for advanced "grouped" usage. 
 The filterbank can be set up with multiple settings (frequency/width groups), then process can be performed with the selected setting (group).

 @param numGroups the number of groups.
 @param numBandsInGroup The number of bands. Must be a multiply of 8.
 @param frequencies The center frequencies of the bands. Example: if you have numGroups = 2 and numBandsInGroup = 8, then you should provide 2 * 8 = 16 frequencies.
 @param widths The width of the bands. 1.0f is one octave, 1.0f / 12.0f is one halfnote. Example: if you have numGroups = 2 and numBandsInGroup = 8, then you should provide 2 * 8 = 16 widths.
 @param samplerate The initial sample rate.
*/
    SuperpoweredBandpassFilterbank(int numGroups, int numBandsInGroup, float *frequencies, float *widths, unsigned int samplerate);
    ~SuperpoweredBandpassFilterbank();

/**
 @brief Sets the sample rate.

 @param samplerate 44100, 48000, etc.
*/
    void setSamplerate(unsigned int samplerate);

/**
 @brief Processes the audio.

 @param input 32-bit interleaved stereo input buffer.
 @param bands Frequency band results (magnitudes) will be ADDED to these. For example: bands[0] += result[0] If you divide each with the number of samples, the result will be between 0.0f and 1.0f.
 @param peak Maximum absolute peak value. Will compare against the input value of peak too. For example: peak = max(peak, fabsf(all samples))
 @param sum The cumulated absolute value will be ADDED to this. For example: sum += (sum of all fabsf(samples))
 @param numberOfSamples The number of samples to process.
 @param group The group index for advanced "grouped" usage.
*/
    void process(float *input, float *bands, float *peak, float *sum, unsigned int numberOfSamples, int group = 0);
/**
 @brief Processes the audio.

 @param input 32-bit interleaved stereo input buffer.
 @param bands Frequency band results. If you divide each with the number of samples, the result will be between 0.0f and 1.0f.
 @param peak Maximum absolute peak value. Will compare against the input value of peak too. For example: peak = max(peak, fabsf(all samples))
 @param sum The cumulated absolute value will be ADDED to this. For example: sum += (sum of all fabsf(samples))
 @param numberOfSamples The number of samples to process.
 @param group The group index for advanced "grouped" usage.
 */
    void processNoAdd(float *input, float *bands, float *peak, float *sum, unsigned int numberOfSamples, int group = 0);

private:
    bandpassFilterbankInternals *internals;
    SuperpoweredBandpassFilterbank(const SuperpoweredBandpassFilterbank&);
    SuperpoweredBandpassFilterbank& operator=(const SuperpoweredBandpassFilterbank&);
};

#endif
