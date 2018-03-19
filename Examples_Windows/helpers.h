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
#pragma comment(lib, "..\\..\\Superpowered\\Windows\\SuperpoweredWinUWP_" CONFIGURATION "_" PLATFORM  ".lib")

#include <Windows.h>
#include <stdio.h>

#define stringToChar(path, cpath) cpath[WideCharToMultiByte(CP_UTF8, 0, path->Data(), -1, cpath, MAX_PATH + 1, NULL, NULL)] = 0;

static void Log(const char *format, ...) {
	char buf[8192];
	va_list args;
	va_start(args, format);
	_vsnprintf_s_l(buf, 8192, 8192, format, NULL, args);
	va_end(args);
	OutputDebugStringA(buf);
}

#endif
