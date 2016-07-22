#include "latencyMeasurer.h"
#include <math.h>
#include <stdlib.h>

/*
 Cross-platform class measuring round-trip audio latency.
 It runs on both iOS and Android, so the same code performs the measurement on every mobile device.
 How one measurement step works:

 - Listen and measure the average loudness of the environment for 1 second.
 - Create a threshold value 24 decibels higher than the average loudness.
 - Begin playing a 1000 Hz sine wave and start counting the samples elapsed.
 - Stop counting and playing if the input's loudness is higher than the threshold, as the output wave is coming back (probably).
 - Divide the the elapsed samples with the sample rate to get the round-trip audio latency value in seconds.
 - We expect the threshold exceeded within 1 second. If it did not, then fail with error. Usually happens when the environment is too noisy (loud).
 
 How the measurement process works:
 
 - Perform 10 measurement steps.
 - Repeat every step until it returns without an error.
 - Store the results in an array of 10 floats.
 - After each step, check the minimum and maximum values. 
 - If the maximum is higher than the minimum's double, stop the measurement process with an error. It indicates an unknown error, perhaps an unwanted noise happened. Double jitter (dispersion) is too high, an audio system can not be so bad.
*/

// Returns with the absolute sum of the audio.
static int sumAudio(short int *audio, int numberOfSamples) {
    int sum = 0;
    while (numberOfSamples) {
        numberOfSamples--;
        sum += abs(audio[0]) + abs(audio[1]);
        audio += 2;
    };
    return sum;
}

latencyMeasurer::latencyMeasurer() : measurementState(idle), nextMeasurementState(idle), samplesElapsed(0), sineWave(0), sum(0), threshold(0), state(0), samplerate(0), latencyMs(0), buffersize(0) {
}

void latencyMeasurer::toggle() {
    if ((state == -1) || ((state > 0) && (state < 11))) { // stop
        state = 0;
        nextMeasurementState = idle;
    } else { // start
        state = 1;
        samplerate = latencyMs = buffersize = 0;
        nextMeasurementState = measure_average_loudness_for_1_sec;
    };
}

void latencyMeasurer::togglePassThrough() {
    if (state != -1) {
        state = -1;
        nextMeasurementState = passthrough;
    } else {
        state = 0;
        nextMeasurementState = idle;
    };
}

void latencyMeasurer::processInput(short int *audio, int _samplerate, int numberOfSamples) {
    rampdec = -1.0f;
    samplerate = _samplerate;
    buffersize = numberOfSamples;

    if (nextMeasurementState != measurementState) {
        if (nextMeasurementState == measure_average_loudness_for_1_sec) samplesElapsed = 0;
        measurementState = nextMeasurementState;
    };

    switch (measurementState) {
        // Measuring average loudness for 1 second.
        case measure_average_loudness_for_1_sec:
            sum += sumAudio(audio, numberOfSamples);
            samplesElapsed += numberOfSamples;

            if (samplesElapsed >= samplerate) { // 1 second elapsed, set up the next step.
                // Look for an audio energy rise of 24 decibel.
                float averageAudioValue = (float(sum) / float(samplesElapsed >> 1)) / 32767.0f;
                float referenceDecibel = 20.0f * log10f(averageAudioValue) + 24.0f;
                threshold = (short int)(powf(10.0f, referenceDecibel / 20.0f) * 32767.0f);

                measurementState = nextMeasurementState = playing_and_listening;
                sineWave = 0;
                samplesElapsed = 0;
                sum = 0;
            };
            break;

        // Playing sine wave and listening if it comes back.
        case playing_and_listening: {
            int averageInputValue = sumAudio(audio, numberOfSamples) / numberOfSamples;
            rampdec = 0.0f;

            if (averageInputValue > threshold) { // The signal is above the threshold, so our sine wave comes back on the input.
                int n = 0;
                short int *input = audio;
                while (n < numberOfSamples) { // Check the location when it became loud enough.
                    if (*input++ > threshold) break;
                    if (*input++ > threshold) break;
                    n++;
                };
                samplesElapsed += n; // Now we know the total round trip latency.

                if (samplesElapsed > numberOfSamples) { // Expect at least 1 buffer of round-trip latency.
                    roundTripLatencyMs[state - 1] = float(samplesElapsed * 1000) / float(samplerate);

                    float sum = 0, max = 0, min = 100000.0f;
                    for (n = 0; n < state; n++) {
                        if (roundTripLatencyMs[n] > max) max = roundTripLatencyMs[n];
                        if (roundTripLatencyMs[n] < min) min = roundTripLatencyMs[n];
                        sum += roundTripLatencyMs[n];
                    };

                    if (max / min > 2.0f) { // Dispersion error.
                        latencyMs = 0;
                        state = 10;
                        measurementState = nextMeasurementState = idle;
                    } else if (state == 10) { // Final result.
                        latencyMs = int(sum * 0.1f);
                        measurementState = nextMeasurementState = idle;
                    } else { // Next step.
                        latencyMs = (int)roundTripLatencyMs[state - 1];
                        measurementState = nextMeasurementState = waiting;
                    };

                    state++;
                } else measurementState = nextMeasurementState = waiting; // Happens when an early noise comes in.

                rampdec = 1.0f / float(numberOfSamples);
            } else { // Still listening.
                samplesElapsed += numberOfSamples;

                // Do not listen to more than a second, let's start over. Maybe the environment's noise is too high.
                if (samplesElapsed > samplerate) {
                    rampdec = 1.0f / float(numberOfSamples);
                    measurementState = nextMeasurementState = waiting;
                    latencyMs = -1;
                };
            };
        }; break;

        case passthrough:
        case idle: break;

        default: // Waiting 1 second.
            samplesElapsed += numberOfSamples;

            if (samplesElapsed > samplerate) { // 1 second elapsed, start over.
                samplesElapsed = 0;
                measurementState = nextMeasurementState = measure_average_loudness_for_1_sec;
            };
    };
}

void latencyMeasurer::processOutput(short int *audio) {
    if (measurementState == passthrough) return;

    if (rampdec < 0.0f) memset(audio, 0, (size_t)buffersize << 2); // Output silence.
    else { // Output sine wave.
        float ramp = 1.0f, mul = (2.0f * float(M_PI) * 1000.0f) / float(samplerate); // 1000 Hz
        int n = buffersize;
        while (n) {
            n--;
            audio[0] = audio[1] = (short int)(sinf(mul * sineWave) * ramp * 32767.0f);
            ramp -= rampdec;
            sineWave += 1.0f;
            audio += 2;
        };
    };
}
