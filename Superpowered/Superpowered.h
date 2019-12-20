#ifndef Header_Superpowered
#define Header_Superpowered

/// @file Superpowered.h
/// @brief Initializes the Superpowered SDKs.

namespace Superpowered {

/// @fn Initialize(const char *licenseKey, bool enableAudioAnalysis, bool enableFFTAndFrequencyDomain, bool enableAudioTimeStretching, bool enableAudioEffects, bool enableAudioPlayerAndDecoder, bool enableCryptographics, bool enableNetworking);
/// @brief Initializes the Superpowered SDKs. Use this only once, when your app or library initializes.
/// Do not use this if Superpowered is loaded dynamically and might be loaded multiple times (a DLL in a VST host for example). @see DynamicInitialize
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

/// @fn DynamicInitialize(const char *licenseKey);
/// @brief Use this if Superpowered is loaded in a dynamically loaded library (such as a DLL on Windows). It allows for multiple loads (DLL in a VST host example). Enables all features.
/// @param licenseKey Visit https://superpowered.com/dev to register license keys.
void DynamicInitialize(const char *licenseKey);

/// @fn DynamicDestroy();
/// @brief Use this if Superpowered is used in a dynamically loaded library (such as a DLL on Windows), when the dynamically loaded library instance unloads (even if multiple loads may happen).
/// This function will block waiting for all Superpowered background threads to exit when the last instance of the library is unloaded.
/// Please note that you still need to "properly" release all Superpowered objects _before_ this call, such as delete all players, effects, etc.
void DynamicDestroy();

}

/**
\mainpage Audio, Networking, and Cryptographics for Android, iOS, macOS, tvOS, Linux and Windows

Details of the latest features/versions can be found at:

Audio: https://superpowered.com/audio-library-sdk

Networking: https://superpowered.com/networking-library-sdk

Cryptographics: https://superpowered.com/crypto-library-sdk
*/

#endif
