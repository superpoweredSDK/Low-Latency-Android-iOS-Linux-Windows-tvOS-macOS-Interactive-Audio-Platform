#import <Accelerate/Accelerate.h>
#import "SuperpoweredFFT.h"
#import <mach/mach_time.h>
#import <math.h>

#define VALUE_OF_PI M_PI

// Result differences to vDSP
#if __arm64__
#define vdspdiff 0.00125f // 64-bit ARM: Superpowered is MORE PRECISE than vDSP due the fused multiply-add instructions inside
#elif __arm__
#define vdspdiff 0.0f // 32-bit ARM: complex and real results match with vDSP
#else
#define vdspdiff 0.000001f // Intel
#endif

// Superpowered calculates MORE PRECISE magnitude than vDSP (due ARM native sqrtf or vrsqrte/s)
#define magnitudediff 0.0001f // 0.01 percent, max. -106 decibel

// Superpowered calculates phase with a different method (different atan2f implementation)
#define phasediff 0.0065f // 0.65 percent, max. 2.34 degrees

// Inverse polar difference
#define invpolardiff 0.017f // 1.7 percent

#if __arm64__ || __arm__
static int checkRealImagErrorsForward(const char *job, float *vdsp_real, float *real, float *vdsp_imag, float *imag, int fftSize, int size) {
    int errors = 0;

    for (unsigned int n = 0; n < size; n++) {
        float realError = fabsf(vdsp_real[n] - real[n]), imagError = fabsf(vdsp_imag[n] - imag[n]);
        if ((realError + imagError > vdspdiff) || !isfinite(real[n]) || !isfinite(imag[n])) {
            printf("%s %i, real %i %f %f   imag %f %f\n", job, fftSize, n, vdsp_real[n], real[n], vdsp_imag[n], imag[n]);
            errors++;
        };
    };

    return errors;
}

static int checkRealImagErrorsInverse(const char *job, float *vdsp_real, float *real, float *vdsp_imag, float *imag, int fftSize, int size) {
    float scale = 1.0f / float(fftSize);
    int errors = 0;

    for (unsigned int n = 0; n < size; n++) {
        vdsp_real[n] *= scale;
        vdsp_imag[n] *= scale;
        real[n] *= scale;
        imag[n] *= scale;

        float realError = fabsf(vdsp_real[n] - real[n]), imagError = fabsf(vdsp_imag[n] - imag[n]);
        if (realError + imagError > vdspdiff) {
            printf("%s %i, real %i %f %f   imag %f %f\n", job, fftSize, n, vdsp_real[n], real[n], vdsp_imag[n], imag[n]);
            errors++;
        };
    };

    return errors;
}

