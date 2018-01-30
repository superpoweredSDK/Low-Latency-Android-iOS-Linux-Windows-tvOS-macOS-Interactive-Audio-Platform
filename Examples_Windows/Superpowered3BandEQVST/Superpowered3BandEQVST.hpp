#ifndef Superpowered3BandEQVST_hpp
#define Superpowered3BandEQVST_hpp

#include "audioeffectx.h"
#include "Superpowered3BandEQ.h"

class Superpowered3BandEQVST: public AudioEffectX {
public:
    Superpowered3BandEQVST(audioMasterCallback audioMaster);
    ~Superpowered3BandEQVST();

    void processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames);
    void setParameter(VstInt32 index, float value);
    float getParameter(VstInt32 index);
    void getParameterName(VstInt32 index, char *text);
    void getParameterDisplay(VstInt32 index, char *text);
    void getParameterLabel(VstInt32 index, char *text);
    bool getEffectName(char *text);
    bool getProductString(char *text);
    bool getVendorString(char *text);
	void setBlockSize(VstInt32 blockSize);
    void setSampleRate(float sampleRate);

private:
    float *stereoBuffer;
    Superpowered3BandEQ *eq;
};

#endif
