#include <jni.h>
#include <malloc.h>
#include <math.h>
#include <AndroidIO/SuperpoweredUSBAudio.h>
#include "latencyMeasurer.h"

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


// Beautifying the ugly Java-C++ bridge (JNI) with these macros.
#define PID com_superpowered_complexusb_SuperpoweredUSBAudio // Java package name and class name. Don't forget to update when you copy this code.
#define MAKE_JNI_FUNCTION(r, n, p) extern "C" JNIEXPORT r JNICALL Java_ ## p ## _ ## n
#define JNI(r, n, p) MAKE_JNI_FUNCTION(r, n, p)

// This is called by the SuperpoweredUSBAudio Java object when a USB device is connected.
JNI(jint, onConnect, PID)(JNIEnv *env, jobject __unused obj, jint deviceID, jint fd, jbyteArray rawDescriptor) {
    jbyte *rd = env->GetByteArrayElements(rawDescriptor, NULL);
    int r = SuperpoweredUSBSystem::onConnect(deviceID, fd, (unsigned char *)rd, env->GetArrayLength(rawDescriptor));
    env->ReleaseByteArrayElements(rawDescriptor, rd, JNI_ABORT);
    return r;
}

// This is called by the SuperpoweredUSBAudio Java object when a USB device is disconnected.
JNI(void, onDisconnect, PID)(JNIEnv * __unused env, jobject __unused obj, jint deviceID) {
    SuperpoweredUSBSystem::onDisconnect(deviceID);
}

#undef PID
#define PID com_superpowered_complexusb_MainActivity // Java package name and class name. Don't forget to update when you copy this code.

// Returns with a string array: { manufacturer, product, info }
JNI(jobjectArray, getUSBAudioDeviceInfo, PID)(JNIEnv *env, jobject __unused obj, jint deviceID) {
    jobjectArray names = env->NewObjectArray(3, env->FindClass("java/lang/String"), NULL);
    const char *manufacurer, *product, *info;
    SuperpoweredUSBAudio::getInfo(deviceID, &manufacurer, &product, &info);
    env->SetObjectArrayElement(names, 0, env->NewStringUTF(manufacurer));
    env->SetObjectArrayElement(names, 1, env->NewStringUTF(product));
    env->SetObjectArrayElement(names, 2, env->NewStringUTF(info));
    return names;
}

// Superpowered simplifies the unlimited possibilities of the USB Audio Class to a hierarchy with 3 levels:
// 1. configuration
// 2. inputs and outputs
// 3. paths

// A USB audio device may have multiple configurations. This returns with their names in order of their indexes (eg. 0, 1, 2, ...).
JNI(jobjectArray, getConfigurationInfo, PID)(JNIEnv *env, jobject __unused obj, jint deviceID) {
    int numConfigurations; char **configurationNames;
    SuperpoweredUSBAudio::getConfigurationInfo(deviceID, &numConfigurations, &configurationNames);

    jobjectArray names = env->NewObjectArray(numConfigurations, env->FindClass("java/lang/String"), NULL);
    for (int n = 0; n < numConfigurations; n++) {
        env->SetObjectArrayElement(names, n, env->NewStringUTF(configurationNames[n]));
        free(configurationNames[n]);
    }

    free(configurationNames);
    return names;
}

// Set the configuration of the USB audio device.
JNI(void, setConfiguration, PID)(JNIEnv * __unused env, jobject __unused obj, jint deviceID, jint configurationIndex) {
    SuperpoweredUSBAudio::setConfiguration(deviceID, configurationIndex);
}

// Get the names of the available inputs. An input has a unique bit depth, channel count and sample rate combination.
JNI(jobjectArray, getInputs, PID)(JNIEnv *env, jobject __unused obj, jint deviceID) {
    SuperpoweredUSBAudioIOInfo *infos;
    int num = SuperpoweredUSBAudio::getInputs(deviceID, &infos);
    jobjectArray names = env->NewObjectArray(num, env->FindClass("java/lang/String"), NULL);
    for (int n = 0; n < num; n++) env->SetObjectArrayElement(names, n, env->NewStringUTF(infos[n].name));
    free(infos);
    return names;
}

