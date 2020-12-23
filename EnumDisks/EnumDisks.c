#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <tchar.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <shlwapi.h>
#pragma comment( lib, "shlwapi.lib" )
#include <setupapi.h>
#pragma comment( lib, "setupapi.lib" )
#include <devguid.h>
#pragma comment( lib, "ntdll.lib" )

#pragma comment(linker, "/align:128")

#include "plugin.h"

//
// PLUGINAPI
//
#if defined(__cplusplus)
#define PLUGINAPI(Name_) extern "C" void __cdecl Name_(HWND hwndParent, int nLength, LPTSTR variables, stack_t **stacktop, extra_parameters *extra, ...)
#else
#define PLUGINAPI(Name_) void __cdecl Name_(HWND hwndParent, int nLength, LPTSTR variables, stack_t **stacktop, extra_parameters *extra, ...)
#endif

//
// DiskInfoData
//
typedef struct _DiskInfoData {
	TCHAR szEnumeratorName[126 + 1];
	TCHAR szFriendlyName[126 + 1];
	TCHAR szSerialNumber[126 + 1];
	TCHAR szProductId[126 + 1];
	TCHAR szVendorId[126 + 1];
	INT64 llLength;
} DiskInfoData;

//
// DiskInfoProc
//
typedef void (CALLBACK *DiskInfoProc)(LPTSTR, int, const DiskInfoData *);

//
// DiskInfoEntry
//
typedef struct _DiskInfoEntry {
	SLIST_ENTRY ent;
	DiskInfoData di;
} DiskInfoEntry;

//
// DiskInfoProp
//
typedef struct _DiskInfoProp {
	DWORD dwProp;
	BYTE *pbData;
	DWORD cbData;
} DiskInfoProp;

//
// DiskInfoType
//
typedef enum _DiskInfoType {
	DEV_INFO_ENUM,
	DEV_INFO_NAME,
	DEV_INFO_SERN,
	DEV_INFO_PROD,
	DEV_INFO_VEND,
	DEV_INFO_SIZE,
} DiskInfoType;

//
// DiskInfoParam
//
typedef struct _DiskInfoParam {
	int nIndex;
	int nType;
} DiskInfoParam;

//
// Global variables
//
static SLIST_HEADER hdr;
static HMODULE hModule;
static HANDLE hHeap;

//
// PluginCallback
//
static UINT_PTR PluginCallback(NSPIM msg)
{
	return 0;
}

// IS_PTR
static BOOL IS_PTR(const void *ptr)
{
	return (ptr != NULL);
}

//
// DiskInfoStrCvtA2T
//
static int CALLBACK DiskInfoStrCvtA2T(LPTSTR pszInfo, int cchInfo, LPCVOID pvData)
{
#if defined(UNICODE) || defined(_UNICODE)
	return MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pvData, -1, pszInfo, cchInfo);
#else
	return lstrlenA(lstrcpynA(pszInfo, (LPCSTR)pvData, cchInfo));
#endif
}

//
// DiskInfoProperty
//
static BOOL CALLBACK DiskInfoProperty(HANDLE hFile, SP_DEVINFO_DATA *pspdd, DiskInfoData *pdi)
{
	BOOL bResult;

	DWORD cbData = 16 << 10U;
	BYTE *pbData = (BYTE *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbData);
	if (pbData != NULL)
	{
		STORAGE_PROPERTY_QUERY spq;
		DWORD cbRead;

		spq.PropertyId = StorageDeviceProperty;
		spq.QueryType = PropertyStandardQuery;

		bResult = DeviceIoControl(hFile, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(spq), pbData, cbData, &cbRead, NULL);
		if (bResult)
		{
			STORAGE_DEVICE_DESCRIPTOR *psdd = (STORAGE_DEVICE_DESCRIPTOR *)pbData;
			DWORD dwOffset;

			if ((dwOffset = psdd->SerialNumberOffset) > 0)
			{
				DiskInfoStrCvtA2T(pdi->szSerialNumber, ARRAYSIZE(pdi->szSerialNumber), &pbData[dwOffset]);
			}
			else
			{
				pdi->szSerialNumber[0] = _T('\0');
			}

			if ((dwOffset = psdd->ProductIdOffset) > 0)
			{
				DiskInfoStrCvtA2T(pdi->szProductId, ARRAYSIZE(pdi->szProductId), &pbData[dwOffset]);
			}
			else
			{
				pdi->szProductId[0] = _T('\0');
			}

			if ((dwOffset = psdd->VendorIdOffset) > 0)
			{
				DiskInfoStrCvtA2T(pdi->szVendorId, ARRAYSIZE(pdi->szVendorId), &pbData[dwOffset]);
			}
			else
			{
				pdi->szVendorId[0] = _T('\0');
			}

			bResult = DeviceIoControl(hFile, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, pbData, cbData, &cbRead, NULL);
			if (bResult)
			{
				LARGE_INTEGER *pSize = &((DISK_GEOMETRY_EX *)pbData)->DiskSize;
				pdi->llLength = pSize->QuadPart;
			}
		} while (0);

		HeapFree(hHeap, 0, pbData);
	}
	else
	{
		bResult = FALSE;
	}

	return bResult;
}

