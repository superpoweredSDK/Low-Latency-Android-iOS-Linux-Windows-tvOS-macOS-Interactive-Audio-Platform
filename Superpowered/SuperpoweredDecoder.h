#ifndef Header_SuperpoweredDecoder
#define Header_SuperpoweredDecoder

#include "SuperpoweredHTTP.h"
#include <stdint.h>

namespace Superpowered {

struct decoderInternals;

/// @brief File/decoder format.
typedef enum Decoder_Format {
    Decoder_Format_MP3,  ///< MP3
    Decoder_Format_AAC,  ///< AAC, HE-AAC
    Decoder_Format_AIFF, ///< AIFF
    Decoder_Format_WAV,  ///< WAV
    Decoder_Format_MediaServer ///< Other format decoded by iOS, macOS or tvOS.
} Decoder_Format;

/// @brief Audio file decoder. Provides raw PCM audio from various compressed formats.
/// Supported file types:
/// - Stereo or mono pcm WAV and AIFF (16-bit int, 24-bit int, 32-bit int or 32-bit IEEE float).
/// - MP3: MPEG-1 Layer III (sample rates: 32000 Hz, 44100 Hz, 48000 Hz). MPEG-2 Layer III is not supported (mp3 with sample rates below 32000 Hz).
/// - AAC or HE-AAC in M4A container (iTunes) or ADTS container (.aac).
/// - ALAC/Apple Lossless (on iOS only).
class Decoder {
public:
/// @brief Error codes for the decodeAudio() and getAudioStartFrame() methods.
    static const int EndOfFile = 0;               ///< End-of-file reached.
    static const int BufferingTryAgainLater = -1; ///< Buffering (waiting for the network to pump enough data).
    static const int NetworkError = -2;           ///< Network (download) error.
    static const int Error = -3;                  ///< Decoding error.
    
/// @brief Error codes for the open() method.
    static const int OpenSuccess = 0;
    static const int OpenError_OutOfMemory =             1000; ///< Some memory allocation failed. Recommended action: check for memory leaks.
    static const int OpenError_PathIsNull =              1001; ///< Path is NULL.
    static const int OpenError_FastMetadataNotLocal =    1002; ///< metaOnly was true, but it works on local files only.
    static const int OpenError_ID3VersionNotSupported =  1003; ///< ID3 version 2.2, 2.3 and 2.4 are supported only.
    static const int OpenError_ID3ReadError =            1004; ///< File read error (such as fread() failed) while reading the ID3 tag.
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
    
/// @return Returns with a human readable error string. If the code is not a decoder status code, then it's a SuperpoweredHTTP status code and returns with that.
/// @param code The return value of the Decoder::open method.
    static const char *statusCodeToString(int code);
    
/// @return Returns with the duration of the current file in frames.
/// @warning Duration may change after each decode() or seekTo(), because some audio formats doesn't contain precise length information.
    int64_t getDurationFrames();
    
/// @return Returns with the duration of the current file in seconds.
/// @warning Duration may change after each decode() or seekTo(), because some audio formats doesn't contain precise length information.
    double getDurationSeconds();
    
/// @return Returns with the sample rate of the current file.
    unsigned int getSamplerate();
    
/// @return Returns with the format of the current file.
    Decoder_Format getFormat();
    
/// @return Returns with how many frames are in one chunk of the source file. For example: MP3 files store 1152 audio frames in a chunk.
    unsigned int getFramesPerChunk();
    
/// @return Returns with the current position in frames. The postion may change after each decode() or seekTo().
    int64_t getPositionFrames();
    
/// @return Indicates which part of the file has fast access. For local files it will always be 0.0f.
    float getBufferedStartPercent();
    
/// @return Indicates which part of the file has fast access. For local files it will always be 1.0f.
    float getBufferedEndPercent();
    
/// @return The file system path of the fully downloaded audio file for progressive downloads. May be NULL until the download finishes.
    const char *getFullyDownloadedPath();

/// @brief Creates an instance of Superpowered Decoder.
    Decoder();
    ~Decoder();
    
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
/// @param customHTTPRequest If custom HTTP communication is required (such as sending http headers for authorization), pass a fully prepared http request object. The player will take ownership of this.
    int open(const char *path, bool metaOnly = false, int offset = 0, int length = 0, int stemsIndex = 0, Superpowered::httpRequest *customHTTPRequest = 0);
    
/// @brief Decodes audio.
/// @return The return value can be:
/// - A positive number above zero: the number of frames decoded.
/// - Decoder::EndOfFile: end of file reached, there is nothing more to decode. Use setPosition.
/// - Decoder::BufferingTryAgainLater: the decoder needs more data to be downloaded. Retry decodeAudio() later (it's recommended to wait at least 0.1 seconds).
/// - Decoder::NetworkError: network (download) error.
/// - Decoder::Error: internal decoder error.
/// @param output Pointer to 16-bit signed integer numbers. The output. Must be at least numberOfFrames * 4 + 16384 bytes big.
/// @param numberOfFrames The requested number of frames. Should NOT be less than framesPerChunk.
    int decodeAudio(short int *output, unsigned int numberOfFrames);
    
/// @brief Jumps to the specified position's chunk (frame) beginning.
/// Some codecs (such as MP3) contain audio in chunks (frames). This method will not jump precisely to the specified position, but to the chunk's beginning the position belongs to.
/// @return Returns with success (true) or failure (false).
/// @param positionFrames The requested position.
    bool setPositionQuick(int64_t positionFrames);
    
/// @brief Jumps to a specific position.
/// This method is a little bit slower than setPositionQuick().
/// @return Returns with success (true) or failure (false).
/// @param positionFrames The requested position.
    bool setPositionPrecise(int64_t positionFrames);
    
/// Detects silence at the beginning.
/// @warning This function changes positionFrames!
/// @return The return value can be:
/// - A positive number or zero: the frame index where audio starts.
/// - Decoder::BufferingTryAgainLater: the decoder needs more data to be downloaded. Retry getAudioStartFrame() later (it's recommended to wait at least 0.1 seconds).
/// - Decoder::NetworkError: network (download) error.
/// - Decoder::Error: internal decoder error.
/// @param limitFrames Optional. How far to search for. 0 means "the entire audio file".
/// @param thresholdDb Optional. Loudness threshold in decibel. 0 means "any non-zero audio". The value -49 is useful for vinyl rips starting with vinyl noise (crackles).
    int getAudioStartFrame(unsigned int limitFrames = 0, int thresholdDb = 0);
    
/// @return Returns with the JSON metadata string for the Native Instruments Stems format, or NULL if the file is not Stems.
    const char *getStemsJSONString();
        
/// @brief Parses all ID3 frames. Do not use startReadingID3/readNextID3Frame after this.
/// @param skipImages Parsing ID3 image frames is significantly slower than parsing other frames. Set this to true if not interested in image information to save CPU.
/// @param maxFrameDataSize The maximum frame size in bytes to retrieve if the decoder can not memory map the entire audio file. Affects memory usage. Useful to skip large payloads (such as images).
    void parseAllID3Frames(bool skipImages, unsigned int maxFrameDataSize);
    
/// @brief Starts reading ID3 frames. Use readNextID3Frame() in an iteration after this.
/// @param skipImages Parsing ID3 image frames is significantly slower than parsing other frames. Set this to true if not interested in image information to save CPU.
/// @param maxFrameDataSize The maximum frame size in bytes to retrieve if the decoder can not memory map the entire audio file. Affects memory usage. Useful to skip large payloads (such as images).
    void startParsingID3Frames(bool skipImages, unsigned int maxFrameDataSize);
    
/// @return Reads the next ID3 frame and returns with its data size or -1 if finished reading.
    unsigned int readNextID3Frame();
    
/// @return Returns with the current ID3 frame name (four char). To be used with readNextID3Frame().
    unsigned int getID3FrameName();
    
/// @return Returns with the raw data of the current ID3 frame. To be used with readNextID3Frame().
    void *getID3FrameData();
    
/// @return Returns with the current ID3 frame data length.
    unsigned int getID3FrameDataLengthBytes();
    
/// @brief Returns with the text inside the current ID3 frame. To be used with readNextID3Frame().
/// @return A pointer to the text in UTF-8 encoding (you take ownership on the data, don't forget to free() when done to prevent memory leaks), or NULL if empty.
/// @warning Use it for frames containing text only!
/// @param offset Parse from this byte index.
    char *getID3FrameAsString(int offset = 0);
    
/// @return Returns with the contents "best" artist tag (TP1-4, TPE1-4, QT atoms). May return NULL.
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
/// @param takeOwnership For advanced use. If true, you take ownership on the data (don't forget to free() when done to prevent memory leaks).
    char *getArtist(bool takeOwnership = false);
    
/// @return Returns with the contents "best" title tag (TT1-3, TIT1-3, QT atoms). May return NULL.
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
/// @param takeOwnership For advanced use. If true, you take ownership on the data (don't forget to free() when done to prevent memory leaks).
    char *getTitle(bool takeOwnership = false);
    
/// @return Returns with the contents of the TALB tag (or QT atom). May return NULL.
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
/// @param takeOwnership For advanced use. If true, you take ownership on the data (don't forget to free() when done to prevent memory leaks).
    char *getAlbum(bool takeOwnership = false);
    
/// @return Returns with the track index (track field or TRCK).
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
    unsigned int getTrackIndex();
    
/// @return Returns with the contents "best" image tag (PIC, APIC, QT atom). May return NULL.
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
/// @param takeOwnership For advanced use. If true, you take ownership on the data (don't forget to free() when done to prevent memory leaks).
    void *getImage(bool takeOwnership = false);
    
/// @return Returns with the the size of the image returned by getImage(). May return NULL.
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
    unsigned int getImageSizeBytes();
    
/// @return Returns with the bpm value of the "best" bpm tag (TBP, TBPM, QT atom). May return 0.
/// Call this after parseAllID3Frames() OR finished reading all ID3 frames with readNextID3Frame().
    float getBPM();
    
/// @brief Apple's built-in codec may be used in some cases, such as decoding ALAC files. Call this after a media server reset or audio session interrupt to resume playback.
    void reconnectToMediaserver();

private:
    decoderInternals *internals;
    Decoder(const Decoder&);
    Decoder& operator=(const Decoder&);
};

}

#endif
