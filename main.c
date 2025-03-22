#include "w32_crt_stub.h"
#include "resource.h"

#include <stdio.h>

#pragma intrinsic(memset,memmove,wcscmp,wcscpy,wcslen)

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

static HICON g_hIcon = NULL;
static HWND g_hDlg = NULL;
static HFONT g_hFont = NULL;
static HBITMAP g_hBitmap = NULL;
static HMENU g_hMenu = NULL;
static BOOL g_alwaysOnTop = FALSE;

void SetText(const WCHAR* text, LONG len) {
	HWND hEdit = GetDlgItem(g_hDlg, IDC_EDIT1);
	SetWindowTextW(hEdit, text);
	SendMessageW(hEdit, EM_SETSEL, len, len);
	SendMessageW(hEdit, EM_SCROLLCARET, 0, 0);
}

void AppendText(const WCHAR* text) {
	HWND hEdit = GetDlgItem(g_hDlg, IDC_EDIT1);
	DWORD startPos, endPos;
	int iscroll = -1;
	SCROLLINFO scroll_info = { sizeof(scroll_info), SIF_ALL, 0, 0, 0, 0, 0 };

	SendMessageW(hEdit, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&endPos);
	GetScrollInfo (hEdit, SB_VERT, &scroll_info);

	if (scroll_info.nPos + scroll_info.nPage < scroll_info.nMax && scroll_info.nTrackPos + scroll_info.nPage < scroll_info.nMax) {
		int firstline = SendMessageW(hEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
		iscroll = SendMessageW(hEdit, EM_LINEINDEX, firstline, 0);
	}

	int len_prev = GetWindowTextLengthW(hEdit);
	SendMessageW(hEdit, EM_SETSEL, len_prev, len_prev);
	SendMessageW(hEdit, EM_REPLACESEL, FALSE, (LPARAM)text);

	if(iscroll >= 0) {
		SendMessageW(hEdit, EM_SETSEL, (WPARAM)iscroll, (LPARAM)iscroll);
	}

	SendMessageW(hEdit, EM_SCROLLCARET, 0, 0);
	if (startPos == endPos)
		SendMessageW(hEdit, EM_SETSEL, -1, 0);
	else
		SendMessageW(hEdit, EM_SETSEL, (WPARAM)startPos, (LPARAM)endPos);
}

HFONT CreateBoldFont(HFONT hFont) {
	LOGFONT lf;
	GetObjectW(hFont, sizeof(LOGFONT), &lf);
	lf.lfWeight = 1000;
	return CreateFontIndirectW(&lf);
}

void SaveOptions(HWND hDlg) {
	HKEY hKey;
	if(!RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\SonicTeam\\PSOBB\\chatlog", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL)) {
		LONG data[3];
		WINDOWPLACEMENT wndpl;
		memset(&wndpl, 0, sizeof(WINDOWPLACEMENT));
		wndpl.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(hDlg, &wndpl);
		data[0] = g_alwaysOnTop;
		data[1] = wndpl.rcNormalPosition.left;
		data[2] = wndpl.rcNormalPosition.top;
		RegSetValueExW(hKey, L"OPTS", 0, REG_BINARY, (LPBYTE)data, sizeof(data));
		RegCloseKey(hKey);
	}
}

void RestoreOptions(HWND hDlg) {
	HKEY hKey;
	if(!RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\SonicTeam\\PSOBB\\chatlog", 0, NULL, 0, KEY_WRITE | KEY_QUERY_VALUE, NULL, &hKey, NULL)) {
		LONG data[3];
		DWORD cbData = sizeof(data);
		DWORD dwType = REG_BINARY;
		if (!RegQueryValueExW(hKey, L"OPTS", 0, &dwType, (LPBYTE)data, &cbData) && dwType == REG_BINARY && cbData >= sizeof(data)) {
			g_alwaysOnTop = data[0];
			if (g_alwaysOnTop) {
				AllowSetForegroundWindow(-1);
				SetForegroundWindow(hDlg);
				SetWindowPos(hDlg, HWND_TOPMOST, data[1], data[2], 0, 0, SWP_NOSIZE);
			}
			else {
				SetWindowPos(hDlg, 0, data[1], data[2], 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}
		RegCloseKey(hKey);
	}
}

INT_PTR __stdcall DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
		{
			g_hFont = CreateBoldFont((HFONT)SendMessageW(hDlg, WM_GETFONT, 0, 0));
			g_hMenu = GetSystemMenu(hDlg, FALSE);
			SendMessageW(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)g_hIcon);
			SendMessageW(hDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hIcon);
			SendMessageW(GetDlgItem(hDlg, IDC_EDIT1), EM_LIMITTEXT, 0x7FFFFFFE, 0);
			SendMessageW(hDlg, WM_SETFONT, (WPARAM)g_hFont, (LPARAM)TRUE);
			SendMessageW(GetDlgItem(hDlg, IDC_EDIT1), WM_SETFONT, (WPARAM)g_hFont, (LPARAM)TRUE);
			if((GetAsyncKeyState(VK_F12) & 0x8000) == 0) { // holding F12 on launch resets to defaults
				RestoreOptions(hDlg);
			}

			MENUITEMINFOW alwaysOnTop;
			memset(&alwaysOnTop, 0, sizeof(MENUITEMINFOW));
			alwaysOnTop.cbSize = sizeof(MENUITEMINFOW);
			alwaysOnTop.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID | MIIM_STATE;
			alwaysOnTop.fType = MFT_STRING;
			alwaysOnTop.dwTypeData = L"Always on top";
			alwaysOnTop.cch = sizeof(L"Always on top")/sizeof(WCHAR);
			alwaysOnTop.wID = 0x1010;
			alwaysOnTop.fState = (g_alwaysOnTop ? MF_CHECKED : MF_UNCHECKED);

			MENUITEMINFOW clearLog;
			memset(&clearLog, 0, sizeof(MENUITEMINFOW));
			clearLog.cbSize = sizeof(MENUITEMINFOW);
			clearLog.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
			clearLog.fType = MFT_STRING;
			clearLog.dwTypeData = L"Clear log";
			clearLog.cch = sizeof(L"Clear log")/sizeof(WCHAR);
			clearLog.wID = 0x1020;

			MENUITEMINFOW separator;
			memset(&separator, 0, sizeof(MENUITEMINFOW));
			separator.cbSize = sizeof(MENUITEMINFO);
			separator.fMask = MIIM_FTYPE;
			separator.fType = MFT_SEPARATOR;

			InsertMenuItemW(g_hMenu, 0, TRUE, &alwaysOnTop);
			InsertMenuItemW(g_hMenu, 1, TRUE, &separator);
			InsertMenuItemW(g_hMenu, 2, TRUE, &clearLog);
			InsertMenuItemW(g_hMenu, 3, TRUE, &separator);
			g_hDlg = hDlg;
			return TRUE;
		}

		case WM_SYSCOMMAND:
		{
			switch (wParam & 0xFFF0) {
				case 0x1010:
				{
					g_alwaysOnTop = !g_alwaysOnTop;
					CheckMenuItem(g_hMenu, 0x1010, MF_BYCOMMAND | (g_alwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
					SetWindowPos(hDlg, (g_alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
					return TRUE;
				}
				case 0x1020:
				{
					SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT1), L"");
					return TRUE;
				}
				case SC_CLOSE:
				{
					SaveOptions(hDlg);
					DestroyWindow(hDlg);
					return TRUE;
				}
			}
			break;
		}

		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLOREDIT:
		{
			if(GetWindowLongW((HWND)lParam, GWL_EXSTYLE) & WS_EX_TRANSPARENT)
			{
				HDC hDC = (HDC)wParam;
				SetTextColor(hDC, RGB(240, 255, 255));
				SetBkMode(hDC, TRANSPARENT);
				HBRUSH hbr = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
				return (LONG)hbr;
			}
			break;
		}

		case WM_ERASEBKGND:
			// don't do it
			return TRUE;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BITMAP bm;
			HDC hDC = BeginPaint(hDlg, &ps);
			HDC MemDC = CreateCompatibleDC(hDC);
			SelectObject(MemDC, g_hBitmap);
			GetObjectW(g_hBitmap, sizeof(bm), &bm);
			BitBlt(hDC, 0, 0, bm.bmWidth, bm.bmHeight, MemDC, 0, 0, SRCCOPY);
			DeleteDC(MemDC);
			EndPaint(hDlg, &ps);
			return TRUE;
		}

#if 0
		case WM_SIZE:
		{
			UINT width = LOWORD(lParam)-9;
			UINT height = HIWORD(lParam)-16;
			SetWindowPos(GetDlgItem(hDlg, IDC_EDIT1), hDlg, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			return FALSE;
		}
#endif

		case WM_CLOSE:
		{
			SaveOptions(hDlg);
			DestroyWindow(hDlg);
			return TRUE;
		}

		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return TRUE;
		}
	}

	return FALSE;
}

WCHAR* GetLatestChatLog(void) {
	static WCHAR filename[MAX_PATH];
	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW(L".\\chat*.txt", &fd);
	WCHAR* cfile = 0;
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				cfile = fd.cFileName;
			}
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
		if (cfile) {
			wcscpy(filename, cfile);
			return filename;
		}
	}
	return 0;
}

