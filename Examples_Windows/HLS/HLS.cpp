#include "HLS.h"
#include "framework.h"
#include <stdio.h>
#include <OpenSource/SuperpoweredWASAPIAudioIO.h>
#include <Superpowered.h>
#include <SuperpoweredAdvancedAudioPlayer.h>
#include <example_helpers.h>

static HWND button = NULL;
static SuperpoweredWASAPIAudioIO *audioIO = NULL;
static Superpowered::AdvancedAudioPlayer *player = NULL;

// Audio output should be provided here. Runs periodically on the audio I/O thread.
static bool audioProcessing(void *clientdata, float *input, float *output, int numberOfFrames, int samplerate) {
    switch (player->getLatestEvent()) {
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_None:
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_Opening: break; // do nothing
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_Opened: player->play(); break;
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_OpenFailed: {
            int openError = player->getOpenErrorCode();
            Log("Open error %i: %s", openError, Superpowered::AdvancedAudioPlayer::statusCodeToString(openError));
        } break;
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_ConnectionLost: Log("Network download failed."); break;
        case Superpowered::AdvancedAudioPlayer::PlayerEvent_ProgressiveDownloadFinished: Log("Download finished. Path: %s", player->getFullyDownloadedFilePath()); break;
    }

    player->outputSamplerate = samplerate;
    return player->processStereo(output, false, numberOfFrames);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_COMMAND:
            if (HIWORD(wParam) != BN_CLICKED) return DefWindowProc(hWnd, message, wParam, lParam);
            if (!audioIO) {
                player = new Superpowered::AdvancedAudioPlayer(44100, 0);
                player->loopOnEOF = true;
                player->openHLS("https://playertest.longtailvideo.com/adaptive/oceans_aes/oceans_aes.m3u8");
                audioIO = new SuperpoweredWASAPIAudioIO(audioProcessing, NULL, 12, 2, false, true);
                audioIO->start();
                SetWindowTextW(button, L"STOP");
            } else {
                player->togglePlayback();
                SetWindowTextW(button, player->isPlaying() ? L"STOP" : L"START");
            }
            break;
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

    // HLS playback requires a temporary folder.
    char path[MAX_PATH + 1];
    GetTempPathA(MAX_PATH + 1, path);
    Superpowered::AdvancedAudioPlayer::setTempFolder(path);

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
