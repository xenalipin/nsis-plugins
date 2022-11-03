﻿#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#define INITGUID
#include "wmiguid.h"
#include "wmidata.h"
#include "wmium.h"
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
	SMBFT_TYPE17_SPEED,
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
	WORD wFlags;
	BYTE nIndex;
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
// smbios_read_capacity
//
DECLSPEC_NOINLINE static LONG CALLBACK smbios_read_capacity(SMBIOS_HEADER *pHeader)
{
	PHYSICAL_MEMORY_ARRAY_INFORMATION *ppmai = (PHYSICAL_MEMORY_ARRAY_INFORMATION *)pHeader;
	if ((ppmai->Length >= SMBIOS_TYPE16_SIZE_0207) && (ppmai->MaximumCapacity == 0x80000000UL))
	{
		return (LONG)(ppmai->ExtendedMaximumCapacity >> 20UL);
	}
	else
	{
		return (LONG)(ppmai->MaximumCapacity >> 10UL);
	}
}

//
// smbios_read_speed
//
DECLSPEC_NOINLINE static LONG CALLBACK smbios_read_speed(SMBIOS_HEADER *pHeader)
{
	MEMORY_DEVICE_INFORMATION *pmdi = (MEMORY_DEVICE_INFORMATION *)pHeader;
	if ((pmdi->Length >= SMBIOS_TYPE17_SIZE_0303) && (pmdi->Speed == 0xFFFFU))
	{
		return (LONG)(pmdi->ExtendedSpeed & 0x7FFFFFFFUL);
	}
	else
	{
		return (LONG)(pmdi->Speed);
	}
}

//
// smbios_read_size
//
DECLSPEC_NOINLINE static LONG CALLBACK smbios_read_size(SMBIOS_HEADER *pHeader)
{
	MEMORY_DEVICE_INFORMATION *pmdi = (MEMORY_DEVICE_INFORMATION *)pHeader;
	switch (pmdi->Size) {
	case 0xFFFFU:
		// if the size is unknown, the field value is FFFFh.
		return (LONG)(SHORT)pmdi->Size;
	case 0x0U:
		// if the value is 0, no memory device is installed in the socket.
		return (LONG)pmdi->Size;
	default:
		// if the size is 32 GB-1 MB or greater, the field value is 7FFFh.
		if ((pmdi->Length >= SMBIOS_TYPE17_SIZE_0207) && (pmdi->Size == 0x7FFFU))
		{
			// the actual size (in megabytes) is stored in the Extended Size field.
			return (LONG)(pmdi->ExtendedSize & 0x7FFFFFFFUL);
		}
		// the granularity in which the value is specified depends on
		// the setting of the most-significant bit (bit 15).
		if (pmdi->Size & 0x8000U)
		{
			// if the bit is 1, the value is specified in kilobyte units.
			return (LONG)(pmdi->Size & 0x7FFFU) >> 10U;
		}
		else
		{
			// if the bit is 0, the value is specified in megabyte units;
			return (LONG)(pmdi->Size & 0x7FFFU);
		}
	}
}

//
// smbios_read_string
//
DECLSPEC_NOINLINE static void CALLBACK smbios_read_string(LPTSTR pszData, DWORD cchData, SMBIOS_HEADER *pHeader, BYTE nOffset)
{
	BYTE  nField = *((BYTE *)pHeader + nOffset);
	BYTE *pbInfo = (BYTE *)pHeader + pHeader->Length;
	BYTE  nIndex = 0;
	BYTE  bData;
	while ((nIndex < nField) && *pbInfo)
	{
		if (++nIndex == nField)
		{
			wnsprintf(pszData, cchData, TEXT("%hs"), pbInfo);
			break;
		}
		do {
			bData = *pbInfo++;
		} while (bData);
	}
}

//
// smbios_type_broker
//
DECLSPEC_NOINLINE static void CALLBACK smbios_type_broker(LPTSTR pszData, DWORD cchData, SMBIOS_HEADER *pHeader, BYTE nLength, BYTE nType, BYTE nOffset)
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
			wnsprintf(pszData, cchData, TEXT("%ld"), smbios_read_capacity(pHeader));
			break;
		case SMBFT_TYPE17_SPEED:
			wnsprintf(pszData, cchData, TEXT("%ld"), smbios_read_speed(pHeader));
			break;
		case SMBFT_TYPE17_SIZE:
			wnsprintf(pszData, cchData, TEXT("%ld"), smbios_read_size(pHeader));
			break;
		case SMBFT_COMMON_DWORD:
			wnsprintf(pszData, cchData, TEXT("%lu"), *(DWORD *)((BYTE *)pHeader + nOffset));
			break;
		case SMBFT_COMMON_WORD:
			wnsprintf(pszData, cchData, TEXT("%hu"), *(WORD *)((BYTE *)pHeader + nOffset));
			break;
		case SMBFT_COMMON_STRING:
			smbios_read_string(pszData, cchData, pHeader, nOffset);
			break;
		default:
			break;
		}
	}
}

