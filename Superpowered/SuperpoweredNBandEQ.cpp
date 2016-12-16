#include <SuperpoweredFilter.h>
#include <math.h>
#include "SuperpoweredNBandEQ.h"

typedef struct nbeqInternals {
    SuperpoweredFilter **filters;
    unsigned int newSamplerate;
    int numFilters;
    bool lastEnabled;
} nbeqInternals;

SuperpoweredNBandEQ::SuperpoweredNBandEQ(unsigned int samplerate, float *frequencies) {
    internals = new nbeqInternals;
    enabled = internals->lastEnabled = false;
    internals->newSamplerate = 0;

    // Count the number of filters.
    for (int numFilters = 0; numFilters < 1024; numFilters++) {
        if (frequencies[numFilters] <= 0.0f) {
            internals->numFilters = numFilters;
            break;
        }
    }

    decibels = new float[internals->numFilters];

    // Create the array of pointers to the filters.
    internals->filters = new SuperpoweredFilter*[internals->numFilters];

    // Create the filters.
    for (int n = 0; n < internals->numFilters; n++) {
        decibels[n] = 0.0f;
        float widthOctave = (frequencies[n + 1] > frequencies[n]) ? logf(frequencies[n + 1] / frequencies[n]) : logf(20000.0f / frequencies[n]);
        // log2f is broken in Android, so we use log(x) / log(2)
        static const float log2fdiv = 1.0f / logf(2.0f);
        widthOctave *= log2fdiv;

        internals->filters[n] = new SuperpoweredFilter(SuperpoweredFilter_Parametric, samplerate);
        internals->filters[n]->setParametricParameters(frequencies[n], widthOctave, 0.0f);
    }
}

SuperpoweredNBandEQ::~SuperpoweredNBandEQ() {
    for (int n = 0; n < internals->numFilters; n++) delete internals->filters[n];
    delete[] internals->filters;
    delete internals;
    delete decibels;
}

void SuperpoweredNBandEQ::setSamplerate(unsigned int samplerate) {
    // This method can be called from any thread. Setting the sample rate of all filters must be synchronous, in the audio processing thread.
    internals->newSamplerate = samplerate;
}

void SuperpoweredNBandEQ::enable(bool flag) {
    // This method can be called from any thread. Switching all filters must be synchronous, in the audio processing thread.
    enabled = flag;
}

void SuperpoweredNBandEQ::reset() {
    for (int n = 0; n < internals->numFilters; n++) internals->filters[n]->reset();
}

void SuperpoweredNBandEQ::setBand(unsigned int index, float gainDecibels) {
    if (index < internals->numFilters) {
        decibels[index] = gainDecibels;
        internals->filters[index]->setParametricParameters(internals->filters[index]->frequency, internals->filters[index]->octave, gainDecibels);
    }
}

bool SuperpoweredNBandEQ::process(float *input, float *output, unsigned int numberOfSamples) {
    if (!input || !output || !numberOfSamples || !internals->numFilters) return false; // Some safety.

    // Change the sample rate of all filters at once if needed.
    unsigned int newSamplerate = __sync_fetch_and_and(&internals->newSamplerate, 0);
    if (newSamplerate > 0) {
        for (int n = 0; n < internals->numFilters; n++) internals->filters[n]->setSamplerate(newSamplerate);
    }

    // Switch all filters if needed.
    if (internals->lastEnabled != enabled) {
        internals->lastEnabled = enabled;
        for (int n = 0; n < internals->numFilters; n++) internals->filters[n]->enable(internals->lastEnabled);
    }

    // Process the first filter, input -> output.
    bool hasAudioInOutput = internals->filters[0]->process(input, output, numberOfSamples);

    // Process all remaining filters, in-place processing output -> output.
    for (int n = 1; n < internals->numFilters; n++) {
        bool filterHasAudio = internals->filters[n]->process(output, output, numberOfSamples);
        hasAudioInOutput |= filterHasAudio;
    }

    return hasAudioInOutput;
}
