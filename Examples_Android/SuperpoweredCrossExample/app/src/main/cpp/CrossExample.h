#ifndef Header_CrossExample
#define Header_CrossExample

#include <math.h>
#include <pthread.h>
#include <SuperpoweredAdvancedAudioPlayer.h>
#include <SuperpoweredFilter.h>
#include <SuperpoweredRoll.h>
#include <SuperpoweredFlanger.h>
#include <OpenSource/SuperpoweredAndroidAudioIO.h>

#define HEADROOM_DECIBEL 3.0f
static const float headroom = powf(10.0f, -HEADROOM_DECIBEL * 0.05f);

class CrossExample {
public:

	CrossExample(unsigned int samplerate, unsigned int buffersize, const char *path, int fileAoffset, int fileAlength, int fileBoffset, int fileBlength);
	~CrossExample();

	bool process(short int *output, unsigned int numberOfSamples, unsigned int samplerate);
	void onPlayPause(bool play);
	void onCrossfader(int value);
	void onFxSelect(int value);
	void onFxOff();
	void onFxValue(int value);

private:
    SuperpoweredAndroidAudioIO *outputIO;
    Superpowered::AdvancedAudioPlayer *playerA, *playerB;
    Superpowered::Roll *roll;
    Superpowered::Filter *filter;
    Superpowered::Flanger *flanger;
	float crossFaderPosition, volA, volB;
	unsigned int activeFx, numPlayersLoaded;
};

#endif
