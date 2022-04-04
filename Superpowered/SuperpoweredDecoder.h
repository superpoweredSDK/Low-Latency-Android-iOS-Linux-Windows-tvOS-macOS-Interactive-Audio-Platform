#ifndef Header_SuperpoweredDecoder
#define Header_SuperpoweredDecoder

#ifndef JSWASM
#define JSWASM
#endif

#include "SuperpoweredHTTP.h"
#include <stdint.h>

namespace Superpowered {

struct decoderInternals;

/// @brief Audio file decoder. Provides raw PCM audio and metadata from various compressed formats.
/// Supported file types:
/// - Stereo or mono pcm WAV and AIFF (16-bit int, 24-bit int, 32-bit int or 32-bit IEEE float).
/// - MP3: MPEG-1 Layer III (sample rates: 32000 Hz, 44100 Hz, 48000 Hz). MPEG-2 Layer III is not supported (mp3 with sample rates below 32000 Hz).
/// - AAC or HE-AAC in M4A container (iTunes) or ADTS container (.aac).
/// - ALAC/Apple Lossless (iOS/macOS/tvOS only).
class Decoder {
public:
    /// @brief File/decoder format.
    typedef enum Format {
        Format_MP3 = 0,  ///< MP3
        Format_AAC = 1,  ///< AAC, HE-AAC
        Format_AIFF = 2, ///< AIFF
        Format_WAV = 3,  ///< WAV
        Format_MediaServer = 4, ///< Other format decoded by iOS, macOS or tvOS.
        Format_HLS = 5   ///< HTTP Live Streaming
    } Format;

/// @brief Error codes for the decodeAudio(), getAudioStartFrame() and getAudioEndFrame() methods.
    static const int EndOfFile = 0;               ///< End-of-file reached.
    static const int BufferingTryAgainLater = -1; ///< Buffering (waiting for the network to pump enough data).
    static const int NetworkError = -2;           ///< Network (download) error.
    static const int Error = -3;                  ///< Decoding error.

/// @brief Error codes for the open...() methods.
    static const int OpenSuccess = 0;                          ///< Opened successfully.
    static const int OpenError_OutOfMemory =             1000; ///< Some memory allocation failed. Recommended action: check for memory leaks.
    static const int OpenError_PathIsNull =              1001; ///< Path is NULL.
    static const int OpenError_FastMetadataNotLocal =    1002; ///< metaOnly was true, but it works on local files only.
    static const int OpenError_ID3VersionNotSupported =  1003; ///< ID3 version 2.2, 2.3 and 2.4 are supported only.
    static const int OpenError_ID3ReadError =            1004; ///< File read error while reading the ID3 tag.
    static const int OpenError_FileFormatNotRecognized = 1005; ///< The decoder is not able to decode this file format.
    static const int OpenError_FileOpenError =           1006; ///< Such as fopen() failed. Recommended action: check if the path and read permissions are correct.
    static const int OpenError_FileLengthError =         1007; ///< Such as fseek() failed. Recommended action: check if read permissions are correct.
    static const int OpenError_FileTooShort =            1008; ///< The file has just a few bytes of data.
    static const int OpenError_AppleAssetFailed =        1009; ///< The Apple system codec could not create an AVURLAsset on the resource. Wrong file format?
    static const int OpenError_AppleMissingTracks =      1010; ///< The Apple system codec did not found any audio tracks. Wrong file format?
    static const int OpenError_AppleDecoding =           1011; ///< The Apple system codec could not get the audio frames. Wrong file format?
    static const int OpenError_ImplementationError0 =    1012; ///< Should never happen. But if it does, please let us know.
    static const int OpenError_ImplementationError1 =    1013; ///< Should never happen. But if it does, please let us know.
    static const int OpenError_ImplementationError2 =    1014; ///< Should never happen. But if it does, please let us know.
    static const int OpenError_UseSetTempFolder =        1015; ///< Call AdvancedAudioPlayer::setTempFolder first.

