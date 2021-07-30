#ifndef Header_SuperpoweredGenerator
#define Header_SuperpoweredGenerator

namespace Superpowered {

struct generatorInternals;

/// @brief Generator for various waveform shapes.
class Generator {
public:
    /// @brief Superpowered generator shapes:
    typedef enum GeneratorShape {
        Sine = 0,       ///< sine
        Triangle = 1,   ///< triangle
        Sawtooth = 2,   ///< sawtooth
        PWM = 3,        ///< pulse wave with adjustable width
        PinkNoise = 4,  ///< pink noise
        WhiteNoise = 5, ///< white noise
        SyncMaster = 6  ///< generates no sound, but sync data to use with generateSyncMaster
    } GeneratorShape;
    
    float frequency; ///< Frequency of generator output in Hz. Minimum is > 0.0001, maximum is limited to sample rate / 2.
    float pulsewidth; ///< Pulse Width for PWM shape. 0.5 results in a square wave. Limited between 0.0001 and 0.9999.
    unsigned int samplerate; ///< Sample rate in Hz.
    GeneratorShape shape; ///< Shape.
        
/// @brief Constructor.
/// @param samplerate The initial sample rate in Hz.
/// @param shape The initial shape.
    JSWASM Generator(unsigned int samplerate, GeneratorShape shape);
    JSWASM ~Generator();
    
/// @brief Generates (outputs) audio.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with this function.
/// @param output Pointer to floating point numbers. 32-bit MONO output.
/// @param numberOfSamples Number of samples to produce.
    JSWASM void generate(float *output, unsigned int numberOfSamples);
    
/// @brief Generates (outputs) audio for a frequency modulated (FM) oscillator.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with this function.
/// @param output Pointer to floating point numbers. 32-bit MONO output.
/// @param numberOfSamples Number of samples to produce.
/// @param fmsource Pointer to floating point numbers. Source for FM modulation containing numberOfSamples samples, usually from a previous call to generate().
/// @param fmdepth Frequency modulation depth. 0 means no modulation, 1000 is a reasonable upper limit.
    JSWASM void generateFM(float *output, unsigned int numberOfSamples, float *fmsource, float fmdepth);
    
/// @brief Generates audio for an oscillator that also serves as synchronization source for another oscillator.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with this function.
/// @param output Pointer to floating point numbers. 32-bit MONO output.
/// @param syncdata Pointer to a buffer to receive hard sync information for syncing oscillators. Should be numberOfSamples + 1 floats big minimum.
/// @param numberOfSamples Number of samples to produce.
    JSWASM void generateAndCreateSync(float *output, float *syncdata, unsigned int numberOfSamples);
    
/// @brief Generates audio for an oscillator that is hard-synced to another oscillator.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with this function.
/// @param output Pointer to floating point numbers. 32-bit MONO output.
/// @param syncdata Pointer to floating point numbers. Input sync data, previously produced by a call to generateAndCreateSync() with same numberOfSamples.
/// @param numberOfSamples Number of samples to produce.
    JSWASM void generateSynchronized(float *output, float *syncdata, unsigned int numberOfSamples);
        
/// @brief Start oscillator with given phase angle. In a synthesizer, this should be called whenever a voice starts.
/// @param phase Start phase of the oscillator between 0.0 (0 degree) and 1.0 (180 degrees).
    JSWASM void reset(float phase = 0.0f);
    
private:
    generatorInternals *internals;
    Generator(const Generator&);
    Generator& operator=(const Generator&);
};

}

#endif
