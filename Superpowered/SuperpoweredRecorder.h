#ifndef Header_SuperpoweredRecorder
#define Header_SuperpoweredRecorder

#include <stdio.h>

/**
 @brief Creates a 16-bit WAV file.
 
 After createWAV, write audio data using the standard fwrite function. Close the file with the closeWAV function. Never use direct disk writing in a live audio processing loop. These functions are recommended for offline processing only.
 
 @return A file handle (success) or NULL (error).

 @param path The full filesystem path of the file.
 @param samplerate Sample rate.
 @param numChannels Number of channels.
*/
FILE *createWAV(const char *path, unsigned int samplerate, unsigned short int numChannels);

/**
 @brief Closes a 16-bit WAV file.
 
 fclose() is not enough to create a valid a WAV file, use this function to close it.
 
 @param fd The file handle to close.
*/
void closeWAV(FILE *fd);

/**
 @brief Called on the internal background thread of the recorder instance, when the recorder finishes writing after stop() and the recorded file is moved to it's target location.

 @param clientData Some custom pointer you set when you created the SuperpoweredRecoder instance.
 */
typedef void (* SuperpoweredRecorderStoppedCallback) (void *clientData);

struct SuperpoweredRecorderInternals;

/**
 @brief Records audio into 16-bit WAV file, with an optional tracklist.

 One instance allocates around 135k * numChannels memory when recording starts. Use this class in a live audio processing loop, where directly writing to a disk is not recommended. For offline processing, use the createWAV, fwrite and closeWAV functions instead of a SuperpoweredRecorder.
 */
class SuperpoweredRecorder {
public:
    /**
     @brief Creates a recorder instance.
     
     @warning Filesystem paths in C are different than paths in Java. /sdcard becomes /mnt/sdcard for example.

     @param tempPath The full filesystem path of a temporary file.
     @param samplerate The current samplerate.
     @param minSeconds The minimum length of a recording. If the number of recorded seconds is lower, then a file will not be saved.
     @param numChannels The number of channels.
     @param applyFade If true, the recorder applies a  fade-in at the beginning and a fade-out at the end of the recording. The fade is 64 samples long.
     @param callback Called when the recorder finishes writing after stop().
     @param clientData A custom pointer your callback receives.
     */
    SuperpoweredRecorder(const char *tempPath, unsigned int samplerate, unsigned int minSeconds = 1, unsigned char numChannels = 2, bool applyFade = false, SuperpoweredRecorderStoppedCallback callback = NULL, void *clientData = NULL);
    ~SuperpoweredRecorder();
    /**
     @brief Starts recording.
     @return False, if another recording is still active or not closed yet.

     @param destinationPath The full filesystem path of the successfully finished recording. .wav will be appended, don't include ".wav" in this.
     */
    bool start(const char *destinationPath);
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
     @param takeOwnership If true, this class will free artist and title.
     */
    void addToTracklist(char *artist, char *title, int offsetSeconds, bool takeOwnership = false);
    /**
     @brief Sets the sample rate.

     @param samplerate 44100, 48000, etc.
     */
    void setSamplerate(unsigned int samplerate);

    /**
     @brief Processes stereo incoming audio.

     Special case: set both input0 and input1 to NULL if there is nothing to record yet. You can cut initial silence this way.

     @return Seconds recorded so far.

     @param input0 Left input channel or stereo interleaved input.
     @param input1 Right input channel. If NULL, input0 is a stereo interleaved input.
     @param numberOfSamples The number of samples in input. Should be 8 minimum.
     */
    unsigned int process(float *input0, float *input1, unsigned int numberOfSamples);

    /**
     @brief Processes incoming interleaved audio.

     Special case: set input to NULL if there is nothing to record yet. You can cut initial silence this way.

     @return Seconds recorded so far.

     @param input Interleaved audio to record.
     @param numberOfSamples The number of samples in input. Should be 8 minimum.
     */
    unsigned int process(float *input, unsigned int numberOfSamples);

private:
    SuperpoweredRecorderInternals *internals;
    SuperpoweredRecorder(const SuperpoweredRecorder&);
    SuperpoweredRecorder& operator=(const SuperpoweredRecorder&);
};

#endif
