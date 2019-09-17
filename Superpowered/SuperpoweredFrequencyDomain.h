#ifndef Header_SuperpoweredFrequencyDomain
#define Header_SuperpoweredFrequencyDomain

#include "SuperpoweredAudioBuffers.h"

namespace Superpowered {

struct frequencyDomainInternals;

/// @brief Transforms between time-domain and frequency-domain audio, including buffering, windowing (HanningZ) and window overlap handling (default: 4:1).
/// One instance allocates around 131 kb. How to use:
/// 1. Audio input using addInput().
/// 2. Call timeDomainToFrequencyDomain(), if it returns false go back to 1.
/// 3. The output of timeDomainToFrequencyDomain is frequency domain data you can work with.
/// 4. Call advance() (if required).
/// 5. Call frequencyDomainToTimeDomain() to create time domain audio from frequency domain data.
class FrequencyDomain {
public:
    Superpowered::AudiopointerList *inputList; ///< For advanced uses: access to the internal audio input pointer list.

/// @brief Constructor.
/// @param fftLogSize FFT log size, between 8 and 13 (FFT 256 - 8192). The default value (11) provides a good compromise in precision (~22 Hz per bin), CPU load and time-domain event sensitivity.
/// @param maxOverlap [Maximum overlap]:1 (default: 4:1).
    FrequencyDomain(unsigned int fftLogSize = 11, unsigned int maxOverlap = 4);
    ~FrequencyDomain();
    
/// @return Returns with how many frames of input should be provided to produce some output.
    unsigned int getNumberOfInputFramesNeeded();

/// @brief This class can handle one stereo audio channel pair by default (left+right). You can extend it to handle more.
/// @param numStereoPairs The number of stereo audio channel pairs. Valid values: one (stereo) to four (8 channels).
    void setStereoPairs(unsigned int numStereoPairs);

/// @brief Add some audio input.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param numberOfFrames The number of input frames.
    void addInput(float *input, int numberOfFrames);
    
/// @brief Add some audio input (advanced use).
/// @param input The input buffer.
    void addInput(AudiopointerlistElement *input);

/// @brief Converts the audio input (added by addInput()) to the frequency domain.
/// Each frequency bin is (samplerate / [FFT SIZE] / 2) wide.
/// @return True, if a conversion was possible (enough frames were available).
/// @param magnitudeL Pointer to floating point numbers. Magnitudes for each frequency bin, left side. Must be at least [FFT SIZE] big.
/// @param magnitudeR Pointer to floating point numbers. Magnitudes for each frequency bin, right side.  Must be at least [FFT SIZE] big.
/// @param phaseL Pointer to floating point numbers. Phases for each frequency bin, left side.  Must be at least [FFT SIZE] big.
/// @param phaseR Pointer to floating point numbers. Phases for each frequency bin, right side.  Must be at least [FFT SIZE] big.
/// @param valueOfPi Pi can be translated to any value (Google: the tau manifesto). Keep it at 0 for M_PI.
/// @param complexMode If true, then it returns with complex numbers (magnitude: real, phase: imag). Performs polar transform otherwise (the output is magnitudes and phases).
/// @param stereoPairIndex The index of the stereo pair to process.
    bool timeDomainToFrequencyDomain(float *magnitudeL, float *magnitudeR, float *phaseL, float *phaseR, float valueOfPi = 0, bool complexMode = false, int stereoPairIndex = 0);

/// @brief Converts mono audio input (added by addInput()) to the frequency domain.
/// Each frequency bin is (samplerate / [FFT SIZE] / 2) wide.
/// @return True, if a conversion was possible (enough frames were available).
/// @param magnitude Pointer to floating point numbers. Magnitudes for each frequency bin. Must be at least [FFT SIZE] big.
/// @param phase Pointer to floating point numbers. Phases for each frequency bin.  Must be at least [FFT SIZE] big.
/// @param valueOfPi Pi can be translated to any value (Google: the tau manifesto). Keep it at 0 for M_PI.
/// @param complexMode If true, then it returns with complex numbers (magnitude: real, phase: imag). Performs polar transform otherwise (the output is magnitudes and phases).
    bool timeDomainToFrequencyDomainMono(float *magnitude, float *phase, float valueOfPi = 0, bool complexMode = false);

/// @brief Advances the input buffer (removes the earliest frames).
/// @param numberOfFrames For advanced use, if you know how window overlapping works. Use 0 (the default value) otherwise for a 4:1 overlap (good compromise in audio quality).
    void advance(int numberOfFrames = 0);

/// @brief Converts frequency domain data to audio output.
/// @param magnitudeL Pointer to floating point numbers. Magnitudes for each frequency bin, left side. Must be at least [FFT SIZE] big.
/// @param magnitudeR Pointer to floating point numbers. Magnitudes for each frequency bin, right side. Must be at least [FFT SIZE] big.
/// @param phaseL Pointer to floating point numbers. Phases for each frequency bin, left side. Must be at least [FFT SIZE] big.
/// @param phaseR Pointer to floating point numbers. Phases for each frequency bin, right side. Must be at least [FFT SIZE] big.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output.
/// @param valueOfPi Pi can be translated to any value (Google: the tau manifesto). Leave it at 0 for M_PI.
/// @param incrementFrames For advanced use, if you know how window overlapping works. Use 0 (the default value) otherwise for a 4:1 overlap (good compromise in audio quality).
/// @param complexMode If true, then the magnitude and phase inputs represent complex numbers (magnitude: real, phase: imag).
/// @param stereoPairIndex The index of the stereo pair to process.
    void frequencyDomainToTimeDomain(float *magnitudeL, float *magnitudeR, float *phaseL, float *phaseR, float *output, float valueOfPi = 0, int incrementFrames = 0, bool complexMode = false, int stereoPairIndex = 0);

/// @brief Reset all internals, sets the instance as good as new.
    void reset();

private:
    frequencyDomainInternals *internals;
    FrequencyDomain(const FrequencyDomain&);
    FrequencyDomain& operator=(const FrequencyDomain&);
};

}

#endif
