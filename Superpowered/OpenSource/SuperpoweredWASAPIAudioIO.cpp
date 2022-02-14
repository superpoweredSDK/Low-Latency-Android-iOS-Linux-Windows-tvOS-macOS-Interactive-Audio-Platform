//#include <pch.h> // Remove this if you're not using precompiled headers.
#include "SuperpoweredWASAPIAudioIO.h"
#include <stdio.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>

#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "winmm.lib")

static void channelConversion(float *input, float *output, int inputNumberOfChannels, int outputNumberOfChannels, int numberOfFrames) {
    if ((inputNumberOfChannels == 1) && (outputNumberOfChannels == 2)) { // Mono to stereo.
        while (numberOfFrames--) {
            *output++ = *input;
            *output++ = *input++;
        }
    } else if ((inputNumberOfChannels == 2) && (outputNumberOfChannels == 1)) { // Stereo to mono.
        while (numberOfFrames--) {
            *output++ = (input[0] + input[1]) * 0.5f;
            input += 2;
        }
    } else if (inputNumberOfChannels < outputNumberOfChannels) { // Copy all input channels, zero the output channels above.
        int zeroBytes = (outputNumberOfChannels - inputNumberOfChannels) * 4;
        while (numberOfFrames--) {
            memcpy(output, input, inputNumberOfChannels * 4);
            memset(output + inputNumberOfChannels, 0, zeroBytes);
            input += inputNumberOfChannels;
            output += outputNumberOfChannels;
        }
    } else { // Copy the channels fit into output.
        while (numberOfFrames--) {
            memcpy(output, input, outputNumberOfChannels * 4);
            input += inputNumberOfChannels;
            output += outputNumberOfChannels;
        }
    }
}

static void convertToFloat(BYTE* buffer, unsigned int numFrames, unsigned int numberOfChannels) {
    static const float mul = 1.0f / 2147483647.0f;
    int* i = (int*)buffer;
    float* f = (float*)buffer;
    unsigned int n = numFrames * numberOfChannels;
    while (n--) *f++ = *i++ * mul;
}

static void convertToPCM(BYTE* buffer, unsigned int numFrames, unsigned int numberOfChannels) {
    int* i = (int*)buffer;
    float* f = (float*)buffer;
    unsigned int n = numFrames * numberOfChannels;
    while (n--) *i++ = (int)(*f++ * 2147483647.0f);
}

// If both audio input and output are enabled, this class handles a non-blocking buffer between input and output.
// It can resample too (in a simple way) if the sample rate of the input is different to the output.
class inputToOutputHandler {
public:
    bool receivedFirstInput;
    
    inputToOutputHandler(int outSamplerate, int outChannels) : outputSamplerate(outSamplerate), inputNumberOfChannels(0), outputNumberOfChannels(outChannels), inputBuffer(NULL), resampleBuffer(NULL), channelConvBuffer(NULL), prev(NULL), slopeCount(0), invSlopeCount(1.0f), afterUnderrun(true), receivedFirstInput(false), writePos(0), readPos(0) {}

    void setNumberOfInputChannels(int inChannels) {
        inputNumberOfChannels = inChannels;
        inputBuffer = (float*)_aligned_malloc(inputNumberOfChannels * 4 * outputSamplerate, 16);
        prev = (float*)_aligned_malloc(inputNumberOfChannels * 4, 16);
        resampleBuffer = (float*)_aligned_malloc(inputNumberOfChannels * 4 * outputSamplerate, 16);
        if (!inputBuffer || !prev || !resampleBuffer) abort();
        memset(prev, 0, inputNumberOfChannels * 4);
        channelConvBuffer = (float *)_aligned_malloc(outputNumberOfChannels * 4 * outputSamplerate, 16);
    }
    
    ~inputToOutputHandler() {
        if (prev) _aligned_free(prev);
        if (inputBuffer) _aligned_free(inputBuffer);
        if (resampleBuffer) _aligned_free(resampleBuffer);
        if (channelConvBuffer) _aligned_free(channelConvBuffer);
    }
    
