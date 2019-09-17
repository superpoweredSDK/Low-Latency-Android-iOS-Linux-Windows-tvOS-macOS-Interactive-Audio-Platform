#include <SuperpoweredFilter.h>
#include <math.h>
#include "SuperpoweredNBandEQ.h"

#if _WIN32
#include "Windows.h"
#include <atomic>
#define ATOMICZERO(var) InterlockedAnd(&var, 0)
#else
#define ATOMICZERO(var) __sync_fetch_and_and(&var, 0)
#endif

typedef struct nbeqInternals {
    Superpowered::Filter **filters;
    unsigned int numFilters;
} nbeqInternals;

SuperpoweredNBandEQ::SuperpoweredNBandEQ(unsigned int _samplerate, float *frequencies) {
    samplerate = _samplerate;
    enabled = false;
    internals = new nbeqInternals;
    internals->numFilters = 0;

    // Count the number of filters.
    for (int numFilters = 0; numFilters < 1024; numFilters++) {
        if (frequencies[numFilters] <= 0.0f) {
            internals->numFilters = numFilters;
            break;
        }
    }

    // Create the array of pointers to the filters.
    internals->filters = new Superpowered::Filter*[internals->numFilters];

    // Create the filters.
    float widthOctave;
    for (unsigned int n = 0; n < internals->numFilters; n++) {
        widthOctave = (frequencies[n + 1] > frequencies[n]) ? logf(frequencies[n + 1] / frequencies[n]) : logf(20000.0f / frequencies[n]);
        // log2f is broken in Android, so we use log(x) / log(2)
        static const float log2fdiv = 1.0f / logf(2.0f);
        widthOctave *= log2fdiv;

        internals->filters[n] = new Superpowered::Filter(Superpowered::Parametric, samplerate);
        internals->filters[n]->frequency = frequencies[n];
        internals->filters[n]->octave = widthOctave;
        internals->filters[n]->decibel = 0.0f;
    }
}

SuperpoweredNBandEQ::~SuperpoweredNBandEQ() {
    for (unsigned int n = 0; n < internals->numFilters; n++) delete internals->filters[n];
    delete[] internals->filters;
    delete internals;
}

void SuperpoweredNBandEQ::setGainDb(unsigned int index, float gainDecibels) {
    if (index < internals->numFilters) internals->filters[index]->decibel = gainDecibels;
}

float SuperpoweredNBandEQ::getBandDb(unsigned int index) {
    return (index < internals->numFilters) ? internals->filters[index]->decibel : 0;
}

bool SuperpoweredNBandEQ::process(float *input, float *output, unsigned int numberOfFrames) {
    if (!input || !output || !numberOfFrames || !internals->numFilters) return false; // Some safety.

    // Sample rate changed?
    if (internals->filters[0]->samplerate != samplerate) {
        unsigned int volatile sr = samplerate; // Read the samplerate once, non-optimized directly to stack
        for (unsigned int n = 0; n < internals->numFilters; n++) internals->filters[n]->samplerate = sr;
    }

    // Enabled changed?
    if (internals->filters[0]->enabled != enabled) {
        bool volatile e = enabled; // Read enabled once, non-optimized directly to stack.
        for (unsigned int n = 0; n < internals->numFilters; n++) internals->filters[n]->enabled = e;
    }

    // Process the first filter, input -> output.
    bool hasAudioInOutput = internals->filters[0]->process(input, output, numberOfFrames);

    // Process all remaining filters, in-place processing output -> output.
    bool filterHasAudio;
    for (unsigned int n = 1; n < internals->numFilters; n++) {
        filterHasAudio = internals->filters[n]->process(output, output, numberOfFrames);
        hasAudioInOutput |= filterHasAudio;
    }

    return hasAudioInOutput;
}
