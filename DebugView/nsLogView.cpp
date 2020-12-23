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
// Global variables
//
static HMODULE hModule = nullptr;

//
// PluginCallback
//
static UINT_PTR PluginCallback(NSPIM msg)
{
	return 0;
}

static LRESULT CALLBACK InstFilesProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	NMHDR *pnmh = reinterpret_cast<NMHDR *>(lParam);

	if ((uMsg == WM_NOTIFY) && (pnmh->code == LVN_INSERTITEM))
	{
		static TCHAR rgcLine[] = { _T('\r'), _T('\n') };
		static DWORD cchLine = ARRAYSIZE(rgcLine);
		static DWORD cchData = 0;

		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

		NMLISTVIEW *pnmlv = reinterpret_cast<NMLISTVIEW *>(pnmh);

		HWND hwndLV = pnmlv->hdr.hwndFrom;
		int iItem = pnmlv->iItem;

		std::vector<TCHAR> vInfo(dwRefData);
		PTSTR pszInfo = static_cast<PTSTR>(vInfo.data());
		DWORD cchInfo = static_cast<DWORD>(vInfo.size());

		LVITEM lvi = { 0 };
		lvi.cchTextMax = cchInfo;
		lvi.pszText = pszInfo;

		DWORD cchRead = (DWORD)SendMessage(hwndLV, LVM_GETITEMTEXT, iItem, reinterpret_cast<LPARAM>(&lvi));
		if (cchRead > 0)
		{
			WriteConsole(hStdOut, pszInfo, cchRead, &cchData, nullptr);
			WriteConsole(hStdOut, rgcLine, cchLine, &cchData, nullptr);
		}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

PLUGINAPI(Start)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (AllocConsole())
	{
		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD dwMode = CONSOLE_FULLSCREEN;
		COORD coord = { 0 };
		SetConsoleDisplayMode(hStdOut, dwMode, &coord);
	}
}

PLUGINAPI(Attach)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	HWND hwndDlg = FindWindowEx(hwndParent, nullptr, WC_DIALOG, nullptr);
	UINT_PTR uSubclassId = reinterpret_cast<UINT_PTR>(hwndParent);
	SetWindowSubclass(hwndDlg, InstFilesProc, uSubclassId, nLength);
}

PLUGINAPI(Detach)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	HWND hwndDlg = FindWindowEx(hwndParent, nullptr, WC_DIALOG, nullptr);
	UINT_PTR uSubclassId = reinterpret_cast<UINT_PTR>(hwndParent);
	RemoveWindowSubclass(hwndDlg, InstFilesProc, uSubclassId);
}

PLUGINAPI(Stop)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	FreeConsole();
}

#if _DEBUG

int _tmain(int argc, _TCHAR *argv[])
{
	setlocale(LC_CTYPE, ".ACP");

	return !getchar();
}

#else

//
// DllMain
//
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
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