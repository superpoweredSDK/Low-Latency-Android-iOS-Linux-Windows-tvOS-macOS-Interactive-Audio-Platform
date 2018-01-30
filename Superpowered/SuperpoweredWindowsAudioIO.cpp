#include <pch.h> // Remove this if you're not using precompiled headers.
#include "SuperpoweredWindowsAudioIO.h"
#include <wrl\implements.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <mfapi.h>

#define NUMCHANNELS 2

// If both audio input and output are enabled, this class handles a non-blocking buffer between input and output.
// It can resample too (in a simple way) if the sample rate of the input is different to the output.
class inputToOutputHandler {
public:
    inputToOutputHandler(int samplerate) : outputSamplerate(samplerate), resampleBuffer(NULL), slopeCount(0), invSlopeCount(1.0f), afterUnderrun(true) {
        buffer = (float *)_aligned_malloc(NUMCHANNELS * 4 * samplerate, 16); // Can store 1 second of audio.
        memset(prev, 0, NUMCHANNELS * 4);
    }
    
    ~inputToOutputHandler() {
        _aligned_free(buffer);
        if (resampleBuffer) _aligned_free(resampleBuffer);
    }
    
    void handleInput(BYTE *input, int numFrames, int samplerate) {
        if (samplerate != outputSamplerate) { // Resampling if needed.
            resampleBuffer = (float *)_aligned_malloc(NUMCHANNELS * 4 * samplerate, 16);
            numFrames = resample((float *)input, resampleBuffer, float(samplerate) / float(outputSamplerate), numFrames);
            input = (BYTE *)resampleBuffer;
		}
        
        int spaceLeft = outputSamplerate - writePos;
        
        if (spaceLeft < numFrames) { // End of buffer.
            memcpy(buffer + writePos * NUMCHANNELS, input, spaceLeft * NUMCHANNELS * 4);
            input += spaceLeft * NUMCHANNELS * 4;
            numFrames -= spaceLeft;
            writePos = 0;
        }
        
        memcpy(buffer + writePos * NUMCHANNELS, input, numFrames * NUMCHANNELS * 4);
        writePos += numFrames;
    }
    
    void handleOutput(BYTE *output, int numFrames) {
        int inputFramesAvailable = writePos - readPos;
        if (inputFramesAvailable < 0) inputFramesAvailable += outputSamplerate;
        int minFramesShouldBe = afterUnderrun ? (numFrames + (numFrames >> 1)) : numFrames;

        if (inputFramesAvailable < minFramesShouldBe) { // Underrun, not enough audio input frames are available.
            afterUnderrun = true;
            memset(output, 0, numFrames * 4 * NUMCHANNELS);
        } else {
            afterUnderrun = false;
            int spaceLeft = outputSamplerate - readPos;
            
            if (spaceLeft < numFrames) { // End of buffer.
                memcpy(output, buffer + readPos * NUMCHANNELS, spaceLeft * NUMCHANNELS * 4);
                output += spaceLeft * NUMCHANNELS * 4;
                numFrames -= spaceLeft;
                readPos = 0;
            }
            
            memcpy(output, buffer + readPos * NUMCHANNELS, numFrames * NUMCHANNELS * 4);
            readPos += numFrames;
        }
    }
    
private:
    float *buffer, *resampleBuffer;
    float slopeCount, invSlopeCount, prev[NUMCHANNELS];
    int outputSamplerate, readPos, writePos;
    bool afterUnderrun;
    
    int resample(float *input, float *output, float rate, int numFrames) {
        int outFrames = 0;
        
        while (true) {
            while (slopeCount > 1.0f) {
                numFrames--;
                slopeCount -= 1.0f;
                invSlopeCount = 1.0f - slopeCount;
                if (!numFrames) return outFrames;
                
                memcpy(prev, input, NUMCHANNELS * 4);
                input += NUMCHANNELS;
            }
            
            for (int ch = 0; ch < NUMCHANNELS; ch++) *output++ = invSlopeCount * prev[ch] + slopeCount * input[ch];
            memcpy(prev, input, NUMCHANNELS * 4);
            outFrames++;
            slopeCount += rate;
            invSlopeCount = 1.0f - slopeCount;
        }
    }
};

typedef struct inputOrOutputHandlerInternals {
    audioProcessingCallback callback;
    void *clientdata;
    inputToOutputHandler **ioHandler;
    IAudioClient3 *client;
    IAudioCaptureClient *captureClient;
    IAudioRenderClient *renderClient;
    IMFAsyncResult *sampleReadyAsyncResult;
    HANDLE sampleReadyEvent;
    MFWORKITEM_KEY cancelKey;
    DWORD workQueueId;
    int bufferSize, numberOfSamples, samplerate;
    bool raw, stop, input, hasOtherHandler;
} inputOrOutputHandlerInternals;

