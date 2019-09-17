#ifndef Header_SuperpoweredMixer
#define Header_SuperpoweredMixer

namespace Superpowered {

struct stereoMixerInternals;
struct monoMixerInternals;

/// @brief Mixes up to 4 stereo inputs. From the traditional mixer hardware point of view, every input and the output has dedicated metering, gain and pan controls. One instance allocates just a few bytes of memory, therefore combining multiple instances of the StereoMixer is the recommended way to support more than 4 channels.
class StereoMixer {
public:
    float inputGain[8];  ///< Gain per input channel. Default value for all: 1. Changes between consecutive process() calls are automatically smoothed. Example: inputGain[0] = input 0 left, inputGain[1] = input 0 right, inputGain[2] = input 1 left, ...
    float inputPeak[8];  ///< The peak absolute audio volume per input channel, updated after every process() call, measured before any gain. Example: inputPeak[0] = input 0 left, inputPeak[1] = input 0 right, inputPeak[2] = input 1 left, ...
    float outputGain[2]; ///< Output gain. [0] is left side, [1] is right side. Default value for all: 1. Changes between consecutive process() calls are automatically smoothed.
    float outputPeak[2]; ///< The peak absolute audio volume for the output, updated after every process() call. [0] is left side, [1] is right side.
    
    /// @brief Constructor.
    StereoMixer();
    ~StereoMixer();
        
    /// @brief Mixes up to 4 interleaved stereo inputs into a stereo output.
    /// @param input0 Pointer to floating point numbers. 32-bit interleaved stereo input buffer for the first input. Can be NULL.
    /// @param input1 Pointer to floating point numbers. 32-bit interleaved stereo input buffer for the second input. Can be NULL.
    /// @param input2 Pointer to floating point numbers. 32-bit interleaved stereo input buffer for the third input. Can be NULL.
    /// @param input3 Pointer to floating point numbers. 32-bit interleaved stereo input buffer for the fourth input. Can be NULL.
    /// @param output Pointer to floating point numbers. 32-bit interleaved stereo output buffer.
    /// @param numberOfFrames Number of frames to process. Must be an even number.
    void process(float *input0, float *input1, float *input2, float *input3, float *output, unsigned int numberOfFrames);

private:
    stereoMixerInternals *internals;
    StereoMixer(const StereoMixer&);
    StereoMixer& operator=(const StereoMixer&);
};

/// @brief Mixes up to 4 mono inputs. Every input and the output has individual gain control. One instance allocates just a few bytes of memory, therefore combining multiple instances of the MonoMixer is the recommended way to support more than 4 channels.
class MonoMixer {
public:
    float inputGain[4]; ///< Gain per input channel. Default value for all: 1. Changes between consecutive process() calls are automatically smoothed.
    float outputGain;   ///< Gain for the output. Default value: 1. Changes between consecutive process() calls are automatically smoothed.
    
    /// @brief Constructor.
    MonoMixer();
    ~MonoMixer();
        
    /// @brief Mixes up to 4 mono inputs into a mono output.
    /// @param input0 Pointer to floating point numbers. 32-bit input buffer for the first input. Can be NULL.
    /// @param input1 Pointer to floating point numbers. 32-bit input buffer for the second input. Can be NULL.
    /// @param input2 Pointer to floating point numbers. 32-bit input buffer for the third input. Can be NULL.
    /// @param input3 Pointer to floating point numbers. 32-bit input buffer for the fourth input. Can be NULL.
    /// @param output Pointer to floating point numbers. 32-bit output buffer.
    /// @param numberOfFrames Number of frames to process. Must be a multiple of 4.
    void process(float *input0, float *input1, float *input2, float *input3, float *output, unsigned int numberOfFrames);
    
private:
    monoMixerInternals *internals;
    MonoMixer(const MonoMixer&);
    MonoMixer& operator=(const MonoMixer&);
};

}

#endif
