#ifndef Header_SuperpoweredAnalyzer
#define Header_SuperpoweredAnalyzer

namespace Superpowered {

static const char *musicalChordNames[24] = {
    "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", /// major
    "Am", "A#m", "Bm", "Cm", "C#m", "Dm", "D#m", "Em", "Fm", "F#m", "Gm", "G#m" /// minor
};

static const char *camelotChordNames[24] = {
    "11B", "6B", "1B", "8B", "3B", "10B", "5B", "12B", "7B", "2B", "9B", "4B", /// major
    "8A", "3A", "10A", "5A", "12A", "7A", "2A", "9A", "4A", "11A", "6A", "1A" /// minor
};

static const char *openkeyChordNames[24] = {
    "4d", "11d", "6d", "1d", "8d", "3d", "10d", "5d", "12d", "7d", "2d", "9d", /// major
    "1m", "8m", "3m", "10m", "5m", "12m", "7m", "2m", "9m", "4m", "11m", "6m" /// minor
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

#define SUPERPOWERED_WAVEFORM_POINTS_PER_SEC 150

struct analyzerInternals;
struct waveformInternals;
struct liveAnalyzerInternals;

/// @brief Performs bpm and key detection, loudness/peak analysis. Provides compact waveform data (150 points/sec and 1 point/sec resolution), beatgrid information.
/// To keep an array result after you destruct the analyzer without an expensive memory copy, do this:
/// float *peakWaveform = analyzer->peakWaveform; analyzer->peakWaveform = NULL;
/// In this case you take ownership on the data, so don't forget to free() the memory to prevent memory leaks. Use _aligned_free() on Windows.
class Analyzer {
public:
    float peakDb;                   ///< Peak volume in decibels. Available after calling makeResults().
    float averageDb;                ///< Average volume in decibels. Available after calling makeResults().
    float loudpartsAverageDb;       ///< The average volume of the "loud" parts in decibel. (Quiet parts excluded.) Available after calling makeResults().
    float bpm;                      ///< Beats per minute. Available after calling makeResults().
    float beatgridStartMs;          ///< Where the beatgrid starts (first beat) in milliseconds. Available after calling makeResults().
    int keyIndex;                   ///< The dominant key (chord) of the music. 0..11 are major keys from A to G#, 12..23 are minor keys from A to G#. Check the static constants in this header for musical, Camelot and Open Key notations.
    
    int waveformSize;               ///< The number of bytes in the peak, average, low, mid and high waveforms and notes.
    unsigned char *peakWaveform;    ///< 150 points/sec waveform data displaying the peak volume. Each byte represents one "pixel". Available after calling makeResults().
    unsigned char *averageWaveform; ///< 150 points/sec waveform data displaying the average volume. Each byte represents one "pixel". Available after calling makeResults().
    unsigned char *lowWaveform;     ///< 150 points/sec waveform data displaying the low frequencies (below 200 Hz). Each byte represents one "pixel". Available after calling makeResults().
    unsigned char *midWaveform;     ///< 150 points/sec waveform data displaying the mid frequencies (200-1600 Hz). Each byte represents one "pixel". Available after calling makeResults().
    unsigned char *highWaveform;    ///< 150 points/sec waveform data displaying the high frequencies (above 1600 Hz). Each byte represents one "pixel". Available after calling makeResults().
    unsigned char *notes;           ///< 150 points/sec data displaying the bass and mid keys. Upper 4 bits are the bass notes 0 to 11, lower 4 bits are the mid notes 0 to 11 (C, C#, D, D#, E, F, F#, G, G#, A, A#, B). The note value is 12 means "unknown note due low volume". Available after calling makeResults().
    
    char *overviewWaveform;         ///< 1 point/sec waveform data displaying the average volume in decibels. Useful for displaying the overall structure of a track. Each byte has the value of -128 to 0, in decibels.
    int overviewSize;               ///< The number bytes in overviewWaveform.

/// @brief Constructor.
/// @param samplerate The sample rate of the audio input.
/// @param lengthSeconds The length in seconds of the audio input. The analyzer will not be able to process more audio than this. You can change this value in the process() method.
    Analyzer(unsigned int samplerate, int lengthSeconds);
    ~Analyzer();

/// @brief Processes some audio. This method can be used in a real-time audio thread if lengthSeconds is -1.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param numberOfFrames Number of frames to process.
/// @param lengthSeconds If the audio input length may change, set this to the current length. Use -1 otherwise. If this value is not -1, this method can NOT be used in a real-time audio thread.
    void process(float *input, unsigned int numberOfFrames, int lengthSeconds = -1);

/// @brief Makes results from the collected data. This method should NOT be used in a real-time audio thread, because it allocates memory.
/// @param minimumBpm Detected bpm will be more than or equal to this. Recommended value: 60.
/// @param maximumBpm Detected bpm will be less than or equal to this. Recommended value: 200.
/// @param knownBpm If you know the bpm set it here. Use 0 otherwise.
/// @param aroundBpm Provides a "hint" for the analyzer with this. Use 0 otherwise.
/// @param getBeatgridStartMs True: calculate beatgridStartMs. False: save some CPU with not calculating it.
/// @param aroundBeatgridStartMs Provides a "hint" for the analyzer with this. Use 0 otherwise.
/// @param makeOverviewWaveform True: make overviewWaveform. False: save some CPU and memory with not making it.
/// @param makeLowMidHighWaveforms True: make the low/mid/high waveforms. False: save some CPU and memory with not making them.
/// @param getKeyIndex True: calculate keyIndex. False: save some CPU with not calculating it.
    void makeResults(float minimumBpm, float maximumBpm, float knownBpm, float aroundBpm, bool getBeatgridStartMs, float aroundBeatgridStartMs, bool makeOverviewWaveform, bool makeLowMidHighWaveforms, bool getKeyIndex);

private:
    analyzerInternals *internals;
    Analyzer(const Analyzer&);
    Analyzer& operator=(const Analyzer&);
};

/// @brief Provides waveform data in 150 points/sec resolution.
/// To keep the array result after you destruct the analyzer without an expensive memory copy, do this:
/// float *peakWaveform = waveform->peakWaveform; waveform->peakWaveform = NULL;
/// In this case you take ownership on the data, so don't forget to free() the memory to prevent memory leaks. Use _aligned_free() on Windows.
class Waveform {
public:
    unsigned char *peakWaveform; ///< 150 points/sec waveform data displaying the peak volume. Each byte represents one "pixel". Available after calling makeResult().
    int waveformSize;            ///< The number of bytes in the peak waveform.

/// @brief Constructor.
/// @param samplerate The sample rate of the audio input.
/// @param lengthSeconds The length in seconds of the audio input. It will not be able to process more audio than this. You can change this value in the process() method.
    Waveform(unsigned int samplerate, int lengthSeconds);
    ~Waveform();

/// @brief Processes some audio. This method can be used in a real-time audio thread if lengthSeconds is -1.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param numberOfFrames Number of frames to process.
/// @param lengthSeconds If the audio input length may change, set this to the current length. Use -1 otherwise. If this value is not -1, this method can NOT be used in a real-time audio thread.
    void process(float *input, unsigned int numberOfFrames, int lengthSeconds = -1);

/// @brief Makes the result from the collected data. This method should NOT be used in a real-time audio thread.
    void makeResult();

private:
    waveformInternals *internals;
    Waveform(const Waveform&);
    Waveform& operator=(const Waveform&);
};

/// @brief Performs bpm and key detection continuously. The update frequency is 2 seconds.
/// @warning High memory usage! This class allocates 320 * samplerate bytes. Example: 48000 Hz * 320 = 15 MB
class LiveAnalyzer {
public:
    float bpm;               ///< Current beats per minute. If the current result is too far from the reality you can do three things:
                             ///< 1. Set bpm to zero. This forces the analyzer to "forget" the last bpm and may find the correct value within 10 seconds.
                             ///< 2. Set bpm to an estimated value. This forces the analyzer to "forget" the last bpm and use your estimate instead. It may find the correct value within 4 seconds.
                             ///< 3. Set bpm to a negative value. This will "hard reset" the analyzer and so it will start fresh.
    int keyIndex;            ///< The dominant key (chord) of the music. 0..11 are major keys from A to G#, 12..23 are minor keys from A to G#. -1: unknown. Check the static constants in this header for musical, Camelot and Open Key notations.
    bool silence;            ///< If true, bpm and key detection is paused, because the analyzer detects a longer silence period (more than 1 seconds of digital silence or 8 seconds below -48 decibels). If false, bpm and key detection is under progress.
    unsigned int samplerate; ///< Sample rate in Hz.
    
/// Constructor.
/// @param samplerate The initial sample rate in Hz.
    LiveAnalyzer(unsigned int samplerate);
    ~LiveAnalyzer();
    
/// @brief Processes some audio. This method can be used in a real-time audio thread.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param numberOfFrames Number of frames to process.
    void process(float *input, unsigned int numberOfFrames);
    
private:
    liveAnalyzerInternals *internals;
    LiveAnalyzer(const LiveAnalyzer&);
    LiveAnalyzer& operator=(const LiveAnalyzer&);
};

}

#endif