    void handleInput(float *input, int numFrames, int samplerate) {
        if (!input) return;

        if (samplerate != outputSamplerate) { // Resampling if needed.
            numFrames = resample(input, resampleBuffer, float(samplerate) / float(outputSamplerate), numFrames);
            input = resampleBuffer;
		}
        
        int spaceLeft = outputSamplerate - writePos;
        
        if (spaceLeft < numFrames) { // End of buffer.
            memcpy(inputBuffer + writePos * inputNumberOfChannels, input, spaceLeft * inputNumberOfChannels * 4);
            input += spaceLeft * inputNumberOfChannels;
            numFrames -= spaceLeft;
            writePos = 0;
        }
        
        if (input) memcpy(inputBuffer + writePos * inputNumberOfChannels, input, numFrames * inputNumberOfChannels * 4);
        writePos += numFrames;
        receivedFirstInput = true;
    }
    
    float *handleOutput(int numFrames) {
        int inputFramesAvailable = writePos - readPos;
        if (inputFramesAvailable < 0) inputFramesAvailable += outputSamplerate;
        int minimumInputFramesShouldBe = afterUnderrun ? (numFrames + (numFrames >> 1)) : numFrames;

        if (inputFramesAvailable < minimumInputFramesShouldBe) { // Underrun, not enough audio input frames are available.
            afterUnderrun = true;
            memset(channelConvBuffer, 0, numFrames * 4 * outputNumberOfChannels);
            return channelConvBuffer;
        }
        
        afterUnderrun = false;
        int spaceLeft = outputSamplerate - readPos;
        bool wrapAround = (spaceLeft < numFrames);
        
        if (!wrapAround && (inputNumberOfChannels == outputNumberOfChannels)) {
            float *r = inputBuffer + readPos * inputNumberOfChannels;
            readPos += numFrames;
            return r;
        }
            
        float *output = channelConvBuffer;
        if (wrapAround) {
            if (inputNumberOfChannels != outputNumberOfChannels) channelConversion(inputBuffer + readPos * inputNumberOfChannels, output, inputNumberOfChannels, outputNumberOfChannels, spaceLeft);
            else memcpy(output, inputBuffer + readPos * inputNumberOfChannels, spaceLeft * outputNumberOfChannels * 4);
            output += spaceLeft * outputNumberOfChannels;
            numFrames -= spaceLeft;
            readPos = 0;
        }
            
        if (inputNumberOfChannels != outputNumberOfChannels) channelConversion(inputBuffer + readPos * inputNumberOfChannels, output, inputNumberOfChannels, outputNumberOfChannels, numFrames);
        else memcpy(output, inputBuffer + readPos * inputNumberOfChannels, numFrames * outputNumberOfChannels * 4);
        readPos += numFrames;
        
        return channelConvBuffer;
    }
    
private:
    float *inputBuffer, *resampleBuffer, *channelConvBuffer, *prev;
    float slopeCount, invSlopeCount;
    int outputSamplerate, inputNumberOfChannels, outputNumberOfChannels, readPos, writePos;
    bool afterUnderrun;
    
    int resample(float *input, float *output, float rate, int numFrames) {
        int outFrames = 0;
        
        while (true) {
            while (slopeCount > 1.0f) {
                numFrames--;
                slopeCount -= 1.0f;
                invSlopeCount = 1.0f - slopeCount;
                if (!numFrames) return outFrames;
                
                memcpy(prev, input, inputNumberOfChannels * 4);
                input += inputNumberOfChannels;
            }
            
            for (int ch = 0; ch < inputNumberOfChannels; ch++) *output++ = invSlopeCount * prev[ch] + slopeCount * input[ch];
            memcpy(prev, input, inputNumberOfChannels * 4);
            outFrames++;
            slopeCount += rate;
            invSlopeCount = 1.0f - slopeCount;
        }
    }
};

