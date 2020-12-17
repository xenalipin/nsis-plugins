#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#pragma comment(linker, "/align:128")

#define INITGUID
#include "wmium.h"
#include "wmidata.h"
#include "wmiguid.h"
#pragma comment(lib, "wmiapi.lib")
#include "smbios.h"

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
// SMBIOS_FIELD_TYPE
//
typedef enum _SMBIOS_FIELD_TYPE {
	SMBFT_COMMON_STRING,
	SMBFT_COMMON_DWORD,
	SMBFT_COMMON_WORD,
	SMBFT_TYPE01_UUID,
	SMBFT_TYPE04_PROCESSOR,
	SMBFT_TYPE16_CAPACITY,
	SMBFT_TYPE17_SIZE,
} SMBIOS_FIELD_TYPE;

//
// SMBIOS_DATA_FLAGS
//
typedef enum _SMBIOS_DATA_FLAGS {
	SMBDF_INDEX = 0x00000001,
} SMBIOS_DATA_FLAGS;

//
// SMBIOS_DATA_PARAM
//
typedef struct _SMBIOS_DATA_PARAM {
	UINT uFlags;
	LONG nIndex;
	BYTE nType;
	BYTE nLength;
	BYTE nData;
	BYTE nOffset;
} SMBIOS_DATA_PARAM;

//
// Global variables
//
static HMODULE hModule = NULL;

//
// PluginCallback
//
static UINT_PTR PluginCallback(NSPIM msg)
{
	return 0;
}

//
// FieldHandler_Capacity
//
DECLSPEC_NOINLINE static LONG CALLBACK FieldHandler_Capacity(SMBIOS_HEADER *pHeader)
{
	PHYSICAL_MEMORY_ARRAY_INFORMATION *ppmai = (PHYSICAL_MEMORY_ARRAY_INFORMATION *)pHeader;
	if ((ppmai->Length >= SMBIOS_TYPE16_SIZE_0207) && (ppmai->MaximumCapacity == 0x80000000UL))
	{
		return (LONG)(ppmai->ExtendedMaximumCapacity >> 20);
	}
	else
	{
		return (LONG)(ppmai->MaximumCapacity >> 10);
	}
}

//
// FieldHandler_Size
//
DECLSPEC_NOINLINE static LONG CALLBACK FieldHandler_Size(SMBIOS_HEADER *pHeader)
{
	MEMORY_DEVICE_INFORMATION *pmdi = (MEMORY_DEVICE_INFORMATION *)pHeader;
	switch (pmdi->Size) {
	case 0xFFFFU:
		return (LONG)(SHORT)pmdi->Size;
	case 0x0U:
		return (LONG)pmdi->Size;
	default:
		if ((pmdi->Length >= SMBIOS_TYPE17_SIZE_0207) && (pmdi->Size == 0x7FFFU))
		{
			return (LONG)(pmdi->ExtendedSize & 0x7FFFFFFFU);
		}
		if (pmdi->Size & 0x8000U)
		{
			return (LONG)(pmdi->Size & 0x7FFFU) >> 10U;
		}
		else
		{
			return (LONG)(pmdi->Size & 0x7FFFU);
		}
	}
}

//
// FieldHandler_String
//
DECLSPEC_NOINLINE static void CALLBACK FieldHandler_String(LPTSTR pszData, DWORD cchData, SMBIOS_HEADER *pHeader, BYTE nOffset)
{
	BYTE nField = *((BYTE *)pHeader + nOffset);
	BYTE *pbInfo = (BYTE *)pHeader + pHeader->Length;
	BYTE nIndex = 0;
	while ((nIndex < nField) && *pbInfo)
	{
		if (++nIndex == nField)
		{
			wnsprintf(pszData, cchData, TEXT("%hs"), pbInfo);
			break;
		}
		BYTE bData;
		do {
			bData = *pbInfo++;
		} while (bData);
	}
}

