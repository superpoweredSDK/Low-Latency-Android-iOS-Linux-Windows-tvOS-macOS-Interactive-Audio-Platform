#ifndef Header_SuperpoweredSpatializer
#define Header_SuperpoweredSpatializer

#include "SuperpoweredReverb.h"

struct spatializerInternals;

/**
 @brief Global Spatializer Reverb object. Should be statically created, such as:

static SuperpoweredSpatializerGlobalReverb globalReverb;
 
 The default sample rate upon creation is 44100.
 */
class SuperpoweredSpatializerGlobalReverb {
public:
// READ ONLY parameter
    static SuperpoweredReverb *reverb;

    SuperpoweredSpatializerGlobalReverb();
    ~SuperpoweredSpatializerGlobalReverb();

/**
 @brief Sets the sample rate.

 @param samplerate 44100, 48000, etc.
*/
    static void setReverbSamplerate(unsigned int samplerate);

/**
 @brief Outputs the reverb audio from the global reverb.

 @param output 32-bit interleaved stereo output buffer.
 @param numberOfSamples Number of samples to process. Should not be higher than 8192.
*/
    static bool process(float *output, unsigned int numberOfSamples);
};

/**
 @brief CPU-friendly 3D audio spatializer.
 
 One instance allocates around 140 kb memory.

 @param inputVolume Input volume.
 @param azimuth From 0 to 360 degrees.
 @param elevation -90 to 90 degrees.
 @param reverbmix The ratio of the spatial reverb (between 0.0f and 1.0f).
 @param occlusion Occlusion factor (between 0.0f and 1.0f);
 @param sound2 Alternative sound option.
 */
class SuperpoweredSpatializer {
public:
// READ_WRITE parameters
    float inputVolume;
    float azimuth;
    float elevation;
    float reverbmix;
    float occlusion;
    bool sound2;

/**
 @brief Create a spatializer instance.
*/
    SuperpoweredSpatializer(unsigned int samplerate);
    ~SuperpoweredSpatializer();

/**
 @brief Sets the sample rate.

 @param samplerate 44100, 48000, etc.
*/
    void setSamplerate(unsigned int samplerate);

/**
 @brief Processes the audio.

 @return Put something into output or not.

 @param inputLeft 32-bit left channel or interleaved stereo input.
 @param inputRight Right channel input. Can be NULL, inputLeft will be used in this case.
 @param outputLeft 32-bit left channel or interleaved stereo output.
 @param outputRight Right channel output. Can be NULL, outputLeft will be used in this case.
 @param numberOfSamples Number of samples to process. Valid between 64-8192.
 @param outputAdd If true, audio will be added to whatever content is in outputLeft or outputRight.
 */
    bool process(float *inputLeft, float *inputRight, float *outputLeft, float *outputRight, unsigned int numberOfSamples, bool outputAdd);

private:
    spatializerInternals *internals;
    SuperpoweredSpatializer(const SuperpoweredSpatializer&);
    SuperpoweredSpatializer& operator=(const SuperpoweredSpatializer&);
};

#endif
