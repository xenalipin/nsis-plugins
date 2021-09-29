#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <tchar.h>

#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#pragma comment( lib, "comctl32.lib" )

#include "plugin.h"

//
// PLUGINAPI
//
#if defined(__cplusplus)
#define PLUGINAPI(Name_) extern "C" __declspec(dllexport) void __cdecl Name_(HWND hwndParent, int nLength, LPTSTR variables, stack_t **stacktop, extra_parameters *extra, ...)
#else
#define PLUGINAPI(Name_) __declspec(dllexport) void __cdecl Name_(HWND hwndParent, int nLength, LPTSTR variables, stack_t **stacktop, extra_parameters *extra, ...)
#endif

//
// Global constants
//
static const char rgcLine[] = { '\r', '\n' };
static const UINT cchLine = ARRAYSIZE(rgcLine);

//
// Global variables
//
static HMODULE hModule = nullptr;
static HANDLE hStdOut = nullptr;

//
// PluginCallback
//
static UINT_PTR PluginCallback(NSPIM msg)
{
	return 0;
}

static LRESULT CALLBACK InstFilesProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	LRESULT lResult;

	if ((uMsg == WM_NOTIFY) && (reinterpret_cast<NMHDR *>(lParam)->code == LVN_INSERTITEM))
	{
		LPVOID pszData = calloc(dwRefData, sizeof(char));
		if (pszData != nullptr)
		{
			LVITEMA lvi = { 0 };

			DWORD cchRead;
			DWORD cchData;

			HWND hwndLV = reinterpret_cast<NMLISTVIEW *>(lParam)->hdr.hwndFrom;
			int iItem = reinterpret_cast<NMLISTVIEW *>(lParam)->iItem;

			lvi.cchTextMax = static_cast<int>(dwRefData);
			lvi.pszText = static_cast<char *>(pszData);

			cchRead = static_cast<DWORD>(SendMessage(hwndLV, LVM_GETITEMTEXTA, iItem, reinterpret_cast<LPARAM>(&lvi)));
			if (cchRead > 0)
			{
				WriteFile(hStdOut, pszData, cchRead, &cchData, nullptr);
				WriteFile(hStdOut, rgcLine, cchLine, &cchData, nullptr);
			}

			free(pszData);
		}
	}

	lResult = DefSubclassProc(hWnd, uMsg, wParam, lParam);

	return lResult;
}

PLUGINAPI(Start)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (AllocConsole())
	{
		hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleDisplayMode(
			hStdOut,
			CONSOLE_FULLSCREEN,
			nullptr);
	}
}

PLUGINAPI(Attach)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	HWND hwndDlg = FindWindowEx(hwndParent, nullptr, WC_DIALOG, nullptr);
	if (IsWindow(hwndDlg))
	{
		UINT_PTR uSubclassId = reinterpret_cast<UINT_PTR>(hwndParent);
		DWORD_PTR dwRefData = static_cast<DWORD_PTR>(nLength);
		SetWindowSubclass(
			hwndDlg,
			InstFilesProc,
			uSubclassId,
			dwRefData);
	}
}

PLUGINAPI(Detach)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	HWND hwndDlg = FindWindowEx(hwndParent, nullptr, WC_DIALOG, nullptr);
	if (IsWindow(hwndDlg))
	{
		UINT_PTR uSubclassId = reinterpret_cast<UINT_PTR>(hwndParent);
		RemoveWindowSubclass(
			hwndDlg,
			InstFilesProc,
			uSubclassId);
	}
}

PLUGINAPI(Stop)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	FreeConsole();
}

#if defined(_DEBUG)

int _tmain(int argc, _TCHAR *argv[])
{
	setlocale(LC_CTYPE, ".ACP");

	return !getchar();
}

#else

//
// DllMain
//
BOOL WINAPI _DllMainCRTStartup(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		hModule = (HMODULE)hinstDLL;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		return TRUE;
	default:
		return TRUE;
	}
}

#endif