//
// FieldHandler
//
DECLSPEC_NOINLINE static void CALLBACK FieldHandler(LPTSTR pszData, DWORD cchData, SMBIOS_HEADER *pHeader, BYTE nLength, BYTE nType, BYTE nOffset)
{
	if (pHeader->Length >= nLength)
	{
		ULARGE_INTEGER pid;
		UUID *puuid;
		switch (nType) {
		case SMBFT_TYPE01_UUID:
			puuid = (UUID *)((SMBIOS_SYSTEM_INFORMATION *)pHeader)->UUID;
			wnsprintf(pszData, cchData,
				TEXT("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"),
				puuid->Data1, puuid->Data2, puuid->Data3,
				puuid->Data4[0], puuid->Data4[1],
				puuid->Data4[2], puuid->Data4[3],
				puuid->Data4[4], puuid->Data4[5],
				puuid->Data4[6], puuid->Data4[7]);
			break;
		case SMBFT_TYPE04_PROCESSOR:
			pid.QuadPart = ((SMBIOS_PROCESSOR_INFORMATION *)pHeader)->ProcessorID;
			wnsprintf(pszData, cchData, TEXT("%08X%08X"), pid.HighPart, pid.LowPart);
			break;
		case SMBFT_TYPE16_CAPACITY:
			wnsprintf(pszData, cchData, TEXT("%ld"), FieldHandler_Capacity(pHeader));
			break;
		case SMBFT_TYPE17_SIZE:
			wnsprintf(pszData, cchData, TEXT("%ld"), FieldHandler_Size(pHeader));
			break;
		case SMBFT_COMMON_DWORD:
			wnsprintf(pszData, cchData, TEXT("%lu"), *(DWORD *)((BYTE *)pHeader + nOffset));
			break;
		case SMBFT_COMMON_WORD:
			wnsprintf(pszData, cchData, TEXT("%hu"), *(WORD *)((BYTE *)pHeader + nOffset));
			break;
		case SMBFT_COMMON_STRING:
			FieldHandler_String(pszData, cchData, pHeader, nOffset);
			break;
		default:
			break;
		}
	}
}

//
// FieldBroker
//
DECLSPEC_NOINLINE static void CALLBACK FieldBroker(LPTSTR pszData, DWORD cchData, BYTE *pbData, DWORD cbData, const SMBIOS_DATA_PARAM *psdp)
{
	BYTE *pbInfo = pbData;
	BYTE nIndex = 0;

	while (pbInfo < &pbData[cbData])
	{
		SMBIOS_HEADER *pHeader = (SMBIOS_HEADER *)pbInfo;

		if (pHeader->Type == psdp->nType && (nIndex++ == psdp->nIndex))
		{
			FieldHandler(pszData, cchData, pHeader, psdp->nLength, psdp->nData, psdp->nOffset);
			break;
		}

		pbInfo += pHeader->Length;

		do {
			BYTE bData;
			do {
				bData = *pbInfo++;
			} while (bData);
		} while (*pbInfo);

		pbInfo++;
	}
}

//
// DataHandler
//
DECLSPEC_NOINLINE static void CALLBACK DataHandler(LPTSTR pszData, DWORD cchData, const SMBIOS_DATA_PARAM *psdp)
{
	WMIHANDLE hBlock;
	ULONG nError = WmiOpenBlock((GUID *)&MSSmBios_RawSMBiosTables_GUID, WMIGUID_QUERY, &hBlock);
	if (nError == NO_ERROR)
	{
		BOOL bResult = FALSE;
		HANDLE hHeap = GetProcessHeap();
		BYTE *pbBlock = NULL;
		DWORD cbBlock = 0;

		do {
			MSSmBios_RawSMBiosTables *pRawData;

			nError = WmiQueryAllData(hBlock, &cbBlock, pbBlock);
			switch (nError) {
			case ERROR_INSUFFICIENT_BUFFER:
				pbBlock = (BYTE *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbBlock);
				break;

			case NO_ERROR:
				pRawData = (MSSmBios_RawSMBiosTables *)(pbBlock + ((WNODE_ALL_DATA *)pbBlock)->DataBlockOffset);
				FieldBroker(pszData, cchData, pRawData->SMBiosData, pRawData->Size, psdp);
				HeapFree(hHeap, 0, pbBlock);
				bResult = TRUE;
				break;

			default:
				break;
			}
		} while (!bResult);

		WmiCloseBlock(hBlock);
	}
}

