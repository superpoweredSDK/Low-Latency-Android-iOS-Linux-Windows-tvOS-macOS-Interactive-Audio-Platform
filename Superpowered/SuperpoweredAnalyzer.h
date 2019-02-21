#ifndef Header_SuperpoweredAnalyzer
#define Header_SuperpoweredAnalyzer

#define SUPERPOWERED_WAVEFORM_POINTS_PER_SEC 150

struct analyzerInternals;
struct waveformInternals;
struct liveAnalyzerInternals;

#define SuperpoweredOfflineAnalyzer SuperpoweredAnalyzer

/**
 @brief Performs bpm and key detection, loudness/peak analysis. Provides compact waveform data (150 points/sec and 1 point/sec resolution), beatgrid information.
 */
class SuperpoweredAnalyzer {
public:

/**
 @param samplerate The sample rate of the source.
 @param bpm If you know the accurate bpm value in advance, set it here. 0 means the analyzer will detect bpm.
 @param lengthSeconds The source's length in seconds.
 @param minimumBpm Detected bpm will be more than or equal to this.
 @param maximumBpm Detected bpm will be less than or equal to this.
 */
    SuperpoweredAnalyzer(unsigned int samplerate, float bpm = 0, int lengthSeconds = 0, float minimumBpm = 60.0f, float maximumBpm = 200.0f);
    ~SuperpoweredAnalyzer();

/**
 @brief Processes a chunk of audio. This method can be used in a real-time audio thread if lengthSeconds is -1.

 @param input 32-bit interleaved stereo floating-point input.
 @param numberOfFrames How many frames to process.
 @param lengthSeconds If the source's length may change, set this to it's current value, leave it at -1 otherwise. If this value is not -1, this method can NOT be used in a real-time audio thread.
 */
    void process(float *input, unsigned int numberOfFrames, int lengthSeconds = -1);

/**
 @brief Get results. Call this method ONCE, after all samples are processed. Delete the analyzer after this, the same instance can not be used further. This method should NOT be used in a real-time audio thread.
 
 @param averageWaveform 150 points/sec waveform data displaying the average volume. Each sample is an unsigned char from 0 to 255. You take ownership on this (must free memory, _aligned_free() on Windows).
 @param peakWaveform 150 points/sec waveform data displaying the peak volume. Each sample is an unsigned char from 0 to 255. You take ownership on this (must free memory, _aligned_free() on Windows).
 @param lowWaveform 150 points/sec waveform data displaying the low frequencies. Each sample is an unsigned char from 0 to 255. You take ownership on this (must free memory, _aligned_free() on Windows).
 @param midWaveform 150 points/sec waveform data displaying the mid frequencies. Each sample is an unsigned char from 0 to 255. You take ownership on this (must free memory, _aligned_free() on Windows).
 @param highWaveform 150 points/sec waveform data displaying the high frequencies. Each sample is an unsigned char from 0 to 255. You take ownership on this (must free memory, _aligned_free() on Windows).
 @param notes 150 points/sec data displaying the bass and mid keys. Upper 4 bits are the bass notes 0 to 11, lower 4 bits are the mid notes 0 to 11 (C, C#, D, D#, E, F, F#, G, G#, A, A#, B). The note value is 12 means "unknown note due low volume". You take ownership on this (must free memory, _aligned_free() on Windows).
 @param waveformSize The number of points in averageWaveform, peakWaveform or lowMidHighWaveform.
 @param overviewWaveform 1 point/sec waveform data displaying the average volume in decibels. Useful for displaying the overall structure of a track. Each sample is a signed char, from -128 to 0 decibel. You take ownership on this (must free memory, _aligned_free() on Windows).
 @param overviewSize The number points in overviewWaveform.
 @param averageDecibel The average loudness of all samples processed in decibel.
 @param loudpartsAverageDecibel The average loudness of the "loud" parts in the music in decibel. (Breakdowns and other quiet parts are excluded.)
 @param peakDecibel The loudest sample in decibel.
 @param bpm Beats per minute.
 @param beatgridStartMs The position where the beatgrid starts. Important! On input set it to 0, or the ms position of the first audio sample.
 @param keyIndex The dominant key (chord) of the music. 0..11 are major keys from A to G#, 12..23 are minor keys from A to G#. Check the static constants in this header for musical, Camelot and Open Key notations.
 @param previousBpm If the analyzer is used in a "live" situation (measuring periodically), submit the previously measured bpm here. 0.0f means no previously measured bpm.
 */
    void getresults(unsigned char **averageWaveform, unsigned char **peakWaveform, unsigned char **lowWaveform, unsigned char **midWaveform, unsigned char **highWaveform, unsigned char **notes, int *waveformSize, char **overviewWaveform, int *overviewSize, float *averageDecibel, float *loudpartsAverageDecibel, float *peakDecibel, float *bpm, float *beatgridStartMs, int *keyIndex, float previousBpm = 0.0f);

private:
    analyzerInternals *internals;
    SuperpoweredAnalyzer(const SuperpoweredAnalyzer&);
    SuperpoweredAnalyzer& operator=(const SuperpoweredAnalyzer&);
};

