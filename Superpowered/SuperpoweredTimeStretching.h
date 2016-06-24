#ifndef Header_SuperpoweredTimeStretching
#define Header_SuperpoweredTimeStretching

#include "SuperpoweredAudioBuffers.h"
struct stretchInternals;

/**
 @brief Time stretching and pitch shifting.
 
 One instance allocates around 220 kb. Check the SuperpoweredOfflineProcessingExample project on how to use.
 
 @param rate 1.0f means no time stretching. Read only.
 @param pitchShift Pitch shift notes, from -12 (one octave down) to 12 (one octave up). 0 means no pitch shift. Read only.
 @param pitchShiftCents Pitch shift cents, from -2400 (two octaves down) to 2400 (two octaves up). 0 means no pitch shift. Read only.
 @param numberOfInputSamplesNeeded How many samples required to some output. Read only.
*/
class SuperpoweredTimeStretching {
public:
    float rate;
    int pitchShift;
    int pitchShiftCents;
    int numberOfInputSamplesNeeded;
    
/**
 @brief Set rate and pitch shift. This method executes very quickly, in a few CPU cycles.
 
 @param newRate Limited to >= 0.01f and <= 4.0f. Values above 2.0f or below 0.5f are not recommended on mobile devices with low latency audio.
 @param newShift Limited to >= -12 and <= 12.
 */
    bool setRateAndPitchShift(float newRate, int newShift);

/**
 @brief Set rate and pitch shift with greater precision. Calling this method requires magnitudes more CPU than setRateAndPitchShift.

 @param newRate Limited to >= 0.01f and <= 4.0f. Values above 2.0f or below 0.5f are not recommended on mobile devices with low latency audio.
 @param newShiftCents Limited to >= -2400 and <= 2400.
*/
    bool setRateAndPitchShiftCents(float newRate, int newShiftCents);
    
/**
 @brief Create a time-stretching with the current sample rate and minimum rate value.
 */
    SuperpoweredTimeStretching(unsigned int samplerate, float minimumRate = 0.0f);
    ~SuperpoweredTimeStretching();

/**
 @brief This class handles one stereo audio channel pair by default. You can extend it to handle more.

 @param numStereoPairs The number of stereo audio channel pairs.
*/
    void setStereoPairs(unsigned int numStereoPairs);
/**
 @brief Sets the sample rate.
 
 @param samplerate 44100, 48000, etc.
 */
    void setSampleRate(unsigned int samplerate);
/**
 @brief Reset all internals, sets the instance as good as new.
 */
    void reset();
/**
 @brief Removes samples from the input buffer (good for looping for example).
 
 @param samples The number of samples to remove.
 */
    void removeSamplesFromInputBuffersEnd(unsigned int samples);

/**
 @brief Processes the audio.
 
 @param input The input buffer.
 @param outputList The output buffer list.
 
 @see @c SuperpoweredAudiopointerList
 */
    void process(SuperpoweredAudiobufferlistElement *input, SuperpoweredAudiopointerList *outputList);
    
private:
    stretchInternals *internals;
    SuperpoweredTimeStretching(const SuperpoweredTimeStretching&);
    SuperpoweredTimeStretching& operator=(const SuperpoweredTimeStretching&);
};

#endif