static void SuperpoweredFFTTest() {
    // setup time measurement
    mach_absolute_time();
    mach_absolute_time();

    printf("               ||                 Apple vDSP              ||              Superpowered               |\n");
    printf("               ||       forward      |       inverse      ||      forward       |       inverse      |\n");
    printf("FFT     | size ||    (mach ticks)    |    (mach ticks)    ||    (mach ticks)    |    (mach ticks)    | speed\n");
    printf("--------|------||--------------------|--------------------||--------------------|--------------------|---------\n");

    // prepare
    uint64_t measurements[14][3][4][10]; // logSize, complex/real/polar, vdspforward/vdspinverse/forward/inverse
    FFTSetup setups[14];
    for (int logSize = 5; logSize < 14; logSize++) setups[logSize] = vDSP_create_fftsetup(logSize, FFT_RADIX2);

    // perform each measurement 10 times
    for (int mea = 0; mea < 10; mea++) {
        for (int logSize = 5; logSize < 14; logSize++) { // FFT size: from 32 to 8192 
            int fftSize = 1 << logSize;

            float *vdsp_real = (float *)malloc(fftSize * sizeof(float));
            float *vdsp_imag = (float *)malloc(fftSize * sizeof(float));
            float *vdsp_magnitude = (float *)malloc(fftSize * sizeof(float));
            float *vdsp_phase = (float *)malloc(fftSize * sizeof(float));
            DSPComplex *polarToRectComplex = (DSPComplex *)malloc(sizeof(DSPComplex) * (fftSize / 2));
            float *real = (float *)malloc(fftSize * sizeof(float));
            float *imag = (float *)malloc(fftSize * sizeof(float));

            if (logSize < 13) { // complex doesn't work with 13

                // fill the input data with values from -1.0f to 1.0f
                float vstep = (1.0f / fftSize), v = -1.0f;
                for (unsigned int m = 0; m < fftSize; m++) {
                    vdsp_real[m] = real[m] = v;
                    vdsp_imag[m] = imag[m] = 1.0f + v;
                    v += vstep;
                };

                // perform forward complex FFT
                uint64_t volatile st = mach_absolute_time();
                SuperpoweredFFTComplex(real, imag, logSize, true);
                measurements[logSize][0][2][mea] = mach_absolute_time() - st;

                DSPSplitComplex complex; complex.realp = vdsp_real; complex.imagp = vdsp_imag;
                st = mach_absolute_time();
                vDSP_fft_zip((FFTSetup)setups[logSize], &complex, 1, logSize, FFT_FORWARD);
                measurements[logSize][0][0][mea] = mach_absolute_time() - st;

                int errors = checkRealImagErrorsForward("forward complex", vdsp_real, real, vdsp_imag, imag, fftSize, fftSize);

                // perform inverse real FFT
                st = mach_absolute_time();
                SuperpoweredFFTComplex(real, imag, logSize, false);
                measurements[logSize][0][3][mea] = mach_absolute_time() - st;

                st = mach_absolute_time();
                vDSP_fft_zip((FFTSetup)setups[logSize], &complex, 1, logSize, FFT_INVERSE);
                measurements[logSize][0][1][mea] = mach_absolute_time() - st;

                errors += checkRealImagErrorsInverse("inverse complex", vdsp_real, real, vdsp_imag, imag, fftSize, fftSize);
                if (errors) exit(0);
            };

            // fill the input data with values from -1.0f to 1.0f
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
            uint64_t volatile st = mach_absolute_time();
            SuperpoweredFFTReal(real, imag, logSize, true);
            measurements[logSize][1][2][mea] = mach_absolute_time() - st;

            DSPSplitComplex complex; complex.realp = vdsp_real; complex.imagp = vdsp_imag;
            st = mach_absolute_time();
            vDSP_fft_zrip((FFTSetup)setups[logSize], &complex, 1, logSize, FFT_FORWARD);
            measurements[logSize][1][0][mea] = mach_absolute_time() - st;

            int errors = checkRealImagErrorsForward("forward real", vdsp_real, real, vdsp_imag, imag, fftSize, fftSize / 2);

            // perform inverse real FFT
            st = mach_absolute_time();
            SuperpoweredFFTReal(real, imag, logSize, false);
            measurements[logSize][1][3][mea] = mach_absolute_time() - st;

            st = mach_absolute_time();
            vDSP_fft_zrip((FFTSetup)setups[logSize], &complex, 1, logSize, FFT_INVERSE);
            measurements[logSize][1][1][mea] = mach_absolute_time() - st;

            errors += checkRealImagErrorsInverse("inverse real", vdsp_real, real, vdsp_imag, imag, fftSize, fftSize / 2);
            if (errors) exit(0);

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
            SuperpoweredPolarFFT(real, imag, logSize, true, VALUE_OF_PI);
            measurements[logSize][2][2][mea] = mach_absolute_time() - st;

            st = mach_absolute_time();
            vDSP_fft_zrip((FFTSetup)setups[logSize], &complex, 1, logSize, FFT_FORWARD);
            vDSP_zvabs(&complex, 1, vdsp_magnitude, 1, fftSize / 2);
            vDSP_zvphas(&complex, 1, vdsp_phase, 1, fftSize / 2);
            measurements[logSize][2][0][mea] = mach_absolute_time() - st;

            // check the results
            errors = 0;
            vdsp_phase[0] = vdsp_magnitude[0] = 0.0f;
            float mul = float(VALUE_OF_PI / M_PI);
            for (unsigned int n = 0; n < fftSize / 2; n++) {
                float phase = vdsp_phase[n] * mul, magnitude = vdsp_magnitude[n];

                float magError = fabsf(magnitude - real[n]), phaseError = fabsf(phase - imag[n]);
                if (magnitude != 0) magError /= fabsf(magnitude);
                if (phase != 0) phaseError /= fabsf(phase);

                if ((magError > magnitudediff) || (phaseError > phasediff) || !isfinite(real[n]) || !isfinite(imag[n])) {
                    printf("forward polar %i  mag %f %f   phase %f %f\n", n, vdsp_magnitude[n], magError, phase, imag[n]);
                    errors++;
                };
            };

            // perform inverse polar FFT
            st = mach_absolute_time();
            SuperpoweredPolarFFT(real, imag, logSize, false, VALUE_OF_PI);
            measurements[logSize][2][3][mea] = mach_absolute_time() - st;

            st = mach_absolute_time();
            complex.realp = vdsp_magnitude;
            complex.imagp = vdsp_phase;
            vDSP_ztoc(&complex, 1, polarToRectComplex, 2, fftSize / 2);
            vDSP_rect((float*)polarToRectComplex, 2, (float*)polarToRectComplex, 2, fftSize / 2);
            complex.realp = vdsp_real;
            complex.imagp = vdsp_imag;
            vDSP_ctoz(polarToRectComplex, 2, &complex, 1, fftSize / 2);
            vDSP_fft_zrip((FFTSetup)setups[logSize], &complex, 1, logSize, FFT_INVERSE);
            measurements[logSize][2][1][mea] = mach_absolute_time() - st;
            
            // check the results
            float scale = 0.5f / float(fftSize);
            for (unsigned int n = 0; n < fftSize / 2; n++) {
                vdsp_real[n] *= scale;
                vdsp_imag[n] *= scale;
                real[n] *= scale;
                imag[n] *= scale;
                
                float realError = fabsf(vdsp_real[n] - real[n]), imagError = fabsf(vdsp_imag[n] - imag[n]);
                if ((realError > invpolardiff) || (imagError > invpolardiff) || !isfinite(real[n]) || !isfinite(imag[n])) {
                    printf("inverse polar %i  real %f %f   imag %f %f\n", n, vdsp_real[n], real[n], vdsp_imag[n], imag[n]);
                    errors++;
                };
            };
            if (errors) exit(0);

            free(vdsp_real);
            free(vdsp_imag);
            free(vdsp_magnitude);
            free(vdsp_phase);
            free(polarToRectComplex);
            free(real);
            free(imag);
        };
    };

    // filter high values (when the thread stops in the middle of processing for example) and print
    static const char *kind[3] = { "complex", "real   ", "polar  " };
    for (int logSize = 5; logSize < 14; logSize++) {
        for (int crp = 0; crp < 3; crp++) {
            uint64_t values[4] = { 0, 0, 0, 0 };
            for (int m = 0; m < 4; m++) {
                uint64_t *dataset = &measurements[logSize][crp][m][0];

                uint64_t smallest = UINT64_MAX;
                for (int n = 0; n < 10; n++) if (dataset[n] < smallest) smallest = dataset[n];

                int goodValues = 0;
                uint64_t sum = 0, max = smallest * 2;
                for (int n = 0; n < 10; n++) if ((dataset[n] >= smallest) && (dataset[n] <= max)) {
                    goodValues++;
                    sum += dataset[n];
                };

                values[m] = sum / goodValues;
            };

            if ((crp == 0) && (logSize > 12)) continue;
            printf("%s | %4i || %18llu | %18llu || %18llu | %18llu | %5.2fx\n", kind[crp], 1 << logSize, values[0], values[1], values[2], values[3], float(values[0] + values[1]) / float(values[2] + values[3]));
        };
        printf("\n");
        vDSP_destroy_fftsetup(setups[logSize]);
    };
};

#else
static void SuperpoweredFFTTest() {
    printf("Please run this test on a mobile device with an ARM processor.");
};
#endif