static const char *state_initializing = "initializing";
static const char *state_stopped = "stopped";
static const char *state_iorunning = "running";
static const char *error_createevent = "CreateEventEx error";
static const char *error_createdeviceenumerator = "CoCreateInstance deviceEnumerator error";
static const char *error_getdevice = "GetDefaultAudioEndpoint error";
static const char *error_activate = "activate error";
static const char *error_setclientproperties = "SetClientProperties error";
static const char *error_getmixformat = "GetMixFormat error";
static const char *error_getframes = "GetSharedModeEnginePeriod error";
static const char *error_getdeviceperiod = "GetDevicePeriod error";
static const char *error_outofmemory = "out of memory";
static const char *error_initstream = "InitializeSharedAudioStream error";
static const char *error_init = "Initialize error";
static const char *error_getbuffersize = "GetBufferSize error";
static const char *error_buffersize16 = "buffer size is below 16 error";
static const char *error_getservice = "GetService error";
static const char *error_seteventhandle = "SetEventHandle error";
static const char *error_start = "start error";
static const char *error_buffer = "AUDCLNT_E_BUFFER_ERROR";
static const char *error_deviceinvalidated = "AUDCLNT_E_DEVICE_INVALIDATED";
static const char *error_isformatsupported = "IsFormatSupported error";
static const char *error_openpropertystore = "OpenPropertyStore error";
static const char *error_getvalue = "GetValue error";
static const char *error_cotaskmemalloc = "CoTaskMemAlloc error";
static const char* error_format = "format is not 32-bit float or 32-bit pcm";

// This class handles audio input OR audio output. Not both, just one of them.
class inputOrOutputHandler {
public:
    const char *state;
    
    inputOrOutputHandler(bool _input, unsigned int preferredBufferSizeMs, int _requiredNumberOfChannels, bool _hasOtherHandler, inputToOutputHandler** _ioHandler, audioProcessingCallback cb, void* cd) : state(state_initializing), channelConvBuffer(NULL), callback(cb), clientdata(cd), ioHandler(_ioHandler), client(NULL), captureClient(NULL), renderClient(NULL), stop(false), input(_input), exclusive(preferredBufferSizeMs == 0), hasOtherHandler(_hasOtherHandler), requiredNumberOfChannels(_requiredNumberOfChannels), startio(false), bufferSize(0), samplerate(48000), numberOfChannels(2), isFloat(true) {
        WAVEFORMATEXTENSIBLE* format = NULL;
        HRESULT hr = S_OK;

        // visit https://hresult.info for a description of HRESULT codes
#define checkReturn(__errorMessage__) if (hr != S_OK) { \
    state = __errorMessage__; \
    char log[256]; \
    _snprintf_s_l(log, 256, 256, "0x%08x %s", NULL, hr, __errorMessage__); \
    OutputDebugStringA(log); \
    if (format) CoTaskMemFree(format); \
    return; \
}

        sampleReadyEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        if (!sampleReadyEvent) hr = E_UNEXPECTED;
        checkReturn(error_createevent);

        IMMDeviceEnumerator* deviceEnumerator = NULL;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
        checkReturn(error_createdeviceenumerator);