//
// DataBroker
//
DECLSPEC_NOINLINE static void CALLBACK DataBroker(stack_t **stacktop, int nLength, SMBIOS_DATA_PARAM *psdp)
{
	if (stacktop != NULL)
	{
		if ((psdp->uFlags & SMBDF_INDEX) && (*stacktop != NULL))
		{
			stack_t *th = *stacktop;
			psdp->nIndex = StrToInt(th->text);
			*stacktop = th->next;
			GlobalFree(th);
		}
		stack_t *th = (stack_t *)GlobalAlloc(GPTR, sizeof(stack_t) + nLength * sizeof(TCHAR));
		if (th != NULL)
		{
			DataHandler(th->text, nLength, psdp);
			th->next = *stacktop;
			*stacktop = th;
		}
	}
}

//
// GetSystemManufacturer
//
PLUGINAPI(GetSystemManufacturer)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.nType = SystemInfo;
	sdp.nLength = SMBIOS_TYPE01_SIZE_0200;
	sdp.nData = SMBFT_COMMON_STRING;
	sdp.nOffset = FIELD_OFFSET(SMBIOS_SYSTEM_INFORMATION, Manufacturer);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

//
// GetSystemSerialNumber
//
PLUGINAPI(GetSystemSerialNumber)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.nType = SystemInfo;
	sdp.nLength = SMBIOS_TYPE01_SIZE_0200;
	sdp.nData = SMBFT_COMMON_STRING;
	sdp.nOffset = FIELD_OFFSET(SMBIOS_SYSTEM_INFORMATION, SerialNumber);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

//
// GetSystemProductName
//
PLUGINAPI(GetSystemProductName)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.nType = SystemInfo;
	sdp.nLength = SMBIOS_TYPE01_SIZE_0200;
	sdp.nData = SMBFT_COMMON_STRING;
	sdp.nOffset = FIELD_OFFSET(SMBIOS_SYSTEM_INFORMATION, ProductName);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

//
// GetSystemUUID
//
PLUGINAPI(GetSystemUUID)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.nType = SystemInfo;
	sdp.nLength = SMBIOS_TYPE01_SIZE_0201;
	sdp.nData = SMBFT_TYPE01_UUID;
	sdp.nOffset = FIELD_OFFSET(SMBIOS_SYSTEM_INFORMATION, UUID);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

//
// GetSystemFamily
//
PLUGINAPI(GetSystemFamily)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.nType = SystemInfo;
	sdp.nLength = SMBIOS_TYPE01_SIZE_0204;
	sdp.nData = SMBFT_COMMON_STRING;
	sdp.nOffset = FIELD_OFFSET(SMBIOS_SYSTEM_INFORMATION, Family);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

//
// GetModuleManufacturer
//
PLUGINAPI(GetModuleManufacturer)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.nType = BaseboardInfo;
	sdp.nLength = FIELD_OFFSET(SMBIOS_BASEBOARD_INFORMATION, Product);
	sdp.nData = SMBFT_COMMON_STRING;
	sdp.nOffset = FIELD_OFFSET(SMBIOS_BASEBOARD_INFORMATION, Manufacturer);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

//
// GetModuleSerialNumber
//
PLUGINAPI(GetModuleSerialNumber)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.nType = BaseboardInfo;
	sdp.nLength = FIELD_OFFSET(SMBIOS_BASEBOARD_INFORMATION, AssetTag);
	sdp.nData = SMBFT_COMMON_STRING;
	sdp.nOffset = FIELD_OFFSET(SMBIOS_BASEBOARD_INFORMATION, SerialNumber);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

//
// GetModuleProductName
//
PLUGINAPI(GetModuleProductName)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.nType = BaseboardInfo;
	sdp.nLength = FIELD_OFFSET(SMBIOS_BASEBOARD_INFORMATION, Version);
	sdp.nData = SMBFT_COMMON_STRING;
	sdp.nOffset = FIELD_OFFSET(SMBIOS_BASEBOARD_INFORMATION, Product);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