// Get the names of the available outputs. An output has a unique bit depth, channel count and sample rate combination.
JNI(jobjectArray, getOutputs, PID)(JNIEnv *env, jobject __unused obj, jint deviceID) {
    SuperpoweredUSBAudioIOInfo *infos;
    int num = SuperpoweredUSBAudio::getOutputs(deviceID, &infos);
    jobjectArray names = env->NewObjectArray(num, env->FindClass("java/lang/String"), NULL);
    for (int n = 0; n < num; n++) env->SetObjectArrayElement(names, n, env->NewStringUTF(infos[n].name));
    free(infos);
    return names;
}

// Every input and output combination may have multiple paths (such as "speaker" or "headphones").
// The following 4 functions form an ugly way to return these.
static int *paths[3];
static int nums[3];
static char **pathNames[3];

// Check MainActivity.java how to handle this.
JNI(void, getIOOptions, PID)(JNIEnv * __unused env, jobject __unused obj, jint deviceID, jint inputIOIndex, jint outputIOIndex) {
    nums[0] = nums[1] = nums[2] = 0;
    if (outputIOIndex >= 0) SuperpoweredUSBAudio::getIOOptions(deviceID, false, outputIOIndex, &paths[0], &pathNames[0], &nums[0], NULL, NULL, NULL);
    if (inputIOIndex >= 0) SuperpoweredUSBAudio::getIOOptions(deviceID, true, inputIOIndex, &paths[1], &pathNames[1], &nums[1], &paths[2], &pathNames[2], &nums[2]);
}

// Check MainActivity.java how to handle this.
JNI(jintArray, getIOOptionsInt, PID)(JNIEnv *env, jobject __unused obj, jint outputInputThru) {
    jintArray ints = env->NewIntArray(nums[outputInputThru]);
    jint *i = env->GetIntArrayElements(ints, NULL);
    for (int n = 0; n < nums[outputInputThru]; n++) i[n] = paths[outputInputThru][n];
    env->ReleaseIntArrayElements(ints, i, NULL);
    return ints;
}

// Check MainActivity.java how to handle this.
JNI(jobjectArray, getIOOptionsString, PID)(JNIEnv *env, jobject __unused obj, jint outputInputThru) {
    jobjectArray names = env->NewObjectArray(nums[outputInputThru], env->FindClass("java/lang/String"), NULL);
    for (int n = 0; n < nums[outputInputThru]; n++) env->SetObjectArrayElement(names, n, env->NewStringUTF(pathNames[outputInputThru][n]));
    return names;
}

// Check MainActivity.java how to handle this.
JNI(void, getIOOptionsEnd, PID)(JNIEnv * __unused env, jobject __unused obj) {
    for (int n = 0; n < 3; n++) if (nums[n]) {
        free(paths[n]);
        for (int i = 0; i < nums[n]; i++) free(pathNames[n][i]);
        free(pathNames[n]);
    }
}

// Get a path's properties.
JNI(jfloatArray, getPathInfo, PID)(JNIEnv *env, jobject __unused obj, jint deviceID, jint pathIndex) {
    int numFeatures;
    float *minVolumes, *maxVolumes, *curVolumes;
    char *mutes;
    SuperpoweredUSBAudio::getPathInfo(deviceID, pathIndex, &numFeatures, &minVolumes, &maxVolumes, &curVolumes, &mutes);

    jfloatArray floats = env->NewFloatArray(numFeatures * 4);
    jfloat *f = env->GetFloatArrayElements(floats, NULL), *ff = f;

    for (int n = 0; n < numFeatures; n++) {
        *ff++ = minVolumes[n];
        *ff++ = maxVolumes[n];
        *ff++ = curVolumes[n];
        *ff++ = mutes[n];
    }

    env->ReleaseFloatArrayElements(floats, f, NULL);
    free(minVolumes);
    free(maxVolumes);
    free(curVolumes);
    free(mutes);
    return floats;
}

JNI(jfloat, setVolume, PID)(JNIEnv * __unused env, jobject __unused obj, jint deviceID, jint pathIndex, jint channel, jfloat db) {
    return SuperpoweredUSBAudio::setVolume(deviceID, pathIndex, channel, db);
}

JNI(jboolean, setMute, PID)(JNIEnv * __unused env, jobject __unused obj, jint deviceID, jint pathIndex, jint channel, jboolean mute) {
    return (jboolean)SuperpoweredUSBAudio::setMute(deviceID, pathIndex, channel, mute);
}


