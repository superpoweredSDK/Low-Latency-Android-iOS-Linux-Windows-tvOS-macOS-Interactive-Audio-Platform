#ifndef Header_SuperpoweredAdvancedAudioPlayer
#define Header_SuperpoweredAdvancedAudioPlayer

struct SuperpoweredAdvancedAudioPlayerInternals;
struct SuperpoweredAdvancedAudioPlayerBase;

typedef struct stemsCompressor {
    bool enabled;
    float inputGainDb;
    float outputGainDb;
    float dryWetPercent;
    float ratio;
    float attackSec;
    float releaseSec;
    float thresholdDb;
    float hpCutoffHz;
} stemsCompressor;

typedef struct stemsLimiter {
    bool enabled;
    float releaseSec;
    float thresholdDb;
    float ceilingDb;
} stemsLimiter;

typedef struct stemsInfo {
    char *names[4];
    char *colors[4];
    stemsCompressor compressor;
    stemsLimiter limiter;
} stemsInfo;

typedef enum SuperpoweredAdvancedAudioPlayerSyncMode {
    SuperpoweredAdvancedAudioPlayerSyncMode_None,
    SuperpoweredAdvancedAudioPlayerSyncMode_Tempo,
    SuperpoweredAdvancedAudioPlayerSyncMode_TempoAndBeat
} SuperpoweredAdvancedAudioPlayerSyncMode;

typedef enum SuperpoweredAdvancedAudioPlayerJogMode {
    SuperpoweredAdvancedAudioPlayerJogMode_Scratch,
    SuperpoweredAdvancedAudioPlayerJogMode_PitchBend,
    SuperpoweredAdvancedAudioPlayerJogMode_Parameter
} SuperpoweredAdvancedAudioPlayerJogMode;

typedef enum SuperpoweredAdvancedAudioPlayerEvent {
    SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess,
    SuperpoweredAdvancedAudioPlayerEvent_LoadError,
    SuperpoweredAdvancedAudioPlayerEvent_EOF,
    SuperpoweredAdvancedAudioPlayerEvent_JogParameter,
    SuperpoweredAdvancedAudioPlayerEvent_DurationChanged,
} SuperpoweredAdvancedAudioPlayerEvent;

typedef struct hlsStreamAlternative {
    char *uri, *name, *language, *groupid;
    int bps;
    bool isDefault, isMp4a;
} hlsStreamAlternative;

#define HLS_DOWNLOAD_EVERYTHING 86401
#define HLS_DOWNLOAD_REMAINING 86400

/**
 @brief Events happen asynchronously, implement this callback to get notified.
 
 LoadSuccess and LoadError are called from an internal thread of this object.
 
 EOF (end of file) and ScratchControl are called from the (probably real-time) audio processing thread, you shouldn't do any expensive there.
 
 @param clientData Some custom pointer you set when you created a SuperpoweredAdvancedAudioPlayer instance.
 @param event What happened (load success, load error, end of file, jog parameter).
 @param value A pointer to a stemsInfo structure or NULL for LoadSuccess (you take ownership over the strings). (const char *) for LoadError, pointing to the error message. (double *) for JogParameter in the range of 0.0 to 1.0. (bool *) for EOF, set it to true to pause playback. Don't call this instance's methods from an EOF event callback!
 */
typedef void (* SuperpoweredAdvancedAudioPlayerCallback) (void *clientData, SuperpoweredAdvancedAudioPlayerEvent event, void *value);

/**
 The maximum playback or scratch speed.
 */
#define SUPERPOWEREDADVANCEDAUDIOPLAYER_MAXSPEED 20.0f