//
// GetProcessorID
//
PLUGINAPI(GetProcessorID)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.nType = ProcessorInfo;
	sdp.nLength = SMBIOS_TYPE04_SIZE_0200;
	sdp.nData = SMBFT_TYPE04_PROCESSOR;
	sdp.nOffset = FIELD_OFFSET(SMBIOS_PROCESSOR_INFORMATION, ProcessorID);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

//
// GetMemoryArrayCapacity
//
PLUGINAPI(GetMemoryArrayCapacity)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.nType = PhysicalMemoryArrayInfo;
	sdp.nLength = SMBIOS_TYPE16_SIZE_0201;
	sdp.nData = SMBFT_TYPE16_CAPACITY;
	sdp.nOffset = FIELD_OFFSET(PHYSICAL_MEMORY_ARRAY_INFORMATION, MaximumCapacity);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

#if 1

//
// GetMemoryArrayNumbers
//
PLUGINAPI(GetMemoryArrayNumbers)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.nType = PhysicalMemoryArrayInfo;
	sdp.nLength = SMBIOS_TYPE16_SIZE_0201;
	sdp.nData = SMBFT_COMMON_WORD;
	sdp.nOffset = FIELD_OFFSET(PHYSICAL_MEMORY_ARRAY_INFORMATION, NumberOfMemoryDevices);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

#endif

//
// GetMemoryManufacturer
//
PLUGINAPI(GetMemoryManufacturer)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.uFlags = SMBDF_INDEX;
	sdp.nType = MemoryDeviceInfo;
	sdp.nLength = SMBIOS_TYPE17_SIZE_0203;
	sdp.nData = SMBFT_COMMON_STRING;
	sdp.nOffset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, Manufacturer);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

//
// GetMemorySerialNumber
//
PLUGINAPI(GetMemorySerialNumber)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.uFlags = SMBDF_INDEX;
	sdp.nType = MemoryDeviceInfo;
	sdp.nLength = SMBIOS_TYPE17_SIZE_0203;
	sdp.nData = SMBFT_COMMON_STRING;
	sdp.nOffset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, SerialNumber);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

//
// GetMemorySpeed
//
PLUGINAPI(GetMemorySpeed)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.uFlags = SMBDF_INDEX;
	sdp.nType = MemoryDeviceInfo;
	sdp.nLength = SMBIOS_TYPE17_SIZE_0203;
	sdp.nData = SMBFT_COMMON_WORD;
	sdp.nOffset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, Speed);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

//
// GetMemorySize
//
PLUGINAPI(GetMemorySize)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.uFlags = SMBDF_INDEX;
	sdp.nType = MemoryDeviceInfo;
	sdp.nLength = SMBIOS_TYPE17_SIZE_0201;
	sdp.nData = SMBFT_TYPE17_SIZE;
	sdp.nOffset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, Size);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	DataBroker(stacktop, nLength, &sdp);
}

#if defined(_DEBUG)

int _tmain(int argc, _TCHAR *argv[])
{
	TCHAR rgcData[1024] = { 0 };
	DWORD cchData = ARRAYSIZE(rgcData);
	HANDLE hFile;
	BOOL bResult;

	SHGetFolderPath(HWND_DESKTOP, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, rgcData);
	bResult = PathAppend(rgcData, TEXT("smbios.dat"));
	hFile = CreateFile(rgcData, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD cbData = GetFileSize(hFile, NULL);
		BYTE *pbData = (BYTE *)LocalAlloc(LPTR, cbData);
		DWORD cbRead;
		bResult = ReadFile(hFile, pbData, cbData, &cbRead, NULL);
		CloseHandle(hFile);
		if (bResult)
		{
			SMBIOS_DATA_PARAM sdp = { 0 };
			sdp.nIndex = 0;
			sdp.nType = MemoryDeviceInfo;
			sdp.nLength = SMBIOS_TYPE17_SIZE_0201;
			sdp.nData = SMBFT_TYPE17_SIZE;
			sdp.nOffset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, Size);
			FieldBroker(rgcData, cchData, pbData, cbData, &sdp);
		}
	}

	return 0;
}

#else

BOOL WINAPI _DllMainCRTStartup(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		hModule = hinstDLL;
	default:
		return TRUE;
	}
}

#endif
