#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <tchar.h>

#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#pragma comment( lib, "shlwapi.lib" )

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
// StringAutoT
//
template <typename CharT> struct StringAutoT
{
};

template <> struct StringAutoT<WCHAR>
{
	typedef std::wstring type;
};

template <> struct StringAutoT<char>
{
	typedef std::string type;
};

//
// StringT
//
template <typename T> using StringT = typename StringAutoT<T>::type;

//
// tstring
//
typedef typename StringAutoT<TCHAR>::type tstring;

//
// Global variables
//
static HMODULE hModule = nullptr;

static const TCHAR szPathKey[] = _T(R"(SYSTEM\CurrentControlSet\Control\Session Manager\Environment)");
static const TCHAR szPathName[] = _T("Path");

static const TCHAR szFormat[] = _T("%d");

static const TCHAR cBackslash = _T('\\');
static const TCHAR cSemicolon = _T(';');

//
// PluginCallback
//
static UINT_PTR PluginCallback(NSPIM msg)
{
	return 0;
}

//
// HRESULT_FROM_ERROR
//
static inline HRESULT HRESULT_FROM_ERROR(DWORD dwError)
{
	return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwError);
}

//
// PathIsEqual
//
static bool PathIsEqual(const tstring &strPath1, const tstring &strPath2)
{
	LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
	auto data1 = strPath1.data();
	auto size1 = strPath1.size();
	auto back1 = strPath1.back();
	auto data2 = strPath2.data();
	auto size2 = strPath2.size();
	auto back2 = strPath2.back();
	int nRet = CompareString(lcid, NORM_IGNORECASE, data1, static_cast<int>(size1), data2, static_cast<int>(size2));
	switch (nRet) {
	case CSTR_GREATER_THAN:
		return (((size2 + 1) == size1) && (back1 == cBackslash) && (back2 != cBackslash));
	case CSTR_LESS_THAN:
		return (((size1 + 1) == size2) && (back2 == cBackslash) && (back1 != cBackslash));
	case CSTR_EQUAL:
		return true;
	default:
		return false;
	}
}

//
// RegGetStringEx
//
static DWORD RegGetStringEx(HKEY hKey, LPCTSTR pszName, DWORD *pdwType, tstring &strData)
{
	DWORD dwError;
	DWORD cbData;
	DWORD dwType;

	dwError = RegQueryValueEx(hKey, pszName, nullptr, &dwType, nullptr, &cbData);
	if (dwError == ERROR_SUCCESS)
	{
		if ((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ))
		{
			HANDLE hHeap = GetProcessHeap();
			PVOID pvData = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbData);
			DWORD cbRead = cbData;
			if (pvData != nullptr)
			{
				dwError = RegQueryValueEx(hKey, pszName, nullptr, pdwType, static_cast<BYTE *>(pvData), &cbRead);
				strData.assign(static_cast<LPCTSTR>(pvData));
				HeapFree(hHeap, 0, pvData);
			}
			else
			{
				dwError = ERROR_OUTOFMEMORY;
			}
		}
		else
		{
			dwError = ERROR_FILE_NOT_FOUND;
		}
	}

	return dwError;
}

//
// RegSetStringEx
//
static DWORD RegSetStringEx(HKEY hKey, LPCTSTR pszName, DWORD dwType, const tstring &strData)
{
	const BYTE *pbData = reinterpret_cast<const BYTE *>(strData.c_str());
	const DWORD cbData = static_cast<DWORD>(strData.size()) * sizeof(TCHAR) + sizeof(TCHAR);
	return RegSetValueEx(hKey, szPathName, 0, dwType, pbData, cbData);
}

//
// namespace Paths
//
namespace Paths
{

//
// UpdatePathItem
//
static tstring &UpdatePathItem(tstring &strData, const tstring &strPath, BOOL fAppend)
{
	tstring::size_type ulOffset = 0;
	tstring::size_type ulCurPos;

	bool fExisted = false;

	if (strData.back() != cSemicolon)
	{
		strData += cSemicolon;
	}

	while ((ulCurPos = strData.find(cSemicolon, ulOffset)) != tstring::npos)
	{
		tstring strItem(&strData[ulOffset], &strData[ulCurPos]);

		if (PathIsEqual(strItem, strPath))
		{
			if (fAppend && !fExisted)
			{
				strData.replace(ulOffset, strItem.size(), strPath);
				ulOffset += (strPath.size() + 1);
				fExisted = true;
			}
			else
			{
				strData.erase(ulOffset, (strItem.size() + 1));
			}
		}
		else
		{
			ulOffset = (ulCurPos + 1);
		}
	}

	if (fAppend && !fExisted)
	{
		strData.append(strPath);
		strData += cSemicolon;
	}

	return strData;
}

//
// UpdateRegistry
//
static DWORD UpdateRegistry(const tstring &strPath, BOOL bAppend)
{
	DWORD dwError;
	HKEY hSubKey;

	dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPathKey, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hSubKey);
	if (dwError == ERROR_SUCCESS)
	{
		DWORD_PTR dwResult;
		tstring strData;
		DWORD dwType;

		dwError = RegGetStringEx(hSubKey, szPathName, &dwType, strData);
		if (dwError == ERROR_SUCCESS)
		{
			dwError = RegSetStringEx(hSubKey, szPathName, dwType, UpdatePathItem(strData, strPath, bAppend));
			if (dwError == ERROR_SUCCESS)
			{
				if (!SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(_T("Environment")), SMTO_ABORTIFHUNG, 128, &dwResult))
				{
					dwError = GetLastError();
				}
			}
		}