/**
 @brief High performance advanced audio player with:
 
 - time-stretching and pitch shifting,
 
 - beat and tempo sync,
 
 - scratching,
 
 - tempo bend,
 
 - looping,
 
 - slip mode,
 
 - fast seeking (cached points),
 
 - momentum and jog wheel handling,
 
 - 0 latency, real-time operation,
 
 - low memory usage (5300 kb plus 200 kb for every cached point),
 
 - thread safety (all methods are thread-safe),
 
 - direct iPod music library access.
 
 Can not be used for offline processing. Supported file types:
 - Stereo or mono pcm WAV and AIFF (16-bit int, 24-bit int, 32-bit int or 32-bit IEEE float).
 - MP3 (all kind).
 - AAC-LC in M4A container (iTunes).
 - AAC-LC in ADTS container (.aac).
 - Apple Lossless (on iOS only).
 - Http Live Streaming (HLS): vod/live/event streams, AAC-LC/MP3 in audio files or MPEG-TS files. Support for byte ranges and AES-128 encryption.
 
 @param positionMs The current position. Always accurate, no matter of time-stretching and other transformations. Read only.
 @param positionPercent The current position as a percentage (0.0f to 1.0f). Read only.
 @param positionSeconds The current position as seconds elapsed. Read only.
 @param durationMs The duration of the current track in milliseconds. Equals to UINT_MAX for live streams. Read only.
 @param durationSeconds The duration of the current track in seconds. Equals to UINT_MAX for live streams. Read only.
 @param waitingForBuffering Indicates if the player waits for audio data to be bufferred. Read only.
 @param playing Indicates if the player is playing or paused. Read only.
 @param tempo The current tempo. Read only.
 @param masterTempo Time-stretching is enabled or not. Read only.
 @param pitchShift Note offset from -12 to 12. 0 means no pitch shift. Read only.
 @param pitchShiftCents Pitch shift cents, from -1200 (one octave down) to 1200 (one octave up). 0 means no pitch shift. Read only.
 @param bpm Must be correct for syncing. There is no auto-bpm detection inside. Read only.
 @param currentBpm The actual bpm of the track (as bpm changes with the current tempo). Read only.
 @param slip If enabled, scratching or reverse will maintain the playback position as if you had never entered those modes. Read only.
 @param scratching The player is in scratching mode or not. Read only.
 @param reverse Indicates if the playback goes backwards. Read only.
 @param looping Indicates if looping is enabled. Read only.
 @param firstBeatMs Tells where the first beat (the beatgrid) begins. Must be correct for syncing. Read only.
 @param msElapsedSinceLastBeat How many milliseconds elapsed since the last beat. Read only.
 @param beatIndex Which beat has just happened (1 [1.0f-1.999f], 2 [2.0f-2.999f], 3 [3.0f-3.99f], 4 [4.0f-4.99f]). A value of 0 means "don't know". Read only.
 @param bufferStartPercent What is buffered from the original source, start point. Will always be 0 for non-network sources (files). Read only.
 @param bufferEndPercent What is buffered from the original source, end point. Will always be 1.0f for non-network sources (files). Read only.
 @param currentBps The current download speed.
 @param syncMode The current sync mode (off, tempo, or tempo and beat).
 @param fixDoubleOrHalfBPM If tempo is >1.4f or <0.6f, it will treat the bpm as half or double. Good for certain genres. False by default.
 @param waitForNextBeatWithBeatSync Wait for the next beat if beat-syncing is enabled. False by default.
 @param dynamicHLSAlternativeSwitching Dynamicly changing the current HLS alternative to match the available network bandwidth. Default is true.
 @param reverseToForwardAtLoopStart If looping and playback direction is reverse, reaching the beginning of the loop will change direction to forward. True by default.
 @param downloadSecondsAhead The HLS content download strategy: how many seconds ahead of the playback position to download. Default is HLS_DOWNLOAD_REMAINING, meaning it will download everything after the playback position, until the end. HLS_DOWNLOAD_EVERYTHING downloads before the playback position too.
 @param minTimeStretchingTempo Will not time-stretch, just resample below this tempo. Default: 0.501f (recommended value for low CPU on older mobile devices, such as the first iPad). Set this before an open() call. 
 @param maxTimeStretchingTempo Will not time-stretch, just resample above this tempo. Default: 2.0f (recommended value for low CPU on older mobile devices, such as the first iPad).
*/
class SuperpoweredAdvancedAudioPlayer {
public:
// READ ONLY parameters, don't set them directly, use the methods below.
    double positionMs;
    float positionPercent;
    unsigned int positionSeconds;
    unsigned int durationMs;
    unsigned int durationSeconds;
    bool waitingForBuffering;
    bool playing;