//
// DiskInfoInit
//
static void CALLBACK DiskInfoInit(const GUID *lpGuid, LPCTSTR pszEnum)
{
	HDEVINFO hDevInfo = SetupDiGetClassDevs(lpGuid, pszEnum, NULL, DIGCF_PRESENT);
	if (hDevInfo != INVALID_HANDLE_VALUE)
	{
		TCHAR szRoot[] = { _T('\\'), _T('\\'), _T('.'), _T('\\'), _T('G'), _T('l'), _T('o'), _T('b'), _T('a'), _T('l'), _T('R'), _T('o'), _T('o'), _T('t') };
		TCHAR szName[ARRAYSIZE(szRoot) + _MAX_FNAME];

		DiskInfoData di = { 0 };
		DiskInfoProp api[] = {
			{ SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, (BYTE *)&szName[ARRAYSIZE(szRoot)], sizeof(TCHAR) * _MAX_FNAME },
			{ SPDRP_ENUMERATOR_NAME, (BYTE *)di.szEnumeratorName, sizeof(di.szEnumeratorName) },
			{ SPDRP_FRIENDLYNAME, (BYTE *)di.szFriendlyName, sizeof(di.szFriendlyName) },
		};

		memcpy(szName, szRoot, sizeof(szRoot));
		RtlInitializeSListHead(&hdr);

		SP_DEVINFO_DATA spdd = { 0 };
		spdd.cbSize = sizeof(spdd);

		WORD wIndex = 0;

		while (SetupDiEnumDeviceInfo(hDevInfo, wIndex++, &spdd))
		{
			for (size_t i = 0; i < ARRAYSIZE(api); i++)
			{
				DiskInfoProp *ppi = (DiskInfoProp *)&api[i];
				DWORD dwType;
				DWORD cbRead;
				BOOL bResult = SetupDiGetDeviceRegistryProperty(hDevInfo, &spdd, ppi->dwProp, &dwType, ppi->pbData, ppi->cbData, &cbRead);
				if (!bResult)
				{
					break;
				}
			}

			HANDLE hDevice = CreateFile(szName, FILE_ANY_ACCESS, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
			if (hDevice != INVALID_HANDLE_VALUE)
			{
				BOOL bResult = DiskInfoProperty(hDevice, &spdd, &di);
				CloseHandle(hDevice);
				if (!bResult)
				{
					continue;
				}

				PVOID pvHeap = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(DiskInfoEntry));
				if (!IS_PTR(pvHeap))
				{
					continue;
				}

				DiskInfoEntry *pdie = (DiskInfoEntry *)pvHeap;
				memcpy(&pdie->di, &di, sizeof(di));
				RtlInterlockedPushEntrySList(&hdr, &pdie->ent);
			}
		}

		SetupDiDestroyDeviceInfoList(hDevInfo);
	}
}

//
// DiskDataBroker
//
static void CALLBACK DiskDataBroker(LPTSTR pszData, int nLength, DiskInfoData *pdi, int nType)
{
	switch (nType) {
	case DEV_INFO_ENUM:
		lstrcpyn(pszData, pdi->szEnumeratorName, nLength);
		break;
	case DEV_INFO_NAME:
		lstrcpyn(pszData, pdi->szFriendlyName, nLength);
		break;
	case DEV_INFO_SERN:
		lstrcpyn(pszData, pdi->szSerialNumber, nLength);
		break;
	case DEV_INFO_PROD:
		lstrcpyn(pszData, pdi->szProductId, nLength);
		break;
	case DEV_INFO_VEND:
		lstrcpyn(pszData, pdi->szVendorId, nLength);
		break;
	case DEV_INFO_SIZE:
		_sntprintf(pszData, nLength, _T("%I64d"), pdi->llLength);
		break;
	default:
		break;
	}
}

//
// DiskEntryBroker
//
static void CALLBACK DiskEntryBroker(LPTSTR pszData, int nLength, DiskInfoParam *pdip)
{
	SLIST_ENTRY *pent = RtlFirstEntrySList(&hdr);
	for (int nIndex = 0; (pent != NULL) && (nIndex < pdip->nIndex); nIndex++)
	{
		pent = pent->Next;
	}

	if (IS_PTR(pent))
	{
		DiskDataBroker(pszData, nLength, &((DiskInfoEntry *)pent)->di, pdip->nIndex);
	}
}

//
// DiskInfoBroker
//
static void CALLBACK DiskInfoBroker(stack_t **stacktop, int nLength, DiskInfoParam *pdip)
{
	if (*stacktop != NULL)
	{
		stack_t *th = *stacktop;
		pdip->nIndex = StrToInt(th->text);
		*stacktop = th->next;
		GlobalFree(th);
	}

	stack_t *th = (stack_t *)GlobalAlloc(GPTR, sizeof(stack_t) + nLength * sizeof(TCHAR));
	if (th != NULL)
	{
		DiskEntryBroker(th->text, nLength, pdip);
		th->next = *stacktop;
		*stacktop = th;
	}
}