typedef struct audioJob { // A helper structure for audio processing.
    char kind;
    float mul;
    unsigned int step;
    latencyMeasurer *measurer;
    short int *intBuf;
};

static int lastDeviceID = 0;
static int latencyMs = -1;

// This is called periodically for audio I/O. Audio is always 32-bit floating point, regardless of the bit depth preference.
static bool audioProcessing(void * clientdata, int __unused deviceID, float *audioIO, int numberOfSamples, int samplerate, int numInputChannels, int numOutputChannels) {
    audioJob *job = (audioJob *)clientdata;
    if (!audioIO) { // If audioIO is NULL, then it's the very last call, IO is closing.
        if (job->measurer) delete job->measurer;
        if (job->intBuf) free(job->intBuf);
        free(job);
        return true;
    }

    switch (job->kind) {
        case 1: { // Output sine wave.
            if (job->mul == 0.0f) job->mul = (2.0f * float(M_PI) * 300.0f) / float(samplerate);
            for (int n = 0; n < numberOfSamples; n++) {
                float v = sinf(job->step++ * job->mul) * 0.5f;
                for (int c = 0; c < numOutputChannels; c++) *audioIO++ = v; // Output sine wave on all output channels.
            }
        } break;
        case 2: { // Latency measurement.
            float *audioFloat = audioIO;
            short int *audioInt = job->intBuf;
            if (numInputChannels >= 2) { // Read the first 2 input channels only.
                for (int n = 0; n < numberOfSamples; n++) {
                    *audioInt++ = (short int) (audioFloat[0] * 32767.0f);
                    *audioInt++ = (short int) (audioFloat[1] * 32767.0f);
                    audioFloat += numInputChannels;
                }
            } else { // Make stereo from mono input.
                for (int n = 0; n < numberOfSamples; n++) {
                    short int s = (short int) (*audioFloat++ * 32767.0f);
                    *audioInt++ = s;
                    *audioInt++ = s;
                }
            }

            job->measurer->processInput(job->intBuf, samplerate, numberOfSamples);
            job->measurer->processOutput(job->intBuf);

            audioFloat = audioIO;
            audioInt = job->intBuf;
            static const float sIntToFloat = 1.0f / 32768.0f;
            for (int n = 0; n < numberOfSamples; n++) { // Output on all output channels.
                float f = float(*audioInt++) * sIntToFloat;
                for (int c = 0; c < numOutputChannels; c++) *audioFloat++ = f;
                audioInt++;
            }
        } break;
        default:; // Pass through.
    }
    return true;
}

JNI(void, startIO, PID)(JNIEnv * __unused env, jobject __unused obj, jint deviceID, jint inputIOIndex, jint outputIOIndex, jint latency, jboolean latencyMeasurement) {
    SuperpoweredUSBAudio::stopIO(lastDeviceID);
    latencyMs = -1;
    lastDeviceID = deviceID;

    audioJob *job = (audioJob *)malloc(sizeof(audioJob));
    job->mul = 0.0f;
    job->step = 0;
    job->measurer = NULL;
    job->intBuf = NULL;

    bool hasInput = inputIOIndex >= 0, hasOutput = outputIOIndex >= 0;
    if (hasInput && hasOutput) job->kind = (char)(latencyMeasurement ? 2 : 0); else job->kind = 1;
    if (job->kind == 2) {
        job->measurer = new latencyMeasurer();
        job->measurer->toggle();
        job->intBuf = (short int *)malloc(sizeof(short int) * 2 * 1024);
    }

    SuperpoweredUSBAudioLatency sl;
    switch (latency) {
        case 1: sl = SuperpoweredUSBLatency_Mid; break;
        case 2: sl = SuperpoweredUSBLatency_High; break;
        default: sl = SuperpoweredUSBLatency_Low;
    }
    SuperpoweredUSBAudio::startIO(deviceID, inputIOIndex, outputIOIndex, sl, job, audioProcessing);
}

JNI(void, stopIO, PID)(JNIEnv * __unused env, jobject __unused obj) {
    SuperpoweredUSBAudio::stopIO(lastDeviceID);
    latencyMs = -1;
}

JNI(jint, getLatencyMs, PID)(JNIEnv * __unused env, jobject __unused obj) {
    return latencyMs;
}

