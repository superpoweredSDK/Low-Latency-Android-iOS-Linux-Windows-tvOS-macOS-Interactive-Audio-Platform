#ifndef Header_SuperpoweredAudioBuffers
#define Header_SuperpoweredAudioBuffers

#include <stdint.h>

struct bufferPoolInternals;
struct pointerListInternals;
struct SuperpoweredAudiobufferlistElement;

/**
 @brief This object manages an audio buffer pool.
 
 It reduces the number of memory allocation requests, increasing efficiency of audio applications.
 Check the SuperpoweredOfflineProcessingExample project on how to use.
 */
class SuperpoweredAudiobufferPool {
public:
/**
 @brief Creates a buffer pool.
 
 @param bytesPerSample Sample size. For example: 2 for 16-bit, 4 for 32-bit audio.
 @param optimalCapacityBytes The optimal memory usage of the entire pool.
 @param freeUnusedSeconds If a buffer is unused, how many seconds to wait with freeing the memory used.
 */
    SuperpoweredAudiobufferPool(unsigned char bytesPerSample, int optimalCapacityBytes, int freeUnusedSeconds = 1);
    ~SuperpoweredAudiobufferPool();
   
/**
 @brief Creates a buffer, with 1 retain count.
 
 @return The buffer's identifier.
 
 @param sizeInSamples The buffer's size in samples, for 2 channels. For example, if you need a buffer for 512 samples of stereo audio, pass 512.
 */
    unsigned int createBuffer(unsigned int sizeInSamples);
/**
 @brief Release a buffer, similar to Objective-C.
 */
    void releaseBuffer(SuperpoweredAudiobufferlistElement *buffer);
/**
 @brief Retains a buffer, similar to Objective-C.
*/
    void retainBuffer(SuperpoweredAudiobufferlistElement *buffer);
/**
 @brief Returns with a pointer to the buffer's storage.
 */
    short int *int16Audio(SuperpoweredAudiobufferlistElement *buffer);
/**
 @brief Returns with a pointer to the buffer's storage.
 */
    float *floatAudio(SuperpoweredAudiobufferlistElement *buffer);
    
/**
 @brief Creates a buffer to be placed into a buffer list.
 
 @see SuperpoweredAudiobufferlistElement
 */
    bool createSuperpoweredAudiobufferlistElement(SuperpoweredAudiobufferlistElement *item, int64_t samplePosition, unsigned int sizeInSamples);

private:
    bufferPoolInternals *internals;
    SuperpoweredAudiobufferPool(const SuperpoweredAudiobufferPool&);
    SuperpoweredAudiobufferPool& operator=(const SuperpoweredAudiobufferPool&);
};


/**
 @brief This object manages an audio buffer list.
 
 Instead of circular buffers and too many memmove/memcpy, this object maintains an audio buffer "chain". You can append, insert, truncate, slice, extend, etc. this without the expensive memory operations.
 Check the SuperpoweredOfflineProcessingExample project on how to use.
 
 @param sampleLength The number of samples inside this list.
 */
class SuperpoweredAudiopointerList {
public:    
    int sampleLength;
    
/**
 @brief Creates an audio buffer list.
 
 @param bufPool The buffer pool, which will manage the memory allocation.
 */
    SuperpoweredAudiopointerList(SuperpoweredAudiobufferPool *bufPool);
    ~SuperpoweredAudiopointerList();
    
/**
 @brief Append a buffer to the end of the list.
 */
    void append(SuperpoweredAudiobufferlistElement *buffer);
/**
 @brief Insert a buffer before the beginning of the list.
 */
    void insert(SuperpoweredAudiobufferlistElement *buffer);
/**
 @brief Sets the last samples in the list to 1.0f, good for debugging purposes.
 */
    void markLastSamples();
/**
 @brief Remove everything from the list.
 */
    void clear();
/**
 @brief Appends all buffers to another buffer list.
 */
    void copyAllBuffersTo(SuperpoweredAudiopointerList *anotherList);
/**
 @brief Removes samples from the beginning or the end.
 
 @param numSamples The number of samples to remove.
 @param fromTheBeginning From the end or the beginning.
 */
    void truncate(int numSamples, bool fromTheBeginning);
/**
 @brief Returns the buffer list beginning's sample position in an audio file or stream.
 */
    int64_t startSamplePosition();
/**
 @brief Returns the buffer list end's sample position in an audio file or stream, plus 1.
*/
    int64_t nextSamplePosition();
    
/**
 @brief Creates a "virtual slice" from this list.
 
 @param fromSample The slice will start from this sample.
 @param lengthSamples The slice will contain this number of samples.
 */
    bool makeSlice(int fromSample, int lengthSamples);
/**
 @brief Returns the slice beginning's sample position in an audio file or stream.
 */
    int64_t samplePositionOfSliceBeginning();
    
/**
 @brief This the slice's forward enumerator method to go through all buffers in it.
 
 @param audio The pointer to the audio.
 @param lengthSamples Returns with the number of samples in audio.
 @param samplesUsed Returns with the number of original number of samples, creating this chunk of audio. Good for time-stretching for example, to track the movement of the playhead.
 */
    bool nextSliceItem(float **audio, int *lengthSamples, float *samplesUsed = 0);
/**
 @brief This the slice's forward enumerator method to go through all buffers in it.
 
 @param audio The pointer to the audio.
 @param lengthSamples Returns with the number of samples in audio.
 @param samplesUsed Returns with the number of original number of samples, creating this chunk of audio. Good for time-stretching for example, to track the movement of the playhead.
 */
    bool nextSliceItem(short int **audio, int *lengthSamples, float *samplesUsed = 0);
/**
 @brief Returns the slice enumerator to the first buffer.
 */
    void rewindSlice();
/**
 @brief Jumps the enumerator to the last buffer.
 */
    void forwardToLastSliceBuffer();
/**
 @brief This the slice's backwards (reverse) enumerator method to go through all buffers in it.
 
 @param audio The pointer to the audio.
 @param lengthSamples Returns with the number of samples in audio.
 @param samplesUsed Returns with the number of original number of samples, creating this chunk of audio. Good for time-stretching for example, to track the movement of the playhead.
*/
    bool prevSliceItem(float **audio, int *lengthSamples, float *samplesUsed = 0);
/**
 @brief This the slice's backwards (reverse) enumerator method to go through all buffers in it.
 
 @param audio The pointer to the audio.
 @param lengthSamples Returns with the number of samples in audio.
 @param samplesUsed Returns with the number of original number of samples, creating this chunk of audio. Good for time-stretching for example, to track the movement of the playhead.
 */
    bool prevSliceItem(short int **audio, int *lengthSamples, float *samplesUsed = 0);
    
private:
    pointerListInternals *internals;
    SuperpoweredAudiopointerList(const SuperpoweredAudiopointerList&);
    SuperpoweredAudiopointerList& operator=(const SuperpoweredAudiopointerList&);
};

/**
 @brief An audio buffer list item.
 
 @param samplePosition The buffer beginning's sample position in an audio file or stream.
 @param bufferID The buffer's identifier in a buffer pool.
 @param startSample The first sample in the buffer.
 @param endSample The last sample in the buffer.
 @param samplesUsed How many "original" samples were used to create this chunk of audio. Good for time-stretching for example, to track the movement of the playhead.
 */
typedef struct SuperpoweredAudiobufferlistElement {
    int64_t samplePosition;
	int bufferID, startSample, endSample;
    float samplesUsed;
} SuperpoweredAudiobufferlistElement;

#endif
