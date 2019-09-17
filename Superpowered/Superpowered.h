#ifndef Header_Superpowered
#define Header_Superpowered

/// @file Superpowered.h
/// @brief Initializes the Superpowered SDKs.

namespace Superpowered {

/// @fn Initialize(const char *licenseKey, bool enableAudioAnalysis, bool enableFFTAndFrequencyDomain, bool enableAudioTimeStretching, bool enableAudioEffects, bool enableAudioPlayerAndDecoder, bool enableCryptographics, bool enableNetworking);
/// @brief Initializes the Superpowered SDKs.
/// @param licenseKey Visit https://superpowered.com/dev to register license keys.
/// @param enableAudioAnalysis Enables Analyzer, LiveAnalyzer, Waveform and BandpassFilterbank.
/// @param enableFFTAndFrequencyDomain Enables FrequencyDomain, FFTComplex, FFTReal and PolarFFT.
/// @param enableAudioTimeStretching Enables TimeStretching.
/// @param enableAudioEffects Enables all effects and every class based on the FX class.
/// @param enableAudioPlayerAndDecoder Enables AdvancedAudioPlayer and Decoder.
/// @param enableCryptographics Enables RSAPublicKey,RSAPrivateKey, hasher and AES.
/// @param enableNetworking Enables httpRequest.

void Initialize(const char *licenseKey,
                bool enableAudioAnalysis,
                bool enableFFTAndFrequencyDomain,
                bool enableAudioTimeStretching,
                bool enableAudioEffects,
                bool enableAudioPlayerAndDecoder,
                bool enableCryptographics,
                bool enableNetworking);

}

/**
\mainpage Audio, Networking, and Cryptographics for Android, iOS, macOS, tvOS, Linux and Windows

Details of the latest features/versions can be found at:

Audio: https://superpowered.com/audio-library-sdk

Networking: https://superpowered.com/networking-library-sdk

Cryptographics: https://superpowered.com/crypto-library-sdk
*/

#endif
