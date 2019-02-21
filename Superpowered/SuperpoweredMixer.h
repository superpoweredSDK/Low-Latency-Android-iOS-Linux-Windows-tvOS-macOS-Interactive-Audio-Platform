#ifndef Header_SuperpoweredMixer
#define Header_SuperpoweredMixer

struct stereoMixerInternals;
struct monoMixerInternals;

/**
 @brief Mixer and splitter.
 
 Mixes max. 4 interleaved stereo inputs together. Output can be interleaved or non-interleaved (split). Separate input channel levels (good for gain and pan), separate output channel levels (master gain and pan). Returns maximum values for metering.
 
 One instance allocates just a few bytes of memory.
 */
class SuperpoweredStereoMixer {
public:
/**
 @brief Creates a stereo mixer instance.
 */
    SuperpoweredStereoMixer();
    ~SuperpoweredStereoMixer();
    
/**
 @brief Mixes max. 4 interleaved stereo inputs into a stereo output.
 
 @param inputs Four pointers to stereo interleaved input buffers. Any pointer can be NULL.
 @param outputs If outputs[1] is NULL, output is interleaved stereo in outputs[0]. If outputs[1] is not NULL, output is non-interleaved (left side in outputs[0], right side in outputs[1]).
 @param inputLevels Input volume level for each channel. Value changes between consecutive processes are automatically smoothed.
 @param outputLevels Output levels [left, right]. Value changes between consecutive processes are automatically smoothed.
 @param inputMeters Returns the maximum values for metering. Can be NULL.
 @param outputMeters Returns the maximum values for metering. Can be NULL.
 @param numberOfSamples The number of samples to process. Minimum 2, maximum 2048, must be exactly divisible with 2.
 */
    void process(float *inputs[4], float *outputs[2], float inputLevels[8], float outputLevels[2], float inputMeters[8], float outputMeters[2], unsigned int numberOfSamples);

/**
 @brief Mixes max. 4 interleaved stereo channels into a stereo output and changes volume in the channels as well. Deprecated, will be removed soon.

 @param channels Four pointers to stereo interleaved input/output buffers. Every pointer should not be NULL.
 @param outputs If outputs[1] is NULL, output is interleaved stereo in outputs[0]. If outputs[1] is not NULL, output is non-interleaved (left side in outputs[0], right side in outputs[1]).
 @param channelSwitches On/off switches for each channel.
 @param channelOutputLevels Volume for each channel output. Value changes between consecutive processes are automatically smoothed.
 @param numberOfSamples The number of samples to process. Minimum 2, maximum 2048, must be exactly divisible with 2.
*/
    void processPFL(float *channels[4], float *outputs[2], bool channelSwitches[4], float channelOutputLevels[4], unsigned int numberOfSamples);

private:
    stereoMixerInternals *internals;
    SuperpoweredStereoMixer(const SuperpoweredStereoMixer&);
    SuperpoweredStereoMixer& operator=(const SuperpoweredStereoMixer&);
};

/**
@brief Mixes max. 4 mono inputs into a mono output.
 
 One instance allocates just a few bytes of memory.
 */
class SuperpoweredMonoMixer {
public:
/**
 @brief Creates a mono mixer instance.
*/
    SuperpoweredMonoMixer();
    ~SuperpoweredMonoMixer();
    
/**
 @brief Processes the audio.
 
 @param inputs Four pointers to input buffers. Any pointer can be NULL.
 @param output Output buffer.
 @param inputLevels Four input volume levels. Value changes between consecutive processes are automatically smoothed.
 @param outputGain Output level. Value changes between consecutive processes are automatically smoothed.
 @param numberOfSamples The number of samples to process. Minimum 8, maximum 2048, must be exactly divisible with 4.
 */
    void process(float **inputs, void *output, float *inputLevels, float outputGain, unsigned int numberOfSamples);
    
private:
    monoMixerInternals *internals;
    SuperpoweredMonoMixer(const SuperpoweredMonoMixer&);
    SuperpoweredMonoMixer& operator=(const SuperpoweredMonoMixer&);
};

#endif
