#ifndef Header_SuperpoweredDecoder
#define Header_SuperpoweredDecoder

#include <stdint.h>
struct decoderInternals;
struct stemsCompressor;
struct stemsLimiter;

#define SUPERPOWEREDDECODER_EOF 0
#define SUPERPOWEREDDECODER_OK 1
#define SUPERPOWEREDDECODER_ERROR 2
#define SUPERPOWEREDDECODER_BUFFERING 3

typedef enum SuperpoweredDecoder_Kind {
    SuperpoweredDecoder_MP3, SuperpoweredDecoder_AAC, SuperpoweredDecoder_AIFF, SuperpoweredDecoder_WAV, SuperpoweredDecoder_MediaServer
} SuperpoweredDecoder_Kind;

/**
 @brief Callback for parsing ID3 frames. Compatible with 2.3 and 2.4 only.
 
 @param clientData Some custom pointer you set when you called getMetaData().
 @param frameName The ID3 frame name (four char).
 @param frameData The data inside the ID3 frame.
 @param frameDataSize Data size in bytes.
 */
typedef void (* SuperpoweredDecoderID3Callback) (void *clientData, void *frameName, void *frameData, int frameDataSize);

/**
 @brief Audio file decoder. Provides uncompresses PCM samples from various compressed formats.
 
 Thread safety: single threaded, not thread safe. After a succesful open(), samplePosition and duration may change.
 Supported file types:
 - Stereo or mono pcm WAV and AIFF (16-bit int, 24-bit int, 32-bit int or 32-bit IEEE float).
 - MP3 (all kind).
 - AAC-LC in M4A container (iTunes).
 - AAC-LC in ADTS container (.aac).
 - Apple Lossless (on iOS only).
 
 @param durationSeconds The duration of the current file in seconds. Read only.
 @param durationSamples The duration of the current file in samples. Read only.
 @param samplePosition The current position in samples. May change after each decode() or seekTo(). Read only.
 @param samplerate The sample rate of the current file. Read only.
 @param samplesPerFrame How many samples are in one frame of the source file. For example: 1152 in mp3 files.
 @param kind The format of the current file.
*/
class SuperpoweredDecoder {
public:
// READ ONLY properties
    double durationSeconds;
    int64_t durationSamples, samplePosition;
    unsigned int samplerate, samplesPerFrame;
    SuperpoweredDecoder_Kind kind;
    
/**
 @brief Opens a file for decoding.
 
 @param path Full file system path.
 @param metaOnly If true, it opens the file for fast metadata reading only, not for decoding audio. Available for fully available local files only (no network access).
 @param offset Byte offset in the file.
 @param length Byte length from offset. Set offset and length to 0 to read the entire file.
 @param stemsIndex Stems track index for Native Instruments Stems format.

 @return NULL if successful, or an error string.
 */
    const char *open(const char *path, bool metaOnly = false, int offset = 0, int length = 0, int stemsIndex = 0);
/**
 @brief Decodes the requested number of samples.
 
 @return End of file (0), ok (1) or error (2).
 
 @param pcmOutput The buffer to put uncompressed audio. Must be at least this big: (*samples * 4) + 16384 bytes.
 @param samples On input, the requested number of samples. Should be >= samplesPerFrame. On return, the samples decoded.
*/
    unsigned char decode(short int *pcmOutput, unsigned int *samples);
/**
 @brief Jumps to a specific position.
 
 @return The new position.
 
 @param sample The position (a sample index).
 @param precise Some codecs may not jump precisely due internal framing. Set precise to true if you want exact positioning (for a little performance penalty of 1 memmove).
*/
    int64_t seekTo(int64_t sample, bool precise);
/**
 @return Returns with the position where audio starts. This function changes position!
 
 @param limitSamples How far to search for. 0 means "the entire audio file".
 @param decibel Optional loudness threshold in decibel. 0 means "any non-zero audio sample". The value -49 is useful for vinyl rips.
 */
    unsigned int audioStartSample(unsigned int limitSamples = 0, int decibel = 0);
/**
 @brief Call this on a phone call or other interruption.
 
 Apple's built-in codec may be used in some cases, for example ALAC files.
 Call this after a media server reset or audio session interrupt to resume playback.
 */
    void reconnectToMediaserver();
/**
 @brief Returns with often used metadata.
 
 @param artist Artist, set to NULL if you're not interested. Returns NULL if can not be retrieved. Ownership passed (you must free() after finished using it).
 @param title Title, set to NULL if you're not interested. Returns NULL if can not be retrieved. Ownership passed (you must free() after finished using it).
 @param image Raw image data (usually PNG or JPG). Set to NULL if you're not interested. Returns NULL if can not be retrieved. Ownership passed (you must free() after finished using it).
 @param imageSizeBytes Size of the raw image data. Set to NULL if you're not interested.
 @param bpm Tempo in beats per minute. Set to NULL if you're not interested.
 @param callback A callback to process other ID3 frames. Set to NULL if you're not interested.
 @param clientData A custom pointer the callback receives.
 @param maxFrameDataSize The maximum frame size in bytes to receive by the callback, if the decoder can not memory map the entire audio file. Affects memory usage.
 */
    void getMetaData(char **artist, char **title, void **image, int *imageSizeBytes, float *bpm, SuperpoweredDecoderID3Callback callback, void *clientData, int maxFrameDataSize);
/**
 @return True if the file is Native Instruments Stems format.
 
 @param names Returns with 4 pointers of 0 terminated stem name strings or NULL if the file is not Stems. You take ownership (must free() after used.).
 @param colors Returns with 4 pointers of 0 terminated stem color strings or NULL if the file is not Stems. You take ownership (must free() after used.).
 @param compressor Struct to receive compressor DSP settings. Can be NULL.
 @param limiter Struct to receive limiter DSP settings. Can be NULL;
*/
    bool getStemsInfo(char *names[4] = 0, char *colors[4] = 0, stemsCompressor *compressor = 0, stemsLimiter *limiter = 0);

    SuperpoweredDecoder();
    ~SuperpoweredDecoder();
    
private:
    decoderInternals *internals;
    SuperpoweredDecoder(const SuperpoweredDecoder&);
    SuperpoweredDecoder& operator=(const SuperpoweredDecoder&);
};

/**
 @brief Helper function to parse ID3 text frames.
 
 @return A pointer to the text in UTF-8 encoding (you must free() it), or NULL if empty.
 
 @param frameData Frame data without the frame header.
 @param frameLength The data length in bytes.
 */
char *getID3TextFrameUTF8(unsigned char *frameData, int frameLength);

#endif