// This class handles audio input OR audio output. Not both, just one of them.
class inputOrOutputHandler: public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, Microsoft::WRL::FtmBase, IMFAsyncCallback, IActivateAudioInterfaceCompletionHandler> {
public:
    bool running;
    
    inputOrOutputHandler(bool input, bool hasOtherHandler, inputToOutputHandler **ioHandler,  DWORD workQueueIdentifier, bool rawProcessingSupported, audioProcessingCallback cb, void *cd) : running(false) {
        internals = new inputOrOutputHandlerInternals;
        memset(internals, 0, sizeof(inputOrOutputHandlerInternals));
        internals->input = input;
        internals->hasOtherHandler = hasOtherHandler;
        internals->ioHandler = ioHandler;
        internals->stop = false;
        internals->workQueueId = workQueueIdentifier;
        internals->raw = rawProcessingSupported;
        internals->callback = cb;
        internals->clientdata = cd;
        internals->sampleReadyEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    }

	STDMETHODIMP GetParameters(DWORD *flags, DWORD *queue) {
		*queue = internals->workQueueId;
		*flags = 0;
		return S_OK;
	}

    // This is the audio processing callback, called by a Media Foundation audio thread.
	STDMETHODIMP Invoke(IMFAsyncResult *result) {
        if (internals->stop) { // Handle stopping.
            if (internals->client) internals->client->Stop();
            // The input handler "owns" the inputToOutputHandler instance.
            if (internals->input && internals->hasOtherHandler && *(internals->ioHandler)) {
                delete *(internals->ioHandler);
                *(internals->ioHandler) = NULL;
            }
			if (internals->input || !internals->hasOtherHandler) fail(0); // Notify the app that we stopped.
            return E_FAIL;
        }
        
        if (internals->input) { // Handle audio input.
            UINT64 devicePosition = 0, qpcPosition = 0;
            
            while (true) {
                UINT32 framesAvailable;
                HRESULT hr = internals->captureClient->GetNextPacketSize(&framesAvailable);
                if (FAILED(hr) || (framesAvailable == 0)) break;
                
                BYTE *buffer;
                DWORD captureFlags;
                if (SUCCEEDED(internals->captureClient->GetBuffer(&buffer, &framesAvailable, &captureFlags, &devicePosition, &qpcPosition))) {
                    if (captureFlags & AUDCLNT_BUFFERFLAGS_SILENT) memset(buffer, 0, framesAvailable * 4 * NUMCHANNELS);
                    
                    if (internals->hasOtherHandler) {
                        if (*(internals->ioHandler)) (*(internals->ioHandler))->handleInput(buffer, framesAvailable, internals->samplerate);
                    } else {
                        internals->callback(internals->clientdata, (float *)buffer, internals->numberOfSamples, internals->samplerate);
                    }
                   
                    internals->captureClient->ReleaseBuffer(framesAvailable);
                }
            }
            
            HRESULT hr = MFPutWaitingWorkItem(internals->sampleReadyEvent, 0, internals->sampleReadyAsyncResult, &internals->cancelKey);
            if (FAILED(hr)) return hr;
        } else { // Handle audio output.
            UINT32 padding;
            HRESULT hr = internals->client->GetCurrentPadding(&padding);
            if (FAILED(hr)) return hr;
            
            int framesLeft = internals->bufferSize - padding;
            if (framesLeft > 0) {
                BYTE *buffer;
                hr = internals->renderClient->GetBuffer(framesLeft, &buffer);
                if (FAILED(hr)) return hr;
                
                bool silence = true;
                int framesWritten = 0;
                
                while (framesLeft >= internals->numberOfSamples) {
                    if (internals->hasOtherHandler) (*(internals->ioHandler))->handleOutput(buffer, internals->numberOfSamples);
                    
                    if (!internals->callback(internals->clientdata, (float *)buffer, internals->numberOfSamples, internals->samplerate)) memset(buffer, 0, internals->numberOfSamples * 4 * NUMCHANNELS); else silence = false;
                    
                    framesLeft -= internals->numberOfSamples;
                    framesWritten += internals->numberOfSamples;
                    buffer += internals->numberOfSamples * 4 * NUMCHANNELS;
                }
                
                internals->renderClient->ReleaseBuffer(framesWritten, silence ? AUDCLNT_BUFFERFLAGS_SILENT : 0);
                hr = MFPutWaitingWorkItem(internals->sampleReadyEvent, 0, internals->sampleReadyAsyncResult, &internals->cancelKey);
                if (FAILED(hr)) return hr;
            }
        }

		return S_OK;
	}