        IMMDevice* device;
        hr = deviceEnumerator->GetDefaultAudioEndpoint(_input ? eCapture : eRender, eMultimedia, &device);
        deviceEnumerator->Release();
        checkReturn(error_getdevice);

        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, 0, (void**)&client);
        if (hr != S_OK) device->Release();
        checkReturn(error_activate);
        
        if (exclusive) {
            IPropertyStore *store = NULL;
            hr = device->OpenPropertyStore(STGM_READ, &store);
            device->Release();
            checkReturn(error_openpropertystore);

            PROPVARIANT prop;
            hr = store->GetValue(PKEY_AudioEngine_DeviceFormat, &prop);
            checkReturn(error_getvalue);
            format = (WAVEFORMATEXTENSIBLE *)CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
            if (!format) hr = E_OUTOFMEMORY;
            checkReturn(error_cotaskmemalloc);
            memcpy(format, prop.blob.pBlobData, sizeof(WAVEFORMATEXTENSIBLE));
            
            REFERENCE_TIME defaultDevicePeriod = 0, minimumDevicePeriod = 0;
            hr = client->GetDevicePeriod(&defaultDevicePeriod, &minimumDevicePeriod);
            checkReturn(error_getdeviceperiod);
            
            hr = client->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)format, NULL);
            checkReturn(error_isformatsupported);
            
            hr = client->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST, minimumDevicePeriod, minimumDevicePeriod, (WAVEFORMATEX*)format, NULL);
            checkReturn(error_init);
        } else {
            device->Release();
            hr = client->GetMixFormat((WAVEFORMATEX **)&format);
            checkReturn(error_getmixformat);
            
            UINT32 defaultPeriodFrames, fundamentalPeriodFrames, minPeriodFrames, maxPeriodFrames;
            hr = client->GetSharedModeEnginePeriod((WAVEFORMATEX*)format, &defaultPeriodFrames, &fundamentalPeriodFrames, &minPeriodFrames, &maxPeriodFrames);
            checkReturn(error_getframes);
                        
            UINT32 preferredFrames = (UINT32)(((double)preferredBufferSizeMs / 1000.0) * (double)format->Format.nSamplesPerSec);
            while (minPeriodFrames < preferredFrames) minPeriodFrames += fundamentalPeriodFrames;
            if (minPeriodFrames > maxPeriodFrames) minPeriodFrames = maxPeriodFrames;
            
            hr = client->InitializeSharedAudioStream(AUDCLNT_STREAMFLAGS_EVENTCALLBACK, minPeriodFrames, (WAVEFORMATEX*)format, nullptr);
            checkReturn(error_initstream);
        }

        if (format->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) isFloat = false;
        else if (format->SubFormat != KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
            hr = E_UNEXPECTED;
            checkReturn(error_format);
        }
        samplerate = format->Format.nSamplesPerSec;
        numberOfChannels = format->Format.nChannels;
        CoTaskMemFree(format);
        format = NULL;

        // Some drivers (such as Sound Blaster) are not able to perform channel count conversion. We need to perform channel count conversion. Allocate a buffer for channel count conversion with the hope of numFrames will never be as much as samplerate.
        if (numberOfChannels != requiredNumberOfChannels) {
            channelConvBuffer = (float *)_aligned_malloc(requiredNumberOfChannels * 4 * samplerate, 16);
            if (!channelConvBuffer) hr = E_OUTOFMEMORY;
            checkReturn(error_outofmemory);
        }
        if (hasOtherHandler) {
            if (input) (*ioHandler)->setNumberOfInputChannels(numberOfChannels);
            else *ioHandler = new inputToOutputHandler(samplerate, requiredNumberOfChannels);
        }
        
        UINT32 uBufferSize = 0;
        hr = client->GetBufferSize(&uBufferSize);
        checkReturn(error_getbuffersize);
        if (uBufferSize < 16) hr = E_INVALIDARG;
        checkReturn(error_buffersize16);
        bufferSize = int(uBufferSize);
        
        if (input) hr = client->GetService(__uuidof(IAudioCaptureClient), (void **)&captureClient);
        else hr = client->GetService(__uuidof(IAudioRenderClient), (void **)&renderClient);
        checkReturn(error_getservice);
        
        hr = client->SetEventHandle(sampleReadyEvent);
        checkReturn(error_seteventhandle);
        CreateThread(0, 0, audioThread, this, 0, NULL);
        while (state == state_initializing) Sleep(10);
    }
    
    ~inputOrOutputHandler() {
        while (state == state_iorunning) {
            stop = true;
            Sleep(50);
        }
        if (captureClient) captureClient->Release();
        if (renderClient) renderClient->Release();
        if (client) client->Release();
        if (channelConvBuffer) _aligned_free(channelConvBuffer);
        if (sampleReadyEvent) CloseHandle(sampleReadyEvent);
    }
    
    static unsigned long WINAPI audioThread(HANDLE param) {
        switch (CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)) {
            case S_OK:
            case S_FALSE:
            case RPC_E_CHANGED_MODE: break;
            default: abort();
        }
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        DWORD taskIndex = 0;
        HANDLE task = AvSetMmThreadCharacteristicsA("Pro Audio", &taskIndex);
        
        ((inputOrOutputHandler *)param)->audioThreadFunction();
        
        if (task) AvRevertMmThreadCharacteristics(task);
        CoUninitialize();
        return 0;
    }

    void audioThreadFunction() {
        if (client->Start() != S_OK) { state = error_start; return; }
        state = state_iorunning;
        unsigned int numBufferErrors = 0;
        
        while (true) {
            if (stop) { // Handle stopping.
                if (client) client->Stop();
                // The input handler "owns" the inputToOutputHandler instance.
                if (input && hasOtherHandler && *(ioHandler)) {
                    delete *(ioHandler);
                    *(ioHandler) = NULL;
                }
                state = state_stopped;
                return;
            }

            if (WaitForSingleObject(sampleReadyEvent, 1000) != WAIT_OBJECT_0) continue;
            
            if (input) { // Handle audio input.
                UINT32 framesAvailable;
                HRESULT hr = captureClient->GetNextPacketSize(&framesAvailable);
                if ((hr == S_OK) && (framesAvailable > 0)) {
                    BYTE *buffer;
                    DWORD captureFlags;
                    hr = captureClient->GetBuffer(&buffer, &framesAvailable, &captureFlags, NULL, NULL);

                    if (hr == S_OK) {
                        numBufferErrors = 0;
                        if (captureFlags & AUDCLNT_BUFFERFLAGS_SILENT) memset(buffer, 0, framesAvailable * 4 * numberOfChannels);
                        else if (!isFloat) convertToFloat(buffer, framesAvailable, numberOfChannels);

                        if (hasOtherHandler) {
                            if (*(ioHandler)) (*(ioHandler))->handleInput((float *)buffer, framesAvailable, samplerate);
                        } else {
                            if (numberOfChannels != requiredNumberOfChannels) {
                                channelConversion((float *)buffer, channelConvBuffer, numberOfChannels, requiredNumberOfChannels, (int)framesAvailable);
                                callback(clientdata, channelConvBuffer, NULL, framesAvailable, samplerate);
                            } else callback(clientdata, (float *)buffer, NULL, framesAvailable, samplerate);
                        }

                        captureClient->ReleaseBuffer(framesAvailable);
                    } else if (hr == AUDCLNT_E_BUFFER_ERROR) {
                        if (++numBufferErrors > 10) { state = error_buffer; return; }
                    } else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) { state = error_deviceinvalidated; return; }
                }
                else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) { state = error_deviceinvalidated; return; }
            } else { // Handle audio output.
                int framesToProduce = bufferSize;

                if (!exclusive) {
                    UINT32 padding = 0;
                    if (client->GetCurrentPadding(&padding) == S_OK) framesToProduce -= padding;
                }

                if (framesToProduce > 0) {
                    BYTE *buffer;
                    HRESULT hr = renderClient->GetBuffer(framesToProduce, &buffer);
                    
                    if (hr == S_OK) {
                        numBufferErrors = 0;

                        if (hasOtherHandler && !(*(ioHandler))->receivedFirstInput) {
                            memset(buffer, 0, framesToProduce * 4 * numberOfChannels);
                            renderClient->ReleaseBuffer(framesToProduce, AUDCLNT_BUFFERFLAGS_SILENT);
                        } else {
                            float *outputBuffer = (numberOfChannels != requiredNumberOfChannels) ? channelConvBuffer : (float *)buffer, *inputBuffer = NULL;

                            if (hasOtherHandler) inputBuffer = (*(ioHandler))->handleOutput(framesToProduce);

                            bool silence = !callback(clientdata, inputBuffer, outputBuffer, framesToProduce, samplerate);

                            if (!silence && (numberOfChannels != requiredNumberOfChannels)) channelConversion(channelConvBuffer, (float *)buffer, requiredNumberOfChannels, numberOfChannels, (int)framesToProduce);

                            if (silence) memset(buffer, 0, framesToProduce * 4 * numberOfChannels);   
                            else if (!isFloat) convertToPCM(buffer, framesToProduce, numberOfChannels);
                            renderClient->ReleaseBuffer(framesToProduce, silence ? AUDCLNT_BUFFERFLAGS_SILENT : 0);
                        }
                    } else if (hr == AUDCLNT_E_BUFFER_ERROR) {
                        if (++numBufferErrors > 10) { state = error_buffer; return; }
                    } else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) { state = error_deviceinvalidated; return; }
                }
            }
        }
	}
    
