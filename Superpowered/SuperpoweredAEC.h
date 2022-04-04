#ifndef Header_SuperpoweredAEC
#define Header_SuperpoweredAEC

#ifndef JSWASM
#define JSWASM
#endif

namespace Superpowered {

struct aecInternals;

/// @brief Acoustic Echo Cancellation
/// One instance allocates 128 kb memory.
class AEC {
public:
    /// @brief Speaker to mic distance and room size.
    typedef enum MicToSpeakerDistance {
        MicToSpeakerDistance_Handheld = 0, ///< Telephony, speaker to mic distance is very short (example: mobile phone).
        MicToSpeakerDistance_Close = 1,    ///< Speaker to mic distance is short, very little room sound (example: studio).
        MicToSpeakerDistance_Room = 2,     ///< Speaker to mic distance is moderate, the mic picks up some room sound (example: living room).
        MicToSpeakerDistance_Large = 3,    ///< Speaker to mic distance is large, the mic picks up reverberation (example: large room, hall).
    } MicToSpeakerDistance;
    
    MicToSpeakerDistance distance; ///< Speaker to mic distance and room size. Default: MicToSpeakerDistance_Handheld.
    unsigned int samplerate; ///< Input/output sample rate. Default: 48000.
    float doubleTalkSensitivity; ///< Sensitivity for (double) talk detection between 0...1. 0: always adapt, default 0.5.
    float qualityVsQuickAdapt; ///< Trade-off between audio quality and how quickly AEC adapts to a changing environment (such as moving away from the microphone). Minimum: 0.1 (high quality, slow adaptation), maximum: 1 (lower quality, quick adaptation). Default: 0.5.

/// @brief Constructor.
    JSWASM AEC();
    JSWASM ~AEC();
    
/// @brief Processes the audio. It's never blocking for real-time usage. Simplified explanation: output = mic - loudspeaker.
/// You can change all properties on any thread, concurrently with process(). Do not call any method concurrently with process().
/// @param loudspeaker Pointer to floating point numbers. 32-bit mono audio, typically sent to the speaker/headset. AEC will remove this signal from the mic signal. VOIP: the "far" signal, audio incoming from the network. Karaoke: the backing track.
/// @param mic Pointer to floating point numbers. 32-bit mono microphone input that should be cleaned (VOIP: "near" signal).
/// @param output Pointer to floating point numbers. 32-bit mono output. The "cleaned" mic audio.
/// @param numberOfFrames Number of frames to process.
    JSWASM void process(float *loudspeaker, float *mic, float *output, unsigned int numberOfFrames);
    
/// @brief Set all internals to initial state.
    JSWASM void reset();

private:
    aecInternals *internals;
    AEC(const AEC&);
    AEC& operator=(const AEC&);
};

};

#endif
