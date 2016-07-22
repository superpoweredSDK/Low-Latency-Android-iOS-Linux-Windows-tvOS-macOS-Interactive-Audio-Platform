#include <jni.h>
#include <math.h>
#include <AndroidIO/SuperpoweredUSBAudio.h>
#include <malloc.h>
#include <pthread.h>

// Called when the application is initialized. You can initialize SuperpoweredUSBSystem at any time btw.
// Although this function is marked __unused, it's due Android Studio's annoying warning only. It's definitely used.
__unused jint JNI_OnLoad(JavaVM * __unused vm, void * __unused reserved) {
    SuperpoweredUSBSystem::initialize(NULL, NULL, NULL, NULL, NULL);
    return JNI_VERSION_1_6;
}

// Called when the application is closed. You can destroy SuperpoweredUSBSystem at any time btw.
// Although this function is marked __unused, it's due Android Studio's annoying warning only. It's definitely used.
__unused void JNI_OnUnload(JavaVM * __unused vm, void * __unused reserved) {
    SuperpoweredUSBSystem::destroy();
}


typedef struct sineWaveOutput { // A helper structure for sine wave output.
    float mul;
    unsigned int step;
};

// This is called periodically for audio I/O. Audio is always 32-bit floating point, regardless of the bit depth preference.
static bool audioProcessing(void *clientdata, int __unused deviceID, float *audioIO, int numberOfSamples, int samplerate, int __unused numInputChannels, int numOutputChannels) {
    if (!audioIO) { // If audioIO is NULL, then it's the very last call, IO is closing.
        free(clientdata);
        return true;
    }
    sineWaveOutput *swo = (sineWaveOutput *)clientdata;
    if (swo->mul == 0.0f) swo->mul = (2.0f * float(M_PI) * 300.0f) / float(samplerate);

    for (int n = 0; n < numberOfSamples; n++) {
        float v = sinf(swo->step++ * swo->mul) * 0.5f;
        for (int c = 0; c < numOutputChannels; c++) *audioIO++ = v; // Output sine wave on all output channels.
    }

    return true; // Return false for silence, true if we put audio output into audioIO.
}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int latestMidiCommand = -1;
static int latestMidiChannel = 0;
static int latestMidiNumber = 0;
static int latestMidiValue = 0;

// This is called when some MIDI data is coming in. We are doing some primitive MIDI data processing here.
static void onMidiReceived(void * __unused clientdata, int __unused deviceID, unsigned char *data, int bytes) {
    while (bytes > 0) {
        if (*data > 127) {
            int command = *data >> 4;
            switch (command) {
                case 8: // note off
                case 9: // note on
                case 11: // control change
                    pthread_mutex_lock(&mutex);
                    latestMidiCommand = command;
                    latestMidiChannel = *data++ & 15;
                    latestMidiNumber = *data++;
                    latestMidiValue = *data++;
                    pthread_mutex_unlock(&mutex);
                    bytes -= 3;
                    break;
                default:
                    data++;
                    bytes--;
            }
        } else {
            data++;
            bytes--;
        }
    }
}


// Beautifying the ugly Java-C++ bridge (JNI) with these macros.
#define PID com_superpowered_simpleusb_SuperpoweredUSBAudio // Java package name and class name. Don't forget to update when you copy this code.
#define MAKE_JNI_FUNCTION(r, n, p) extern "C" JNIEXPORT r JNICALL Java_ ## p ## _ ## n
#define JNI(r, n, p) MAKE_JNI_FUNCTION(r, n, p)

// This is called by the SuperpoweredUSBAudio Java object when a USB device is connected.
JNI(jint, onConnect, PID)(JNIEnv *env, jobject __unused obj, jint deviceID, jint fd, jbyteArray rawDescriptor) {
    jbyte *rd = env->GetByteArrayElements(rawDescriptor, NULL);
    int r = SuperpoweredUSBSystem::onConnect(deviceID, fd, (unsigned char *)rd, env->GetArrayLength(rawDescriptor));
    env->ReleaseByteArrayElements(rawDescriptor, rd, JNI_ABORT);

    // r is 0 if SuperpoweredUSBSystem can't do anything with the connected device.
    // r & 2 is true if the device has MIDI. Start receiving events.
    if (r & 2) SuperpoweredUSBMIDI::startIO(deviceID, NULL, onMidiReceived);

    // r & 1 is true if the device has audio. Start output.
    if (r & 1) {
        sineWaveOutput *swo = (sineWaveOutput *)malloc(sizeof(sineWaveOutput));
        if (swo) {
            swo->mul = 0.0f;
            swo->step = 0;
            // Our preferred settings: 44100 Hz, 16 bits, 0 input channels, 256 output channels, low latency.
            // Superpowered will set up the audio device as close as it can to these.
            SuperpoweredUSBAudio::easyIO(deviceID, 44100, 16, 0, 256, SuperpoweredUSBLatency_Low, swo, audioProcessing);
        }
    }
    return r;
}

// This is called by the SuperpoweredUSBAudio Java object when a USB device is disconnected.
JNI(void, onDisconnect, PID)(JNIEnv * __unused env, jobject __unused obj, jint deviceID) {
    SuperpoweredUSBSystem::onDisconnect(deviceID);
}

#undef PID
#define PID com_superpowered_simpleusb_MainActivity

// This is called by the MainActivity Java object periodically.
JNI(jintArray, getLatestMidiMessage, PID)(JNIEnv *env, jobject __unused obj) {
    jintArray ints = env->NewIntArray(4);
    jint *i = env->GetIntArrayElements(ints, NULL);
    pthread_mutex_lock(&mutex);
    i[0] = latestMidiCommand;
    i[1] = latestMidiChannel;
    i[2] = latestMidiNumber;
    i[3] = latestMidiValue;
    pthread_mutex_unlock(&mutex);
    env->ReleaseIntArrayElements(ints, i, NULL);
    return ints;
}