BOOL IsLatestChatLog(WCHAR* compare) {
	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW(L".\\chat*.txt", &fd);
	WCHAR* cfile = 0;
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				cfile = fd.cFileName;
			}
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
		if (cfile) {
			return (!wcscmp(cfile, compare));
		}
	}
	return TRUE;
}

void FormatChatLog(WCHAR* str) {
	WCHAR* p1 = str;
	while (*p1) {
		BOOL colon = FALSE;
		while (*p1 && *p1 != L'\t') {
			if (*p1 == L':') colon = TRUE;
			p1++;
		}
		if (*p1 == '\t' && colon) {
			WCHAR* p2 = p1+1;
			if (*p2 >= L'0' && *p2 <= L'9') {
				p2++;
				while (*p2 >= L'0' && *p2 <= L'9') p2++;
				if (*p2 == '\t' && *++p2) {
					p1++;
					*p1++ = L'<';
					while (*p2 && *p2 != L'\t') *p1++ = *p2++;
					if (*p2 == L'\t') {
						*p1++ = L'>';
						*p2 = L' ';
						memmove(p1, p2, wcslen(p2) * 2 + 2);
					}
				}
			}
		}
		p1++;
	}
}

void ReadWholeFile(FILE* fp) {
	fseek(fp, 0, SEEK_END);
	long lSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	WCHAR* buf = (WCHAR*)MALLOC(lSize+2);
	if (buf) {
		size_t nelem = fread(buf, 2, lSize / 2, fp);
		if (nelem) {
			buf[nelem] = 0;
			FormatChatLog(buf);
			SetText(buf, nelem);
		}
		FREE(buf);
	}
}

