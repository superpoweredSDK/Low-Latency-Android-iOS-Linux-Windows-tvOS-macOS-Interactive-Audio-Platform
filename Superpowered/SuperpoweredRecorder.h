#ifndef Header_SuperpoweredRecorder
#define Header_SuperpoweredRecorder

namespace Superpowered {

struct recorderInternals;

/// @brief Records audio into 16-bit WAV file, with an optional tracklist.
/// Use this class in a real-time audio processing thread, where directly writing to a disk is not recommended.
/// For offline processing, instead of a Superpowered Recorder use the createWAV(), writeWAV() and closeWAV() functions in SuperpoweredSimple.h.
/// One instance allocates around 135k * numChannels memory when recording starts.
class Recorder {
public:
/// @brief Creates a recorder instance.
/// @warning Filesystem paths in C are different than paths in Java. /sdcard becomes /mnt/sdcard for example.
/// @param tempPath The full filesystem path of a temporary file. The recorder will create and write into this while recording. Can be NULL if only preparefd() is used.
/// @param mono True: mono, false: stereo.
    Recorder(const char *tempPath, bool mono = false);
    ~Recorder();
    
/// @brief Prepares for recording. The actual recording will begin on the first call of the process() method.
/// @return Returns true on success or false if another recording is still active or not finished writing to disk yet.
/// Do not call this in a real-time audio processing thread.
/// @param destinationPath The full filesystem path of the successfully finished recording. Do not include ".wav" in this, the .wav extension will be automatically appended.
/// @param samplerate The sample rate of the audio input and the recording, in Hz. The input sample rate must remain the same during the entire recording.
/// @param fadeInFadeOut If true, the recorder applies a fade-in at the beginning and a fade-out at the end of the recording. The fade is 64 frames long.
/// @param minimumLengthSeconds The minimum length of the recording. If the number of recorded seconds is lower, then a file will not be saved.
    bool prepare(const char *destinationPath, unsigned int samplerate, bool fadeInFadeOut, unsigned int minimumLengthSeconds);
    
/// @brief Prepares for recording. The actual recording will begin on the first call of the process() method.
/// @return Returns true on success or false if another recording is still active or not finished writing to disk yet.
/// Do not call this in a real-time audio processing thread.
/// @param audiofd Existing file descriptor for the audio file. Superpowered will fdopen on this using "w" mode.
/// @param logfd Existing file descriptor for the .txt file. Can be 0 if addtoTracklist() is not used.
/// @param samplerate The sample rate of the audio input and the recording, in Hz. The input sample rate must remain the same during the entire recording.
/// @param fadeInFadeOut If true, the recorder applies a fade-in at the beginning and a fade-out at the end of the recording. The fade is 64 frames long.
/// @param minimumLengthSeconds The minimum length of the recording. If the number of recorded seconds is lower, then a file will not be saved.
    bool preparefd(int audiofd, int logfd, unsigned int samplerate, bool fadeInFadeOut, unsigned int minimumLengthSeconds);
    
/// @brief Stops recording.
/// After calling stop() the recorder needs a little bit more time to write the last pieces of audio to disk. You can periodically call isFinished() to check when it's done.
/// It's safe to call this method in any thread.
    void stop();
    
/// @return Returns true is the recorder has finished writing after stop() was called, so the recording is fully available at the destination path.
/// It's safe to call this method in any thread.
    bool isFinished();
    
/// @brief Each recording may include a tracklist, which is a .txt file placed next to the destination audio file. This method adds an entry to the tracklist.
/// The tracklist file will not be created if this method is never called.
/// Do not call this in a real-time audio processing thread.
/// @param artist Artist, can be NULL.
/// @param title Title, can be NULL.
/// @param becameAudibleSeconds When the current track begins to be audible in this recording, relative to "now". For example, 0 means "now", -10 means "10 seconds ago".
/// @param takeOwnership If true, this class will free() artist and title.
    void addToTracklist(char *artist, char *title, int becameAudibleSeconds = 0, bool takeOwnership = false);
    
/// @brief Processes incoming non-interleaved stereo audio.
/// @return Seconds recorded so far.
/// It's never blocking for real-time usage.
/// @param left Pointer to floating point numbers. Left input channel.
/// @param right Pointer to floating point numbers. Right input channel.
/// @param numberOfFrames The number of frames in input.
    unsigned int recordNonInterleaved(float *left, float *right, unsigned int numberOfFrames);

/// @brief Processes incoming interleaved stereo audio.
/// @return Seconds recorded so far.
/// It's never blocking for real-time usage.
/// @param input Pointer to floating point numbers. Stereo interleaved audio to record.
/// @param numberOfFrames The number of frames in input.
    unsigned int recordInterleaved(float *input, unsigned int numberOfFrames);
    
/// @brief Processes incoming mono audio.
/// @return Seconds recorded so far.
/// It's never blocking for real-time usage.
/// @param input Pointer to floating point numbers.
/// @param numberOfFrames The number of frames in input.
    unsigned int recordMono(float *input, unsigned int numberOfFrames);

private:
    recorderInternals *internals;
    Recorder(const Recorder&);
    Recorder& operator=(const Recorder&);
};

}

#endif
