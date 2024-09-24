#ifndef Header_SuperpoweredAdvancedAudioPlayer
#define Header_SuperpoweredAdvancedAudioPlayer

#include "SuperpoweredDecoder.h"

namespace Superpowered {

class AdvancedAudioPlayer;
struct PlayerInternals;

/// @brief Status codes.
#define Player_Success 0
#define PlayerError_FileTooShort         2000 ///< The audio has less than 512 frames.
#define HLSError_PlaylistTypeMismatch    2001 ///< Some alternatives in the HLS stream are different to the first one.
#define HLSError_Empty                   2002 ///< The HLS stream contains no alternatives.

/// @brief High performance advanced audio player with:
/// - time-stretching and pitch shifting,
/// - beat and tempo sync,
/// - scratching,
/// - tempo bend,
/// - looping,
/// - slip mode,
/// - fast seeking (cached points),
/// - momentum and jog wheel handling,
/// - 0 latency, real-time operation,
/// - low memory usage,
/// - thread safety (all methods are thread-safe),
/// - direct iPod music library access.
/// Supported file types:
/// - Stereo or mono pcm WAV and AIFF (16-bit int, 24-bit int, 32-bit int or 32-bit IEEE float).
/// - MP3: MPEG-1 Layer III (sample rates: 32000 Hz, 44100 Hz, 48000 Hz). MPEG-2 Layer III is not supported (mp3 with sample rates below 32000 Hz).
/// - AAC or HE-AAC in M4A container (iTunes) or ADTS container (.aac).
/// - ALAC/Apple Lossless (on iOS only).
/// - Http Live Streaming (HLS): vod/live/event streams, AAC-LC/MP3 in audio files or MPEG-TS files. Support for byte ranges and AES-128 encryption.

class AdvancedAudioPlayer {
public:
    static const int HLSDownloadEverything;  ///< Will download everything after the playback position until the end.
    static const int HLSDownloadRemaining;   ///< Downloads everything from the beginning to the end, regardless the playback position.
    static const float MaxPlaybackRate;      ///< The maximum playback rate or scratching speed: 20.

    /// @brief Jog Wheel Mode, to be used with the jogT... methods.
    typedef enum JogMode {
        JogMode_Scratch = 0,   ///< Jog wheel controls scratching.
        JogMode_PitchBend = 1, ///< Jog wheel controls pitch bend.
        JogMode_Parameter = 2  ///< Jog wheel changes a parameter.
    } JogMode;

    /// @brief Player events.
    typedef enum PlayerEvent {
        PlayerEvent_None = 0,       ///< Open was not called yet.
        PlayerEvent_Opening = 1,    ///< Trying to open the content.
        PlayerEvent_OpenFailed = 2, ///< Failed to open the content.
        PlayerEvent_Opened = 10,    ///< Successfully opened the content, playback can start.
        PlayerEvent_ConnectionLost = 3, ///< Network connection lost to the HLS stream or progressive download. Can only be "recovered" by a new open(). May happen after PlayerEvent_Opened has been delivered.
        PlayerEvent_ProgressiveDownloadFinished = 11 ///< The content has finished downloading and is fully available locally. May happen after PlayerEvent_Opened has been delivered.
    } PlayerEvent;

    /// @brief Synchronization modes.
    typedef enum SyncMode {
        SyncMode_None = 0,        ///< No synchronization.
        SyncMode_Tempo = 1,       ///< Sync tempo only.
        SyncMode_TempoAndBeat = 2 ///< Sync tempo and beat.
    } SyncMode;