DWORD __stdcall MainThread(void* param) {
	WCHAR buffer[4096];
	while (!g_hDlg) {
		if (WaitForSingleObject(param, 100) != WAIT_TIMEOUT) return 0;
	}
	WCHAR* filename = GetLatestChatLog();
	while (!filename) {
		if (WaitForSingleObject(param, 100) != WAIT_TIMEOUT) return 0;
		filename = GetLatestChatLog();
	}
	FILE *fp = _wfopen(filename, L"rb");
	if (!fp) return -1;
	ReadWholeFile(fp);
	while (WaitForSingleObject(param, 100) == WAIT_TIMEOUT) {
		while (fgetws(buffer, sizeof(buffer) / sizeof(WCHAR), fp)) {
			FormatChatLog(buffer);
			AppendText(buffer);
		}
		if (!IsLatestChatLog(filename)) { // day changed
			fclose(fp);
			filename = GetLatestChatLog();
			if (!filename) return -1;
			fp = _wfopen(filename, L"rb");
			if (!fp) return -1;
		}
	}
	fclose(fp);
	return 0;
}

int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
	HWND hDlg;
	MSG msg;
	BOOL ret;

	g_hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON1));
	g_hBitmap = LoadBitmapW(hInstance, MAKEINTRESOURCEW(IDB_BITMAP1));
	hDlg = CreateDialogParamW(hInstance, MAKEINTRESOURCEW(IDD_DIALOG1), 0, DialogProc, 0);
	ShowWindow(hDlg, SW_SHOWNORMAL);

	HANDLE hExitEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
	if (!hExitEvent) return -1;

	HANDLE hThread = CreateThread(0, 0, MainThread, hExitEvent, 0, 0);
	if (!hThread) return -1;

	while((ret = GetMessageW(&msg, 0, 0, 0)) != 0) {
		if(ret == -1)
			break;

		if (WaitForSingleObject(hThread, 0) != WAIT_TIMEOUT)
			break;

		if(!IsDialogMessageW(hDlg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}

	SetEvent(hExitEvent);
	if (WaitForSingleObject(hThread, 5000) != WAIT_OBJECT_0) {
		TerminateThread(hThread, 0);
	}

	CloseHandle(hThread);
	CloseHandle(hExitEvent);

	return ret;
}
