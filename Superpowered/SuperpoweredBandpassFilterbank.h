#ifndef Header_SuperpoweredBandpassFilterBank
#define Header_SuperpoweredBandpassFilterBank

namespace Superpowered {

struct bandpassFilterbankInternals;

/// @brief Efficient bandpass filter bank for real-time zero latency frequency analysis. Each band is a separated bandpass filter with custom width and center frequency.
class BandpassFilterbank {
public:
    unsigned int samplerate; ///< Sample rate in Hz.
    float *bands;            ///< The magnitude of the frequency bands. Will be updated after each process() or processNoAdd() call.
    
    /// @brief Constructor.
    /// @param numBands The number of bands. Must be a multiply of 8.
    /// @param frequencies Center frequencies of each band in Hz.
    /// @param widths Widths of each band in octave (1.0f is one octave, 1.0f / 12.0f is one halfnote).
    /// @param samplerate The initial sample rate in Hz.
    /// @param numGroups For advanced use.
    /// The filter bank can be set up with multiple frequency + width groups, then process() or processNoAdd() can be performed with one specific frequency + width group. For example, set up one group with wide frequency coverage for the 20-20000 Hz range and three additional groups for 20-200 Hz, 200-2000 Hz and 2000-20000 Hz. When processing with the wide group of 20-20000 Hz and the highest magnitude can be found at 1000 Hz, use the 200-2000 Hz group for the next process() or processNoAdd() call, so the filter bank will have a "focus" on a narrower range.
    /// If numGroups > 0, then the number of frequencies and widths should be numGroups * numBands. Example: for numBands = 8 and numGroups = 2, provide 8 + 8 frequencies and 8 + 8 widths.
    BandpassFilterbank(unsigned int numBands, float *frequencies, float *widths, unsigned int samplerate, unsigned int numGroups = 0);

    ~BandpassFilterbank();

    /// @brief Processes the audio.
    /// It will ADD to the current magnitude in bands (like bands[0] += 0.123), so you can "measure" the magnitude of each frequency for a longer period of time.
    /// To calculate a result between 0 and 1 for multiple consecutive process() calls, divide each value in bands with the total number of frames passed to the consecutive process() calls.
    /// @param input 32-bit interleaved stereo input buffer.
    /// @param numberOfFrames Number of frames to process.
    /// @param group The group index for advanced "grouped" usage.
    void process(float *input, unsigned int numberOfFrames, int group = 0);
    
    /// @brief Processes the audio. It will replace the contents of bands.
    /// @param input 32-bit interleaved stereo input buffer.
    /// @param numberOfFrames Number of frames to process.
    /// @param group The group index for advanced "grouped" usage.
    void processNoAdd(float *input, unsigned int numberOfFrames, int group = 0);
    
    /// @brief Sets all values of bands to 0.
    void resetBands();
    
    /// @brief Returns with the average volume of all audio passed to all previous process() or processNoAdd() calls.
    float getAverageVolume();
    
    /// @brief Returns with the cumulated absolute value of all audio passed to all previous process() or processNoAdd() calls. Like you would add the absolute value of all audio samples together.
    float getSumVolume();
    
    /// @brief Resets the sum and average volume value to start measurement anew.
    void resetSumAndAverageVolume();
    
    /// @brief Returns with the peak volume of all audio passed to all previous process() or processNoAdd() calls.
    float getPeakVolume();
    
    /// @brief Resets the peak volume value to start measurement anew.
    void resetPeakVolume();
    
private:
    bandpassFilterbankInternals *internals;
    BandpassFilterbank(const BandpassFilterbank&);
    BandpassFilterbank& operator=(const BandpassFilterbank&);
};

}

#endif
