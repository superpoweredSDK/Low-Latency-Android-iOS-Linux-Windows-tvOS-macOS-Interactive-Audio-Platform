#ifndef Header_SuperpoweredFFT
#define Header_SuperpoweredFFT

/**
 @file SuperpoweredFFT.h
 @brief Super fast FFT.
 
 Single threaded, not thread safe. 
 
 @remark Superpowered FFT benefits from ideas in Construction of a High-Performance FFT by Eric Postpischil (http://edp.org/resume.htm).
*/

/**
 @fn SuperpoweredFFTComplex(float *real, float *imag, int logSize, bool forward);
 @brief Complex in-place FFT.
 
 Data packing is same as Apple's vDSP. Check the "Using Fourier Transforms" page of Apple's vDSP documentation ("Data Packing for Real FFTs").
 
 @param real Real part.
 @param imag Imaginary part.
 @param logSize Should be 4 - 12 (FFT sizes 16 - 4096).
 @param forward Forward or inverse.
 */
void SuperpoweredFFTComplex(float *real, float *imag, int logSize, bool forward);


/**
 @fn SuperpoweredFFTReal(float *real, float *imag, int logSize, bool forward);
 @brief Real in-place FFT.
 
 Data packing is same as Apple's vDSP. Check the "Using Fourier Transforms" page of Apple's vDSP documentation ("Data Packing for Real FFTs").
 
 @param real Real part.
 @param imag Imaginary part.
 @param logSize Should be 5 - 13 (FFT sizes 32 - 8192).
 @param forward Forward or inverse.
 */
void SuperpoweredFFTReal(float *real, float *imag, int logSize, bool forward);


/**
 @fn SuperpoweredPolarFFT(float *mag, float *phase, int logSize, bool forward, float valueOfPi);
 @brief Polar FFT.
 
 Data packing is same as Apple's vDSP. Check the "Using Fourier Transforms" page of Apple's vDSP documentation ("Data Packing for Real FFTs").
 
 @param mag Input: split real part. Output: magnitudes.
 @param phase Input: split real part. Output: phases.
 @param logSize Should be 5 - 13 (FFT sizes 32 - 8192).
 @param forward Forward or inverse.
 @param valueOfPi The function can translate pi to any value (Google: the tau manifesto). Leave it at 0 for M_PI.
 */
void SuperpoweredPolarFFT(float *mag, float *phase, int logSize, bool forward, float valueOfPi = 0);

#endif
