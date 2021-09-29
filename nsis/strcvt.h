#ifndef _NSIS_STRCVT_H
#define _NSIS_STRCVT_H

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//
// helper macros
//
#if !defined(UNICODE) && !defined(_UNICODE)
#define StrCvtT2W(pszInfo, cchInfo, pszData) StrCvtA2W(pszInfo, cchInfo, reinterpret_cast<LPCSTR>(pszData));
#define StrCvtW2T(pszInfo, cchInfo, pszData) StrCvtW2A(pszInfo, cchInfo, reinterpret_cast<LPCWCH>(pszData));
#define StrCvtT2A(pszInfo, cchInfo, pszData) lstrcpynA(pszInfo, reinterpret_cast<LPCSTR>(pszData), cchInfo);
#define StrCvtA2T(pszInfo, cchInfo, pszData) lstrcpynA(pszInfo, reinterpret_cast<LPCSTR>(pszData), cchInfo);
#else
#define StrCvtT2A(pszInfo, cchInfo, pszData) StrCvtW2A(pszInfo, cchInfo, reinterpret_cast<LPCWCH>(pszData));
#define StrCvtA2T(pszInfo, cchInfo, pszData) StrCvtA2W(pszInfo, cchInfo, reinterpret_cast<LPCSTR>(pszData));
#define StrCvtT2W(pszInfo, cchInfo, pszData) lstrcpynW(pszInfo, reinterpret_cast<LPCWCH>(pszData), cchInfo);
#define StrCvtW2T(pszInfo, cchInfo, pszData) lstrcpynW(pszInfo, reinterpret_cast<LPCWCH>(pszData), cchInfo);
#endif

//
// StrCvtW2A
//
static inline void StrCvtW2A(LPSTR pszInfo, int cchInfo, LPCWCH pszData)
{
	WideCharToMultiByte(CP_ACP, 0, pszData, -1, pszInfo, cchInfo, nullptr, nullptr);
}

//
// StrCvtA2W
//
static inline void StrCvtA2W(LPWCH pszInfo, int cchInfo, LPCSTR pszData)
{
	MultiByteToWideChar(CP_ACP, 0, pszData, -1, pszInfo, cchInfo);
}

#endif