    bool HLSAutomaticAlternativeSwitching;   ///< If true, then the decoder will automatically switch between the HLS alternatives according to the available network bandwidth. Default: true.
    int HLSMaximumDownloadAttempts;          ///< How many times to retry if a HLS segment download fails. Default: 100.
    int HLSBufferingSeconds;                 ///< How many seconds ahead of the playback position to download. Default value: HLS_DOWNLOAD_REMAINING.

/// @brief Creates an instance of Superpowered Decoder.
    JSWASM Decoder();
    JSWASM ~Decoder();

/// @return Returns with a human readable error string. If the code is not a decoder status code, then it's a SuperpoweredHTTP status code and returns with that.
/// @param code The return value of the open...() method.
    JSWASM static const char *statusCodeToString(int code);

/// @return Returns with the duration of the current file in frames.
/// @warning Duration may change after each decode() or seekTo(), because some audio formats doesn't contain precise length information.
    JSWASM int getDurationFrames();

/// @return Returns with the duration of the current file in seconds.
/// @warning Duration may change after each decode() or seekTo(), because some audio formats doesn't contain precise length information.
    JSWASM double getDurationSeconds();

/// @return Returns with the sample rate of the current file.
    JSWASM unsigned int getSamplerate();

/// @return Returns with the format of the current file.
    JSWASM Format getFormat();

/// @return Returns with how many frames are in one chunk of the source file. For example: MP3 files store 1152 audio frames in a chunk.
    JSWASM unsigned int getFramesPerChunk();

/// @return Returns with the current position in frames. The postion may change after each decode() or seekTo().
    JSWASM int getPositionFrames();

/// @return Indicates which part of the file has fast access. For local files it will always be 0.0f.
    float getBufferedStartPercent();

/// @return Indicates which part of the file has fast access. For local files it will always be 1.0f.
    float getBufferedEndPercent();

/// @return The file system path of the fully downloaded audio file for progressive downloads. May be NULL until the download finishes.
    const char *getFullyDownloadedPath();

/// @return Returns with the current download speed in bytes per second.
    unsigned int getCurrentBps();

/// @brief Opens a file for decoding.
/// @return The return value can be grouped into four categories:
/// - Decoder::OpenSuccess: successful open.
/// - httpResponse::StatusCode_Progress: the open method needs more time opening an audio file from the network. In this case retry open() later until it returns something else than StatusCode_Progress (it's recommended to wait at least 0.1 seconds).
/// - A value above 1000: internal decoder error.
/// - A HTTP status code for network errors.
/// @param path Full file system path or progressive download path (http or https).
/// @param metaOnly If true, it opens the file for fast metadata reading only, not for decoding audio. Available for fully available local files only (no network access).
/// @param offset Byte offset in the file. Primarily designed to open raw files from an apk.
/// @param length Byte length from offset. Set offset and length to 0 to read the entire file.
/// @param stemsIndex Stems track index for Native Instruments Stems format.
/// @param customHTTPRequest If custom HTTP communication is required (such as sending http headers for authorization), pass a fully prepared http request object. The decoder will copy this object.
    int open(const char *path, bool metaOnly = false, int offset = 0, int length = 0, int stemsIndex = 0, Superpowered::httpRequest *customHTTPRequest = 0);

/// @brief Opens a memory location for decoding. @see open() for the return value.
/// @param pointer Pointer to an audio file loaded onto the heap. Should be allocated using malloc() (and not _aligned_malloc() or similar). The Decoder will take ownership on this data.
/// @param sizeBytes The audio file length in bytes.
/// @param metaOnly If true, it opens the file for fast metadata reading only, not for decoding audio.
    JSWASM int openAudioFileInMemory(void *pointer, unsigned int sizeBytes, bool metaOnly = false);

/// @brief Opens a memory location in Superpowered AudioInMemory format for decoding. @see open() for the return value.
/// @param pointer Pointer to information in Superpowered AudioInMemory format. Ownership is controlled by the AudioInMemory format.
/// @param metaOnly If true, it opens the file for fast metadata reading only, not for decoding audio.
    JSWASM int openMemory(void *pointer, bool metaOnly = false);

/// @brief Opens a HTTP Live Streaming stream. @see open() for the return value.
/// @param url Stream URL.
/// @param liveLatencySeconds When connecting or reconnecting to a HLS live stream, the decoder will try to skip audio to maintain this latency. Default: -1 (the decoder wil not skip audio and the live stream starts at the first segment specified by the server).
/// @param customHTTPRequest If custom HTTP communication is required (such as sending http headers for authorization), pass a fully prepared http request object. The decoder will copy this object.
    int openHLS(const char *url, char liveLatencySeconds = -1, Superpowered::httpRequest *customHTTPRequest = 0);

/// @brief Decodes audio.
/// @return The return value can be:
/// - A positive number above zero: the number of frames decoded.
/// - Decoder::EndOfFile: end of file reached, there is nothing more to decode. Use setPosition.
/// - Decoder::BufferingTryAgainLater: the decoder needs more data to be downloaded. Retry decodeAudio() later (it's recommended to wait at least 0.1 seconds).
/// - Decoder::NetworkError: network (download) error.
/// - Decoder::Error: internal decoder error.
/// @param output Pointer to allocated memory. The output. Must be at least numberOfFrames * 4 + 16384 bytes big.
/// @param numberOfFrames The requested number of frames. Should NOT be less than the value returned by getFramesPerChunk().
    JSWASM int decodeAudio(short int *output, unsigned int numberOfFrames);

/// @brief Decodes an entire audio file in memory to AudioInMemory format in a single call. The output can be loaded by the AdvancedAudioPlayer.
/// @return Pointer or NULL on error.
/// @param pointer Pointer to an audio file loaded onto the heap. Should be allocated using malloc() (and not _aligned_malloc() or similar). The Decoder will free() this data.
/// @param sizeBytes The audio file length in bytes.
    JSWASM static void *decodeToAudioInMemory(void *pointer, unsigned int sizeBytes);

/// @brief Jumps to the specified position's chunk (frame) beginning.
/// Some codecs (such as MP3) contain audio in chunks (frames). This method will not jump precisely to the specified position, but to the chunk's beginning the position belongs to.
/// @return Returns with success (true) or failure (false).
/// @param positionFrames The requested position.
    JSWASM bool setPositionQuick(int positionFrames);

/// @brief Jumps to a specific position.
/// This method is a little bit slower than setPositionQuick().
/// @return Returns with success (true) or failure (false).
/// @param positionFrames The requested position.
    JSWASM bool setPositionPrecise(int positionFrames);

/// Detects silence at the beginning.
/// @warning This function changes the position!
/// @return The return value can be:
/// - A positive number or zero: the frame index where audio starts.
/// - Decoder::BufferingTryAgainLater: the decoder needs more data to be downloaded. Retry getAudioStartFrame() or getAudioEndFrame() later (it's recommended to wait at least 0.1 seconds).
/// - Decoder::NetworkError: network (download) error.
/// - Decoder::Error: internal decoder error.
/// @param limitFrames Optional. How far to search for. 0 means "the entire audio file".
/// @param thresholdDb Optional. Loudness threshold in decibel. 0 means "any non-zero audio". The value -49 is useful for vinyl rips starting with vinyl noise (crackles).
    JSWASM int getAudioStartFrame(unsigned int limitFrames = 0, int thresholdDb = 0);

/// Detects silence at the end.
/// @warning This function changes the position!
/// @return The return value can be:
/// - A positive number or zero: the frame index where audio ends.
/// - Decoder::BufferingTryAgainLater: the decoder needs more data to be downloaded. Retry getAudioEndFrame() later (it's recommended to wait at least 0.1 seconds).
/// - Decoder::NetworkError: network (download) error.
/// - Decoder::Error: internal decoder error.
/// @param limitFrames Optional. How far to search for from the end (the duration in frames). 0 means "the entire audio file".
/// @param thresholdDb Optional. Loudness threshold in decibel. 0 means "any non-zero audio". The value -49 is useful for vinyl rips having vinyl noise (crackles).
    JSWASM int getAudioEndFrame(unsigned int limitFrames = 0, int thresholdDb = 0);

/// @return Returns with the JSON metadata string for the Native Instruments Stems format, or NULL if the file is not Stems.
    JSWASM const char *getStemsJSONString();

/// @brief Parses all ID3 frames. Do not use startReadingID3/readNextID3Frame after this.
/// @param skipImages Parsing ID3 image frames is significantly slower than parsing other frames. Set this to true if not interested in image information to save CPU.
/// @param maxFrameDataSize The maximum frame size in bytes to retrieve if the decoder can not memory map the entire audio file. Affects memory usage. Useful to skip large payloads (such as images).
    JSWASM void parseAllID3Frames(bool skipImages, unsigned int maxFrameDataSize);

/// @brief Starts reading ID3 frames. Use readNextID3Frame() in an iteration after this.
/// @param skipImages Parsing ID3 image frames is significantly slower than parsing other frames. Set this to true if not interested in image information to save CPU.
/// @param maxFrameDataSize The maximum frame size in bytes to retrieve if the decoder can not memory map the entire audio file. Affects memory usage. Useful to skip large payloads (such as images).
    JSWASM void startParsingID3Frames(bool skipImages, unsigned int maxFrameDataSize);

/// @return Reads the next ID3 frame and returns with its data size or -1 if finished reading.
    JSWASM unsigned int readNextID3Frame();

/// @return Returns with the current ID3 frame name (four char). To be used with readNextID3Frame().
    JSWASM unsigned int getID3FrameName();

/// @return Returns with the raw data of the current ID3 frame. To be used with readNextID3Frame().
    JSWASM void *getID3FrameData();

/// @return Returns with the current ID3 frame data length.
    JSWASM unsigned int getID3FrameDataLengthBytes();

/// @brief Returns with the text inside the current ID3 frame. To be used with readNextID3Frame().
/// @return A pointer to the text in UTF-8 encoding (you take ownership on the data, don't forget to free() when done to prevent memory leaks), or NULL if empty.
/// @warning Use it for frames containing text only!
/// @param offset Parse from this byte index.
    JSWASM char *getID3FrameAsString(int offset = 0);

/// @return Returns with the contents "best" artist tag (TP1-4, TPE1-4, QT atoms). May return NULL.
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
/// @param takeOwnership For advanced use. If true, you take ownership on the data (don't forget to free() when done to prevent memory leaks).
    JSWASM char *getArtist(bool takeOwnership = false);

/// @return Returns with the contents "best" title tag (TT1-3, TIT1-3, QT atoms). May return NULL.
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
/// @param takeOwnership For advanced use. If true, you take ownership on the data (don't forget to free() when done to prevent memory leaks).
    JSWASM char *getTitle(bool takeOwnership = false);

/// @return Returns with the contents of the TALB tag (or QT atom). May return NULL.
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
/// @param takeOwnership For advanced use. If true, you take ownership on the data (don't forget to free() when done to prevent memory leaks).
    JSWASM char *getAlbum(bool takeOwnership = false);

/// @return Returns with the track index (track field or TRCK).
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
    JSWASM unsigned int getTrackIndex();

/// @return Returns with the contents "best" image tag (PIC, APIC, QT atom). May return NULL.
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
/// @param takeOwnership For advanced use. If true, you take ownership on the data (don't forget to free() when done to prevent memory leaks).
    JSWASM void *getImage(bool takeOwnership = false);

/// @return Returns with the the size of the image returned by getImage(). May return 0.
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
    JSWASM unsigned int getImageSizeBytes();

/// @return Returns with the bpm value of the "best" bpm tag (TBP, TBPM, QT atom). May return 0.
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
    JSWASM float getBPM();

/// @brief Apple's built-in codec may be used in some cases, such as decoding ALAC files. Call this after a media server reset or audio session interrupt to resume playback.
    void reconnectToMediaserver();

private:
    decoderInternals *internals;
    Decoder(const Decoder&);
    Decoder& operator=(const Decoder&);
};

/// @brief The Superpowered AudioInMemory format allows the Decoder and the AdvancedAudioPlayer to read compressed audio files or raw 16-bit PCM audio directly from memory.
///
/// The payload can be fully loaded onto the heap in one big piece, or multiple chunks can be added at any time to support for progressive loading (eg. add memory in chunks as downloaded).
///
/// The format consists of a main table and zero or more buffer tables. Every item is a 64-bit number to support native 32/64-bit systems and the web (WebAssembly).
///
/// The main table consists of six numbers, 64-bit (int64_t) each: [ version, retain count, samplerate, size, completed, address of the first buffer table ].
///
///     0: Version should be set to 0.
///     1: The retain count can be used to track usage (eg. multiple simultaneous access). If set to 0, the Decoder and the AdvancedAudioPlayer will take ownership on all the memory allocated (the main table, the buffer tables and their payloads) and will automatically free them using free(). The allocation should happen using malloc() (and not _aligned_malloc() or similar).
///     2. Samplerate is used with the AdvancedAudioPlayer only (valid range is 8192 to 384000) and must be correctly set right at the beginning. Set it to 0 if using the Decoder.
///     3. Size. AdvancedAudioPlayer: the total duration in frames. Decoder: the total file size in bytes. Set to 0 if unknown at the beginning (eg. progressive download). Set it at any point later if it gets known.
///     4. Completed: set it to 0 if additional buffers can be added (such as progressive download). Set it to 1 if finished adding additional buffers.
///     5. Memory address of the first buffer table.
///
/// Every buffer table consists of four numbers, 64-bit (int64_t) each: [ payload address, size, address of the next buffer table, reserved ].
///
///     0. Memory address of the payload.
///     1. Size. AdvancedAudioPlayer: the number of audio frames inside the payload. Decoder: the size of the payload in bytes.
///     2. Memory address of the next buffer table, or 0 if there are none (yet).
///     3. Reserved. Don't change the contents of this.
///
/// Special "self-contained" case: if Completed is 1 and the memory address of the first buffer table is 0, then buffer tables are not used and the payload immediately starts after the main table, in the same memory region (so it's allocated together with the main table).
///
/// Example for a 1 MB big audio file loaded into the memory, to be decoded by the Decoder:
///
/// main table at address 0x1000: [ 0, 0, 0, 1048576, 1, 0x2000 ]
/// buffer table at address 0x2000: [ 0x3000, 1048576, 0, 0 ]
///
/// Example for 1 minute 16-bit PCM audio in the memory, to be played by the AdvancedAudioPlayer:
///
/// main table at address 0x1000: [ 0, 0, 44100, 2646000, 1, 0x2000 ]
/// buffer table at address 0x2000: [ 0x3000, 2646000, 0, 0 ]
class AudioInMemory {
public:
    /// @return Creates a main table.
    /// @param retainCount The retain count can be used to track usage (eg. multiple simultaneous access). If set to 0, the Decoder and the AdvancedAudioPlayer will take ownership on all the memory allocated (the main table, the buffer tables and their payloads) and will automatically free them using free(). The allocation should happen using malloc() (and not _aligned_malloc() or similar).
    /// @param samplerate For compressed (not decoded) audio data this must be set to 0. For raw 16-bit PCM audio it must be correctly set right at the beginning (valid range: 8192 to 384000).
    /// @param size For compressed (not decoded) audio data: the total payload size in bytes. For raw 16-bit PCM audio: the total duration in frames. Set to 0 if unknown at the beginning (eg. progressive download). Set it at any point later if it gets known.
    /// @param completed False if additional buffers can be added (such as progressive download), true otherwise.
    JSWASM static void *create(unsigned int retainCount, unsigned int samplerate, unsigned int size, bool completed);
    
