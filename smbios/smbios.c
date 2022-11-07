#include <stdio.h>
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
#include "strcvt.h"

//
// PLUGINAPI
//
#if defined(__cplusplus)
#define PLUGINAPI(Name_) extern "C" __declspec(dllexport) void __cdecl Name_(HWND hwndParent, int nLength, LPTSTR variables, stack_t **stacktop, extra_parameters *extra, ...)
#else
#define PLUGINAPI(Name_) __declspec(dllexport) void __cdecl Name_(HWND hwndParent, int nLength, LPTSTR variables, stack_t **stacktop, extra_parameters *extra, ...)
#endif

//
// SMBIOS_VERSION
//
#define SMBIOS_VERSION(hi, lo) MAKEWORD(lo, hi)

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
	WORD Flags;
	WORD Version;
	// DMI_ENTRY_TYPE
	BYTE Entry;
	// index for type with more than one entry
	BYTE Index;
	// byte offset of the data to read
	BYTE Offset;
	// SMBIOS_FIELD_TYPE
	BYTE Field;
} SMBIOS_DATA_PARAM;

//
// Global variables
//
static HMODULE hModule = NULL;
static BYTE *pbBlock = NULL;

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
DECLSPEC_NOINLINE static LONG CALLBACK smbios_read_capacity(WORD wVersion, const DMI_HEADER *pHeader)
{
	const PHYSICAL_MEMORY_ARRAY_INFORMATION *ppmai = (const PHYSICAL_MEMORY_ARRAY_INFORMATION *)pHeader;
	if ((wVersion >= SMBIOS_VERSION(2, 7)) && (ppmai->MaximumCapacity == 0x80000000UL))
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
DECLSPEC_NOINLINE static LONG CALLBACK smbios_read_speed(WORD wVersion, const DMI_HEADER *pHeader)
{
	const MEMORY_DEVICE_INFORMATION *pmdi = (const MEMORY_DEVICE_INFORMATION *)pHeader;
	if ((wVersion >= SMBIOS_VERSION(3, 3)) && (pmdi->Speed == 0xFFFFU))
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
DECLSPEC_NOINLINE static LONG CALLBACK smbios_read_size(WORD wVersion, const DMI_HEADER *pHeader)
{
	const MEMORY_DEVICE_INFORMATION *pmdi = (const MEMORY_DEVICE_INFORMATION *)pHeader;
	WORD wSize = pmdi->Size;
	switch (wSize) {
	case 0xFFFFU:
		// if the size is unknown, the field value is FFFFh.
		return (LONG)(SHORT)wSize;
	case 0x0U:
		// if the value is 0, no memory device is installed in the socket.
		return (LONG)wSize;
	default:
		// if the size is 32 GB-1 MB or greater, the field value is 7FFFh.
		if (((wVersion >= SMBIOS_VERSION(2, 7))) && (wSize == 0x7FFFU))
		{
			// the actual size (in megabytes) is stored in the Extended Size field.
			return (LONG)(pmdi->ExtendedSize & 0x7FFFFFFFUL);
		}
		// the granularity in which the value is specified depends on
		// the setting of the most-significant bit (bit 15).
		if (wSize & 0x8000U)
		{
			// if the bit is 1, the value is specified in kilobyte units.
			return (LONG)(wSize & 0x7FFFU) >> 10U;
		}
		else
		{
			// if the bit is 0, the value is specified in megabyte units;
			return (LONG)(wSize & 0x7FFFU);
		}
	}
}

//
// smbios_type_broker
//
DECLSPEC_NOINLINE static void CALLBACK smbios_type_broker(LPTSTR pszData, DWORD cchData, const DMI_HEADER *pHeader, WORD wVersion, BYTE Field, BYTE Offset)
{
	switch (Field) {
	case SMBFT_TYPE01_UUID:
		do {
			const UUID *puuid = (const UUID *)((const BYTE *)pHeader + pHeader->Length);
			wnsprintf(
				pszData,
				cchData,
				TEXT("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"),
				puuid->Data1,
				puuid->Data2,
				puuid->Data3,
				puuid->Data4[0],
				puuid->Data4[1],
				puuid->Data4[2],
				puuid->Data4[3],
				puuid->Data4[4],
				puuid->Data4[5],
				puuid->Data4[6],
				puuid->Data4[7]);
		} while (0);
		break;
	case SMBFT_TYPE04_PROCESSOR:
		do {
			const ULARGE_INTEGER *ppid = (const ULARGE_INTEGER *)((const BYTE *)pHeader + pHeader->Length);
			wnsprintf(pszData, cchData, TEXT("%08X%08X"), ppid->HighPart, ppid->LowPart);
		} while (0);
		break;
	case SMBFT_TYPE16_CAPACITY:
		wnsprintf(pszData, cchData, TEXT("%ld"), smbios_read_capacity(wVersion, pHeader));
		break;
	case SMBFT_TYPE17_SPEED:
		wnsprintf(pszData, cchData, TEXT("%ld"), smbios_read_speed(wVersion, pHeader));
		break;
	case SMBFT_TYPE17_SIZE:
		wnsprintf(pszData, cchData, TEXT("%ld"), smbios_read_size(wVersion, pHeader));
		break;
	case SMBFT_COMMON_DWORD:
		wnsprintf(pszData, cchData, TEXT("%lu"), *(DWORD *)((BYTE *)pHeader + Offset));
		break;
	case SMBFT_COMMON_WORD:
		wnsprintf(pszData, cchData, TEXT("%hu"), *(WORD *)((BYTE *)pHeader + Offset));
		break;
	case SMBFT_COMMON_STRING:
		do {
			BYTE  nField = *((BYTE *)pHeader + Offset);
			BYTE *pbData = (BYTE *)pHeader + pHeader->Length;
			BYTE  Index = 0;
			while ((Index < nField) && *pbData)
			{
				if (++Index == nField)
				{
					StrCvtA2T(pszData, cchData, pbData);
					break;
				}
				BYTE  bData;
				do {
					bData = *pbData++;
				} while (bData);
			}
		} while (0);
		break;
	default:
		break;
	}
}

//
// smbios_data_broker
//
DECLSPEC_NOINLINE static void CALLBACK smbios_data_broker(LPTSTR pszData, DWORD cchData, WORD wVersion, BYTE *pbData, DWORD cbData, const SMBIOS_DATA_PARAM *psdp)
{
	BYTE *pbStop = &pbData[cbData];
	BYTE  Index = 0;

	while (pbData < pbStop)
	{
		DMI_HEADER *pHeader = (DMI_HEADER *)pbData;

		if (pHeader->Type == psdp->Entry && (Index++ == psdp->Index) && (wVersion >= psdp->Version))
		{
			smbios_type_broker(pszData, cchData, pHeader, wVersion, psdp->Field, psdp->Offset);
			break;
		}

		pbData += pHeader->Length;

		do {
			BYTE bData;
			do {
				bData = *pbData++;
			} while (bData);
		} while (*pbData);

		pbData++;
	}
}

#if 0

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
		WORD wVersion;

	TryAgain:
		nError = WmiQueryAllData(hBlock, &cbBlock, pbBlock);
		switch (nError) {
		case ERROR_SUCCESS:
			pRawData = (MSSmBios_RawSMBiosTables *)(pbBlock + ((WNODE_ALL_DATA *)pbBlock)->OffsetInstanceDataAndLength[0].OffsetInstanceData);
			wVersion = SMBIOS_VERSION(pRawData->SmbiosMajorVersion, pRawData->SmbiosMinorVersion);
			smbios_data_broker(pszData, cchData, wVersion, pRawData->SMBiosData, pRawData->Size, psdp);
			HeapFree(hHeap, 0, pbBlock);
			break;
		case ERROR_INSUFFICIENT_BUFFER:
			pbBlock = (BYTE *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbBlock);
			if (pbBlock != NULL)
			{
				goto TryAgain;
			}
		default:
			break;
		}

		WmiCloseBlock(hBlock);
	}
}

