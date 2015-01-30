#ifndef Header_SuperpoweredAudiofilePlayer
#define Header_SuperpoweredAudiofilePlayer

struct audiofilePlayerInternals;

/**
 @brief Low-power, easy-to-use real-time audio player.
 
 Thread safety: you can call any method from any thread, just make sure the same stuff is not running somewhere else currently.
 All methods are non-blocking. Can not be used for offline processing.
 
 @param playing Play/pause (read-write).
 @param loopCount Playback count (read-write). If playback reaches to end and loopCount is 0 (default), then playback stops and playing becomes false.
 @param durationSeconds Duration in seconds (read only). Special values: -1 (load error), 0 (nothing is loaded or loading), >0 (loaded successfully)
 @param positionSeconds The current position as seconds elapsed. Read only.
 @param positionPercent The current position as a percentage (0.0f to 1.0f). Read only.
*/
class SuperpoweredAudiofilePlayer {
public:
// Read-write parameters.
    bool playing;
    int loopCount;
    
// READ ONLY parameters.
    int durationSeconds;
    int positionSeconds;
    float positionPercent;
    
/**
 Create a player instance with the current sample rate value.
*/
    SuperpoweredAudiofilePlayer(unsigned int sampleRate);
    ~SuperpoweredAudiofilePlayer();
    
/**
 @brief Opens an audio file. If something was loaded before, it becomes replaced.
 
 Opening happens on a background thread, with setting durationSeconds to 0.
 Opening ends when durationSeconds becomes >0 (loaded successfully) or -1 (load error).
 Use getError() to get the error message.
 
 @param path The full file system path of the audio file.
*/
    void open(const char *path);
/**
 @brief Opens a file. If something was loaded before, it becomes replaced.
 
 Opening happens on a background thread, with setting durationSeconds to 0.
 Opening ends when durationSeconds becomes >0 (loaded successfully) or -1 (load error).
 Use getError() to get the error message.
 
 @param path The full file system path of the file.
 @param offset The byte offset inside the file.
 @param length The byte length from the offset.
 */
    void open(const char *path, int offset, int length);
/**
 @brief Returns with the last error.
 
 @param errorMessage A freshly allocated buffer with the message. Don't forget to free errorMessage after you used it. Can be NULL on no error.
 */
    void getError(char **errorMessage);
/**
 @brief Seek to a specific position.
 */
    void seekTo(int positionSeconds);
/**
 @brief Simple seeking to a percentage.
 */
    void seekTo(float percent);
/**
 @brief Sets the sample rate.
     
 @param samplerate 44100, 48000, etc.
*/
    void setSampleRate(unsigned int samplerate); // 44100, 48000
    
/**
 @brief Processes the audio.
 
 @return Put something into output or not.
 */
    bool process(float *output, int frames);
    
private:
    audiofilePlayerInternals *internals;
    SuperpoweredAudiofilePlayer(const SuperpoweredAudiofilePlayer&);
    SuperpoweredAudiofilePlayer& operator=(const SuperpoweredAudiofilePlayer&);
};

#endif