		RegCloseKey(hSubKey);
	}

	return dwError;
}

//
// UpdateResult
//
static void UpdateResult(LPTSTR pszData, int cchData, const tstring &strPath, BOOL bAppend)
{
	wnsprintf(pszData, cchData, szFmtHex, UpdateRegistry(strPath, bAppend));
}

//
// VerifyPathItem
//
static bool VerifyPathItem(tstring &strData, const tstring &strPath)
{
	tstring::size_type ulOffset = 0;
	tstring::size_type ulCurPos;

	bool bPresent = false;

	if (strData.back() != cSemicolon)
	{
		strData += cSemicolon;
	}

	while (!bPresent && (ulCurPos = strData.find(cSemicolon, ulOffset)) != tstring::npos)
	{
		tstring strItem(&strData[ulOffset], &strData[ulCurPos]);
		bPresent = PathIsEqual(strItem, strPath);
		if (!bPresent)
		{
			ulOffset = (ulCurPos + 1);
		}
	}

	return bPresent;
}

//
// VerifyRegistry
//
static BOOL VerifyRegistry(const tstring &strPath)
{
	BOOL bResult;

	DWORD dwError;
	HKEY hKey;

	dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPathKey, 0, KEY_QUERY_VALUE, &hKey);
	if (dwError == ERROR_SUCCESS)
	{
		tstring strData;

		dwError = RegGetStringEx(hKey, szPathName, nullptr, strData);
		if (dwError == ERROR_SUCCESS)
		{
			bResult = VerifyPathItem(strData, strPath);
		}
		else
		{
			bResult = FALSE;
		}

		RegCloseKey(hKey);
	}
	else
	{
		bResult = FALSE;
	}

	return bResult;
}

//
// VerifyResult
//
static void VerifyResult(LPTSTR pszData, int cchData, const tstring &strPath)
{
	wnsprintf(pszData, cchData, szFormat, VerifyRegistry(strPath));
}

}

//
// PathsUpdateImpl
//
static void PathsUpdateImpl(stack_t **stacktop, int nLength, BOOL bAppend)
{
	tstring strPath;

	if (*stacktop != nullptr)
	{
		stack_t *th = *stacktop;
		strPath.assign(th->text);
		*stacktop = th->next;
		GlobalFree(th);
	}

	stack_t *th = (stack_t *)GlobalAlloc(GPTR, (sizeof(stack_t) + nLength * sizeof(TCHAR)));
	if (th != nullptr)
	{
		Paths::UpdateResult(th->text, nLength, strPath, bAppend);
		th->next = *stacktop;
		*stacktop = th;
	}
}

//
// PathsVerifyImpl
//
static void PathsVerifyImpl(stack_t **stacktop, int nLength)
{
	tstring strPath;

	if (*stacktop != nullptr)
	{
		stack_t *th = *stacktop;
		strPath.assign(th->text);
		*stacktop = th->next;
		GlobalFree(th);
	}

	stack_t *th = (stack_t *)GlobalAlloc(GPTR, (sizeof(stack_t) + nLength * sizeof(TCHAR)));
	if (th != nullptr)
	{
		Paths::VerifyResult(th->text, nLength, strPath);
		th->next = *stacktop;
		*stacktop = th;
	}
}

//
// Append
//
PLUGINAPI(Append)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (stacktop != nullptr)
	{
		PathsUpdateImpl(stacktop, nLength, TRUE);
	}
}

//
// Remove
//
PLUGINAPI(Remove)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (stacktop != nullptr)
	{
		PathsUpdateImpl(stacktop, nLength, FALSE);
	}
}

//
// Exists
//
PLUGINAPI(Exists)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (stacktop != nullptr)
	{
		PathsVerifyImpl(stacktop, nLength);
	}
}

#if defined(_DEBUG)

int _tmain(int argc, _TCHAR *argv[])
{
	TCHAR rgcInfo[256] = { 0 };
	DWORD cchInfo = ARRAYSIZE(rgcInfo);

	tstring strPath(_T(R"(D:\Git\Bin)"));

	tstring strData1(_T(R"(C:\Windows;C:\Windows\System32;D:\Git\bin\;D:\Git\bin;E:\GitHub;D:\git\bin\;D:\git\bin;F:\GitLab)"));
	Paths::UpdatePathItem(strData1, strPath, FALSE);

	tstring strData2(_T(R"(C:\Windows;C:\Windows\System32;D:\Git\bin\;D:\Git\bin;E:\GitHub;D:\git\bin\;D:\git\bin;F:\GitLab)"));
	Paths::UpdatePathItem(strData2, strPath, TRUE);

	tstring strData3(_T(R"(C:\Windows;C:\Windows\System32;E:\GitHub)"));
	Paths::UpdatePathItem(strData3, strPath, FALSE);

	tstring strData4(_T(R"(C:\Windows;C:\Windows\System32;E:\GitHub)"));
	Paths::UpdatePathItem(strData4, strPath, TRUE);

	Paths::UpdateResult(rgcInfo, cchInfo, strPath, FALSE);
	Paths::UpdateResult(rgcInfo, cchInfo, strPath, TRUE);

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