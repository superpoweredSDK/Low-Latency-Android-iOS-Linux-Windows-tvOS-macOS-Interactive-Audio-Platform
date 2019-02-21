#ifndef Header_SuperpoweredDelay
#define Header_SuperpoweredDelay

#include "SuperpoweredFX.h"
struct delayInternals;

/**
 @brief Simple delay with minimum memory operations.
 
 @param delayMs Delay in milliseconds.
 */
class SuperpoweredDelay {
public:
    float delayMs; // READ-WRITE parameter
    
/**
 @brief Create a delay instance.
 
 @param maximumDelayMs Maximum delay in milliseconds. Affects memory usage.
 @param maximumSamplerate Maximum sample rate to support. Affects memory usage.
 @param maximumNumberOfFramesToProcess Maximum number of frames for the process() call. Has minimum effect on memory usage.
 @param samplerate The current sample rate.
 */
    SuperpoweredDelay(unsigned int maximumDelayMs, unsigned int maximumSamplerate, unsigned int maximumNumberOfFramesToProcess, unsigned int samplerate);
    ~SuperpoweredDelay();
    
/**
 @brief Sets the sample rate.
     
 @param samplerate 44100, 48000, etc.
*/
    void setSamplerate(unsigned int samplerate); // 44100, 48000, etc.
    
/**
     @brief Reset all internals, sets the instance as good as new.
*/
    void reset();
    
/**
 @brief Processes the audio.
 
 @return Pointer the output having numberOfFrames audio available. It is valid until the next call to process().
 
 @param input 32-bit interleaved stereo input.
 @param numberOfFrames Number of frames to input and output.
 @param fx Optional. If NULL, then simple memory copy will be used to pass audio from input to the internal buffer. If not NULL, the audio processing of fx (such as a SuperpoweredFilter) will be used to pass audio from input to the internal buffer.
 */
    const float * const process(float *input, int numberOfFrames, SuperpoweredFX *fx = 0);
   
private:
    delayInternals *internals;
    SuperpoweredDelay(const SuperpoweredDelay&);
    SuperpoweredDelay& operator=(const SuperpoweredDelay&);
};

#endif
