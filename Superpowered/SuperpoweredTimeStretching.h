#ifndef Header_SuperpoweredTimeStretching
#define Header_SuperpoweredTimeStretching

#include "SuperpoweredAudioBuffers.h"
struct stretchInternals;

/**
 @brief Time stretching and pitch shifting.
 
 One instance allocates around 100 kb. Check the SuperpoweredOfflineProcessingExample project on how to use.
 
 @param rate 1.0f means no time stretching. Read only.
 @param pitchShift Should be -12 (one octave down) to 12 (one octave up). 0 means no pitch shift. Read only.
 @param numberOfInputSamplesNeeded How many samples required to some output. Read only.
*/
class SuperpoweredTimeStretching {
public:
    float rate;
    int pitchShift;
    int numberOfInputSamplesNeeded;
    
/**
 @brief Set rate and pitch shift with this.
 
 @param newRate Limited to >= 0.5f and <= 2.0f.
 @param newShift Limited to >= -12 and <= 12.
 */
    bool setRateAndPitchShift(float newRate, int newShift);
    
/**
 @brief Create a time-stretching instance with an audio buffer pool and the current sample rate.
 
 @see @c SuperpoweredAudiobufferPool
 */
    SuperpoweredTimeStretching(SuperpoweredAudiobufferPool *p, unsigned int samplerate);
    ~SuperpoweredTimeStretching();
    
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
