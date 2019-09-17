#ifndef Header_SuperpoweredAudioBuffers
#define Header_SuperpoweredAudioBuffers

#include <stdint.h>

namespace Superpowered {

struct bufferPoolInternals;
struct pointerListInternals;

/// @brief Manages a global audio buffer pool, reducing the number of memory allocation requests, increasing efficiency of audio applications.
/// Check the offline example project on how to use.
class AudiobufferPool {
public:
/// @brief Initializes the buffer pool.
    static void initialize();
    
/// @brief Creates a buffer with retain count set to 1, similar to an NSObject in Objective-C. Use releaseBuffer() to release.
/// This method never blocks, never locks, is very fast and safe to use in a realtime thread. Can be called concurrently on any thread.
/// @return The buffer.
/// @param sizeBytes The requested buffer's size in bytes. Maximum: 8388608 (8 MB).
    static void *getBuffer(unsigned int sizeBytes);
    
/// @brief Release a buffer, similar to an NSObject in Objective-C (decreases the retain count by 1, the memory will be freed after retain count reaches 0).
 /// This method never blocks, never locks, is very fast and safe to use in a realtime thread. Can be called concurrently on any thread.
 /// @param buffer The buffer.
    static void releaseBuffer(void *buffer);
    
/// @brief Retains a buffer, similar to Objective-C (increases the retain count by 1, similar to an NSObject in Objective-C).
/// This method never blocks, never locks, is very fast and safe to use in a realtime thread. Can be called concurrently on any thread.
/// @param buffer The buffer.
    static void retainBuffer(void *buffer);

private:
    bufferPoolInternals *internals;
    AudiobufferPool(const AudiobufferPool&);
    AudiobufferPool& operator=(const AudiobufferPool&);
};

/// @brief An audio buffer list item.
typedef struct AudiopointerlistElement {
    void *buffers[4];       ///< The buffers, coming from Superpowered AudiobufferPool.
    int firstFrame;         ///< The index of the first frame in the buffer.
    int lastFrame;          ///< The index of last frame in the buffer. The length of the buffer: lastFrame - firstFrame.
    int64_t positionFrames; ///< Can be used to track position information.
    float framesUsed;       ///< Can be used to track how many "original" frames were used to create this chunk of audio. Useful for time-stretching or resampling to precisely track the movement of the playhead.
} AudiopointerlistElement;

/// @brief Manages an audio buffer list. Instead of circular buffers and too many memmove/memcpy, this object maintains an audio buffer "chain". You can append, insert, truncate, slice, extend, etc. this without the expensive memory operations.
/// Check the offline example project on how to use.
class AudiopointerList {
public:
/// @brief Creates an audio buffer list.
/// @param bytesPerFrame Frame size. For example: 4 for 16-bit stereo, 8 for 32-bit stereo audio.
/// @param initialNumElements Each list item uses 52 bytes memory. This number sets the initial memory usage of this list.
    AudiopointerList(unsigned int bytesPerFrame, unsigned int initialNumElements);
    ~AudiopointerList();
    
/// @brief Append a buffer to the end of the list. The list will increase the retain count of the buffer by 1, similar to Objective-C.
/// Not safe to use in a real-time thread, because it may use blocking memory operations.
    void append(AudiopointerlistElement *buffer);

/// @brief Insert a buffer before the beginning of the list. The list will increase the retain count of the buffer by 1, similar to Objective-C.
/// Not safe to use in a real-time thread, because it may use blocking memory operations.
    void insert(AudiopointerlistElement *buffer);
        
/// @brief Append all buffers to another buffer list. The anotherList will increase the retain count of all buffers by 1.
/// Not safe to use in a real-time thread, because it may use blocking memory operations.
    void copyAllBuffersTo(AudiopointerList *anotherList);
    
/// @brief Remove frames from the beginning. If all of a buffer's contents are from this list, it will decrease the buffer's retain count by 1.
/// Safe to use in a real-time thread.
/// @param numFrames The number of frames to remove.
    void removeFromStart(int numFrames);
    
/// @brief Remove frames from the end. If all of a buffer's contents are from this list, it will decrease the buffer's retain count by 1.
/// Safe to use in a real-time thread.
/// @param numFrames The number of frames to remove.
    void removeFromEnd(int numFrames);
    
/// @brief Remove everything from the list. It will decrease the retain count of all buffers by 1.
/// Safe to use in a real-time thread.
    void clear();
    
/// @return Returns with the length of audio in the list.
    int getLengthFrames();
    
/// @brief Returns the start position in an audio file or stream.
/// Safe to use in a real-time thread.
    int64_t getPositionFrames();
    
/// @brief Returns the end position in an audio file or stream, plus 1.
/// Safe to use in a real-time thread.
    int64_t getNextPositionFrames();
    
/// @brief Creates a "virtual slice" from this list.
/// Safe to use in a real-time thread.
/// @param fromFrame The slice will start from this frame.
/// @param lengthFrames The slice will contain this number of frames.
    bool makeSlice(int fromFrame, int lengthFrames);

/// @return This the slice's forward enumerator method to go through all buffers in it. Returns a pointer to the audio, or NULL.
/// Safe to use in a real-time thread.
/// @param lengthFrames Returns the number of frames in audio.
/// @param framesUsed Returns the number of original number of frames, creating this chunk of audio. Good for time-stretching for example, to track the movement of the playhead.
/// @param stereoPairIndex Index of AudiopointerlistElement.buffers.
    void *nextSliceItem(int *lengthFrames, float *framesUsed = 0, int stereoPairIndex = 0);
    
/// @return This the slice's backwards (reverse) enumerator method to go through all buffers in it. Returns a pointer to the audio, or NULL.
/// Safe to use in a real-time thread.
/// @param lengthFrames Returns the number of frames in audio.
/// @param framesUsed Returns the number of original number of frames, creating this chunk of audio. Good for time-stretching for example, to track the movement of the playhead.
/// @param stereoPairIndex Index of AudiopointerlistElement.buffers.
    void *prevSliceItem(int *lengthFrames, float *framesUsed = 0, int stereoPairIndex = 0);
    
/// @brief Returns the slice enumerator to the first buffer.
/// Safe to use in a real-time thread.
    void rewindSlice();
        
/// @brief Jumps the enumerator to the last buffer.
/// Safe to use in a real-time thread.
    void forwardToLastSliceBuffer();
    
/// @brief Returns the slice start position in an audio file or stream.
/// Safe to use in a real-time thread.
    int64_t getSlicePositionFrames();
    
private:
    pointerListInternals *internals;
    AudiopointerList(const AudiopointerList&);
    AudiopointerList& operator=(const AudiopointerList&);
};

}

#endif
