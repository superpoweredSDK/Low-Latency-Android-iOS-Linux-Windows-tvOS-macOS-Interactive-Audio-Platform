#import "SuperpoweredDeinterleave.h"
#if !defined(__i386__) && !defined(__x86_64__)
#import <arm/arch.h>
#endif

void SuperpoweredDeinterleave(float *input, float *outputLeft, float *outputRight, int frames) {
#ifndef _ARM_ARCH_7
    while (frames) {
        float L = *input++, R = *input++;
        *outputLeft++ = L;
        *outputRight++ = R;
        frames--;
    };
#else
    __asm__ volatile (
                      "lsr %[frames], #2 \n\t"
                      "1: \n\t"
                      "pld [%[input], #96] \n\t"
                      "subs %[frames], #1 \n\t"
                      "vld1.32 { q0-q1 }, [%[input]]! \n\t"
                      "vst4.32 { d0[0], d1[0], d2[0], d3[0] }, [%[outL]]! \n\t"
                      "vst4.32 { d0[1], d1[1], d2[1], d3[1] }, [%[outR]]! \n\t"
                      "it ne \n\t"
                      "bne 1b \n\t"
                      :
                      : [input] "r" (input), [outL] "r" (outputLeft), [outR] "r" (outputRight), [frames] "r" (frames)
                      : "cc", "q0", "q1"
                      );
#endif
}
