#ifndef Header_SuperpoweredFFT
#define Header_SuperpoweredFFT

/**
 @file SuperpoweredFFT.h
 @brief Super fast FFT.
 
 Single threaded, not thread safe.
*/


/**
 @fn SuperpoweredFFTPrepare(int logSize, bool real);
 @brief Call this once, before processing. No subsequent calls required.
 
 @param logSize Should be 5 - 13 (FFT sizes 32 - 8192).
 @param real True if you prepare for a real function, false for complex.
 */
void SuperpoweredFFTPrepare(int logSize, bool real);


/**
 @fn SuperpoweredFFTCleanup();
 @brief Cleans up the memory allocated by FFTPrepare.
 */
void SuperpoweredFFTCleanup();


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
 @fn SuperpoweredPolarFFT(float *mag, float *phase, int logSize, bool forward);
 @brief Polar FFT.
 
 Data packing is same as Apple's vDSP. Check the "Using Fourier Transforms" page of Apple's vDSP documentation ("Data Packing for Real FFTs").
 
 @param mag Input: split real part. Output: magnitudes.
 @param phase Input: split real part. Output: phases.
 @param logSize Should be 5 - 13 (FFT sizes 32 - 8192).
 @param forward Forward or inverse.
 */
void SuperpoweredPolarFFT(float *mag, float *phase, int logSize, bool forward);

#endif
