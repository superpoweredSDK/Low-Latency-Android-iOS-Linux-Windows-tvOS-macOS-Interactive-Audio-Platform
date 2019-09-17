#ifndef Header_SuperpoweredCompressor
#define Header_SuperpoweredCompressor

#include "SuperpoweredFX.h"

namespace Superpowered {

struct compressorInternals;

/// @brief Compressor with 0 latency.
/// It doesn't allocate any internal buffers and needs less than 1 kb of memory.
class Compressor: public FX {
public:
    float inputGainDb;  ///< Input gain in decibels, limited between -24 and 24. Default: 0.
    float outputGainDb; ///< Output gain in decibels, limited between -24 and 24. Default: 0.
    float wet;          ///< Dry/wet ratio, limited between 0 (completely dry) and 1 (completely wet). Default: 1.
    float attackSec;    ///< Attack in seconds (not milliseconds!). Limited between 0.0001 and 1. Default: 0.003 (3 ms).
    float releaseSec;   ///< Release in seconds (not milliseconds!). Limited between 0.1 and 4. Default: 0.3 (300 ms).
    float ratio;        ///< Ratio, rounded to 1.5, 2.0, 3.0, 4.0, 5.0 or 10. Default: 3.
    float thresholdDb;  ///< Threshold in decibels, limited between 0 and -40. Default: 0.
    float hpCutOffHz;   ///< Key highpass filter frequency, limited between 1 and 10000. Default: 1.
    
/// @return Returns the maximum gain reduction in decibels since the last getGainReductionDb() call.
    float getGainReductionDb();

/// @brief Constructor. Enabled is false by default.
/// @param samplerate The initial sample rate in Hz.
    Compressor(unsigned int samplerate);
    ~Compressor();
    
/// @brief Processes the audio. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation.
/// It's never blocking for real-time usage. You can change all properties and call getGainReductionDb() on any thread, concurrently with process().
/// If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output. Can point to the same location with input (in-place processing).
/// @param numberOfFrames Number of frames to process. Recommendations for best performance: multiply of 4, minimum 64.
    bool process(float *input, float *output, unsigned int numberOfFrames);

private:
    compressorInternals *internals;
    Compressor(const Compressor&);
    Compressor& operator=(const Compressor&);
};


struct compressorProtoInternals;

/// @brief The new Superpowered Compressor prototype. It can be used in production, but we're still collecting feedback on its feature set, therefore the documentation is not complete on this one.
class CompressorProto: public FX {
public:
    float outputGainDb; ///< decibels
    float wet;          ///< 0 to 1
    float attackSec;    ///< 0 to 1
    float releaseSec;   ///< 0.001 to 4
    float ratio;        ///< 1 to 1000
    float thresholdDb;  ///< 0 to -100
    float kneeDb;       ///< 0 to 100
    unsigned char lookaheadMs; ///< doesn't work yet
    bool rms;           ///< false: follow peaks, true: follow RMS
    
    CompressorProto(unsigned int samplerate);
    ~CompressorProto();
    
    float getGainReductionDb();
    /// input is the sidechain
    bool process(float *input, float *output, unsigned int numberOfSamples);
    /// separate sidechain
    bool process(float *input, float *sidechain, float *output, unsigned int numberOfSamples);
    
private:
    compressorProtoInternals *internals;
    CompressorProto(const CompressorProto&);
    CompressorProto& operator=(const CompressorProto&);
};

}

#endif