//
// Init
//
PLUGINAPI(Init)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (stacktop != NULL)
	{
		if (*stacktop != NULL)
		{
			stack_t *th = *stacktop;
			DiskInfoInit(&GUID_DEVCLASS_DISKDRIVE, StrCmpC(th->text, _T("*")) != 0 ? th->text : NULL);
			*stacktop = th->next;
			GlobalFree(th);
		}
	}
}

//
// Count
//
PLUGINAPI(Count)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (stacktop != NULL)
	{
		stack_t *th = (stack_t *)GlobalAlloc(GPTR, sizeof(stack_t) + nLength * sizeof(TCHAR));
		if (th != NULL)
		{
			_sntprintf(th->text, nLength, _T("%hu"), RtlQueryDepthSList(&hdr));
			th->next = *stacktop;
			*stacktop = th;
		}
	}
}

//
// Clear
//
PLUGINAPI(Clear)
{
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (stacktop != NULL)
	{
		SLIST_ENTRY *pent;
		while (IS_PTR(pent = RtlInterlockedPopEntrySList(&hdr)))
		{
			RtlInterlockedFlushSList(&hdr);
			HeapFree(hHeap, 0, pent);
		}
	}
}

//
// GetEnumerator
//
PLUGINAPI(GetEnumerator)
{
	DiskInfoParam dip = { 0 };
	dip.nType = DEV_INFO_ENUM;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (stacktop != NULL)
	{
		DiskInfoBroker(stacktop, nLength, &dip);
	}
}

//
// GetFriendlyName
//
PLUGINAPI(GetFriendlyName)
{
	DiskInfoParam dip = { 0 };
	dip.nType = DEV_INFO_NAME;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (stacktop != NULL)
	{
		DiskInfoBroker(stacktop, nLength, &dip);
	}
}

//
// GetSerialNumber
//
PLUGINAPI(GetSerialNumber)
{
	DiskInfoParam dip = { 0 };
	dip.nType = DEV_INFO_SERN;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (stacktop != NULL)
	{
		DiskInfoBroker(stacktop, nLength, &dip);
	}
}

//
// GetProductId
//
PLUGINAPI(GetProductId)
{
	DiskInfoParam dip = { 0 };
	dip.nType = DEV_INFO_PROD;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (stacktop != NULL)
	{
		DiskInfoBroker(stacktop, nLength, &dip);
	}
}

//
// GetVendorId
//
PLUGINAPI(GetVendorId)
{
	DiskInfoParam dip = { 0 };
	dip.nType = DEV_INFO_VEND;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (stacktop != NULL)
	{
		DiskInfoBroker(stacktop, nLength, &dip);
	}
}

//
// GetDiskSize
//
PLUGINAPI(GetDiskSize)
{
	DiskInfoParam dip = { 0 };
	dip.nType = DEV_INFO_SIZE;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	if (stacktop != NULL)
	{
		DiskInfoBroker(stacktop, nLength, &dip);
	}
}

#if defined(_DEBUG)

int _tmain( int argc, _TCHAR *argv[] )
{
	TCHAR rgcInfo[64] = { 0 };
	DWORD cchInfo = ARRAYSIZE(rgcInfo);

	SLIST_ENTRY *curr;
	SLIST_ENTRY *next;

	hHeap = GetProcessHeap();

	if (argc > 1)
	{
		SetupDiInitDeviceInfo(&GUID_DEVCLASS_DISKDRIVE, StrCmpC(argv[1], _T("*")) != 0 ? argv[1] : NULL);
	}
	else
	{
		SetupDiInitDeviceInfo(&GUID_DEVCLASS_DISKDRIVE, NULL);
	}

	for (curr = RtlFirstEntrySList(&hdr); curr != NULL; curr = next)
	{
		DiskInfoEntry *pdie = (DiskInfoEntry *)curr;

		_tprintf(_T("============================================================\n"));
		_tprintf(_T("友好名称: [%s]\n"), pdie->di.szFriendlyName);
		_tprintf(_T("产品序列: [%s]\n"), pdie->di.szSerialNumber);
		_tprintf(_T("产品标识: [%s]\n"), pdie->di.szProductId);
		_tprintf(_T("厂家标识: [%s]\n"), pdie->di.szVendorId);
		_sntprintf(rgcInfo, cchInfo, _T("%I64d"), pdie->di.llLength);
		_tprintf(_T("磁盘大小: [%s]\n"), rgcInfo);

		next = curr->Next;

		HeapFree(hHeap, 0, curr);
	}

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
		hHeap = GetProcessHeap();
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		return TRUE;
	default:
		return TRUE;
	}
}

#endif