/**
 @return Returns the frequency of a specific note.
 
 @param note The number of the note. Note 0 is the standard A note at 440 Hz.
 */
float frequencyOfNote(int note);

static const char *musicalChordNames[24] = {
    "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", // major
    "Am", "A#m", "Bm", "Cm", "C#m", "Dm", "D#m", "Em", "Fm", "F#m", "Gm", "G#m" // minor
};

static const char *camelotChordNames[24] = {
    "11B", "6B", "1B", "8B", "3B", "10B", "5B", "12B", "7B", "2B", "9B", "4B", // major
    "8A", "3A", "10A", "5A", "12A", "7A", "2A", "9A", "4A", "11A", "6A", "1A" // minor
};

static const char *openkeyChordNames[24] = {
    "4d", "11d", "6d", "1d", "8d", "3d", "10d", "5d", "12d", "7d", "2d", "9d", // major
    "1m", "8m", "3m", "10m", "5m", "12m", "7m", "2m", "9m", "4m", "11m", "6m" // minor
};

static const int chordToSyllable[24] = {
    5, 5, 6, 0, 0, 1, 1, 2, 3, 3, 4, 4,
    5, 5, 6, 0, 0, 1, 1, 2, 3, 3, 4, 4,
};

static const int chordToNoteStartingFromC[24] = {
    9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8,
    9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8,
};

static const int camelotSort[24] = {
    21, 11, 1, 15, 5, 19, 9, 23, 13, 3, 17, 7,
    14, 4, 18, 8, 22, 12, 2, 16, 6, 20, 10, 0
};

/**
 @brief Provides compact waveform data (150 points/sec and 1 point/sec resolution)
*/
class SuperpoweredWaveform {
public:

/**
 @param samplerate The sample rate of the source.
 @param lengthSeconds The source's length in seconds.
*/
    SuperpoweredWaveform(unsigned int samplerate, int lengthSeconds);
    ~SuperpoweredWaveform();

/**
 @brief Processes a chunk of audio. This method can be used in a real-time audio thread if lengthSeconds is -1.

 @param input 32-bit interleaved floating-point input.
 @param numberOfFrames How many frames to process.
 @param lengthSeconds If the source's length may change, set this to it's current value, leave it at -1 otherwise. If this value is not -1, this method can NOT be used in a real-time audio thread.
*/
    void process(float *input, unsigned int numberOfFrames, int lengthSeconds = -1);

/**
 @return Returns the 150 points/sec waveform data displaying the peak volume. Each sample is an unsigned char from 0 to 255. You take ownership on this (must free memory). This method should NOT be used in a real-time audio thread.
 
 @param size The number of points in the waveform data.
 */
    unsigned char *getresult(int *size);

private:
    waveformInternals *internals;
    SuperpoweredWaveform(const SuperpoweredWaveform&);
    SuperpoweredWaveform& operator=(const SuperpoweredWaveform&);
};

class SuperpoweredLiveAnalyzer {
public:
    float bpm;
    int keyIndex; // READ ONLY
    bool silence; // READ ONLY
    
/**
 @param samplerate The sample rate of the source.
*/
    SuperpoweredLiveAnalyzer(unsigned int samplerate);
    ~SuperpoweredLiveAnalyzer();
    
/**
 @brief Processes a chunk of audio. This method can be used in a real-time audio thread.
     
 @param input 32-bit interleaved stereo floating-point input.
 @param numberOfFrames How many frames to process.
*/
    void process(float *input, unsigned int numberOfFrames);
    
/**
 @brief Sets the sample rate.
     
 @param samplerate 44100, 48000, etc.
 */
    void setSamplerate(unsigned int samplerate);
    
private:
    liveAnalyzerInternals *internals;
    SuperpoweredLiveAnalyzer(const SuperpoweredLiveAnalyzer&);
    SuperpoweredLiveAnalyzer& operator=(const SuperpoweredLiveAnalyzer&);
};
/**
 @brief Performs bpm and key detection continuously. The update frequency is 2 seconds.
 
 @warning High memory usage! This class allocates 320 * samplerate bytes. Example: 48000 Hz * 320 = 15 MB
 
 @param bpm Current beats per minute. If the current result is too far from the reality you can do three things:
 
 1. Set bpm to zero. This forces the analyzer to "forget" the last bpm and may find the correct value within 10 seconds.
 2. Set bpm to an estimated value (if you can estimate). This forces the analyzer to "forget" the last bpm and use your estimate instead. It may find the correct value within 4 seconds.
 3. Set bpm to a negative value. This will "hard reset" the analyzer and so it will start fresh.
 
 @param keyIndex The dominant key (chord) of the music. 0..11 are major keys from A to G#, 12..23 are minor keys from A to G#. -1: unknown. Check the static constants in this header for musical, Camelot and Open Key notations. READ ONLY.
 @param silence Bpm and key detection is paused, because the analyzer detects a longer silence period (more than 1 seconds of digital silence or 8 seconds below -48 decibels). READ ONLY.
 */

#endif
