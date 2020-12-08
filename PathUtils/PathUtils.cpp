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
#define PLUGINAPI(Name_) extern "C" __declspec(dllexport) void __cdecl Name_(HWND hwndParent, int nLength, LPTSTR variables, stack_t **stacktop, extra_parameters *extra, ...)

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
// tstring
//
using tstring = typename StringAutoT<TCHAR>::type;

//
// StringT
//
template <typename T>
	using StringT = typename StringAutoT<T>::type;

//
// ArrayT
//
template <typename T>
	using ArrayT = std::vector<T>;

//
// Global variables
//
static HMODULE hModule = nullptr;

static const TCHAR szPathKey[] = _T(R"(SYSTEM\CurrentControlSet\Control\Session Manager\Environment)");
static const TCHAR szPathName[] = _T("Path");

static const TCHAR szFmtHex[] = _T("0x%08X");
static const TCHAR szFmtDec[] = _T("%d");

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
	int nRet = CompareString(lcid, NORM_IGNORECASE, strPath1.data(), static_cast<int>(strPath1.size()), strPath2.data(), static_cast<int>(strPath2.size()));
	switch (nRet) {
	case CSTR_GREATER_THAN:
		return (((strPath2.size() + 1) == strPath1.size()) && (strPath1.back() == cBackslash) && (strPath2.back() != cBackslash));
	case CSTR_LESS_THAN:
		return (((strPath1.size() + 1) == strPath2.size()) && (strPath2.back() == cBackslash) && (strPath1.back() != cBackslash));
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
			DWORD cbRead = cbData;
			PVOID pvData = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbData);
			if (pvData != nullptr)
			{
				dwError = RegQueryValueEx(hKey, pszName, nullptr, pdwType, static_cast<BYTE *>(pvData), &cbRead);
				strData.assign(static_cast<PCTSTR>(pvData));
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
// ModifyPathData
//
static tstring &ModifyPathData(tstring &strData, const tstring &strPath, BOOL bAppend)
{
	tstring::size_type ulOffset = 0;
	tstring::size_type ulCurPos;

	bool bPresent = false;

	if (strData.back() != cSemicolon)
	{
		strData += cSemicolon;
	}

	while ((ulCurPos = strData.find(cSemicolon, ulOffset)) != tstring::npos)
	{
		tstring strItem(&strData[ulOffset], &strData[ulCurPos]);

		if (PathIsEqual(strItem, strPath))
		{
			if (bAppend && !bPresent)
			{
				strData.replace(ulOffset, strItem.size(), strPath);
				ulOffset += (strPath.size() + 1);
				bPresent = TRUE;
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

	if (bAppend && !bPresent)
	{
		strData.append(strPath);
		strData += cSemicolon;
	}

	return strData;
}

//
// ModifyPathRegistry
//
static HRESULT ModifyPathRegistry(const tstring &strPath, BOOL bAppend)
{
	HRESULT hr;

	DWORD dwError;
	HKEY hKey;

	dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPathKey, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey);
	if (dwError == ERROR_SUCCESS)
	{
		tstring strData;
		DWORD dwType;

		dwError = RegGetStringEx(hKey, szPathName, &dwType, strData);
		if (dwError == ERROR_SUCCESS)
		{
			dwError = RegSetStringEx(hKey, szPathName, dwType, ModifyPathData(strData, strPath, bAppend));
			if (dwError == ERROR_SUCCESS)
			{
				DWORD_PTR dwResult;
				if (!SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(_T("Environment")), SMTO_ABORTIFHUNG, 128, &dwResult))
				{
					hr = HRESULT_FROM_ERROR(GetLastError());
				}
				else
				{
					hr = S_OK;
				}
			}
			else
			{
				hr = HRESULT_FROM_ERROR(dwError);
			}
		}
		else
		{
			hr = HRESULT_FROM_ERROR(dwError);
		}

		RegCloseKey(hKey);
	}
	else
	{
		hr = HRESULT_FROM_ERROR(dwError);
	}

	return hr;
}

//
// ModifyPathWithStatus
//
static void ModifyPathWithStatus(LPTSTR pszData, int nData, const tstring &strPath, BOOL bAppend)
{
	HRESULT hr;

	if (!strPath.empty())
	{
		hr = ModifyPathRegistry(strPath, bAppend);
	}
	else
	{
		hr = E_INVALIDARG;
	}

	if (SUCCEEDED(hr))
	{
		wnsprintf(pszData, nData, szFmtDec, hr);
	}
	else
	{
		wnsprintf(pszData, nData, szFmtHex, hr);
	}
}

//
// ModifyPathFromNSIS
//
static void ModifyPathFromNSIS(stack_t **stacktop, int nLength, BOOL bAppend)
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
		ModifyPathWithStatus(th->text, nLength, strPath, bAppend);
		th->next = *stacktop;
		*stacktop = th;
	}
}

//
// VerifyPathData
//
static bool VerifyPathData(tstring &strData, const tstring &strPath)
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
// VerifyPathRegistry
//
static BOOL VerifyPathRegistry(const tstring &strPath)
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
			bResult = VerifyPathData(strData, strPath);
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
// VerifyPathWithStatus
//
static void VerifyPathWithStatus(LPTSTR pszData, int nData, const tstring &strPath)
{
	wnsprintf(pszData, nData, szFmtDec, VerifyPathRegistry(strPath));
}

//
// VerifyPathFromNSIS
//
static void VerifyPathFromNSIS(stack_t **stacktop, int nLength)
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
		VerifyPathWithStatus(th->text, nLength, strPath);
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
		ModifyPathFromNSIS(stacktop, nLength, TRUE);
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
		ModifyPathFromNSIS(stacktop, nLength, FALSE);
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
		VerifyPathFromNSIS(stacktop, nLength);
	}
}

#if _DEBUG

int _tmain(int argc, _TCHAR *argv[])
{
	TCHAR rgcInfo[256] = { 0 };
	DWORD cchInfo = ARRAYSIZE(rgcInfo);

	tstring strPath(_T(R"(D:\Git\Bin)"));

	tstring strData1(_T(R"(C:\Windows;C:\Windows\System32;D:\Git\bin\;D:\Git\bin;E:\GitHub;D:\git\bin\;D:\git\bin;F:\GitLab)"));
	ModifyPathData(strData1, strPath, FALSE);

	tstring strData2(_T(R"(C:\Windows;C:\Windows\System32;D:\Git\bin\;D:\Git\bin;E:\GitHub;D:\git\bin\;D:\git\bin;F:\GitLab)"));
	ModifyPathData(strData2, strPath, TRUE);

	tstring strData3(_T(R"(C:\Windows;C:\Windows\System32;E:\GitHub)"));
	ModifyPathData(strData3, strPath, FALSE);

	tstring strData4(_T(R"(C:\Windows;C:\Windows\System32;E:\GitHub)"));
	ModifyPathData(strData4, strPath, TRUE);

	ModifyPathWithStatus(rgcInfo, cchInfo, strPath, FALSE);
	ModifyPathWithStatus(rgcInfo, cchInfo, strPath, TRUE);

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