private:
    audioProcessingCallback callback;
    void *clientdata;
    inputToOutputHandler **ioHandler;
    float *channelConvBuffer;
    IAudioClient3 *client;
    IAudioCaptureClient *captureClient;
    IAudioRenderClient *renderClient;
    HANDLE sampleReadyEvent;
    int bufferSize, samplerate, requiredNumberOfChannels, numberOfChannels;
    bool stop, startio, input, hasOtherHandler, exclusive, isFloat;
};

typedef struct SuperpoweredWASAPIAudioIOInternals {
    inputOrOutputHandler *outputHandler, *inputHandler;
    audioProcessingCallback callback;
    void *clientdata;
    inputToOutputHandler *ioHandler;
    unsigned int numberOfChannels, preferredBufferSizeMs;
    bool enableInput, enableOutput;
} SuperpoweredWASAPIAudioIOInternals;

// This is the main class. It handles the two handlers for audio input and output.
SuperpoweredWASAPIAudioIO::SuperpoweredWASAPIAudioIO(audioProcessingCallback callback, void *clientdata, unsigned int preferredBufferSizeMs, unsigned int numberOfChannels, bool enableInput, bool enableOutput) {
    internals = new SuperpoweredWASAPIAudioIOInternals;
    memset(internals, 0, sizeof(SuperpoweredWASAPIAudioIOInternals));
    internals->callback = callback;
    internals->clientdata = clientdata;
    internals->enableInput = enableInput;
    internals->enableOutput = enableOutput;
    internals->numberOfChannels = numberOfChannels;
    internals->preferredBufferSizeMs = preferredBufferSizeMs;
}