    // Called by Windows when the audio interface is activated.
	STDMETHOD(ActivateCompleted)(IActivateAudioInterfaceAsyncOperation *operation) {
        if (!internals->sampleReadyEvent) return fail(4);
        
		IUnknown *audioInterface = nullptr;
		HRESULT hrActivateResult = S_OK, hr = operation->GetActivateResult(&hrActivateResult, &audioInterface);
        if (FAILED(hr)) return fail(5);
        if (FAILED(hrActivateResult)) return fail(6);

		audioInterface->QueryInterface(IID_PPV_ARGS(&internals->client));
        if (!internals->client) return fail(7);

		AudioClientProperties properties = { 0 };
		properties.cbSize = sizeof(AudioClientProperties);
		properties.eCategory = AudioCategory_Media;
        if (internals->raw) properties.Options |= AUDCLNT_STREAMOPTIONS_RAW;
        if (FAILED(internals->client->SetClientProperties(&properties))) return fail(8);

		WAVEFORMATEX *format;
		if (FAILED(internals->client->GetMixFormat(&format))) return fail(9);
        
		UINT32 defaultPeriodFrames, fundamentalPeriodFrames, minPeriodFrames, maxPeriodFrames;
		hr = internals->client->GetSharedModeEnginePeriod(format, &defaultPeriodFrames, &fundamentalPeriodFrames, &minPeriodFrames, &maxPeriodFrames);
		if (FAILED(hr)) {
			CoTaskMemFree(format);
			return fail(12);
		}
	
        internals->samplerate = format->nSamplesPerSec;
        internals->numberOfSamples = minPeriodFrames;
        format->nChannels = NUMCHANNELS;
        if (internals->hasOtherHandler && !internals->input) *internals->ioHandler = new inputToOutputHandler(internals->samplerate);
        
		hr = internals->client->InitializeSharedAudioStream(AUDCLNT_STREAMFLAGS_EVENTCALLBACK, minPeriodFrames, format, nullptr);
		CoTaskMemFree(format);
		if (FAILED(hr)) return fail(13);

        UINT32 uBufferSize = 0;
		if (FAILED(internals->client->GetBufferSize(&uBufferSize))) return fail(14);
        internals->bufferSize = int(uBufferSize);

        if (internals->input) {
            if (FAILED(internals->client->GetService(__uuidof(IAudioCaptureClient), (void **)&internals->captureClient))) return fail(15);
        } else {
            if (FAILED(internals->client->GetService(__uuidof(IAudioRenderClient), (void **)&internals->renderClient))) return fail(15);
        }
		if (FAILED(MFCreateAsyncResult(nullptr, this, nullptr, &internals->sampleReadyAsyncResult))) return fail(16);
		if (FAILED(internals->client->SetEventHandle(internals->sampleReadyEvent))) return fail(17);
		if (FAILED(internals->client->Start())) return fail(18);
		if (FAILED(MFPutWaitingWorkItem(internals->sampleReadyEvent, 0, internals->sampleReadyAsyncResult, &internals->cancelKey))) return fail(19);

        running = true;
		return S_OK;
	}
    
    // Delete the handler. The actual operation will happen at Invoke().
    static void release(Microsoft::WRL::ComPtr<inputOrOutputHandler> stream) {
        if (stream == nullptr) return;
        inputOrOutputHandler *handler = *stream.GetAddressOf();
        if (handler->running) handler->internals->stop = true;
    }
    
private:
    inputOrOutputHandlerInternals *internals;
    
    HRESULT fail(int code) {
        if (internals->cancelKey != 0) MFCancelWorkItem(internals->cancelKey);
        if (internals->client) internals->client->Release();
        if (internals->renderClient) internals->renderClient->Release();
        if (internals->captureClient) internals->captureClient->Release();
        if (internals->sampleReadyAsyncResult) internals->sampleReadyAsyncResult->Release();
        if (internals->sampleReadyEvent != INVALID_HANDLE_VALUE) CloseHandle(internals->sampleReadyEvent);
        internals->callback(internals->clientdata, NULL, 0, code);
        delete internals;
        return E_FAIL;
    }
};

