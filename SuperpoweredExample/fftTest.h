#import <Accelerate/Accelerate.h>
#import "SuperpoweredFFT.h"
#import <mach/mach_time.h>
#import <math.h>

#define TICKSTONANOSECOND(ticks) (ticks * timebaseInfo.numer / timebaseInfo.denom)
#define TICKSTOMHZ(ticks) (double(ticks * timebaseInfo.numer / timebaseInfo.denom) / nanoSecondsInOneMhz)

static void SuperpoweredFFTTest() {
    mach_timebase_info_data_t timebaseInfo;
    (void) mach_timebase_info(&timebaseInfo);
// Put your iOS device's CPU Mhz here. There is no reliable way to detect this programmatically.
    double mHz = 1300.0;
    double nanoSecondsInOneMhz = 1000000000.0 / mHz;
    
    printf("CPU max freq: %.0f mHz, dynamic freq enabled (can't disable on iOS)\n\n", mHz);
    printf("             ||                 Apple vDSP                ||                Superpowered               |\n");
    printf("             ||       forward       |       inverse       ||       forward       |       inverse       |\n");
    printf("FFT   | size ||    ns    |    mHz   |    ns    |    mHz   ||    ns    |    mHz   |    ns    |    mHz   | speedup\n");
    printf("------|------||----------|----------|----------|----------||----------|----------|----------|----------|---------\n");
    
    float *vdsp_real = (float *)malloc(8192 * 4), *vdsp_imag = (float *)malloc(8192 * 4), *real = (float *)malloc(8192 * 4), *imag = (float *)malloc(8192 * 4), *vdsp_magnitude = (float *)malloc(8192 * 4), *vdsp_phase = (float *)malloc(8192 * 4);
    
    for (int logSize = 5; logSize < 14; logSize++) { // FFT size: from 32 to 8192
        FFTSetup setup = vDSP_create_fftsetup(logSize, FFT_RADIX2);
        SuperpoweredFFTPrepare(logSize, true);
        
        // fill the input data with values from -1.0f to 1.0f
        int fftSize = 1 << logSize;
        float vstep = (1.0f / (fftSize / 2)), v = -1.0f;
        for (unsigned int m = 0, i = 0; m < fftSize; m++, i++) {
            vdsp_real[i] = real[i] = v;
            v += vstep;
            
            m++;
            
            if (m == fftSize - 1) v = 1.0f;
            vdsp_imag[i] = imag[i] = v;
            v += vstep;
        };
        
        // perform forward real FFT
        DSPSplitComplex complex; complex.realp = vdsp_real; complex.imagp = vdsp_imag;
        uint64_t st = mach_absolute_time();
        vDSP_fft_zrip((FFTSetup)setup, &complex, 1, logSize, FFT_FORWARD);
        uint64_t vdsp_forward = mach_absolute_time() - st;
        
        st = mach_absolute_time();
        SuperpoweredFFTReal(real, imag, logSize, true);
        uint64_t forward = mach_absolute_time() - st;
        
        // check the results
        unsigned int errors = 0;
        for (unsigned int n = 0; n < fftSize / 2; n++) {
            float realError = fabsf(vdsp_real[n] - real[n]), imagError = fabsf(vdsp_imag[n] - imag[n]);
            if (realError + imagError > 0.001f) {
                printf("forward complex real %f %f   imag %f %f\n", vdsp_real[n], real[n], vdsp_imag[n], imag[n]);
                errors++;
            };
        };
        
        // perform inverse real FFT
        st = mach_absolute_time();
        vDSP_fft_zrip((FFTSetup)setup, &complex, 1, logSize, FFT_INVERSE);
        uint64_t vdsp_inverse = mach_absolute_time() - st;
        
        st = mach_absolute_time();
        SuperpoweredFFTReal(real, imag, logSize, false);
        uint64_t inverse = mach_absolute_time() - st;
        
        // check the results
        float scale = 1.0f / float(fftSize);
        for (unsigned int n = 0; n < fftSize / 2; n++) {
            vdsp_real[n] *= scale;
            vdsp_imag[n] *= scale;
            real[n] *= scale;
            imag[n] *= scale;
            
            float realError = fabsf(vdsp_real[n] - real[n]), imagError = fabsf(vdsp_imag[n] - imag[n]);
            if (realError + imagError > 0.001f) {
                printf("inverse complex real %f %f   imag %f %f\n", vdsp_real[n], real[n], vdsp_imag[n], imag[n]);
                errors++;
            };
        };

        if (errors) exit(0);
        printf("real  | %4i || %8llu | %8.4f | %8llu | %8.4f || %8llu | %8.4f | %8llu | %8.4f | %5.1fx\n", 1 << logSize, TICKSTONANOSECOND(vdsp_forward), TICKSTOMHZ(vdsp_forward), TICKSTONANOSECOND(vdsp_inverse), TICKSTOMHZ(vdsp_inverse), TICKSTONANOSECOND(forward), TICKSTOMHZ(forward), TICKSTONANOSECOND(inverse), TICKSTOMHZ(inverse), float(vdsp_forward + vdsp_inverse) / float(forward + inverse));
        
        // fill the input data with values from -1.0f to 1.0f
        vstep = (1.0f / (fftSize / 2)); v = -1.0f;
        for (unsigned int m = 0, i = 0; m < fftSize; m++, i++) {
            vdsp_real[i] = real[i] = v;
            v += vstep;
            
            m++;
            
            if (m == fftSize - 1) v = 1.0f;
            vdsp_imag[i] = imag[i] = v;
            v += vstep;
        };
        
        // perform forward polar FFT
        st = mach_absolute_time();
        vDSP_fft_zrip((FFTSetup)setup, &complex, 1, logSize, FFT_FORWARD);
        vDSP_zvabs(&complex, 1, vdsp_magnitude, 1, fftSize / 2);
        vDSP_zvphas(&complex, 1, vdsp_phase, 1, fftSize / 2);
        uint64_t vdsp_forwardpolar = mach_absolute_time() - st;
        
        st = mach_absolute_time();
        SuperpoweredPolarFFT(real, imag, logSize, true);
        uint64_t forwardpolar = mach_absolute_time() - st;
        
        // check the results against std math functions
        errors = 0;
        vdsp_imag[0] = vdsp_real[0] = 0.0f;
        for (unsigned int n = 0; n < fftSize / 2; n++) {
            float phase = atan2f(vdsp_imag[n], vdsp_real[n]) * float(0.5 / M_PI);
            float magnitude = sqrtf(vdsp_imag[n] * vdsp_imag[n] + vdsp_real[n] * vdsp_real[n]);
            vdsp_real[n] *= 0.5f;
            vdsp_imag[n] *= 0.5f;
            
            float magError = fabsf(magnitude - real[n]) / fabsf(magnitude), phaseError = fabsf(phase - imag[n]) / fabsf(phase);
            if ((magError > 0.07f) || (phaseError > 0.01f)) {
                printf("forward polar %i  mag %f %f   phase %f %f\n", n, magnitude, magError, phase, imag[n]);
                errors++;
            };
            
            imag[n] *= 2.0f; // inverse needs -1 - 1 range
            real[n] *= 0.5f;
        };
        
        // perform inverse polar FFT
        st = mach_absolute_time();
        complex.realp = vdsp_magnitude;
        complex.imagp = vdsp_phase;
        DSPComplex *tempComplex = new DSPComplex[fftSize / 2];
        vDSP_ztoc(&complex, 1, tempComplex, 2, fftSize / 2);
        vDSP_rect((float*)tempComplex, 2, (float*)tempComplex, 2, fftSize / 2);
        vDSP_ctoz(tempComplex, 2, &complex, 1, fftSize / 2);
        
        complex.realp = vdsp_real;
        complex.imagp = vdsp_imag;
        vDSP_fft_zrip((FFTSetup)setup, &complex, 1, logSize, FFT_INVERSE);
        uint64_t vdsp_inversepolar = mach_absolute_time() - st;
        
        st = mach_absolute_time();
        SuperpoweredPolarFFT(real, imag, logSize, false);
        uint64_t inversepolar = mach_absolute_time() - st;
        
        // check the results
        for (unsigned int n = 0; n < fftSize / 2; n++) {
            vdsp_real[n] *= scale;
            vdsp_imag[n] *= scale;
            real[n] *= scale;
            imag[n] *= scale;
            
            float realError = fabsf(vdsp_real[n] - real[n]), imagError = fabsf(vdsp_imag[n] - imag[n]);
            if ((realError > 0.05f) || (imagError > 0.05f)) {
                printf("inverse polar %i  real %f %f   imag %f %f\n", n, vdsp_real[n], real[n], vdsp_imag[n], imag[n]);
                errors++;
            };
        };
        
        if (errors) exit(0);
        printf("polar | %4i || %8llu | %8.4f | %8llu | %8.4f || %8llu | %8.4f | %8llu | %8.4f | %5.1fx\n", 1 << logSize, TICKSTONANOSECOND(vdsp_forwardpolar), TICKSTOMHZ(vdsp_forwardpolar), TICKSTONANOSECOND(vdsp_inversepolar), TICKSTOMHZ(vdsp_inversepolar), TICKSTONANOSECOND(forwardpolar), TICKSTOMHZ(forwardpolar), TICKSTONANOSECOND(inversepolar), TICKSTOMHZ(inversepolar), float(vdsp_forwardpolar + vdsp_inversepolar) / float(forwardpolar + inversepolar));
        
        vDSP_destroy_fftsetup(setup);
    }
}
