#ifndef Header_SuperpoweredRecorder
#define Header_SuperpoweredRecorder

struct SuperpoweredRecorderInternals;

/**
 @brief Records audio into a stereo 16-bit WAV file, with an optional tracklist.
 
 One instance allocates around 512k memory when recording starts.
 */
class SuperpoweredRecorder {
public:
/**
 @brief Creates a recorder instance.
 
 @param tempPath The full filesystem path of a temporarily file.
 @param samplerate The current samplerate.
 */
    SuperpoweredRecorder(const char *tempPath, unsigned int samplerate);
    ~SuperpoweredRecorder();
/**
 @brief Starts recording.
 
 @param destinationPath The full filesystem path of the successfully finished recording.
 */
    void start(const char *destinationPath);
/**
 @brief Stops recording.
*/
    void stop();
/**
 @brief Adds an artist + title to the tracklist.
 
 The tracklist is a .txt file placed next to the audio file, if at least one track has been added to it.
 
 @param artist Artist, can be NULL.
 @param title Title, can be NULL.
 @param offsetSeconds When the track began to be audible in this recording, relative to the current time. For example, 0 means "now", -10 means "10 seconds ago".
 */
    void addToTracklist(char *artist, char *title, int offsetSeconds);
/**
 @brief Sets the sample rate.
     
 @param samplerate 44100, 48000, etc.
 */
    void setSamplerate(unsigned int samplerate);
    
/**
 @brief Processes incoming audio.
 
 Special case: set both input0 and input1 to NULL if there is nothing to record yet. You can cut initial silence this way.
 
 @return Seconds recorded so far.
 
 @param input0 Left input channel or stereo interleaved input.
 @param input1 Right input channel. If NULL, input0 is a stereo interleaved input.
 @param numberOfSamples The number of samples in input. Should be 4 minimum.
 */
    unsigned int process(float *input0, float *input1, unsigned int numberOfSamples);
    
private:
    SuperpoweredRecorderInternals *internals;
    SuperpoweredRecorder(const SuperpoweredRecorder&);
    SuperpoweredRecorder& operator=(const SuperpoweredRecorder&);
};

#endif
