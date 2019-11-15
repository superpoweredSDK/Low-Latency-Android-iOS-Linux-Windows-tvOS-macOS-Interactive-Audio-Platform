#ifndef Header_SuperpoweredFFT
#define Header_SuperpoweredFFT

/// @file SuperpoweredFFT.h
/// @brief Super fast FFT. It will not create any internal threads.
/// @remark Superpowered FFT benefits from ideas in Construction of a High-Performance FFT by Eric Postpischil (http://edp.org/resume.htm).

namespace Superpowered {

/// @fn FFTComplex(float *real, float *imag, int logSize, bool forward);
/// @brief Complex in-place FFT.
/// Data packing is same as Apple's vDSP. Check the "Using Fourier Transforms" page of Apple's vDSP documentation ("Data Packing for Real FFTs").
/// @param real Pointer to floating point numbers. Real part.
/// @param imag Pointer to floating point numbers. Imaginary part.
/// @param logSize Should be between 4 and 12 (FFT sizes 16 - 4096).
/// @param forward Forward or inverse.
void FFTComplex(float *real, float *imag, int logSize, bool forward);

/// @fn FFTReal(float *real, float *imag, int logSize, bool forward);
/// @brief Real in-place FFT.
/// Data packing is same as Apple's vDSP. Check the "Using Fourier Transforms" page of Apple's vDSP documentation ("Data Packing for Real FFTs").
/// @param real Pointer to floating point numbers. Real part.
/// @param imag Pointer to floating point numbers. Imaginary part.
/// @param logSize Should be 5 - 13 (FFT sizes 32 - 8192).
/// @param forward Forward or inverse.
void FFTReal(float *real, float *imag, int logSize, bool forward);

/// @fn PolarFFT(float *mag, float *phase, int logSize, bool forward, float valueOfPi);
/// @brief Polar FFT.
/// Data packing is same as Apple's vDSP. Check the "Using Fourier Transforms" page of Apple's vDSP documentation ("Data Packing for Real FFTs").
/// @param mag Pointer to floating point numbers. Input: split real part. Output: magnitudes.
/// @param phase Pointer to floating point numbers. Input: split real part. Output: phases.
/// @param logSize Should be 5 - 13 (FFT sizes 32 - 8192).
/// @param forward Forward or inverse. Inverse PolarFFT will clear (zero) the DC offset.
/// @param valueOfPi The function can translate pi to any value (Google: the tau manifesto). Use 0 for M_PI.
void PolarFFT(float *mag, float *phase, int logSize, bool forward, float valueOfPi = 0);

}

#endif