#endif

//
// smbios_api_broker
//
DECLSPEC_NOINLINE static void CALLBACK smbios_api_broker(stack_t **stacktop, int nLength, SMBIOS_DATA_PARAM *psdp)
{
	if (stacktop != NULL)
	{
		if ((psdp->Flags & SMBDF_INDEX) && (*stacktop != NULL))
		{
			stack_t *th = *stacktop;
			psdp->Index = StrToInt(th->text);
			*stacktop = th->next;
			GlobalFree(th);
		}
		stack_t *th = (stack_t *)GlobalAlloc(GPTR, sizeof(stack_t) + nLength * sizeof(TCHAR));
		if (th != NULL)
		{
#if 1
			MSSmBios_RawSMBiosTables *pRawData = (MSSmBios_RawSMBiosTables *)(pbBlock + ((WNODE_ALL_DATA *)pbBlock)->OffsetInstanceDataAndLength[0].OffsetInstanceData);
			WORD wVersion = SMBIOS_VERSION(pRawData->SmbiosMajorVersion, pRawData->SmbiosMinorVersion);
			smbios_data_broker(th->text, nLength, wVersion, pRawData->SMBiosData, pRawData->Size, psdp);
#else
			smbios_api_impl(th->text, nLength, psdp);
#endif
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
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = 0;
	sdp.Version = SMBIOS_VERSION(2, 0);
	sdp.Entry = DMI_ENTRY_SYSTEM;
	sdp.Index = 0;
	sdp.Offset = FIELD_OFFSET(SMBIOS_SYSTEM_INFORMATION, Manufacturer);
	sdp.Field = SMBFT_COMMON_STRING;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetSystemSerialNumber
//
PLUGINAPI(GetSystemSerialNumber)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = 0;
	sdp.Version = SMBIOS_VERSION(2, 0);
	sdp.Entry = DMI_ENTRY_SYSTEM;
	sdp.Index = 0;
	sdp.Offset = FIELD_OFFSET(SMBIOS_SYSTEM_INFORMATION, SerialNumber);
	sdp.Field = SMBFT_COMMON_STRING;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetSystemProductName
//
PLUGINAPI(GetSystemProductName)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = 0;
	sdp.Version = SMBIOS_VERSION(2, 0);
	sdp.Entry = DMI_ENTRY_SYSTEM;
	sdp.Index = 0;
	sdp.Offset = FIELD_OFFSET(SMBIOS_SYSTEM_INFORMATION, ProductName);
	sdp.Field = SMBFT_COMMON_STRING;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetSystemUUID
//
PLUGINAPI(GetSystemUUID)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = 0;
	sdp.Version = SMBIOS_VERSION(2, 1);
	sdp.Entry = DMI_ENTRY_SYSTEM;
	sdp.Index = 0;
	sdp.Offset = FIELD_OFFSET(SMBIOS_SYSTEM_INFORMATION, UUID);
	sdp.Field = SMBFT_TYPE01_UUID;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetSystemFamily
//
PLUGINAPI(GetSystemFamily)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = 0;
	sdp.Version = SMBIOS_VERSION(2, 4);
	sdp.Entry = DMI_ENTRY_SYSTEM;
	sdp.Index = 0;
	sdp.Offset = FIELD_OFFSET(SMBIOS_SYSTEM_INFORMATION, Family);
	sdp.Field = SMBFT_COMMON_STRING;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetModuleManufacturer
//
PLUGINAPI(GetModuleManufacturer)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = 0;
	sdp.Version = SMBIOS_VERSION(2, 0);
	sdp.Entry = DMI_ENTRY_BASEBOARD;
	sdp.Index = 0;
	sdp.Offset = FIELD_OFFSET(SMBIOS_BASEBOARD_INFORMATION, Manufacturer);
	sdp.Field = SMBFT_COMMON_STRING;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetModuleSerialNumber
//
PLUGINAPI(GetModuleSerialNumber)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = 0;
	sdp.Version = SMBIOS_VERSION(2, 0);
	sdp.Entry = DMI_ENTRY_BASEBOARD;
	sdp.Index = 0;
	sdp.Offset = FIELD_OFFSET(SMBIOS_BASEBOARD_INFORMATION, SerialNumber);
	sdp.Field = SMBFT_COMMON_STRING;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetModuleProductName
//
PLUGINAPI(GetModuleProductName)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = 0;
	sdp.Version = SMBIOS_VERSION(2, 0);
	sdp.Entry = DMI_ENTRY_BASEBOARD;
	sdp.Index = 0;
	sdp.Offset = FIELD_OFFSET(SMBIOS_BASEBOARD_INFORMATION, Product);
	sdp.Field = SMBFT_COMMON_STRING;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetProcessorID
//
PLUGINAPI(GetProcessorID)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = 0;
	sdp.Version = SMBIOS_VERSION(2, 0);
	sdp.Entry = DMI_ENTRY_PROCESSOR;
	sdp.Index = 0;
	sdp.Offset = FIELD_OFFSET(SMBIOS_PROCESSOR_INFORMATION, ProcessorID);
	sdp.Field = SMBFT_TYPE04_PROCESSOR;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetMemoryArrayCapacity
//
PLUGINAPI(GetMemoryArrayCapacity)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = 0;
	sdp.Version = SMBIOS_VERSION(2, 1);
	sdp.Entry = DMI_ENTRY_PHYS_MEM_ARRAY;
	sdp.Index = 0;
//	sdp.Offset = FIELD_OFFSET(PHYSICAL_MEMORY_ARRAY_INFORMATION, MaximumCapacity);
	sdp.Field = SMBFT_TYPE16_CAPACITY;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetMemoryArrayNumbers
//
PLUGINAPI(GetMemoryArrayNumbers)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = 0;
	sdp.Version = SMBIOS_VERSION(2, 1);
	sdp.Entry = DMI_ENTRY_PHYS_MEM_ARRAY;
	sdp.Index = 0;
	sdp.Offset = FIELD_OFFSET(PHYSICAL_MEMORY_ARRAY_INFORMATION, NumberOfMemoryDevices);
	sdp.Field = SMBFT_COMMON_WORD;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetMemoryManufacturer
//
PLUGINAPI(GetMemoryManufacturer)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = SMBDF_INDEX;
	sdp.Version = SMBIOS_VERSION(2, 3);
	sdp.Entry = DMI_ENTRY_MEM_DEVICE;
	sdp.Index = 0;
	sdp.Offset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, Manufacturer);
	sdp.Field = SMBFT_COMMON_STRING;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetMemorySerialNumber
//
PLUGINAPI(GetMemorySerialNumber)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = SMBDF_INDEX;
	sdp.Version = SMBIOS_VERSION(2, 3);
	sdp.Entry = DMI_ENTRY_MEM_DEVICE;
	sdp.Index = 0;
	sdp.Offset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, SerialNumber);
	sdp.Field = SMBFT_COMMON_STRING;
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetMemorySpeed
//
PLUGINAPI(GetMemorySpeed)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = SMBDF_INDEX;
	sdp.Version = SMBIOS_VERSION(2, 3);
	sdp.Entry = DMI_ENTRY_MEM_DEVICE;
	sdp.Field = SMBFT_TYPE17_SPEED;
//	sdp.Offset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, Speed);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

//
// GetMemorySize
//
PLUGINAPI(GetMemorySize)
{
	SMBIOS_DATA_PARAM sdp;
	sdp.Flags = SMBDF_INDEX;
	sdp.Version = SMBIOS_VERSION(2, 1);
	sdp.Entry = DMI_ENTRY_MEM_DEVICE;
	sdp.Field = SMBFT_TYPE17_SIZE;
//	sdp.Offset = FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, Size);
	extra->RegisterPluginCallback(hModule, PluginCallback);
	smbios_api_broker(stacktop, nLength, &sdp);
}

#if defined(_DEBUG)

typedef struct _WMIString {
	USHORT Length;
	TCHAR Buffer[];
} WMIString;

//
// main
//
int _tmain(int argc, _TCHAR *argv[])
{
	WMIHANDLE hBlock;
	ULONG nError;

	nError = WmiOpenBlock((GUID *)&MSSmBios_RawSMBiosTables_GUID, WMIGUID_QUERY, &hBlock);
	if (nError == NO_ERROR)
	{
		HANDLE hHeap = GetProcessHeap();

		BYTE *pbBlock = NULL;
		DWORD cbBlock = 0;

		MSSmBios_RawSMBiosTables *pInstData;
		WMIString *pInstName;

		ULONG *pOffset;
		ULONG Linkage;

	TryAgain:
		nError = WmiQueryAllData(hBlock, &cbBlock, pbBlock);
		switch (nError) {
		case NO_ERROR:
			pOffset = (ULONG *)(pbBlock + ((WNODE_ALL_DATA *)pbBlock)->OffsetInstanceNameOffsets);
			for (BYTE *pbData = pbBlock; ; pbData += Linkage)
			{
				WNODE_ALL_DATA *pAllData = (WNODE_ALL_DATA *)pbData;
				Linkage = pAllData->WnodeHeader.Linkage;
				if (!(pAllData->WnodeHeader.Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE))
				{
					ULONG cInstance = pAllData->InstanceCount;
					for (ULONG i = 0; i < cInstance; i++)
					{
						ULONG nLength = pAllData->OffsetInstanceDataAndLength[i].LengthInstanceData;
						ULONG Offset = pAllData->OffsetInstanceDataAndLength[i].OffsetInstanceData;
						pInstData = (MSSmBios_RawSMBiosTables *)(pbData + Offset);
						pInstName = (WMIString *)(pbData + pOffset[i]);
					}
				}
				else
				{
					ULONG cInstance = pAllData->InstanceCount;
					ULONG nLength = pAllData->FixedInstanceSize;
					ULONG Offset = pAllData->DataBlockOffset;
					for (ULONG i = 0; i < cInstance; i++)
					{
						pInstData = (MSSmBios_RawSMBiosTables *)(pbData + Offset);
						pInstName = (WMIString *)(pbData + pOffset[i]);
						Offset += nLength;
					}
				}
				if (Linkage == 0)
				{
					break;
				}
			}
			HeapFree(hHeap, 0, pbBlock);
			break;
		case ERROR_INSUFFICIENT_BUFFER:
			pbBlock = (BYTE *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbBlock);
			if (pbBlock != NULL)
			{
				goto TryAgain;
			}
		default:
			break;
		}

		WmiCloseBlock(hBlock);
	}

	return 0;
}

#else

//
// DllMain
//
BOOL WINAPI _DllMainCRTStartup(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	BOOL bResult = FALSE;
	HANDLE hHeap = GetProcessHeap();
	WMIHANDLE hBlock;
	ULONG nError;
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		nError = WmiOpenBlock((GUID *)&MSSmBios_RawSMBiosTables_GUID, WMIGUID_QUERY, &hBlock);
		if (nError == ERROR_SUCCESS)
		{
			DWORD cbBlock = 0;
		TryAgain:
			nError = WmiQueryAllData(hBlock, &cbBlock, pbBlock);
			switch (nError) {
			case ERROR_SUCCESS:
				hModule = (HMODULE)hinstDLL;
				bResult = TRUE;
				break;
			case ERROR_INSUFFICIENT_BUFFER:
				pbBlock = (BYTE *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbBlock);
				if (pbBlock != NULL)
				{
					goto TryAgain;
				}
			default:
				break;
			}
			WmiCloseBlock(hBlock);
		}
		return bResult;
	case DLL_PROCESS_DETACH:
		bResult = HeapFree(hHeap, 0, pbBlock);
		return bResult;
	default:
		return TRUE;
	}
}

#endif