typedef struct SuperpoweredWindowsAudioIOInternals {
    Microsoft::WRL::ComPtr<inputOrOutputHandler> outputHandler, inputHandler;
    audioProcessingCallback callback;
    void *clientdata;
    inputToOutputHandler *ioHandler;
    DWORD workQueueId;
    bool enableInput, enableOutput;
} SuperpoweredWindowsAudioIOInternals;

// This is the main class. It handles the two handlers for audio input and output and sets up Media Foundation for Pro Audio, to receive proper scheduling.
SuperpoweredWindowsAudioIO::SuperpoweredWindowsAudioIO(audioProcessingCallback callback, void *clientdata, bool enableInput, bool enableOutput) {
    internals = new SuperpoweredWindowsAudioIOInternals;
    memset(internals, 0, sizeof(SuperpoweredWindowsAudioIOInternals));
    internals->callback = callback;
    internals->clientdata = clientdata;
    internals->enableInput = enableInput;
    internals->enableOutput = enableOutput;
}

void SuperpoweredWindowsAudioIO::start() {
    Platform::String^ outputDeviceId = Windows::Media::Devices::MediaDevice::GetDefaultAudioRenderId(Windows::Media::Devices::AudioDeviceRole::Default);
    Platform::String^ inputDeviceId = Windows::Media::Devices::MediaDevice::GetDefaultAudioCaptureId(Windows::Media::Devices::AudioDeviceRole::Default);

    if ((internals->enableOutput && !outputDeviceId) || (internals->enableInput && !inputDeviceId)) {
        internals->callback(internals->clientdata, NULL, 1, 3);
        return;
    }
    
    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE))) {
        internals->callback(internals->clientdata, NULL, 0, 1);
        return;
    }

    DWORD taskId = 0;
    if (FAILED(MFLockSharedWorkQueue(L"Pro Audio", 0, &taskId, &internals->workQueueId))) {
        MFShutdown();
        internals->callback(internals->clientdata, NULL, 0, 2);
        return;
    }

    auto properties = ref new Platform::Collections::Vector<Platform::String^>();
    properties->Append("System.Devices.AudioDevice.RawProcessingSupported");
    
    if (internals->enableOutput) {
        Concurrency::create_task(Windows::Devices::Enumeration::DeviceInformation::CreateFromIdAsync(outputDeviceId, properties)).then([outputDeviceId, this](Windows::Devices::Enumeration::DeviceInformation^ deviceInformation) {
            auto obj = deviceInformation->Properties->Lookup("System.Devices.AudioDevice.RawProcessingSupported");
            bool rawProcessingSupported = false;
            if (obj) rawProcessingSupported = obj->Equals(true);
            
            internals->outputHandler = Microsoft::WRL::Make<inputOrOutputHandler>(false, internals->enableInput, &internals->ioHandler, internals->workQueueId, rawProcessingSupported, internals->callback, internals->clientdata);
            IActivateAudioInterfaceAsyncOperation *asyncOperation;
            ActivateAudioInterfaceAsync(outputDeviceId->Data(), __uuidof(IAudioClient3), nullptr, *internals->outputHandler.GetAddressOf(), &asyncOperation);
        });
    }
    
    if (internals->enableInput) {
        Concurrency::create_task(Windows::Devices::Enumeration::DeviceInformation::CreateFromIdAsync(inputDeviceId, properties)).then([inputDeviceId, this](Windows::Devices::Enumeration::DeviceInformation^ deviceInformation) {
            auto obj = deviceInformation->Properties->Lookup("System.Devices.AudioDevice.RawProcessingSupported");
            bool rawProcessingSupported = false;
            if (obj) rawProcessingSupported = obj->Equals(true);
            
            internals->inputHandler = Microsoft::WRL::Make<inputOrOutputHandler>(true, internals->enableOutput, &internals->ioHandler, internals->workQueueId, rawProcessingSupported, internals->callback, internals->clientdata);
            IActivateAudioInterfaceAsyncOperation *asyncOperation;
            ActivateAudioInterfaceAsync(inputDeviceId->Data(), __uuidof(IAudioClient3), nullptr, *internals->inputHandler.GetAddressOf(), &asyncOperation);
        });
    }
}

void SuperpoweredWindowsAudioIO::stop() {
    inputOrOutputHandler::release(internals->outputHandler);
    inputOrOutputHandler::release(internals->inputHandler);
    internals->outputHandler = internals->inputHandler = nullptr;
    MFShutdown();
}

SuperpoweredWindowsAudioIO::~SuperpoweredWindowsAudioIO() {
    inputOrOutputHandler::release(internals->outputHandler);
    inputOrOutputHandler::release(internals->inputHandler);
	delete internals;
}
