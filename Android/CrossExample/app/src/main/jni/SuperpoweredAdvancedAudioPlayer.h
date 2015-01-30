#ifndef Header_SuperpoweredAdvancedAudioPlayer
#define Header_SuperpoweredAdvancedAudioPlayer

struct SuperpoweredAdvancedAudioPlayerInternals;
struct SuperpoweredAdvancedAudioPlayerBase;

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
    SuperpoweredAdvancedAudioPlayerEvent_JogParameter
} SuperpoweredAdvancedAudioPlayerEvent;


/**
 @brief Events happen asynchronously, implement this callback to get notified.
 
 LoadSuccess and LoadError are called from an internal thread of this object.
 
 EOF (end of file) and ScratchControl are called from the (probably real-time) audio processing thread, you shouldn't do any expensive there.
 
 @param clientData Some custom pointer you set when you created a SuperpoweredAdvancedAudioPlayer instance.
 @param event What happened (load success, load error, end of file, jog parameter).
 @param value NULL for LoadSuccess. (const char *) for LoadError, pointing to the error message. (double *) for JogParameter in the range of 0.0 to 1.0. (bool *) for EOF, set it to true to pause playback. Don't call this instance's methods from an EOF event callback!
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
 
 - thread safety (all methods are thread-safe).
 
 Can not be used for offline processing.
 
 @param positionMs The current position. Always accurate, no matter of time-stretching and other transformations. Read only.
 @param positionPercent The current position as a percentage (0.0f to 1.0f). Read only.
 @param positionSeconds The current position as seconds elapsed. Read only.
 @param durationMs The duration of the current track in milliseconds. Read only.
 @param durationSeconds The duration of the current track in seconds. Read only.
 @param playing Indicates if the player is playing or paused. Read only.
 @param tempo The current tempo. Read only.
 @param masterTempo Time-stretching is enabled or not. Read only.
 @param pitchShift Note offset from -12 to 12. 0 means no pitch shift. Read only.
 @param bpm Must be correct for syncing. There is no auto-bpm detection inside. Read only.
 @param currentBpm The actual bpm of the track (as bpm changes with the current tempo). Read only.
 @param slip If enabled, scratching or reverse will maintain the playback position as if you had never entered those modes. Read only.
 @param scratching The player is in scratching mode or not. Read only.
 @param reverse Indicates if the playback goes backwards. Read only.
 @param looping Indicates if looping is enabled. Read only.
 @param firstBeatMs Tells where the first beat (the beatgrid) begins. Must be correct for syncing. Read only.
 @param msElapsedSinceLastBeat How many milliseconds elapsed since the last beat. Read only.
 @param beatIndex Which beat has just happened (1, 2, 3, 4). A value of 0 means "don't know". Read only.
 @param syncMode The current sync mode (off, tempo, or tempo and beat).
 @param fixDoubleOrHalfBPM If tempo is >1.4f or <0.6f, it will treat the bpm as half or double. Good for certain genres. True by default.
 @param waitForNextBeatWithBeatSync Wait for the next beat if beat-syncing is enabled. False by default.
*/
class SuperpoweredAdvancedAudioPlayer {
public:
// READ ONLY parameters, don't set them directly, use the methods below.
    double positionMs;
    float positionPercent;
    unsigned int positionSeconds;
    unsigned int durationMs;
    unsigned int durationSeconds;
    bool playing;
    
    float tempo;
    bool masterTempo;
    int pitchShift;
    float bpm;
    float currentBpm;
    
    bool slip;
    bool scratching;
    bool reverse;
    bool looping;
    
    double firstBeatMs;
    double msElapsedSinceLastBeat;
    unsigned char beatIndex;

// READ-WRITE parameters
    SuperpoweredAdvancedAudioPlayerSyncMode syncMode;
    bool fixDoubleOrHalfBPM;
    bool waitForNextBeatWithBeatSync;
    
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
*/
    void open(const char *path);
    
/**
 @brief Opens a file, with playback paused.
 
 Tempo, pitchShift, masterTempo and syncMode are NOT changed if you open a new one.
 
 @param path The full file system path of the file.
 @param offset The byte offset inside the file.
 @param length The byte length from the offset.
*/
    void open(const char *path, int offset, int length);

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
    void seek(float percent);
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
    void setBpm(float newBpm);
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
 */
    void jogTouchEnd(float decelerate);
/**
 @brief Sets the relative tempo of the playback.
 
 @param tempo 1.0f is "original speed".
 @param masterTempo Enable or disable time-stretching.
 */
    void setTempo(float tempo, bool masterTempo);
/**
 @brief Sets the pitch shift value. Needs masterTempo enabled.
 
 @param pitchShift Note offset from -12 to 12. 0 means no pitch shift.
 */
    void setPitchShift(int pitchShift);
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
 
 @warning This is an advance method, use it only if you don't want the jogT... methods.
 
 @param slipMs Enable slip mode for a specific amount of time for scratching, or 0 to not slip.
 @param stopImmediately Stop now or not.
 */
    void startScratch(unsigned int slipMs, bool stopImmediately);
/**
 @brief Scratch movement.
 
 @warning This is an advance method, use it only if you don't want the jogT... methods.
 
 @param pitch The current speed.
 @param smoothing Should be between 0.05f (max. smoothing) and 1.0f (no smoothing).
 */
    void scratch(float pitch, float smoothing);
/**
 @brief Ends scratching.
 
 @warning This is an advance method, use it only if you don't want the jogT... methods.
 
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
    bool process(float *buffer, bool bufferAdd, unsigned int numberOfSamples, float volume, float masterBpm, double masterMsElapsedSinceLastBeat);
    
private:
    SuperpoweredAdvancedAudioPlayerInternals *internals;
    SuperpoweredAdvancedAudioPlayerBase *base;
    SuperpoweredAdvancedAudioPlayer(const SuperpoweredAdvancedAudioPlayer&);
    SuperpoweredAdvancedAudioPlayer& operator=(const SuperpoweredAdvancedAudioPlayer&);
};

#endif
