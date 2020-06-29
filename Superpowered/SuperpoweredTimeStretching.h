#ifndef Header_SuperpoweredTimeStretching
#define Header_SuperpoweredTimeStretching

#include "SuperpoweredAudioBuffers.h"

namespace Superpowered {

struct stretchInternals;

/// @brief Time stretching and pitch shifting.
/// One instance allocates around 220 kb memory.
class TimeStretching {
public:
    float rate;          ///< Time stretching rate (tempo). 1 means no time stretching. Maximum: 4. Values above 2 or below 0.5 are not recommended on mobile devices with low latency audio due high CPU load and risk of audio dropouts.
    int pitchShiftCents; ///< Pitch shift cents, limited from -2400 (two octaves down) to 2400 (two octaves up). Examples: 0 (no pitch shift), -100 (one note down), 300 (3 notes up).
                         ///< When the value if a multiply of 100 and is >= -1200 and <= 1200, changing the pitch shift needs only a few CPU clock cycles. Any change in pitchShiftCents involves significant momentary CPU load otherwise.
    unsigned int samplerate; ///< Sample rate in Hz. High quality pitch shifting requires 44100 Hz or more, the sound is "echoing" on lower sample rates.
    unsigned char sound; ///< Valid values are: 0 (best to save CPU with slightly lower audio quality), 1 (best for DJ apps, modern and "complete" music), 2 (best for instrumental loops and single instruments). Default: 1.
    Superpowered::AudiopointerList *outputList; ///< The AudiopointerList storing the audio output. To be used with the advancedProcess() method. Read only.
    
/// @brief Constructor.
/// @param samplerate The initial sample rate in Hz.
/// @param minimumRate The minimum value of rate. For example: if the rate will never go below 0.5, minimumRate = 0.5 will save significant computing power and memory. Minimum value of this: 0.01.
    TimeStretching(unsigned int samplerate, float minimumRate = 0.01f);
    ~TimeStretching();
    
/// @return Returns with how many frames of input should be provided to the time stretcher to produce some output.
/// It's never blocking for real-time usage. Use it in the same thread with the other real-time methods of this class.
/// The result can be 0 if rate is 1 and pitch shift is 0, because in that case the time stretcher is fully "transparent" and any number of input frames will produce some output.
    unsigned int getNumberOfInputFramesNeeded();
    
/// @return Returns with how many frames of output is available.
/// It's never blocking for real-time usage. Use it in the same thread with the other real-time methods of this class.
    unsigned int getOutputLengthFrames();
    
/// @brief Processes audio.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process(). Use it in the same thread with the other real-time methods of this class.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param numberOfFrames Number of frames to process.
    void addInput(float *input, int numberOfFrames);
    
/// @brief Gets the audio output into a buffer.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process(). Use it in the same thread with the other real-time methods of this class.
/// @return True if it has enough output frames stored and output is successfully written, false otherwise.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output.
/// @param numberOfFrames Number of frames to return with.
    bool getOutput(float *output, int numberOfFrames);
    
/// @brief Reset all internals, sets the instance as good as new.
/// Don't call this concurrently with process() and in a real-time thread.
    void reset();
    
/// @brief Processes audio. For advanced uses to:
/// 1. Save memory bandwidth (uses no memory copy) for maximum performance.
/// 2. Support up to 8 audio channels.
/// 3. Provide sub-sample precise positioning.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process(). Use it in the same thread with the other real-time methods of this class.
/// Do not use this method with addInput() or getOutput().
/// @param input The input buffer. It will take ownership on the input.
    void advancedProcess(AudiopointerlistElement *input);
    
/// @brief This class can handle one stereo audio channel pair by default (left+right). Maybe you need more if you load some music with 4 channels, then less if you load a regular stereo track.
/// Don't call this concurrently with process() and in a real-time thread.
/// @param numStereoPairs The number of stereo audio channel pairs. Valid values: one (stereo) to four (8 channels).
    void setStereoPairs(unsigned int numStereoPairs);
        
/// @brief Removes audio from the end of the input buffer. Can be useful for some looping use-cases for example.
/// It's never blocking for real-time usage. Use it in the same thread with the other real-time methods of this class.
/// @param numberOfFrames The number of frames to remove.
    void removeFramesFromInputBuffersEnd(unsigned int numberOfFrames);
    
private:
    stretchInternals *internals;
    TimeStretching(const TimeStretching&);
    TimeStretching& operator=(const TimeStretching&);
};

}

#endif
