// w32_crt_stub.h

#pragma once

#pragma warning(disable : 4005) // macro redefinition

#define WINVER 0x501
#define _WIN32_WINNT 0x501
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _NO_CRT_STDIO_INLINE

#define UNICODE
#define _UNICODE

#include <windows.h>

int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd);

int __stdcall wWinMainCRTStartup() {
	STARTUPINFOW StartupInfo;
	int wShowWindow;
	WCHAR *lpCmdLine;

	StartupInfo.dwFlags = 0;
	GetStartupInfoW(&StartupInfo);
	wShowWindow = (StartupInfo.dwFlags & STARTF_USESHOWWINDOW) ? StartupInfo.wShowWindow : SW_SHOWDEFAULT;

	lpCmdLine = GetCommandLineW();
	if (lpCmdLine && *lpCmdLine) {
		if (*lpCmdLine == L'"') {
			++lpCmdLine;
			while (*lpCmdLine)
				if (*lpCmdLine++ == L'"')
					break;
		} else {
			while (*lpCmdLine && *lpCmdLine != L' ' && *lpCmdLine != L'\t')
				++lpCmdLine;
		}
		while (*lpCmdLine == L' ' || *lpCmdLine == L'\t')
			++lpCmdLine;

		return wWinMain(GetModuleHandleW(0), 0, lpCmdLine, wShowWindow);
	}
	else {
		return wWinMain(GetModuleHandleW(0), 0, L"", wShowWindow);
	}
}
