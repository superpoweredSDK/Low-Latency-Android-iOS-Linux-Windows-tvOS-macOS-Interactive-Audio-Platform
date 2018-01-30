#include "Superpowered3BandEQVST.hpp"
#include "SuperpoweredSimple.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if _DEBUG
#define CONFIGURATION "Debug"
#else
#define CONFIGURATION "Release"
#endif
#if _M_ARM
#define PLATFORM "ARM"
#elif _M_X64
#define PLATFORM "x64"
#else
#define PLATFORM "x86"
#endif
#pragma comment(lib, "..\\..\\Superpowered\\Windows\\SuperpoweredWin120_" CONFIGURATION "_MT_" PLATFORM ".lib")

#define NUMCHANNELS 2

AudioEffect *createEffectInstance(audioMasterCallback audioMaster) {
    return new Superpowered3BandEQVST(audioMaster);
}

Superpowered3BandEQVST::Superpowered3BandEQVST(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, 1, 3) {
    setUniqueID(CCONST('S', 'P', 'E', 'Q'));
    setNumInputs(NUMCHANNELS);
    setNumOutputs(NUMCHANNELS);
    canProcessReplacing();

    stereoBuffer = (float *)malloc(getBlockSize() * sizeof(float) * NUMCHANNELS);

    eq = new Superpowered3BandEQ((unsigned int)getSampleRate());
    eq->enable(true);
}

Superpowered3BandEQVST::~Superpowered3BandEQVST() {
    delete eq;
    free(stereoBuffer);
}

void Superpowered3BandEQVST::setBlockSize(VstInt32 blockSize) {
	float *newBuffer = (float *)realloc(stereoBuffer, blockSize * sizeof(float) * NUMCHANNELS);
	if (newBuffer) stereoBuffer = newBuffer;
}

void Superpowered3BandEQVST::setParameter(VstInt32 index, float value) {
	if ((index >= 0) && (index < 3)) {
		if (value == 0.5f) value = 1.0f;
		else if (value > 0.5f) value *= 2.0f;
		else value = 1.0f + (value - 0.5f) * 2.0f;
		eq->bands[index] = value;
	}
}

float Superpowered3BandEQVST::getParameter(VstInt32 index) {
    return ((index >= 0) && (index < 3)) ? eq->bands[index] * 0.5f : 0;
}

void Superpowered3BandEQVST::getParameterName(VstInt32 index, char *text) {
    switch (index) {
        case 0: strcpy(text, "Low"); break;
        case 1: strcpy(text, "Mid"); break;
        case 2: strcpy(text, "High"); break;
        default: text[0] = 0;
    }
}

void Superpowered3BandEQVST::getParameterDisplay(VstInt32 index, char *text) {
	if ((index >= 0) && (index < 3)) {
		if (eq->bands[index] < 0.01f) _snprintf(text, 16, "kill");
		else _snprintf(text, 16, "%.2f db", 20.0f * log10f(eq->bands[index]));
	} else text[0] = 0;
}

void Superpowered3BandEQVST::getParameterLabel(VstInt32 index, char *text) {
    text[0] = 0;
}

bool Superpowered3BandEQVST::getEffectName(char *text) {
    strcpy(text, "Superpowered 3 Band EQ");
    return true;
}

bool Superpowered3BandEQVST::getProductString(char *text) {
    strcpy(text, "Superpowered 3 Band EQ");
    return true;
}

bool Superpowered3BandEQVST::getVendorString(char *text) {
    strcpy(text, "Superpowered Inc.");
    return true;
}

void Superpowered3BandEQVST::setSampleRate(float sampleRate) {
    this->sampleRate = sampleRate;
    eq->setSamplerate((unsigned int)sampleRate);
}

void Superpowered3BandEQVST::processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames) {
	SuperpoweredInterleave(inputs[0], inputs[1], stereoBuffer, sampleFrames);
	eq->process(stereoBuffer, stereoBuffer, sampleFrames);
	SuperpoweredDeInterleave(stereoBuffer, outputs[0], outputs[1], sampleFrames);
}