const char *SuperpoweredWASAPIAudioIO::start() {
    switch (CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)) {
        case S_OK:
        case S_FALSE:
        case RPC_E_CHANGED_MODE: break;
        default: abort();
    }
    stop();
    
    if (internals->enableOutput) {
        internals->outputHandler = new inputOrOutputHandler(false, internals->preferredBufferSizeMs, internals->numberOfChannels, internals->enableInput, &internals->ioHandler, internals->callback, internals->clientdata);
        if (internals->outputHandler->state != state_iorunning) {
            const char *r = internals->outputHandler->state;
            stop();
            return r;
        }
    }
    
    if (internals->enableInput) {
        internals->inputHandler = new inputOrOutputHandler(true, internals->preferredBufferSizeMs, internals->numberOfChannels, internals->enableOutput, &internals->ioHandler, internals->callback, internals->clientdata);
        if (internals->inputHandler->state != state_iorunning) {
            const char *r = internals->inputHandler->state;
            stop();
            return r;
        }
    }
    
    return NULL;
}

void SuperpoweredWASAPIAudioIO::stop() {
    if (internals->outputHandler) delete internals->outputHandler;
    if (internals->inputHandler) delete internals->inputHandler;
    internals->outputHandler = internals->inputHandler = NULL;
}

SuperpoweredWASAPIAudioIO::~SuperpoweredWASAPIAudioIO() {
    stop();
	delete internals;
}
