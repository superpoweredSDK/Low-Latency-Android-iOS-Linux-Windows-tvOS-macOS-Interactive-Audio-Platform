#ifndef Header_SuperpoweredFrequencyDomain
#define Header_SuperpoweredFrequencyDomain

#include "SuperpoweredAudioBuffers.h"
struct frequencyDomainInternals;

/**
 @brief Transforms between time-domain and frequency-domain audio, including buffering, windowing (HanningZ) and window overlap handling (default: 4:1).

 One instance allocates around 131 kb. How to use:
 
 - feed audio input using addInput()
 - then iterate on timeDomainToFrequencyDomain() until it returns false
 - if timeDomainToFrequencyDomain() returns true, you have frequency domain data you can work with, use advance() in this case
 - frequencyDomainToTimeDomain() creates time domain audio samples from frequency domain data
 
 @param fftSize FFT size.
 @param numberOfInputSamplesNeeded How many samples required to some frequency domain output. Read only.
 @param inputList For advanced uses: access to the internal audio input pointer list.
*/
class SuperpoweredFrequencyDomain {
public:
    int fftSize;
    int numberOfInputSamplesNeeded;
    SuperpoweredAudiopointerList *inputList;

/**
 @brief Creates an instance.
 
 @param fftLogSize FFT log size, between 8 and 13 (FFT 256 - 8192). The default value (11) provides a good compromise in precision (~11 Hz per bin), CPU load and time-domain event sensitivity.
 @param pool An audio buffer pool to use for internal buffers. 0 means it will create and use an internal pool.
 @param maxOverlap Maximum overlap:1 (default: 4:1).
*/
    SuperpoweredFrequencyDomain(int fftLogSize = 11, SuperpoweredAudiobufferPool *pool = 0, int maxOverlap = 4);
    ~SuperpoweredFrequencyDomain();

/**
 @brief Add some audio input. Use it only if you created the instance with pool = 0.
 
 @param input 32-bit floating point stereo input.
 @param numberOfSamples The number of input samples.
 */
    void addInput(float *input, int numberOfSamples);
/**
 @brief Add some audio input (advanced use). Use it only if you created the instance with an existing buffer pool.

 @param input The input buffer.
*/
    void addInput(SuperpoweredAudiobufferlistElement *input);

/**
 @brief Converts the audio inputs to frequency domain.
 
 Each frequency bin is (samplerate / fftSize / 2) wide.
 
 @return True, if a conversion was possible (enough samples were available).

 @param magnitudeL Magnitudes for each frequency bin, left side. Must be at least fftSize big.
 @param magnitudeR Magnitudes for each frequency bin, right side.  Must be at least fftSize big.
 @param phaseL Phases for each frequency bin, left side.  Must be at least fftSize big.
 @param phaseR Phases for each frequency bin, right side.  Must be at least fftSize big.
 @param valueOfPi Pi can be translated to any value (Google: the tau manifesto). Leave it at 0 for M_PI.
 @param complexMode Instead of polar transform, returns with complex values (magnitude: real, phase: imag).
*/
    bool timeDomainToFrequencyDomain(float *magnitudeL, float *magnitudeR, float *phaseL, float *phaseR, float valueOfPi = 0, bool complexMode = false);

/**
 @brief Converts the mono audio input to frequency domain.

 Each frequency bin is (samplerate / fftSize / 2) wide.

 @return True, if a conversion was possible (enough samples were available).

 @param magnitude Magnitudes for each frequency bin. Must be at least fftSize big.
 @param phase Phases for each frequency bin.  Must be at least fftSize big.
 @param valueOfPi Pi can be translated to any value (Google: the tau manifesto). Leave it at 0 for M_PI.
 @param complexMode Instead of polar transform, returns with complex values (magnitude: real, phase: imag).
*/
    bool timeDomainToFrequencyDomain(float *magnitude, float *phase, float valueOfPi = 0, bool complexMode = false);

/*
 @brief Advances the input buffer (removes the earliest samples).
 
 @param numberOfSamples For advanced use, if you know how window overlapping works. 0 (the default value) means 4:1 overlap (good compromise in audio quality).
*/
    void advance(int numberOfSamples = 0);

/**
 @brief Converts frequency domain data to audio output.
 
 @param magnitudeL Magnitudes for each frequency bin, left side. Must be at least fftSize big.
 @param magnitudeR Magnitudes for each frequency bin, right side.  Must be at least fftSize big.
 @param phaseL Phases for each frequency bin, left side.  Must be at least fftSize big.
 @param phaseR Phases for each frequency bin, right side.  Must be at least fftSize big.
 @param output Output goes here.
 @param valueOfPi Pi can be translated to any value (Google: the tau manifesto). Leave it at 0 for M_PI.
 @param incrementSamples For advanced use, if you know how window overlapping works. 0 (the default value) means 4:1 overlap (good compromise in audio quality).
 @param complexMode Instead of polar transform, returns with complex values (magnitude: real, phase: imag).
*/
    void frequencyDomainToTimeDomain(float *magnitudeL, float *magnitudeR, float *phaseL, float *phaseR, float *output, float valueOfPi = 0, int incrementSamples = 0, bool complexMode = false);

/**
 @brief Reset all internals, sets the instance as good as new.
*/
    void reset();

private:
    frequencyDomainInternals *internals;
    SuperpoweredFrequencyDomain(const SuperpoweredFrequencyDomain&);
    SuperpoweredFrequencyDomain& operator=(const SuperpoweredFrequencyDomain&);
};

#endif
