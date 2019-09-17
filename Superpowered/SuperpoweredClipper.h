#ifndef Header_SuperpoweredClipper
#define Header_SuperpoweredClipper

namespace Superpowered {

struct clipperInternals;

/// @brief Hard knee clipping with 0 latency.
/// It doesn't allocate any internal buffers and needs just a few bytes of memory.
class Clipper {
public:
    float thresholdDb; ///< Audio below this will be unchanged, above this will be attenuated. Limited between -100 and 0.
    float maximumDb;   ///< Audio will reach 1.0f at this point. Limited between -48 and 48.

/// @brief Constructor;
    Clipper();
    ~Clipper();

/// @brief Processes the audio.
/// It's never blocking for real-time usage. You can change all properties on any thread, concurrently with process().
/// @param input Pointer to floating point numbers. 32-bit interleaved stereo input.
/// @param output Pointer to floating point numbers. 32-bit interleaved stereo output. Can point to the same location with input (in-place processing).
/// @param numberOfFrames Should be 4 minimum and exactly divisible with 4.
    void process(float *input, float *output, unsigned int numberOfFrames);

private:
    clipperInternals *internals;
    Clipper(const Clipper&);
    Clipper& operator=(const Clipper&);
};

}

#endif
