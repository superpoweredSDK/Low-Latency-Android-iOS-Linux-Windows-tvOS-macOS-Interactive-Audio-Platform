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
 @brief Let the system know that we are start using this class.
 */
    static void ping();
/**
 @brief Creates a buffer with retain count set to 1.
 
 @return The buffer.
 
 Never blocks, never locks, very fast and safe to use in a realtime thread -- only if the fixed memory region is able to satisfy the request.
 @see allocBuffer otherwise.
 Can be called concurrently.
 
 @param sizeBytes The buffer's size in bytes.
 */
    static void *getBuffer(unsigned int sizeBytes);
/**
 @brief Allocates a buffer using malloc, aligned to 16 bytes.  Retain count will be set to 1.
 
 Don't use this function in a realtime thread, as it calls malloc.
 The returned buffer however can safely be used and released in any kind of thread.
 Can not be called concurrently.
 
 @return The buffer.
 
 @param sizeBytes The buffer's size in bytes.
 */
    static void *allocBuffer(unsigned int sizeBytes);
/**
 @brief Release a buffer, similar to Objective-C.
 
 Never blocks, never locks, very fast and safe to use in a realtime thread. Can be called concurrently.
 
 @param buffer The buffer.
 */
    static void releaseBuffer(void *buffer);
/**
 @brief Retain a buffer, similar to Objective-C.
 
 Never blocks, never locks, very fast and safe to use in a realtime thread. Can be called concurrently.
 
 @param buffer The buffer.
*/
    static void retainBuffer(void *buffer);

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
 
 @param bytesPerSample Sample size. For example: 4 for 16-bit stereo, 8 for 32-bit stereo audio.
 @param typicalNumElements Each list item uses 28 bytes memory on a 64-bit device. This number sets the initial memory usage of this list.
 */
    SuperpoweredAudiopointerList(unsigned int bytesPerSample, unsigned int typicalNumElements);
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
 @return This the slice's forward enumerator method to go through all buffers in it. Returns with a pointer to the audio, or NULL.

 @param lengthSamples Returns with the number of samples in audio.
 @param samplesUsed Returns with the number of original number of samples, creating this chunk of audio. Good for time-stretching for example, to track the movement of the playhead.
 @param stereoPairIndex Not implemented yet.
 @param nextSamplePosition Not implemented yet.
 */
    void *nextSliceItem(int *lengthSamples, float *samplesUsed = 0, int stereoPairIndex = 0, int64_t *nextSamplePosition = 0);
/**
 @brief Returns the slice enumerator to the first buffer.
 */
    void rewindSlice();
/**
 @brief Jumps the enumerator to the last buffer.
 */
    void forwardToLastSliceBuffer();
/**
 @return This the slice's backwards (reverse) enumerator method to go through all buffers in it. Returns with a pointer to the audio, or NULL.

 @param lengthSamples Returns with the number of samples in audio.
 @param samplesUsed Returns with the number of original number of samples, creating this chunk of audio. Good for time-stretching for example, to track the movement of the playhead.
 @param stereoPairIndex Not implemented yet.
*/
    void *prevSliceItem(int *lengthSamples, float *samplesUsed = 0, int stereoPairIndex = 0);
private:
    pointerListInternals *internals;
    SuperpoweredAudiopointerList(const SuperpoweredAudiopointerList&);
    SuperpoweredAudiopointerList& operator=(const SuperpoweredAudiopointerList&);
};

/**
 @brief An audio buffer list item.

 @param buffers The buffers, coming from SuperpoweredAudiobufferPool.
 @param samplePosition The buffer beginning's sample position in an audio file or stream.
 @param startSample The first sample in the buffer.
 @param endSample The last sample in the buffer.
 @param samplesUsed How many "original" samples were used to create this chunk of audio. Good for time-stretching for example, to track the movement of the playhead.
 */
typedef struct SuperpoweredAudiobufferlistElement {
    void *buffers[4];
    int64_t samplePosition;
	int startSample, endSample;
    float samplesUsed;
} SuperpoweredAudiobufferlistElement;

#endif