    /// @return Creates a main table with the payload included (special "self-contained" case).
    /// @param retainCount The retain count can be used to track usage (eg. multiple simultaneous access). If set to 0, the Decoder and the AdvancedAudioPlayer will take ownership on all the memory allocated (the main table, the buffer tables and their payloads) and will automatically free them using free(). The allocation should happen using malloc() (and not _aligned_malloc() or similar).
    /// @param samplerate Samplerate is used with the AdvancedAudioPlayer only (valid range is 8192 to 384000) and must be correctly set right at the beginning. Set it to 0 if using the Decoder.
    /// @param size For compressed (not decoded) audio data: the total payload size in bytes. For raw 16-bit PCM audio: the total duration in frames. Set to 0 if unknown at the beginning (eg. progressive download). Set it at any point later if it gets known.
    JSWASM static void *createSelfContained(unsigned int retainCount, unsigned int samplerate, unsigned int size);
    
    /// @brief Adds 1 to retain count.
    /// @param pointer Pointer to the main table.
    JSWASM static void retain(void *pointer);
    
    /// @brief Decrease the retain count by 1.
    /// @param pointer Pointer to the main table.
    JSWASM static void release(void *pointer);
    
    /// @brief Sets completed to 1.
    /// @param pointer Pointer to the main table.
    JSWASM static void complete(void *pointer);
    
    /// @brief Set size.
    /// @param pointer Pointer to the main table.
    /// @param size For compressed (not decoded) audio data: the total payload size in bytes. For raw 16-bit PCM audio: the total duration in frames. Set to 0 if unknown at the beginning (eg. progressive download). Set it at any point later if it gets known.
    JSWASM static void setSize(void *pointer, unsigned int size);
    
    /// @return Returns with the size.
    /// @param pointer Pointer to the main table.
    JSWASM static unsigned int getSize(void *pointer);
    
    /// @return Returns with the sample rate.
    /// @param pointer Pointer to the main table.
    JSWASM static unsigned int getSamplerate(void *pointer);
    
    /// @brief Adds data.
    /// @param table Pointer to the main table.
    /// @param payload Pointer to the payload. Should be allocated with malloc() (NOT new or _aligned_malloc).
    /// @param size For compressed (not decoded) audio data: the size of the payload in bytes. For raw 16-bit PCM audio: the number of audio frames inside the payload.
    JSWASM static void append(void *table, void *payload, unsigned int size);
};

}

#endif
