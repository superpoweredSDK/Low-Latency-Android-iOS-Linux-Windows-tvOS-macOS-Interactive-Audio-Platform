#include "framework.h"
#include "Player.h"
#include <stdio.h>
#include <example_helpers.h>
#include <Superpowered.h>
#include <OpenSource/SuperpoweredWASAPIAudioIO.h>
#include <SuperpoweredAdvancedAudioPlayer.h>

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
    if (player->eofRecently()) player->setPosition(0, false, false);
    return player->processStereo(output, false, numberOfFrames);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_COMMAND:
            if (HIWORD(wParam) != BN_CLICKED) return DefWindowProc(hWnd, message, wParam, lParam);
            if (!audioIO) {
                player = new Superpowered::AdvancedAudioPlayer(44100, 0);
                player->open(MP3_PATH);
                audioIO = new SuperpoweredWASAPIAudioIO(audioProcessing, NULL, 12, 2, false, true);
                audioIO->start();
                SetWindowTextW(button, L"STOP");
            } else {
                audioIO->stop();
                delete audioIO;
                delete player;
                audioIO = NULL;
                SetWindowTextW(button, L"START");
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