    unsigned int outputSamplerate;           ///< The player output sample rate in Hz.
    double playbackRate;                     ///< The playback rate. Must be positive and above 0.00001. Default: 1.
    bool timeStretching;                     ///< Enable/disable time-stretching. Default: true.
    float formantCorrection;                 ///< Amount of formant correction, between 0 (none) and 1 (full). Default: 0.
    double originalBPM;                      ///< The original bpm of the current music. There is no auto-bpm detection inside, this must be set to a correct value for syncing. Maximum 300. A value below 20 will be automatically set to 0. Default: 0 (no bpm value known).
    bool fixDoubleOrHalfBPM;                 ///< If true and playbackRate is above 1.4f or below 0.6f, it will sync the tempo as half or double. Default: false.
    double firstBeatMs;                      ///< The start position of the beatgrid in milliseconds. Must be set to a correct value for syncing. Default: 0.
    double defaultQuantum;                   ///< Sets the quantum for quantized synchronization. Example: 4 means 4 beats. Default: 1.
    SyncMode syncMode;                       ///< The current sync mode (off, tempo, or tempo+beat). Default: off.
    double syncToBpm;                        ///< A bpm value to sync with. Use 0 for no syncing. Default: 0.
    double syncToMsElapsedSinceLastBeat;     ///< The number of milliseconds elapsed since the last beat on audio the player has to sync with. Use -1.0 to ignore. Default: -1.
    double syncToPhase;                      ///< Used for quantized synchronization. The phase to sync with between 0 and 1. Use -1 to ignore. Default: -1.
    double syncToQuantum;                    ///< Used for quantized synchronization. The quantum to sync with. Use -1 to ignore. Default: -1.
    int pitchShiftCents;                     ///< Pitch shift cents, from -2400 (two octaves down) to 2400 (two octaves up). Low CPU load for multiples of 100 between -1200 and 1200. Default: 0 (no pitch shift).
    bool loopOnEOF;                          ///< If true, jumps back and continues playback. If false, playback stops. Default: false.
    bool reverseToForwardAtLoopStart;        ///< If this is true with playing backwards and looping, then reaching the beginning of the loop will change playback direction to forwards. Default: false.
    bool HLSAutomaticAlternativeSwitching;   ///< If true, then the player will automatically switch between the HLS alternatives according to the available network bandwidth. Default: true.
    char HLSLiveLatencySeconds;              ///< When connecting or reconnecting to a HLS live stream, the player will try to skip audio to maintain this latency. Default: -1 (the player will not skip audio and the live stream starts at the first segment specified by the server).
    int HLSMaximumDownloadAttempts;          ///< How many times to retry if a HLS segment download fails. Default: 100.
    int HLSBufferingSeconds;                 ///< How many seconds ahead of the playback position to download. Default value: HLSDownloadRemaining.
    unsigned char timeStretchingSound;       ///< The sound parameter of the internal TimeStretching instance. @see TimeStretching

/// @brief Set the folder to store for temporary files. Used for HLS and progressive download only.
/// Call this before any player instance is created.
/// It will create a subfolder with the name "SuperpoweredAAP" in the specified folder (and will clear all content inside SuperpoweredAAP if it exists already).
/// If you need to clear the folder before your app quits, use NULL for the path.
/// @param path File system path of the folder.
    static void setTempFolder(const char *path);

/// @return Returns with the temporary folder path.
    static const char *getTempFolderPath();

/// @return Returns with a human readable error string. If the code is not a decoder status code, then it's a SuperpoweredHTTP status code and returns with that.
/// @param code The return value of the open...() method.
    JSWASM static const char *statusCodeToString(int code);

/// @brief Creates a player instance.
/// @param samplerate The initial sample rate of the player output in hz.
/// @param cachedPointCount How many positions can be cached in the memory. Jumping to a cached point happens with zero latency. Loops are automatically cached.
/// @param internalBufferSizeSeconds The number of seconds to buffer internally for playback and cached points. The value 0 enables "offline mode", where the player can not be used for real-time playback, but can process audio in an iteration. If not zero, the AdvancedAudioPlayer can only be used for real-time playback. Default: 2.
/// @param negativeSeconds The number of seconds of silence in the negative direction, before the beginning of the track.
/// @param minimumTimestretchingPlaybackRate Will not time-stretch but resample below this playback rate. Default: 0.501f (the recommended value for low CPU load on older mobile devices, such as the first iPad). Will be applied after changing playbackRate or scratching. Default: 0.501f
/// @param maximumTimestretchingPlaybackRate Will not time-stretch but resample above this playback rate. Default: 2.0f (the recommended value for low CPU load on older mobile devices, such as the first iPad). Will be applied after changing playbackRate or scratching.
/// @param enableStems If true and a Native Instruments STEMS file is loaded, output 4 stereo channels. Default: false (stereo master mix output).
    JSWASM AdvancedAudioPlayer(unsigned int samplerate, unsigned char cachedPointCount, unsigned int internalBufferSizeSeconds = 2, unsigned int negativeSeconds = 0, float minimumTimestretchingPlaybackRate = 0.501f, float maximumTimestretchingPlaybackRate = 2.0f, bool enableStems = false);
    JSWASM ~AdvancedAudioPlayer();

/// @brief Opens an audio file with playback paused.
/// Playback rate, pitchShift, timeStretching and syncMode are NOT changed if you open a new file.
/// @warning This method has no effect if the previous open didn't finish or if called in the audio processing thread.
/// @param path Full file system path or progressive download path (http or https).
/// @param customHTTPRequest If custom HTTP communication is required (such as sending http headers for authorization), pass a fully prepared http request object. The player will copy this object.
/// @param skipSilenceAtBeginning If true, the player will set the position to skip the initial digital silence of the audio file (up to 10 seconds).
/// @param measureSilenceAtEnd If true, the player will check the length of the digital silence at the end of the audio file.
    void open(const char *path, Superpowered::httpRequest *customHTTPRequest = 0, bool skipSilenceAtBeginning = false, bool measureSilenceAtEnd = false);

/// @brief Opens an audio file with playback paused.
/// Playback rate, pitchShift, timeStretching and syncMode are NOT changed if you open a new file.
/// @warning This method has no effect if the previous open didn't finish or if called in the audio processing thread.
/// @param path Full file system path or progressive download path (http or https).
/// @param offset The byte offset inside the path.
/// @param length The byte length from the offset.
/// @param customHTTPRequest If custom HTTP communication is required (such as sending http headers for authorization), pass a fully prepared http request object. The player will copy this object.
/// @param skipSilenceAtBeginning If true, the player will set the position to skip the initial digital silence of the audio file (up to 10 seconds).
/// @param measureSilenceAtEnd If true, the player will check the length of the digital silence at the end of the audio file.
    void open(const char *path, int offset, int length, Superpowered::httpRequest *customHTTPRequest = 0, bool skipSilenceAtBeginning = false, bool measureSilenceAtEnd = false);

/// @brief Opens raw 16-bit sterteo PCM audio in memory, with playback paused.
/// Playback rate, pitchShift, timeStretching and syncMode are NOT changed if you open a new file.
/// @warning This method has no effect if the previous open didn't finish or if called in the audio processing thread.
/// @param pointer Pointer to 16-bit integer numbers, raw stereo interleaved pcm audio.
/// @param samplerate The sample rate in Hz. Valid from 8192 to 384000.
/// @param durationFrames The duration of the audio in frames.
/// @param skipSilenceAtBeginning If true, the player will set the position to skip the initial digital silence of the audio file (up to 10 seconds).
/// @param measureSilenceAtEnd If true, the player will check the length of the digital silence at the end of the audio file.
    JSWASM void openPCM16AudioInMemory(void *pointer, unsigned int samplerate, unsigned int durationFrames, bool skipSilenceAtBeginning = false, bool measureSilenceAtEnd = false);

/// @brief Opens a memory location in Superpowered AudioInMemory format, with playback paused. This feature supports progressive loading via AudioInMemory::append (and the AudioInMemory doesn't even need to hold any data when openMemory is called).
/// Playback rate, pitchShift, timeStretching and syncMode are NOT changed if you open a new file.
/// @warning This method has no effect if the previous open didn't finish or if called in the audio processing thread.
/// @param pointer Pointer to data in Superpowered AudioInMemory format, pointing to raw stereo interleaved pcm audio inside.
/// @param skipSilenceAtBeginning If true, the player will set the position to skip the initial digital silence of the audio file (up to 10 seconds).
/// @param measureSilenceAtEnd If true, the player will check the length of the digital silence at the end of the audio file.
    JSWASM void openMemory(void *pointer, bool skipSilenceAtBeginning = false, bool measureSilenceAtEnd = false);

/// @brief Opens a HTTP Live Streaming stream with playback paused.
/// Playback rate, pitchShift, timeStretching and syncMode are NOT changed if you open a new one.
/// Do not call openHLS() in the audio processing thread.
/// @param url Stream URL.
/// @param customHTTPRequest If custom HTTP communication is required (such as sending http headers for authorization), pass a fully prepared http request object. The player will copy this object.
    void openHLS(const char *url, Superpowered::httpRequest *customHTTPRequest = 0);

/// @return Returns with the latest player event. This method should be used in a periodically running code, at one place only, because it returns a specific event just once per open() call. Best to be used in a UI loop.
    JSWASM PlayerEvent getLatestEvent();

/// @return If getLatestEvent returns with OpenFailed, retrieve the error code or HTTP status code here.
    JSWASM int getOpenErrorCode();

/// @return Returns with the full filesystem path of the locally cached file if the player is in the PlayerEvent_Opened_ProgressiveDownloadFinished state, NULL otherwise.
    const char *getFullyDownloadedFilePath();

/// @return Returns true if end-of-file has been reached recently (will never indicate end-of-file if loopOnEOF is true). This method should be used in a periodically running code at one place only, because it returns a specific end-of-file event just once. Best to be used in a UI loop.
    JSWASM bool eofRecently();

/// @return Indicates if the player is waiting for data (such as waiting for a network download).
    JSWASM bool isWaitingForBuffering();

/// @return Returns with the length of the digital silence at the beginning of the file if open...() was called with skipSilenceAtBeginning = true, 0 otherwise.
    JSWASM double getAudioStartMs();

/// @return Returns with the length of the digital silence at the end of the file if open...() was called with measureSilenceAtEnd = true, 0 otherwise.
    JSWASM double getAudioEndMs();

/// @return The current playhead position in milliseconds. Not changed by any pending setPosition() or seek() call, always accurate regardless of time-stretching and other transformations.
    JSWASM double getPositionMs();

/// @return The current position in milliseconds, immediately updated after setPosition() or seek(). Use this for UI display.
    JSWASM double getDisplayPositionMs();

/// @return Similar to getDisplayPositionMs(), but as a percentage (0 to 1).
    JSWASM float getDisplayPositionPercent();

/// @return Similar to getDisplayPositionMs(), but as seconds elapsed.
    JSWASM int getDisplayPositionSeconds();

/// @return The position in milliseconds where the player will continue playback after slip mode ends.
    JSWASM double afterSlipModeWillJumpBackToPositionMs();

/// @return The duration of the current track in milliseconds. Returns -1 for live streams.
    JSWASM double getDurationMs();

/// @return The duration of the current track in seconds. Returns UINT_MAX for live streams.
    JSWASM unsigned int getDurationSeconds();

/// @brief Starts playback immediately without any synchronization.
    JSWASM void play();

/// @brief Starts beat or tempo synchronized playback.
    JSWASM void playSynchronized();

/// @brief Starts playback at a specific position. isPlaying() will return false and the position will not be updated until this function succeeds starting playback at the specified position.
/// You can call this in a real-time thread (audio processing callback) with a continuously updated time for a precise on-the-fly launch.
/// @param positionMs Start position in milliseconds.
    JSWASM void playSynchronizedToPosition(double positionMs);

/// @brief Pause playback.
/// There is no need for a "stop" method, this player is very efficient with the battery and has no significant "stand-by" processing.
/// @param decelerateSeconds Optional momentum. 0 means to pause immediately.
/// @param slipMs Enable slip mode for a specific amount of time, or 0 to not slip.
    JSWASM void pause(float decelerateSeconds = 0, unsigned int slipMs = 0);

/// @brief Toggle play/pause (no synchronization).
    JSWASM void togglePlayback();

/// @return Indicates if the player is playing or paused.
    JSWASM bool isPlaying();

/// @brief Simple seeking to a percentage.
/// @param percent The position in percentage.
    JSWASM void seek(double percent);

/// @brief Precise seeking.
/// @param ms Position in milliseconds.
/// @param andStop If true, stops playback.
/// @param synchronisedStart If andStop is false, makes a beat-synced start possible.
/// @param forceDefaultQuantum If true and using quantized synchronization, will use the defaultQuantum instead of the syncToQuantum.
/// @param preferWaitingforSynchronisedStart Wait or start immediately when synchronized.
    JSWASM void setPosition(double ms, bool andStop, bool synchronisedStart, bool forceDefaultQuantum = false, bool preferWaitingforSynchronisedStart = false);

/// @brief Caches a position for zero latency seeking.
/// @param ms Position in milliseconds.
/// @param pointID Use this to provide a custom identifier, so you can overwrite the same point later. Use 255 for a point with no identifier.
    JSWASM void cachePosition(double ms, unsigned char pointID = 255);

/// @brief Outputs audio, stereo version.
/// @return True: buffer has audio output from the player. False: the contents of the buffers were not changed (typically happens when the player is paused).
/// @warning Duration may change to a more precise value after this, because some file formats have no precise duration information.
/// @param buffer Pointer to floating point numbers. 32-bit interleaved stereo input/output buffer. Should be numberOfFrames * 8 + 64 bytes big.
/// @param mix If true, the player output will be mixed with the contents of buffer. If false, the contents of buffer will be overwritten with the player output.
/// @param numberOfFrames The number of frames requested.
/// @param volume 0.0f is silence, 1.0f is "original volume". Changes are automatically smoothed between consecutive processes.
    JSWASM bool processStereo(float *buffer, bool mix, unsigned int numberOfFrames, float volume = 1.0f);

/// @brief Outputs audio, 8 channels version.
/// @return True: buffers has audio output from the player. False: the contents of buffer were not changed (typically happens when the player is paused).
/// @warning Duration may change to a more precise value after this, because some file formats have no precise duration information.
/// @param buffer0 Pointer to floating point numbers. 32-bit interleaved stereo input/output buffer for the 1st stereo channels. Should be numberOfFrames * 8 + 64 bytes big.
/// @param buffer1 Pointer to floating point numbers. 32-bit interleaved stereo input/output buffer for the 2nd stereo channels. Should be numberOfFrames * 8 + 64 bytes big.
/// @param buffer2 Pointer to floating point numbers. 32-bit interleaved stereo input/output buffer for the 3rd stereo channels. Should be numberOfFrames * 8 + 64 bytes big.
/// @param buffer3 Pointer to floating point numbers. 32-bit interleaved stereo input/output buffer for the 4th stereo channels. Should be numberOfFrames * 8 + 64 bytes big.
/// @param mix If true, the player output will be added to the contents of buffers. If false, the contents of buffers will be overwritten with the player output.
/// @param numberOfFrames The number of frames requested.
/// @param volume0 Volume for buffer0. 0.0f is silence, 1.0f is "original volume". Changes are automatically smoothed between consecutive processes.
/// @param volume1 Volume for buffer1. 0.0f is silence, 1.0f is "original volume". Changes are automatically smoothed between consecutive processes.
/// @param volume2 Volume for buffer2. 0.0f is silence, 1.0f is "original volume". Changes are automatically smoothed between consecutive processes.
/// @param volume3 Volume for buffer3. 0.0f is silence, 1.0f is "original volume". Changes are automatically smoothed between consecutive processes.
    JSWASM bool process8Channels(float *buffer0, float *buffer1, float *buffer2, float *buffer3, bool mix, unsigned int numberOfFrames, float volume0, float volume1, float volume2, float volume3);

/// @return Returns true if a STEMS file was loaded (and the player was initialized with enableStems == true).
    JSWASM bool isStems();

/// @brief Performs the last stage of STEMS processing, the master compressor and limiter. Works only if a STEMS file was loaded.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input buffer.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output buffer.
/// @param numberOfFrames The number of frames to process.
/// @param volume Output volume. 0.0f is silence, 1.0f is "original volume". Changes are automatically smoothed between consecutive processes.
    JSWASM void processSTEMSMaster(float *input, float *output, unsigned int numberOfFrames, float volume = 1.0f);

/// @return Returns with a stem's name if a STEMS file was loaded, NULL otherwise.
/// @param index The index of the stem.
    JSWASM const char *getStemName(unsigned char index);

/// @return Returns with a stem's color if a STEMS file was loaded, NULL otherwise.
/// @param index The index of the stem.
    JSWASM const char *getStemColor(unsigned char index);

/// @brief Apple's built-in codec may be used in some cases, such as decoding ALAC files. Call this after a media server reset or audio session interrupt to resume playback.
    void onMediaserverInterrupt();

/// @return Returns with the beginning of the buffered part. Will always be 0 for non-network sources (such as local files).
    float getBufferedStartPercent();

/// @return Returns with the end of the buffered part. Will always be 1.0f for non-network sources (such as local files).
    float getBufferedEndPercent();

/// @return For HLS only. Returns with the actual network throughput (for best stream selection).
    unsigned int getCurrentHLSBPS();

/// @return The current bpm of the track (as changed by the playback rate).
    JSWASM double getCurrentBpm();

/// @return How many milliseconds elapsed since the last beat.
    JSWASM double getMsElapsedSinceLastBeat();

/// @return Which beat has just happened. Possible values:
/// 0        : unknown
/// 1 - 1.999: first beat
/// 2 - 2.999: second beat
/// 3 - 3.999: third beat
/// 4 - 4.999: fourth beat
    JSWASM float getBeatIndex();

/// @return Returns with the current phase for quantized synchronization, between 0 (beginning of the quantum) and 1 (end of the quantum).
    JSWASM double getPhase();

/// @return Returns with the current quantum for quantized synchronization, such as 2 for two beats, 4 for four beats, etc...
    JSWASM double getQuantum();

/// @return Returns with the distance (in milliseconds) to a specific quantum and phase for quantized synchronization.
/// @param phase The phase to calculate against.
/// @param quantum The quantum to calculate against.
    JSWASM double getMsDifference(double phase, double quantum);

/// @return If the player is waiting to a synchronization event (such as synchronized playback start or restarting a loop), the return value indicates the time remaining in milliseconds (continously updated). 0 means not waiting to such event.
    JSWASM double getMsRemainingToSyncEvent();

/// @brief Loop from a start point to some length.
/// @param startMs Loop from this milliseconds.
/// @param lengthMs Loop length in milliseconds.
/// @param jumpToStartMs If the playhead is within the loop, jump to startMs or not.
/// @param pointID Looping caches startMs, therefore you can specify a pointID too (or set to 255 if you don't care).
/// @param synchronisedStart Beat-synced start.
/// @param numLoops Number of times to loop. 0 means: until exitLoop() is called.
/// @param forceDefaultQuantum If true and using quantized synchronization, will use the defaultQuantum instead of the syncToQuantum.
/// @param preferWaitingforSynchronisedStart Wait or start immediately when synchronized.
    JSWASM void loop(double startMs, double lengthMs, bool jumpToStartMs, unsigned char pointID, bool synchronisedStart, unsigned int numLoops = 0, bool forceDefaultQuantum = false, bool preferWaitingforSynchronisedStart = false);

/// @brief Loop between a start and end points.
/// @param startMs Loop from this milliseconds.
/// @param endMs Loop to this milliseconds.
/// @param jumpToStartMs If the playhead is within the loop, jump to startMs or not.
/// @param pointID Looping caches startMs, therefore you can specify a pointID too (or set to 255 if you don't care).
/// @param synchronisedStart Beat-synced start.
/// @param numLoops Number of times to loop. 0 means: until exitLoop() is called.
/// @param forceDefaultQuantum If true and using quantized synchronization, will use the defaultQuantum instead of the syncToQuantum.
/// @param preferWaitingforSynchronisedStart Wait or start immediately when synchronized.
    JSWASM void loopBetween(double startMs, double endMs, bool jumpToStartMs, unsigned char pointID, bool synchronisedStart, unsigned int numLoops = 0, bool forceDefaultQuantum = false, bool preferWaitingforSynchronisedStart = false);

/// @brief Exit from the current loop.
/// @param synchronisedStart Synchronized start or re-synchronization after the loop exit.
    JSWASM void exitLoop(bool synchronisedStart = false);

/// @return Indicates if looping is enabled.
    JSWASM bool isLooping();

/// @return Returns true if a position is inside the current loop.
/// @param ms The position in milliseconds.
    JSWASM bool msInLoop(double ms);

/// @return Returns with the position of the closest beat.
/// @param ms The position in milliseconds where to find the closest beat.
/// @param beatIndex Set to 1-4 to retrieve the position of a specific beat index relative to ms, or 0 for any beat index.
    JSWASM double closestBeatMs(double ms, unsigned char beatIndex = 0);

/// @return Returns with the beat index of the closest beat.
/// @param ms The position in milliseconds where to find the closest beat.
    JSWASM unsigned char closestBeatIndex(double ms);

/// @brief Sets playback direction.
/// @param reverse True: reverse. False: forward.
/// @param slipMs Enable slip mode for a specific amount of time, or 0 to not slip.
    JSWASM void setReverse(bool reverse, unsigned int slipMs = 0);

/// @return If true, the player is playing backwards.
    JSWASM bool isReverse();

/// @brief Starts or changes pitch bend (temporary playback rate change).
/// @param maxPercent The maximum playback rate range for pitch bend, should be between 0.01f and 0.3f (1% and 30%).
/// @param bendStretch Use time-stretching for pitch bend or not (false makes it "audible").
/// @param faster True: faster, false: slower.
/// @param holdMs How long to maintain the pitch bend state. A value >= 1000 will hold until endContinuousPitchBend is called. A value < 40 will not "ramp up" pitch bend, but will apply it immediately.
    JSWASM void pitchBend(float maxPercent, bool bendStretch, bool faster, unsigned int holdMs);

/// @brief Ends pitch bend.
    JSWASM void endContinuousPitchBend();

/// @return Returns with the distance (in milliseconds) to the beatgrid while using pitch bend for correction.
    JSWASM double getBendOffsetMs();

/// @return Returns with the current pitch bend percent. Will be 1 if there is no pitch bend happening.
    JSWASM float getCurrentPitchBendPercent();
    
/// @brief Reset the pitch bend offset to the beatgrid to zero.
    JSWASM void resetBendMsOffset();

/// @brief Set the pitch bend offset to the beatgrid.
/// @param ms The value.
    JSWASM void setBendOffsetMs(double ms);

/// @return Indicates if returning from scratching or reverse playback will maintain the playback position as if the player had never entered into scratching or reverse playback.
    JSWASM bool isPerformingSlip();

/// @brief "Virtual jog wheel" or "virtual turntable" handling.
/// @param ticksPerTurn Sets the sensitivity of the virtual wheel. Use around 2300 for pixel-perfect touchscreen waveform control.
/// @param mode Jog wheel mode (scratching, pitch bend, or parameter set in the 0-1 range).
/// @param scratchSlipMs Enables slip mode for a specific amount of time for scratching, or 0 to not slip.
    JSWASM void jogTouchBegin(int ticksPerTurn, JogMode mode, unsigned int scratchSlipMs = 0);

/// @brief A jog wheel should send some "ticks" with the movement. A waveform's movement in pixels for example.
/// @param value The ticks value.
/// @param bendStretch Use time-stretching for pitch bend or not (false makes it "audible").
/// @param bendMaxPercent The maximum playback rate change for pitch bend, should be between 0.01f and 0.3f (1% and 30%).
/// @param bendHoldMs How long to maintain the pitch bend state. A value >= 1000 will hold until endContinuousPitchBend is called.
/// @param parameterModeIfNoJogTouchBegin True: if there was no jogTouchBegin, turn to JogMode_Parameter mode. False: if there was no jogTouchBegin, turn to JogMode_PitchBend mode.
    JSWASM void jogTick(int value, bool bendStretch, float bendMaxPercent, unsigned int bendHoldMs, bool parameterModeIfNoJogTouchBegin);

/// @brief Call this when the jog touch ends.
/// @param decelerate The decelerating rate for momentum. Set to 0.0f for automatic.
/// @param synchronisedStart Beat-synced start after decelerating.
    JSWASM void jogTouchEnd(float decelerate, bool synchronisedStart);

/// @brief Direct turntable handling. Call this when scratching starts.
/// @warning This is an advanced method, use it only if not using the jogT... methods.
/// @param slipMs Enable slip mode for a specific amount of time for scratching, or 0 to not slip.
/// @param stopImmediately Stop playback or not.
    JSWASM void startScratch(unsigned int slipMs, bool stopImmediately);

/// @brief Scratch movement.
/// @warning This is an advanced method, use it only if not using the jogT... methods.
/// @param pitch The current speed.
/// @param smoothing Should be between 0.05f (max. smoothing) and 1.0f (no smoothing).
    JSWASM void scratch(double pitch, float smoothing);

/// @brief Ends scratching.
/// @warning This is an advanced method, use it only if not using the jogT... methods.
/// @param returnToStateBeforeScratch Return to the previous playback state (direction, speed) or not.
    JSWASM void endScratch(bool returnToStateBeforeScratch);

/// @return Indicates if the player is in scratching mode.
    JSWASM bool isScratching();

/// @return If jog wheel mode is JogMode_Parameter, returns with the current parameter typically in the range of -1 to 1, or more than 1000000.0 if there was no jog wheel movement. processStereo or processMulti updates this value, therefore it's recommended to read it after those calls were made, in the same thread.
    JSWASM double getJogParameter();

private:
    PlayerInternals *internals;
    AdvancedAudioPlayer(const AdvancedAudioPlayer&);
    AdvancedAudioPlayer& operator=(const AdvancedAudioPlayer&);
};

}

#endif
