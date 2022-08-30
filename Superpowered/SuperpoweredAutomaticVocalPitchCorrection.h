#ifndef Header_SuperpoweredAutomaticVocalPitchCorrection
#define Header_SuperpoweredAutomaticVocalPitchCorrection

#ifndef JSWASM
#define JSWASM
#endif

namespace Superpowered {

struct tunerInternals;

/// @brief Automatic Vocal Pitch Correction (Automatic Tune).
/// One instance allocates around kb memory.
class AutomaticVocalPitchCorrection {
public:
    typedef enum TunerScale {
        CHROMATIC,
        CMAJOR,         AMINOR      = CMAJOR,
        CSHARPMAJOR,    ASHARPMINOR = CSHARPMAJOR,
        DMAJOR,         BMINOR      = DMAJOR,
        DSHARPMAJOR,    CMINOR      = DSHARPMAJOR,
        EMAJOR,         CSHARPMINOR = EMAJOR,
        FMAJOR,         DMINOR      = FMAJOR,
        FSHARPMAJOR,    DSHARPMINOR = FSHARPMAJOR,
        GMAJOR,         EMINOR      = GMAJOR,
        GSHARPMAJOR,    FMINOR      = GSHARPMAJOR,
        AMAJOR,         FSHARPMINOR = AMAJOR,
        ASHARPMAJOR,    GMINOR      = ASHARPMAJOR,
        BMAJOR,         GSHARPMINOR = BMAJOR,
        CUSTOM
   } TunerScale;
    
    typedef enum TunerRange {
        WIDE,   ///< wide range, 40 to 3000 Hz
        BASS,   ///< bass singer, 40 to 350 Hz
        TENOR,  ///< tenor, 100 to 600 Hz
        ALTO,   ///< alto, 150 to 1400 Hz
        SOPRANO ///< soprano, 200 to 3000 Hz
    } TunerRange;
        
    typedef enum TunerSpeed {
        SUBTLE, ///< subtle pitch correction
        MEDIUM, ///< medium pitch correction
        EXTREME ///< classic pitch correction effect
    } TunerSpeed;
    
    TunerScale scale; ///< Music scale. Default: CHROMATIC (all twelve keys of the octave are allowed)
    TunerRange range; ///< Vocal range for pitch detection. Default: WIDE (40 to 3000 Hz)
    TunerSpeed speed; ///< Speed for tune correction. Default: EXTREME
    float frequencyOfA;       ///< Frequency for middle A, between 410-470 Hz. Default: 440
    unsigned int samplerate;  ///< Input/output sample rate in Hz. Default: 48000
    
/// @brief Constructor.
    JSWASM AutomaticVocalPitchCorrection();
    JSWASM ~AutomaticVocalPitchCorrection();
    
/// @brief Processes the audio.
/// It's never blocking for real-time usage. You can change any properties concurrently with process().
/// @param input Pointer to floating point numbers. 32-bit mono or interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit mono or interleaved stereo input.
/// @param stereo Stereo or mono input and output.
/// @param numberOfFrames Number of frames to process.
    JSWASM void process(float *input, float *output, bool stereo, unsigned int numberOfFrames);

/// @brief Set all internals to initial state.
    JSWASM void reset();
    
/// @brief Set note (on all octaves) for scale == CUSTOM.
/// @param note The note (between 0..11).
/// @param enabled Note enabled or not.
    JSWASM void setCustomScaleNote(unsigned char note, bool enabled);
    
/// @return Get note (on all octaves) for scale == CUSTOM.
/// @param note The note (between 0..11).
    JSWASM bool getCustomScaleNote(unsigned char note);
    
private:
    tunerInternals *internals;
    AutomaticVocalPitchCorrection(const AutomaticVocalPitchCorrection&);
    AutomaticVocalPitchCorrection& operator=(const AutomaticVocalPitchCorrection&);
};

};

#endif
