#ifndef Header_SuperpoweredSpatializer
#define Header_SuperpoweredSpatializer

#include "SuperpoweredReverb.h"

namespace Superpowered {

struct spatializerInternals;

/// @brief CPU-friendly 3D audio spatializer.
/// One instance allocates around 140 kb memory.
/// The spatializer class also has one Global Spatializer Reverb instance to simulate "room sound". It collects audio from all Superpowered Spatializer instances and puts a reverb on the signal.
class Spatializer {
public:
    unsigned int samplerate; ///< Sample rate in Hz.
    float inputVolume;       ///< Input volume (gain).
    float azimuth;           ///< From 0 to 360 degrees.
    float elevation;         ///< -90 to 90 degrees.
    float reverbmix;         ///< The ratio of how much audio the Global Spatializer Reverb can collect from this instance (between 0 and 1).
    float occlusion;         ///< Occlusion factor (between 0 and 1);
    bool sound2;             ///< Alternative sound option. True on, false off.
    
    static float reverbWidth;      ///< Global Spatializer Reverb stereo width. >= 0 and <= 1.
    static float reverbDamp;       ///< Global Spatializer Reverb high frequency damping. >= 0 and <= 1.
    static float reverbRoomSize;   ///< Global Spatializer Reverb room size. >= 0 and <= 1.
    static float reverbPredelayMs; ///< Global Spatializer Reverb pre-delay in milliseconds. 0 to 500.
    static float reverbLowCutHz;   ///< Global Spatializer Reverb frequency of the low cut in Hz (-12 db point). Default: 0 (no low frequency cut).
    
/// @brief Constructor.
/// @param samplerate The initial sample rate in Hz.
    Spatializer(unsigned int samplerate);
    ~Spatializer();

/// @brief Processes the audio.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param inputLeft Pointer to floating point numbers. 32-bit left channel or interleaved stereo input.
/// @param inputRight Pointer to floating point numbers. 32-bit right channel input. Can be NULL, inputLeft will be used in this case as interleaved stereo input.
/// @param outputLeft Pointer to floating point numbers. 32-bit left channel or interleaved stereo output.
/// @param outputRight Pointer to floating point numbers. 32-bit right channel output. Can be NULL, outputLeft will be used in this case as interleaved stereo output.
/// @param numberOfFrames Number of frames to process. Valid between 64-8192.
/// @param outputAdd If true, audio will be added to whatever content is in outputLeft or outputRight.
    bool process(float *inputLeft, float *inputRight, float *outputLeft, float *outputRight, unsigned int numberOfFrames, bool outputAdd);
    
/// @brief Outputs the Global Spatializer Reverb. Always call it in the audio processing callback, regardless if the effect is enabled or not for smooth, audio-artifact free operation. Should be called after every Superpowered Spatializer's process() method.
/// It's never blocking for real-time usage. You can change all properties of the globalReverb on any thread, concurrently with process().
/// @return If process() returns with true, the contents of output are replaced with the audio output. If process() returns with false, the contents of output are not changed.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output.
/// @param numberOfFrames Number of framesto process. Should not be higher than 8192.
    static bool reverbProcess(float *output, unsigned int numberOfFrames);

private:
    spatializerInternals *internals;
    Spatializer(const Spatializer&);
    Spatializer& operator=(const Spatializer&);
};

}

#endif
