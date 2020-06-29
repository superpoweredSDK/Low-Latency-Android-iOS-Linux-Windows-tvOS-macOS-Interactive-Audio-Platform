#ifndef Header_SuperpoweredAdvancedAudioPlayer
#define Header_SuperpoweredAdvancedAudioPlayer

#include "SuperpoweredDecoder.h"

namespace Superpowered {

class AdvancedAudioPlayer;
struct PlayerSwap;
struct PlayerInternals;

/// @brief Status codes.
#define Player_Success 0
#define PlayerError_FileTooShort         2000 ///< The audio has less than 512 frames.
#define HLSError_PlaylistTypeMismatch    2001 ///< Some alternatives in the HLS stream are different to the first one.
#define HLSError_Empty                   2002 ///< The HLS stream contains no alternatives.

/// @brief Synchronization modes.
typedef enum SyncMode {
    SyncMode_None,        ///< No synchronization.
    SyncMode_Tempo,       ///< Sync tempo only.
    SyncMode_TempoAndBeat ///< Sync tempo and beat.
} SyncMode;

/// @brief Jog Wheel Mode, to be used with the jogT... methods.
typedef enum JogMode {
    JogMode_Scratch,   ///< Jog wheel controls scratching.
    JogMode_PitchBend, ///< Jog wheel controls pitch bend.
    JogMode_Parameter  ///< Jog wheel changes a parameter.
} JogMode;

/// @brief Player events.
typedef enum PlayerEvent {
    PlayerEvent_None = 0,       ///< Open was not called yet.
    PlayerEvent_Opening = 1,    ///< Trying to open the content.
    PlayerEvent_OpenFailed = 2, ///< Failed to open the content.
    PlayerEvent_Opened = 10,    ///< Successfully opened the content, playback can begin.
    PlayerEvent_ConnectionLost = 3,              ///< Network connection lost to the HLS stream or progressive download. Can only be "recovered" by a new open(). May happen after PlayerEvent_Opened has been delivered.
    PlayerEvent_ProgressiveDownloadFinished = 11 ///< The content finished downloading and is fully available locally. May happen after PlayerEvent_Opened has been delivered.
} PlayerEvent;

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
/// Can be used in a real-time audio processing thread. Can not be used for offline processing.
/// Supported file types:
/// - Stereo or mono pcm WAV and AIFF (16-bit int, 24-bit int, 32-bit int or 32-bit IEEE float).
/// - MP3: MPEG-1 Layer III (sample rates: 32000 Hz, 44100 Hz, 48000 Hz). MPEG-2 Layer III is not supported (mp3 with sample rates below 32000 Hz).
/// - AAC or HE-AAC in M4A container (iTunes) or ADTS container (.aac).
/// - ALAC/Apple Lossless (on iOS only).
/// - Http Live Streaming (HLS): vod/live/event streams, AAC-LC/MP3 in audio files or MPEG-TS files. Support for byte ranges and AES-128 encryption.
class AdvancedAudioPlayer {
public:
    unsigned int outputSamplerate;           ///< The player output sample rate in Hz.
    double playbackRate;                     ///< The playback rate. Must be positive and above 0.00001. Default: 1.
    bool timeStretching;                     ///< Enable/disable time-stretching. Default: true.
    float minimumTimestretchingPlaybackRate; ///< Will not time-stretch but resample below this playback rate. Default: 0.501f (the recommended value for low CPU load on older mobile devices, such as the first iPad). Will be applied after changing playbackRate or scratching.
    float maximumTimestretchingPlaybackRate; ///< Will not time-stretch but resample above this playback rate. Default: 2.0f (the recommended value for low CPU load on older mobile devices, such as the first iPad). Will be applied after changing playbackRate or scratching.
    double originalBPM;                      ///< The original bpm of the current music. There is no auto-bpm detection inside, this must be set to a correct value for syncing. Maximum 300. A value below 20 will be automatically set to 0. Default: 0 (no bpm value known).
    bool fixDoubleOrHalfBPM;                 ///< If true and playbackRate is above 1.4f or below 0.6f, it will sync the tempo as half or double. Default: false.
    double firstBeatMs;                      ///< Tells where the first beat is (the beatgrid starts). Must be set to a correct value for syncing. Default: 0.
    double defaultQuantum;                   ///< Sets the quantum for quantized synchronization. Example: 4 means 4 beats.
    SyncMode syncMode;                       ///< The current sync mode (off, tempo, or tempo+beat). Default: off.
    double syncToBpm;                        ///< A bpm value to sync with. Use 0.0f for no syncing.
    double syncToMsElapsedSinceLastBeat;     ///< The number of milliseconds elapsed since the last beat on audio the player has to sync with. Use -1.0 to ignore.
    double syncToPhase;                      ///< Used for quantized synchronization. The phase to sync with.
    double syncToQuantum;                    ///< Used for quantized synchronization. The quantum to sync with.
    int pitchShiftCents;                     ///< Pitch shift cents, from -1200 (one octave down) to 1200 (one octave up). Use values representing notes (multiply of 100) for low CPU load. Default: 0 (no pitch shift).
    bool loopOnEOF;                          ///< If true, jumps back and continues playback. If false, playback stops. Default: false.
    bool reverseToForwardAtLoopStart;        ///< If this is true with playing backwards and looping, then reaching the beginning of the loop will change playback direction to forwards. Default: false.
    bool enableStems;                        ///< If true and a Native Instruments STEMS file is loaded, output 4 stereo channels. Default: false (stereo master mix output).
    bool HLSAutomaticAlternativeSwitching;   ///< If true, then the player will automatically swtich between the HLS alternatives according to the available network bandwidth. Default: true.
    char HLSLiveLatencySeconds;              ///< When connecting or reconnecting to a HLS live stream, the player will try to skip audio to maintain this latency. Default: -1 (the player wil not skip audio and the live stream starts at the first segment specified by the server).
    int HLSMaximumDownloadAttempts;          ///< How many times to retry if a HLS segment download fails. Default: 100.
    int HLSBufferingSeconds;                 ///< How many seconds ahead of the playback position to download. Default value: HLS_DOWNLOAD_REMAINING.
    static const int HLSDownloadEverything;  ///< Will download everything after the playback position until the end.
    static const int HLSDownloadRemaining;   ///< Downloads everything from the beginning to the end, regardless the playback position.
    static const float MaxPlaybackRate;      ///< The maximum playback rate or scratching speed: 20.
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
/// @param code The return value of the Decoder::open method.
    static const char *statusCodeToString(int code);
    
/// @brief Creates a player instance with the current sample rate value.
/// @param samplerate The initial sample rate of the player output in hz.
/// @param cachedPointCount How many positions can be cached in the memory. Jumping to a cached point happens with zero latency. Loops are automatically cached.
/// @param internalBufferSizeSeconds The number of seconds to buffer internally for playback and cached points. Minimum 2, maximum 60. Default: 2.
/// @param negativeSeconds The number of seconds of silence in the negative direction, before the beginning of the track.
    AdvancedAudioPlayer(unsigned int samplerate, unsigned char cachedPointCount, unsigned int internalBufferSizeSeconds = 2, unsigned int negativeSeconds = 0);
    ~AdvancedAudioPlayer();
    
/// @brief Opens an audio file with playback paused.
/// Playback rate, pitchShift, timeStretching and syncMode are NOT changed if you open a new file.
/// @warning This method has no effect if the previous open didn't finish or if called in the audio processing thread.
/// @param path Full file system path or progressive download path (http or https).
/// @param customHTTPRequest If custom HTTP communication is required (such as sending http headers for authorization), pass a fully prepared http request object. The player will copy this object.
/// @param skipSilenceAtBeginning If true, the player will set the position to skip the initial digital silence of the audio file (up to 10 seconds).
    void open(const char *path, Superpowered::httpRequest *customHTTPRequest = 0, bool skipSilenceAtBeginning = false);
    
/// @brief Opens an audio file with playback paused.
/// Playback rate, pitchShift, timeStretching and syncMode are NOT changed if you open a new file.
/// @warning This method has no effect if the previous open didn't finish or if called in the audio processing thread.
/// @param path Full file system path or progressive download path (http or https).
/// @param offset The byte offset inside the path.
/// @param length The byte length from the offset.
/// @param customHTTPRequest If custom HTTP communication is required (such as sending http headers for authorization), pass a fully prepared http request object. The player will copy this object.
/// @param skipSilenceAtBeginning If true, the player will set the position to skip the initial digital silence of the audio file (up to 10 seconds).
    void open(const char *path, int offset, int length, Superpowered::httpRequest *customHTTPRequest = 0, bool skipSilenceAtBeginning = false);
    
/// @brief Opens a HTTP Live Streaming stream with playback paused.
/// Playback rate, pitchShift, timeStretching and syncMode are NOT changed if you open a new one.
/// Do not call openHLS() in the audio processing thread.
/// @param url Stream URL.
/// @param customHTTPRequest If custom HTTP communication is required (such as sending http headers for authorization), pass a fully prepared http request object. The player will copy this object.
    void openHLS(const char *url, Superpowered::httpRequest *customHTTPRequest = 0);
    
/// @return Returns with the latest player event. This method should be used in a periodically running code, at one place only, because it returns a specific event just once per open() call. Best to be used in a UI loop.
    PlayerEvent getLatestEvent();

/// @return If getLatestEvent returns with OpenFailed, retrieve the error code or HTTP status code here.
    int getOpenErrorCode();
    
/// @return Returns with the full filesystem path of the locally cached file if the player is in the PlayerEvent_Opened_ProgressiveDownloadFinished state, NULL otherwise.
    const char *getFullyDownloadedFilePath();
    
/// @return Returns true if end-of-file has been reached recently (will never indicate end-of-file if loopOnEOF is true). This method should be used in a periodically running code at one place only, because it returns a specific end-of-file event just once. Best to be used in a UI loop.
    bool eofRecently();
 
/// @return Indicates if the player is waiting for data (such as waiting for a network download).
    bool isWaitingForBuffering();
    
/// @return Returns with the length of the digital silence at the beginning of the file if open() was called skipSilenceAtBeginning = true, 0 otherwise.
    double getAudioStartMs();
            
/// @return The current playhead position in milliseconds. Not changed by any pending setPosition() or seek() call, always accurate regardless of time-stretching and other transformations.
    double getPositionMs();
    
/// @return The current position in milliseconds, immediately updated after setPosition() or seek(). Use this for UI display.
    double getDisplayPositionMs();
    
/// @return Similar to getDisplayPositionMs(), but as a percentage (0 to 1).
    float getDisplayPositionPercent();
    
/// @return Similar to getDisplayPositionMs(), but as seconds elapsed.
    int getDisplayPositionSeconds();
    
/// @return The duration of the current track in milliseconds. Returns UINT_MAX for live streams.
    unsigned int getDurationMs();
    
/// @return The duration of the current track in seconds. Returns UINT_MAX for live streams.
    unsigned int getDurationSeconds();
    
/// @brief Starts playback immediately without any synchronization.
    void play();
    
/// @brief Starts beat or tempo synchronized playback.
    void playSynchronized();
    
/// @brief Starts playback at a specific position. isPlaying() will return false until this function succeeds starting playback at the specified position.
/// You can call this in a real-time thread (audio processing callback) with a continuously updated time for a precise on-the-fly launch.
/// @param positionMs Start position in milliseconds.
    void playSynchronizedToPosition(double positionMs);
    
/// @brief Pause playback.
/// There is no need for a "stop" method, this player is very efficient with the battery and has no significant "stand-by" processing.
/// @param decelerateSeconds Optional momentum. 0 means to pause immediately.
/// @param slipMs Enable slip mode for a specific amount of time, or 0 to not slip.
    void pause(float decelerateSeconds = 0, unsigned int slipMs = 0);
    
/// @brief Toggle play/pause (no synchronization).
    void togglePlayback();
    
/// @return Indicates if the player is playing or paused.
    bool isPlaying();
    
/// @brief Simple seeking to a percentage.
/// @param percent The position in percentage.
    void seek(double percent);
    
/// @brief Precise seeking.
/// @param ms Position in milliseconds.
/// @param andStop If true, stops playback.
/// @param synchronisedStart If andStop is false, makes a beat-synced start possible.
/// @param forceDefaultQuantum If true and using quantized synchronization, will use the defaultQuantum instead of the syncToQuantum.
/// @param preferWaitingforSynchronisedStart Wait or start immediately when synchronized.
    void setPosition(double ms, bool andStop, bool synchronisedStart, bool forceDefaultQuantum = false, bool preferWaitingforSynchronisedStart = false);
    
/// @brief Caches a position for zero latency seeking.
/// @param ms Position in milliseconds.
/// @param pointID Use this to provide a custom identifier, so you can overwrite the same point later. Use 255 for a point with no identifier.
    void cachePosition(double ms, unsigned char pointID = 255);
            
/// @brief Processes audio, stereo version.
/// @return True: buffer has audio output from the player. False: the contents of buffer were not changed (typically happens when the player is paused).
/// @warning Duration may change to a more precise value after this, because some file formats have no precise duration information.
/// @param buffer Pointer to floating point numbers. 32-bit interleaved stereo input/output buffer. Should be numberOfFrames * 8 + 64 bytes big.
/// @param mix If true, the player output will be mixed with the contents of buffer. If false, the contents of buffer will be overwritten with the player output.
/// @param numberOfFrames The number of frames requested.
/// @param volume 0.0f is silence, 1.0f is "original volume". Changes are automatically smoothed between consecutive processes.
/// @param jogParameter If jog wheel mode is JogMode_Parameter, returns with the current parameter typically in the range of -1 to 1, or less than -1000000.0 if there was no jog wheel movement. Can be NULL if not interested.
    bool processStereo(float *buffer, bool mix, unsigned int numberOfFrames, float volume = 1.0f, double *jogParameter = 0);

/// @brief Processes audio, multi-channel version.
/// @return True: buffers has audio output from the player. False: the contents of buffer were not changed (typically happens when the player is paused).
/// @warning Duration may change to a more precise value after this, because some file formats have no precise duration information.
/// @param buffers 32-bit interleaved stereo input/output buffer pairs. Each pair should be numberOfFrames * 8 + 64 bytes big.
/// @param mix If true, the player output will be added to the contents of buffers. If false, the contents of buffers will be overwritten with the player output.
/// @param numberOfFrames The number of frames requested.
/// @param volumes A volume for each buffer in buffers. 0.0f is silence, 1.0f is "original volume". Changes are automatically smoothed between consecutive processes.
/// @param jogParameter If jog wheel mode is JogMode_Parameter, returns with the current parameter typically in the range of -1 to 1, or more than 1000000.0 if there was no jog wheel movement. Can be NULL if not interested.
    bool processMulti(float **buffers, bool mix, unsigned int numberOfFrames, float *volumes, double *jogParameter = 0);
    
/// @return Returns true if a STEMS file was loaded (and enableStems is also true).
    bool isStems();
    
/// @brief Performs the last stage of STEMS processing, the master compressor and limiter. Works only if a STEMS file was loaded.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input buffer.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output buffer.
/// @param numberOfFrames The number of frames to process.
/// @param volume 0.0f is silence, 1.0f is "original volume". Changes are automatically smoothed between consecutive processes.
    void processSTEMSMaster(float *input, float *output, unsigned int numberOfFrames, float volume = 1.0f);
    
/// @return Returns with a stem's name if a STEMS file was loaded, NULL otherwise.
    const char *getStemName(unsigned char index);
    
/// @return Returns with a stem's color if a STEMS file was loaded, NULL otherwise.
    const char *getStemColor(unsigned char index);
    
/// @brief Apple's built-in codec may be used in some cases, such as decoding ALAC files. Call this after a media server reset or audio session interrupt to resume playback.
    void onMediaserverInterrupt();
    
/// @return Returns with the beginning of the buffered part. Will always be 0 for non-network sources (such as local files).
    float getBufferedStartPercent();
    
/// @return Returns with the end of the buffered part. Will always be 1.0f for non-network sources (such as local files).
    float getBufferedEndPercent();
    
/// @return For HLS only. Returns with the actual network throughput (for best stream selection).
    unsigned int getCurrentHLSBPS();
    
/// @return The current bpm of the track (as changed by the playback rate).
    double getCurrentBpm();
        
/// @return How many milliseconds elapsed since the last beat.
    double getMsElapsedSinceLastBeat();
        
/// @return Which beat has just happened. Possible values:
/// 0        : unknown
/// 1 - 1.999: first beat
/// 2 - 2.999: second beat
/// 3 - 3.999: third beat
/// 4 - 4.999: fourth beat
    float getBeatIndex();
        
/// @return Returns with the current phase for quantized synchronization.
    double getPhase();
        
/// @return Returns with the current quantum for quantized synchronization.
    double getQuantum();
    
/// @return Returns with the distance (in milliseconds) to a specific quantum and phase for quantized synchronization.
/// @param phase The phase to calculate against.
/// @param quantum The quantum to calculate against.
    double getMsDifference(double phase, double quantum);
    
/// @return Indicates if the player is waiting to start playback on a synchronization event (such as the next beat) for how many milliseconds (continously updated). Happens when you call a method with synchronisedStart enabled. 0 means not waiting.
    double getWaitingForSyncStartMs();
        
/// @return Indicates if the player is waiting for a synchronization event (such as the next beat) for how many milliseconds (continously updated). 0 means not waiting.
    double getWillSyncMs();
    
/// @brief Loop from a start point to some length.
/// @param startMs Loop from this milliseconds.
/// @param lengthMs Loop length in milliseconds.
/// @param jumpToStartMs If the playhead is within the loop, jump to startMs or not.
/// @param pointID Looping caches startMs, therefore you can specify a pointID too (or set to 255 if you don't care).
/// @param synchronisedStart Beat-synced start.
/// @param numLoops Number of times to loop. 0 means: until exitLoop() is called.
/// @param forceDefaultQuantum If true and using quantized synchronization, will use the defaultQuantum instead of the syncToQuantum.
/// @param preferWaitingforSynchronisedStart Wait or start immediately when synchronized.
    bool loop(double startMs, double lengthMs, bool jumpToStartMs, unsigned char pointID, bool synchronisedStart, unsigned int numLoops = 0, bool forceDefaultQuantum = false, bool preferWaitingforSynchronisedStart = false);

/// @brief Loop between a start and end points.
/// @param startMs Loop from this milliseconds.
/// @param endMs Loop to this milliseconds.
/// @param jumpToStartMs If the playhead is within the loop, jump to startMs or not.
/// @param pointID Looping caches startMs, therefore you can specify a pointID too (or set to 255 if you don't care).
/// @param synchronisedStart Beat-synced start.
/// @param numLoops Number of times to loop. 0 means: until exitLoop() is called.
/// @param forceDefaultQuantum If true and using quantized synchronization, will use the defaultQuantum instead of the syncToQuantum.
/// @param preferWaitingforSynchronisedStart Wait or start immediately when synchronized.
    bool loopBetween(double startMs, double endMs, bool jumpToStartMs, unsigned char pointID, bool synchronisedStart, unsigned int numLoops = 0, bool forceDefaultQuantum = false, bool preferWaitingforSynchronisedStart = false);
        
/// @brief Exit from the current loop.
/// @param synchronisedStart Synchronized start or re-synchronization after the loop exit.
    void exitLoop(bool synchronisedStart = false);
    
/// @return Indicates if looping is enabled.
    bool isLooping();
        
/// @return Returns true if a position is inside the current loop.
/// @param ms The position in milliseconds.
    bool msInLoop(double ms);
    
/// @return Returns with the position of the closest beat.
/// @param ms The position in milliseconds where to find the closest beat.
/// @param beatIndex Pointer to a beat index value. Set to NULL if beat index is not important. Set to 0 to retrieve the beat index of the position. Set to 1-4 to retrieve the position of a specific beat index.
    double closestBeatMs(double ms, unsigned char *beatIndex = 0);
    
/// @brief Sets playback direction.
/// @param reverse True: reverse. False: forward.
/// @param slipMs Enable slip mode for a specific amount of time, or 0 to not slip.
    void setReverse(bool reverse, unsigned int slipMs = 0);
    
/// @return If true, the player is playing backwards.
    bool isReverse();

/// @brief Starts on changes pitch bend (temporary playback rate change).
/// @param maxPercent The maximum playback rate range for pitch bend, should be between 0.01f and 0.3f (1% and 30%).
/// @param bendStretch Use time-stretching for pitch bend or not (false makes it "audible").
/// @param faster True: faster, false: slower.
/// @param holdMs How long to maintain the pitch bend state. A value >= 1000 will hold until endContinuousPitchBend is called.
    void pitchBend(float maxPercent, bool bendStretch, bool faster, unsigned int holdMs);
        
/// @brief Ends pitch bend.
    void endContinuousPitchBend();
    
/// @return Returns with the distance (in milliseconds) to the beatgrid while using pitch bend for correction.
    double getBendOffsetMs();
    
/// @brief Reset the pitch bend offset to the beatgrid to zero.
    void resetBendMsOffset();
    
/// @return Indicates if returning from scratching or reverse playback will maintain the playback position as if the player had never entered into scratching or reverse playback.
    bool isPerformingSlip();
    
/// @brief "Virtual jog wheel" or "virtual turntable" handling.
/// @param ticksPerTurn Sets the sensitivity of the virtual wheel. Use around 2300 for pixel-perfect touchscreen waveform control.
/// @param mode Jog wheel mode (scratching, pitch bend, or parameter set in the 0-1 range).
/// @param scratchSlipMs Enables slip mode for a specific amount of time for scratching, or 0 to not slip.
    void jogTouchBegin(int ticksPerTurn, JogMode mode, unsigned int scratchSlipMs = 0);
        
/// @brief A jog wheel should send some "ticks" with the movement. A waveform's movement in pixels for example.
/// @param value The icks value.
/// @param bendStretch Use time-stretching for pitch bend or not (false makes it "audible").
/// @param bendMaxPercent The maximum playback rate change for pitch bend, should be between 0.01f and 0.3f (1% and 30%).
/// @param bendHoldMs How long to maintain the pitch bend state. A value >= 1000 will hold until endContinuousPitchBend is called.
/// @param parameterModeIfNoJogTouchBegin True: if there was no jogTouchBegin, turn to JogMode_Parameter mode. False: if there was no jogTouchBegin, turn to JogMode_PitchBend mode.
    void jogTick(int value, bool bendStretch, float bendMaxPercent, unsigned int bendHoldMs, bool parameterModeIfNoJogTouchBegin);

/// @brief Call this when the jog touch ends.
/// @param decelerate The decelerating rate for momentum. Set to 0.0f for automatic.
/// @param synchronisedStart Beat-synced start after decelerating.
    void jogTouchEnd(float decelerate, bool synchronisedStart);
    
/// @brief Direct turntable handling. Call this when scratching starts.
/// @warning This is an advanced method, use it only if not using the jogT... methods.
/// @param slipMs Enable slip mode for a specific amount of time for scratching, or 0 to not slip.
/// @param stopImmediately Stop playback or not.
    void startScratch(unsigned int slipMs, bool stopImmediately);
        
/// @brief Scratch movement.
/// @warning This is an advanced method, use it only if not using the jogT... methods.
/// @param pitch The current speed.
/// @param smoothing Should be between 0.05f (max. smoothing) and 1.0f (no smoothing).
    void scratch(double pitch, float smoothing);
        
/// @brief Ends scratching.
/// @warning This is an advanced method, use it only if not using the jogT... methods.
/// @param returnToStateBeforeScratch Return to the previous playback state (direction, speed) or not.
    void endScratch(bool returnToStateBeforeScratch);
    
/// @return Indicates if the player is in scratching mode.
    bool isScratching();
    
private:
    PlayerSwap *swap;
    PlayerInternals *internals;
    AdvancedAudioPlayer(const AdvancedAudioPlayer&);
    AdvancedAudioPlayer& operator=(const AdvancedAudioPlayer&);
};

}

#endif
