struct musicFileInternals;

#define SUPERPOWEREDDECODER_EOF 0
#define SUPERPOWEREDDECODER_OK 1
#define SUPERPOWEREDDECODER_ERROR 2

typedef enum SuperpoweredDecoder_Kind {
    SuperpoweredDecoder_MP3, SuperpoweredDecoder_AAC, SuperpoweredDecoder_AIFF, SuperpoweredDecoder_WAV, SuperpoweredDecoder_MediaServer
} SuperpoweredDecoder_Kind;

/**
 @brief Audio file decoder. Provides uncompresses PCM samples from various compressed formats.
 
 Thread safety: single threaded, not thread safe. After a succesful open(), samplePosition changes only.
 
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
    unsigned int durationSamples, samplePosition, samplerate, samplesPerFrame;
    SuperpoweredDecoder_Kind kind;
    
/**
 @brief Opens a file for decoding.
 
 @return NULL if successful, or an error string.
 */
    const char *open(const char *path);
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
    unsigned int seekTo(unsigned int sample, bool precise);
/**
 @brief Call this on a phone call or other interruption.
 
 Apple's built-in codec may be used in some cases, for example ALAC files.
 Call this after a media server reset or audio session interrupt to resume playback.
 */
    void reconnectToMediaserver();
    
/**
 @brief Lightweight constructor, doesn't do or allocate much. 
 
 @param mediaServerOnly Set it to true, if you don't want the internal codecs used (iOS only).
 */
    SuperpoweredDecoder(bool mediaServerOnly);
    ~SuperpoweredDecoder();
    
private:
    musicFileInternals *internals;
    SuperpoweredDecoder(const SuperpoweredDecoder&);
    SuperpoweredDecoder& operator=(const SuperpoweredDecoder&);
};