//
// smbios_data_broker
//
DECLSPEC_NOINLINE static void CALLBACK smbios_data_broker(LPTSTR pszData, DWORD cchData, BYTE *pbData, DWORD cbData, const SMBIOS_DATA_PARAM *psdp)
{
	BYTE *pbInfo = pbData;
	BYTE nIndex = 0;

	while (pbInfo < &pbData[cbData])
	{
		SMBIOS_HEADER *pHeader = (SMBIOS_HEADER *)pbInfo;

		if (pHeader->Type == psdp->nType && (nIndex++ == psdp->nIndex))
		{
			smbios_type_broker(pszData, cchData, pHeader, psdp->nLength, psdp->nData, psdp->nOffset);
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
// smbios_api_impl
//
DECLSPEC_NOINLINE static void CALLBACK smbios_api_impl(LPTSTR pszData, DWORD cchData, const SMBIOS_DATA_PARAM *psdp)
{
	WMIHANDLE hBlock;
	ULONG nError;

	nError = WmiOpenBlock((GUID *)&MSSmBios_RawSMBiosTables_GUID, WMIGUID_QUERY, &hBlock);
	if (nError == ERROR_SUCCESS)
	{
		HANDLE hHeap = GetProcessHeap();
		BYTE *pbBlock = NULL;
		DWORD cbBlock = 0;

		MSSmBios_RawSMBiosTables *pRawData;

	TryAgain:
		nError = WmiQueryAllData(hBlock, &cbBlock, pbBlock);
		switch (nError) {
		case ERROR_INSUFFICIENT_BUFFER:
			pbBlock = (BYTE *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbBlock);
			if (pbBlock != NULL)
			{
				goto TryAgain;
			}
			break;

		case ERROR_SUCCESS:
			pRawData = (MSSmBios_RawSMBiosTables *)(pbBlock + ((WNODE_ALL_DATA *)pbBlock)->DataBlockOffset);
			smbios_data_broker(pszData, cchData, pRawData->SMBiosData, pRawData->Size, psdp);
			HeapFree(hHeap, 0, pbBlock);
			break;

		default:
			break;
		}

		WmiCloseBlock(hBlock);
	}
}

//
// smbios_api_broker
//
DECLSPEC_NOINLINE static void CALLBACK smbios_api_broker(stack_t **stacktop, int nLength, SMBIOS_DATA_PARAM *psdp)
{
	if (stacktop != NULL)
	{
		if ((psdp->wFlags & SMBDF_INDEX) && (*stacktop != NULL))
		{
			stack_t *th = *stacktop;
			psdp->nIndex = StrToInt(th->text);
			*stacktop = th->next;
			GlobalFree(th);
		}
		stack_t *th = (stack_t *)GlobalAlloc(GPTR, sizeof(stack_t) + nLength * sizeof(TCHAR));
		if (th != NULL)
		{
			smbios_api_impl(th->text, nLength, psdp);
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
	smbios_api_broker(stacktop, nLength, &sdp);
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
	smbios_api_broker(stacktop, nLength, &sdp);
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
	smbios_api_broker(stacktop, nLength, &sdp);
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
//	sdp.nOffset = FIELD_OFFSET(SMBIOS_SYSTEM_INFORMATION, UUID);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
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
	smbios_api_broker(stacktop, nLength, &sdp);
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
	smbios_api_broker(stacktop, nLength, &sdp);
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
	smbios_api_broker(stacktop, nLength, &sdp);
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
	smbios_api_broker(stacktop, nLength, &sdp);
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
//	sdp.nOffset = FIELD_OFFSET(SMBIOS_PROCESSOR_INFORMATION, ProcessorID);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
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
//	sdp.nOffset = FIELD_OFFSET(PHYSICAL_MEMORY_ARRAY_INFORMATION, MaximumCapacity);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

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
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetMemoryManufacturer
//
PLUGINAPI(GetMemoryManufacturer)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.wFlags = SMBDF_INDEX;
	sdp.nType = MemoryDeviceInfo;
	sdp.nLength = SMBIOS_TYPE17_SIZE_0203;
	sdp.nData = SMBFT_COMMON_STRING;
	sdp.nOffset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, Manufacturer);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetMemorySerialNumber
//
PLUGINAPI(GetMemorySerialNumber)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.wFlags = SMBDF_INDEX;
	sdp.nType = MemoryDeviceInfo;
	sdp.nLength = SMBIOS_TYPE17_SIZE_0203;
	sdp.nData = SMBFT_COMMON_STRING;
	sdp.nOffset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, SerialNumber);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetMemorySpeed
//
PLUGINAPI(GetMemorySpeed)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.wFlags = SMBDF_INDEX;
	sdp.nType = MemoryDeviceInfo;
	sdp.nLength = SMBIOS_TYPE17_SIZE_0203;
	sdp.nData = SMBFT_TYPE17_SPEED;
//	sdp.nOffset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, Speed);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetMemorySize
//
PLUGINAPI(GetMemorySize)
{
	SMBIOS_DATA_PARAM sdp = { 0 };
	sdp.wFlags = SMBDF_INDEX;
	sdp.nType = MemoryDeviceInfo;
	sdp.nLength = SMBIOS_TYPE17_SIZE_0201;
	sdp.nData = SMBFT_TYPE17_SIZE;
//	sdp.nOffset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, Size);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

#if defined(_DEBUG)

static GUID wmiGuid = SMBIOS_DATA_GUID;

typedef MSSmBios_RawSMBiosTables WMIType;

typedef struct _WMIString {
	USHORT Length;
	TCHAR Buffer[];
} WMIString;

//
// main
//
int _tmain(int argc, _TCHAR *argv[])
{
	HRESULT hr;

	ULONG nError;

	WMIHANDLE hBlock = nullptr;
	nError = WmiOpenBlock(&wmiGuid, WMIGUID_QUERY, &hBlock);
	if (nError == NO_ERROR)
	{
		BOOL bResult = FALSE;
		HANDLE hHeap = GetProcessHeap();
		WNODE_ALL_DATA *pbBlock = NULL;
		DWORD cbBlock = 0;

		do {
			WMIString *pInstName;
			WMIType *pWmiData;
			ULONG *pOffsets;

			nError = WmiQueryAllData(hBlock, &cbBlock, pbBlock);
			switch (nError) {
			case ERROR_INSUFFICIENT_BUFFER:
				pbBlock = (WNODE_ALL_DATA *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbBlock);
				break;

			case NO_ERROR:
				pOffsets = (ULONG *)((BYTE *)pbBlock + pbBlock->OffsetInstanceNameOffsets);
				for (WNODE_ALL_DATA *pAllData = pbBlock; ; pAllData = (WNODE_ALL_DATA *)((BYTE *)pAllData + pAllData->WnodeHeader.Linkage))
				{
					ULONG nInstance = pAllData->InstanceCount;
					ULONG nOffset;
					ULONG nLength;
					if (pAllData->WnodeHeader.Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE)
					{
						nOffset = pAllData->DataBlockOffset;
						nLength = pAllData->FixedInstanceSize;
						for (ULONG i = 0; i < nInstance; i++)
						{
							pInstName = (WMIString *)((BYTE *)pAllData + pOffsets[i]);
							pWmiData = (WMIType *)((BYTE *)pAllData + nOffset);
							nOffset += nLength;
						}
					}
					else
					{
						for (ULONG i = 0; i < nInstance; i++)
						{
							nOffset = pAllData->OffsetInstanceDataAndLength[i].OffsetInstanceData;
							nLength = pAllData->OffsetInstanceDataAndLength[i].LengthInstanceData;
							pInstName = (WMIString *)((BYTE *)pAllData + pOffsets[i]);
							pWmiData = (WMIType *)((BYTE *)pAllData + nOffset);
						}
					}
					if (pAllData->WnodeHeader.Linkage == 0)
					{
						break;
					}
				}
				HeapFree(hHeap, 0, pbBlock);
				bResult = TRUE;
				break;

			default:
				break;
			}
		} while (!bResult);

		WmiCloseBlock(hBlock);
	}

	BOOL bResult;
	TCHAR rgcData[1024] = { 0 };
	DWORD cchData = ARRAYSIZE(rgcData);
	HANDLE hFile;

	hr = SHGetFolderPath(HWND_DESKTOP, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, rgcData);
	bResult = PathAppend(rgcData, TEXT("smbios.dat"));
	hFile = CreateFile(rgcData, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD cbData = GetFileSize(hFile, NULL);
		BYTE *pbData = (BYTE *)LocalAlloc(LPTR, cbData);
		DWORD cbRead;

		bResult = ReadFile(hFile, pbData, cbData, &cbRead, NULL);
		CloseHandle(hFile);

		SMBIOS_DATA_PARAM sdp = { 0 };
		sdp.nIndex = 0;
		sdp.nType = MemoryDeviceInfo;
		sdp.nLength = SMBIOS_TYPE17_SIZE_0201;
		sdp.nData = SMBFT_TYPE17_SIZE;
//		sdp.nOffset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, Size);
		smbios_data_broker(rgcData, cchData, pbData, cbData, &sdp);

		LocalFree(pbData);
	}

	return 0;
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
