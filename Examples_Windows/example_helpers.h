#ifndef helpers
#define helpers

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
#pragma comment(lib, "..\\..\\Superpowered\\libWindows\\SuperpoweredWin143_" CONFIGURATION "_MT_" PLATFORM  ".lib")
#pragma comment(lib, "Ws2_32.lib")

static void Log(const char* format, ...) {
    char buf[8192];
    va_list args;
    va_start(args, format);
    _vsnprintf_s_l(buf, 8192, 8192, format, NULL, args);
    va_end(args);
    OutputDebugStringA(buf);
}

#define MP3_PATH PROJECTDIR "..\\track.mp3"

static const wchar_t* mainWindowClass = L"MainWindow";

static HWND makeWindow(HINSTANCE hInstance, WNDPROC WndProc) {
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = mainWindowClass;
    wcex.hIconSm = NULL;
    RegisterClassExW(&wcex);
    return CreateWindowW(mainWindowClass, L"", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
}

#endif