    double tempo;
    bool masterTempo;
    int pitchShift;
    int pitchShiftCents;
    double bpm;
    double currentBpm;
    
    bool slip;
    bool scratching;
    bool reverse;
    bool looping;
    
    double firstBeatMs;
    double msElapsedSinceLastBeat;
    float beatIndex;

    float bufferStartPercent;
    float bufferEndPercent;
    int currentBps;

// READ-WRITE parameters
    SuperpoweredAdvancedAudioPlayerSyncMode syncMode;
    bool fixDoubleOrHalfBPM;
    bool waitForNextBeatWithBeatSync;
    bool dynamicHLSAlternativeSwitching;
    bool reverseToForwardAtLoopStart;
    int downloadSecondsAhead;
    float minTimeStretchingTempo;
    float maxTimeStretchingTempo;

/**
 @brief Set the folder path for temporary files. Used for HLS only. 
 
 Call this first before any player instance is created. It will create a subfolder with the name "SuperpoweredHLS" in this folder.
 
 @param path File system path of the folder.
 */
    static void setTempFolder(const char *path);

/**
 @brief Remove the temp folder. Use this when your program ends.
 */
    static void clearTempFolder();
    
/**
 @brief Create a player instance with the current sample rate value.
 
 Example: SuperpoweredAdvancedAudioPlayer player = new SuperpoweredAdvancedAudioPlayer(this, playerCallback, 44100, 4);
 
 @param clientData A custom pointer your callback receives.
 @param callback Your callback to receive player events.
 @param samplerate The current samplerate.
 @param cachedPointCount Sets how many positions can be cached in the memory. Jumping to a cached point happens with 0 latency. Loops are automatically cached.
*/
    SuperpoweredAdvancedAudioPlayer(void *clientData, SuperpoweredAdvancedAudioPlayerCallback callback, unsigned int samplerate, unsigned int cachedPointCount);
    ~SuperpoweredAdvancedAudioPlayer();
/**
 @brief Opens a new audio file, with playback paused. 
 
 Tempo, pitchShift, masterTempo and syncMode are NOT changed if you open a new one.
 
 @param path The full file system path of the audio file.
 @param customHTTPHeaders NULL terminated list of custom headers for http communication.
 @param stemsIndex The stems track index for Native Instruments Stems format (0 for sum track, 1/2/3/4 for stem tracks).
*/
    void open(const char *path, char **customHTTPHeaders = 0, int stemsIndex = 0);
    
/**
 @brief Opens a file, with playback paused.
 
 Tempo, pitchShift, masterTempo and syncMode are NOT changed if you open a new one.
 
 @param path The full file system path of the file.
 @param offset The byte offset inside the file.
 @param length The byte length from the offset.
 @param customHTTPHeaders NULL terminated list of custom headers for http communication.
 @param stemsIndex The stems track index for Native Instruments Stems format.
*/
    void open(const char *path, int offset, int length, char **customHTTPHeaders = 0, int stemsIndex = 0);

/**
 @brief Starts playback.
 
 @param synchronised Set it to true for a beat-synced or tempo-synced start.
 */
    void play(bool synchronised);
    
/**
 @brief Pause playback. 
 
 There is no need for a "stop" method, this player is very efficient with the battery and has no significant "stand-by" processing.
 
 @param decelerateSeconds Optional momentum. 0 means pause immediately.
 @param slipMs Enable slip mode for a specific amount of time, or 0 to not slip.
 */
    void pause(float decelerateSeconds = 0, unsigned int slipMs = 0);
    
/**
 @brief Toggle play/pause.
 */
    void togglePlayback();
/**
 @brief Simple seeking to a percentage.
 */
    void seek(double percent);
/**
 @brief Precise seeking.
 
 @param ms Position in milliseconds.
 @param andStop If true, stops playback.
 @param synchronisedStart If andStop is false, a beat-synced start is possible.
 */
    void setPosition(double ms, bool andStop, bool synchronisedStart);
/**
 @brief Cache a position for zero latency seeking. It will cache around +/- 1 second around this point.
 
 @param ms Position in milliseconds.
 @param pointID Use this to provide a custom identifier, so you can overwrite the same point later. Use 255 for a point with no identifier.
*/
    void cachePosition(double ms, unsigned char pointID);
/**
 @brief Loop from a start point with some length.
 
 @param startMs Loop from this millisecond.
 @param lengthMs Length in millisecond.
 @param jumpToStartMs If the playhead is within the loop, jump to startMs or not.
 @param pointID Looping caches startMs, so you can specify a pointID too (or set to 255 if you don't care).
 @param synchronisedStart Beat-synced start.
 */
    bool loop(double startMs, double lengthMs, bool jumpToStartMs, unsigned char pointID, bool synchronisedStart);
/**
 @brief Loop from a start to an end point.
     
 @param startMs Loop from this millisecond.
 @param endMs Loop to this millisecond.
 @param jumpToStartMs If the playhead is within the loop, jump to startMs or not.
 @param pointID Looping caches startMs, so you can specify a pointID too (or set to 255 if you don't care).
 @param synchronisedStart Beat-synced start.
*/
    bool loopBetween(double startMs, double endMs, bool jumpToStartMs, unsigned char pointID, bool synchronisedStart);
/**
 @brief Exits from the current loop.
 */
    void exitLoop();
/**
 @brief Checks if ms fall into the current loop.
 
 @param ms The position to check in milliseconds.
 */
    bool msInLoop(double ms);
/**
 @brief There is no auto-bpm detection inside, you must set the original bpm of the track with this for syncing.
 
 Should be called after a successful open().
 
 @param newBpm The bpm value. A number below 10.0f means "bpm unknown", and sync will not work.
*/
    void setBpm(double newBpm);
/**
  @brief Beat-sync works only if the first beat's position is known. Set it here.
 
  Should be called after a successful open().
 */
    void setFirstBeatMs(double ms);
/**
 @brief Shows you where the closest beat is to a specific position.
 
 @param ms The position in milliseconds.
 @param beatIndex Set to 0 if beat index is not important, 1-4 otherwise.
*/
    double closestBeatMs(double ms, unsigned char beatIndex);
    
/**
 @brief "Virtual jog wheel" or "virtual turntable" handling. 
 
 @param ticksPerTurn Sets the sensitivity of the virtual wheel. Use around 2300 for pixel-perfect touchscreen waveform control.
 @param mode Jog wheel mode (scratching, pitch bend, or parameter set in the range 0.0 to 1.0).
 @param scratchSlipMs Enable slip mode for a specific amount of time for scratching, or 0 to not slip.
*/
    void jogTouchBegin(int ticksPerTurn, SuperpoweredAdvancedAudioPlayerJogMode mode, unsigned int scratchSlipMs);
/**
 @brief A jog wheel should send some "ticks" according to the movement. A waveform's movement in pixels for example.
 
 @param value The cumulated ticks value.
 @param bendStretch Use time-stretching for bending or not (false makes it "audible").
 @param bendMaxPercent The maximum tempo change for pitch bend, should be between 0.01f and 0.3f (1% and 30%).
 @param bendHoldMs How long to maintain the bended state. A value >= 1000 will hold until endContinuousPitchBend is called.
 @param parameterMode True: if there was no jogTouchBegin, SuperpoweredAdvancedAudioPlayerJogMode_Parameter applies. False: if there was no jogTouchBegin, SuperpoweredAdvancedAudioPlayerJogMode_PitchBend applies.
*/
    void jogTick(int value, bool bendStretch, float bendMaxPercent, unsigned int bendHoldMs, bool parameterMode);
/**
 @brief Call this when the jog touch ends.
 
 @param decelerate The decelerating rate for momentum. Set to 0.0f for automatic.
 @param synchronisedStart Beat-synced start after decelerating.
 */
    void jogTouchEnd(float decelerate, bool synchronisedStart);
/**
 @brief Sets the relative tempo of the playback.
 
 @param tempo 1.0f is "original speed".
 @param masterTempo Enable or disable time-stretching.
 */
    void setTempo(double tempo, bool masterTempo);
/**
 @brief Sets the pitch shift value. Needs masterTempo enabled.
 
 @param pitchShift Note offset from -12 to 12. 0 means no pitch shift.
 */
    void setPitchShift(int pitchShift);
    
/**
 @brief Sets the pitch shift value with greater precision. Calling this method requires magnitudes more CPU than setPitchShift.
 
 @param pitchShiftCents Limited to >= -1200 and <= 1200. 0 means no pitch shift.
 */
    void setPitchShiftCents(int pitchShiftCents);

/**
 @brief Sets playback direction.
 
 @param reverse Playback direction.
 @param slipMs Enable slip mode for a specific amount of time, or 0 to not slip.
 */
    void setReverse(bool reverse, unsigned int slipMs);
/**
 @brief Pitch bend (temporary tempo change).
 
 @param maxPercent The maximum tempo change for pitch bend, should be between 0.01f and 0.3f (1% and 30%).
 @param bendStretch Use time-stretching for bending or not (false makes it "audible").
 @param faster Playback speed change direction.
 @param holdMs How long to maintain the bended state. A value >= 1000 will hold until endContinuousPitchBend is called.
*/
    void pitchBend(float maxPercent, bool bendStretch, bool faster, unsigned int holdMs);
/**
 @brief Ends pitch bend.
 */
    void endContinuousPitchBend();
/**
 @brief Call when scratching starts.
 
 @warning This is an advanced method, use it only if you don't want the jogT... methods.
 
 @param slipMs Enable slip mode for a specific amount of time for scratching, or 0 to not slip.
 @param stopImmediately Stop now or not.
 */
    void startScratch(unsigned int slipMs, bool stopImmediately);
/**
 @brief Scratch movement.
 
 @warning This is an advanced method, use it only if you don't want the jogT... methods.
 
 @param pitch The current speed.
 @param smoothing Should be between 0.05f (max. smoothing) and 1.0f (no smoothing).
 */
    void scratch(double pitch, float smoothing);
/**
 @brief Ends scratching.
 
 @warning This is an advanced method, use it only if you don't want the jogT... methods.
 
 @param returnToStateBeforeScratch Return to the previous playback state (direction, speed) or not.
 */
    void endScratch(bool returnToStateBeforeScratch);
/**
 @brief Returns the last process() numberOfSamples converted to milliseconds.
 */
    double lastProcessMs();
/**
 @brief Sets the sample rate.
     
 @param samplerate 44100, 48000, etc.
*/
    void setSamplerate(unsigned int samplerate);
/**
 @brief Call this on a phone call or other interruption.
 
 Apple's built-in codec may be used in some cases, for example ALAC files. 
 Call this after a media server reset or audio session interrupt to resume playback.
*/
    void onMediaserverInterrupt();

/**
 @brief Processes the audio.
 
 @return Put something into output or not.
 
 @param buffer 32-bit interleaved stereo input/output buffer. Should be numberOfSamples * 8 + 64 bytes big.
 @param bufferAdd If true, the contents of buffer will be preserved and audio will be added to them. If false, buffer is completely overwritten.
 @param numberOfSamples The number of samples to provide.
 @param volume 0.0f is silence, 1.0f is "original volume". Changes are automatically smoothed between consecutive processes.
 @param masterBpm A bpm value to sync with. Use 0.0f for no syncing.
 @param masterMsElapsedSinceLastBeat How many milliseconds elapsed since the last beat on the other stuff we are syncing to. Use -1.0 to ignore.
*/
    bool process(float *buffer, bool bufferAdd, unsigned int numberOfSamples, float volume = 1.0f, double masterBpm = 0.0f, double masterMsElapsedSinceLastBeat = -1.0);
    
private:
    SuperpoweredAdvancedAudioPlayerInternals *internals;
    SuperpoweredAdvancedAudioPlayerBase *base;
    SuperpoweredAdvancedAudioPlayer(const SuperpoweredAdvancedAudioPlayer&);
    SuperpoweredAdvancedAudioPlayer& operator=(const SuperpoweredAdvancedAudioPlayer&);
};

#endif
