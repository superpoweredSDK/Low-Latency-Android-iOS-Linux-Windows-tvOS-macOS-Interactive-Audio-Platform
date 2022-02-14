#include "framework.h"
#include "Recorder.h"
#include <stdio.h>
#include <shlobj.h>
#include <example_helpers.h>
#include <Superpowered.h>
#include <OpenSource/SuperpoweredWASAPIAudioIO.h>
#include <SuperpoweredRecorder.h>

static HWND button = NULL;
static SuperpoweredWASAPIAudioIO *audioIO = NULL;
static Superpowered::Recorder *recorder = NULL;
enum State { Idle, Starting, Recording, Stopping };
static int currentSamplerate = 0;
static State state = Idle;

// Audio output should be provided here. Runs periodically on the audio I/O thread.
static bool audioProcessing(void *clientdata, float *input, float *output, int numberOfFrames, int samplerate) {
    currentSamplerate = samplerate;
	if (state == Recording) recorder->recordInterleaved(input, numberOfFrames);
	return false;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_COMMAND:
            if (HIWORD(wParam) != BN_CLICKED) return DefWindowProc(hWnd, message, wParam, lParam);
            switch (state) {
                case Idle: {
                    // Create a recorder with a temporary file path.
                    char path[MAX_PATH + 1];
                    GetTempPathA(MAX_PATH + 1, path);
                    sprintf_s(path, MAX_PATH + 1, "%s\\temp.wav", path);
                    DeleteFileA(path);
                    recorder = new Superpowered::Recorder(path);
            
                    // Create and start audio input.
                    audioIO = new SuperpoweredWASAPIAudioIO(audioProcessing, NULL, 12, 2, true, false);
                    audioIO->start();
            
                    currentSamplerate = 0;
                    SetWindowTextW(button, L"STARTING...");
                    state = Starting;
                    SetTimer(hWnd, 1, 100, (TIMERPROC)NULL);
                } break;
                case Recording:
                    // Stop and delete audio input.
                    recorder->stop();
                    audioIO->stop();
                    delete audioIO;
            
                    SetWindowTextW(button, L"STOPPING...");
                    state = Stopping;
                    SetTimer(hWnd, 1, 100, (TIMERPROC)NULL);
                    break;
                default:;
            } break;
        case WM_TIMER:
            switch (state) {
                case Starting: if (currentSamplerate > 0) {
                    KillTimer(hWnd, 1);
                    // Start a new recording with the destination file path on the Desktop.
                    wchar_t wpath[MAX_PATH + 1] = { 0 };
                    SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, wpath);
                    char path[MAX_PATH * 4 + 128];
                    path[WideCharToMultiByte(CP_UTF8, 0, wpath, lstrlenW(wpath), path, MAX_PATH * 4 + 1, NULL, NULL)] = 0;
                    sprintf_s(path, MAX_PATH * 4 + 128, "%s\\recording", path);
                    DeleteFileA(path);
                    recorder->prepare(path, currentSamplerate, true, 1);
                    Log("Will record to: %s\n", path);
				    SetWindowTextW(button, L"STOP");
                    state = Recording;
                } break;
                case Stopping: if (recorder->isFinished()) {
                    // Delete the recorder, stop periodic UI updates and open the destination folder.
                    KillTimer(hWnd, 1);
                    delete recorder;
				    SetWindowTextW(button, L"START");
                    state = Idle;
                } break;
                default:;
            } break;
        case WM_CLOSE: return DefWindowProc(hWnd, message, wParam, lParam);
        case WM_DESTROY: PostQuitMessage(0); break;
        default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    Superpowered::Initialize("ExampleLicenseKey-WillExpire-OnNextUpdate");

    HWND hWnd = makeWindow(hInstance, WndProc);
    button = CreateWindowW(L"BUTTON", L"START", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 10, 100, 100, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
