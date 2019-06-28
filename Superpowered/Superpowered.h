#ifndef Header_Superpowered
#define Header_Superpowered

/**
 @file Superpowered.h
 @brief Initializes the Superpowered SDKs.
 */

/**
 @fn SuperpoweredInitialize(
        const char *licenseKey,
        bool enableAudioAnalysis,
        bool enableFFTAndFrequencyDomain,
        bool enableAudioTimeStretching,
        bool enableAudioEffects,
        bool enableAudioPlayerAndDecoder,
        bool enableCryptographics,
        bool enableNetworking
    );
 
 @brief Initializes the Superpowered SDKs.
 
 @param licenseKey Visit https://superpowered.com/dev to register license keys.
 @param enableAudioAnalysis Enables SuperpoweredAnalyzer, SuperpoweredLiveAnalyzer, SuperpoweredWaveform and SuperpoweredBandpassFilterbank.
 @param enableFFTAndFrequencyDomain Enables SuperpoweredFrequencyDomain, SuperpoweredFFTComplex, SuperpoweredFFTReal and SuperpoweredPolarFFT.
 @param enableAudioTimeStretching Enables SuperpoweredTimeStretching.
 @param enableAudioEffects Enables all effects and every class based on the SuperpoweredFX class.
 @param enableAudioPlayerAndDecoder Enables SuperpoweredAdvancedAudioPlayer and SuperpoweredDecoder.
 @param enableCryptographics Enables Superpowered::RSAPublicKey, Superpowered::RSAPrivateKey, Superpowered::hasher and Superpowered::AES.
 @param enableNetworking Enables Superpowered::httpRequest.
 */

void SuperpoweredInitialize(const char *licenseKey,
                            bool enableAudioAnalysis,
                            bool enableFFTAndFrequencyDomain,
                            bool enableAudioTimeStretching,
                            bool enableAudioEffects,
                            bool enableAudioPlayerAndDecoder,
                            bool enableCryptographics,
                            bool enableNetworking);

#endif
