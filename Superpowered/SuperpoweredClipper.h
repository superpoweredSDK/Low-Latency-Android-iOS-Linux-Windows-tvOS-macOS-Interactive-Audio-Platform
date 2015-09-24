#ifndef Header_SuperpoweredClipper
#define Header_SuperpoweredClipper

struct clipperInternals;

/**
 @brief Hard knee clipping with 0 latency.
 
 It doesn't allocate any internal buffers and needs just a few bytes of memory.
 
 @param thresholdDb Audio below this will be unchanged, above this will be attenuated. Limited between -100.0f and 0.0f.
 @param maximumDb Audio will reach 1.0f at this point. Limited between -48.0f and 48.0f.
*/
class SuperpoweredClipper {
public:
    float thresholdDb;
    float maximumDb;

    SuperpoweredClipper();
    ~SuperpoweredClipper();

/**
 @brief Processes the audio.

 @param input 32-bit interleaved stereo input buffer. Can point to the same location with output (in-place processing).
 @param output 32-bit interleaved stereo output buffer. Can point to the same location with input (in-place processing).
 @param numberOfSamples Should be 4 minimum and exactly divisible with 4.
*/
    void process(float *input, float *output, unsigned int numberOfSamples);

private:
    clipperInternals *internals;
};

#endif
