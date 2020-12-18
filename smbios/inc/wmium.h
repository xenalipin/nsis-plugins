/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    Wmium.h

Abstract:

    Public headers for WMI data consumers and providers

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#ifndef _WMIUM_
#define _WMIUM_

#pragma once

#ifdef _WMI_SOURCE_
#define WMIAPI __stdcall
#else
#define WMIAPI DECLSPEC_IMPORT __stdcall
#endif

#include <guiddef.h>

#include <basetsd.h>
#include <wmistr.h>
#include <evntrace.h>

typedef PVOID WMIHANDLE, *PWMIHANDLE;
typedef PVOID MOFHANDLE, *PMOFHANDLE;

//
// When set the guid can be opened and accessed
#define MOFCI_RESERVED0  0x00000001

#define MOFCI_RESERVED1  0x00000002
#define MOFCI_RESERVED2  0x00000004

typedef struct {
	LPWSTR ImagePath;       // Path to image containing MOF resource
	LPWSTR ResourceName;    // Name of resource in image
	ULONG  ResourceSize;    // Number of bytes in resource
	UCHAR *ResourceBuffer;  // Reserved
} MOFRESOURCEINFOW, *PMOFRESOURCEINFOW;

typedef struct {
	LPSTR  ImagePath;       // Path to image containing MOF resource
	LPSTR  ResourceName;    // Name of resource in image
	ULONG  ResourceSize;    // Number of bytes in resource
	UCHAR *ResourceBuffer;  // Reserved
} MOFRESOURCEINFOA, *PMOFRESOURCEINFOA;

#ifdef UNICODE
typedef MOFRESOURCEINFOW MOFRESOURCEINFO;
typedef PMOFRESOURCEINFOW PMOFRESOURCEINFO;
#else
typedef MOFRESOURCEINFOA MOFRESOURCEINFO;
typedef PMOFRESOURCEINFOA PMOFRESOURCEINFO;
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Data consumer apis

/*
	Success: WMIHANDLE != INVALID_HANDLE_VALUE
*/
ULONG
WMIAPI
WmiOpenBlock(
	GUID *Guid,
	ULONG DesiredAccess,
	WMIHANDLE *DataBlockHandle
);

ULONG
WMIAPI
WmiCloseBlock(
	WMIHANDLE DataBlockHandle
);

ULONG
WMIAPI
WmiQueryAllDataA(
	WMIHANDLE DataBlockHandle,
	ULONG *BufferSize,
	LPVOID Buffer
);

ULONG
WMIAPI
WmiQueryAllDataW(
	WMIHANDLE DataBlockHandle,
	ULONG *BufferSize,
	LPVOID Buffer
);

#ifdef UNICODE
#define WmiQueryAllData WmiQueryAllDataW
#else
#define WmiQueryAllData WmiQueryAllDataA
#endif

ULONG
WMIAPI
WmiQueryAllDataMultipleA(
	WMIHANDLE *HandleList,
	ULONG HandleCount,
	ULONG *InOutBufferSize,
	LPVOID OutBuffer
);

ULONG
WMIAPI
WmiQueryAllDataMultipleW(
	WMIHANDLE *HandleList,
	ULONG HandleCount,
	ULONG *InOutBufferSize,
	LPVOID OutBuffer
);

#ifdef UNICODE
#define WmiQueryAllDataMultiple WmiQueryAllDataMultipleW
#else
#define WmiQueryAllDataMultiple WmiQueryAllDataMultipleA
#endif

/*
	Success: ERROR_SUCCESS
*/
ULONG
WMIAPI
WmiQuerySingleInstanceA(
	WMIHANDLE DataBlockHandle,
	LPCSTR InstanceName,
	PULONG BufferSize,
	LPVOID Buffer
);

/*
	Success: ERROR_SUCCESS
*/
ULONG
WMIAPI
WmiQuerySingleInstanceW(
	WMIHANDLE DataBlockHandle,
	LPCWSTR InstanceName,
	PULONG BufferSize,
	LPVOID Buffer
);

#ifdef UNICODE
#define WmiQuerySingleInstance WmiQuerySingleInstanceW
#else
#define WmiQuerySingleInstance WmiQuerySingleInstanceA
#endif

/*
	Success: ERROR_SUCCESS
*/
ULONG
WMIAPI
WmiQuerySingleInstanceMultipleW(
	PWMIHANDLE HandleList,
	LPCWSTR *InstanceNames,
	ULONG HandleCount,
	PULONG InOutBufferSize,
	LPVOID OutBuffer
);

/*
	Success: ERROR_SUCCESS
*/
ULONG
WMIAPI
WmiQuerySingleInstanceMultipleA(
	PWMIHANDLE HandleList,
	LPCSTR *InstanceNames,
	ULONG HandleCount,
	PULONG InOutBufferSize,
	LPVOID OutBuffer
);

#ifdef UNICODE
#define WmiQuerySingleInstanceMultiple WmiQuerySingleInstanceMultipleW
#else
#define WmiQuerySingleInstanceMultiple WmiQuerySingleInstanceMultipleA
#endif

ULONG
WMIAPI
WmiSetSingleInstanceA(
	WMIHANDLE DataBlockHandle,
	LPCSTR InstanceName,
	ULONG Reserved,
	ULONG ValueBufferSize,
	PVOID ValueBuffer
);

ULONG
WMIAPI
WmiSetSingleInstanceW(
	WMIHANDLE DataBlockHandle,
	LPCWSTR InstanceName,
	ULONG Reserved,
	ULONG ValueBufferSize,
	PVOID ValueBuffer
);

#ifdef UNICODE
#define WmiSetSingleInstance WmiSetSingleInstanceW
#else
#define WmiSetSingleInstance WmiSetSingleInstanceA
#endif

ULONG
WMIAPI
WmiSetSingleItemA(
	WMIHANDLE DataBlockHandle,
	LPCSTR InstanceName,
	ULONG DataItemId,
	ULONG Reserved,
	ULONG ValueBufferSize,
	PVOID ValueBuffer
);

ULONG
WMIAPI
WmiSetSingleItemW(
	WMIHANDLE DataBlockHandle,
	LPCWSTR InstanceName,
	ULONG DataItemId,
	ULONG Reserved,
	ULONG ValueBufferSize,
	PVOID ValueBuffer
);

#ifdef UNICODE
#define WmiSetSingleItem WmiSetSingleItemW
#else
#define WmiSetSingleItem WmiSetSingleItemA
#endif

ULONG
WMIAPI
WmiExecuteMethodA(
	WMIHANDLE MethodDataBlockHandle,
	LPCSTR MethodInstanceName,
	ULONG MethodId,
	ULONG InputValueBufferSize,
	PVOID InputValueBuffer,
	ULONG *OutputBufferSize,
	PVOID OutputBuffer
);

ULONG
WMIAPI
WmiExecuteMethodW(
	WMIHANDLE MethodDataBlockHandle,
	LPCWSTR MethodInstanceName,
	ULONG MethodId,
	ULONG InputValueBufferSize,
	PVOID InputValueBuffer,
	ULONG *OutputBufferSize,
	PVOID OutputBuffer
);

#ifdef UNICODE
#define WmiExecuteMethod WmiExecuteMethodW
#else
#define WmiExecuteMethod WmiExecuteMethodA
#endif

// Set this Flag when calling NotficationRegistration to enable or
// disable a trace logging guid
#define NOTIFICATION_TRACE_FLAG       0x00010000

// Set this flag when enabling a notification that should be delivered via
// a direct callback. Any notifications received will be given their own
// thread and the callback function called immediately.
#define NOTIFICATION_CALLBACK_DIRECT  0x00000004

//
// Set this flag (and only this flag) when you want to only check if the
// caller has permission to receive events for the guid
//
#define NOTIFICATION_CHECK_ACCESS     0x00000008

//
// Set this flag when enabling a lightweight notification.
#define NOTIFICATION_LIGHTWEIGHT_FLAG 0x00000020

//
// Event notification callback function prototype
typedef void (WINAPI *NOTIFICATIONCALLBACK)(
	PWNODE_HEADER Wnode,
	UINT_PTR NotificationContext
);

#if !defined(MIDL_PASS)

//
// This guid is for notifications of changes to registration
// {B48D49A1-E777-11d0-A50C-00A0C9062910}
DEFINE_GUID(GUID_REGISTRATION_CHANGE_NOTIFICATION,
	0xb48d49a1,
	0xe777,
	0x11d0,
	0xa5, 0xc, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0x10
);

//
// This guid id for notifications of new mof resources being added
// {B48D49A2-E777-11d0-A50C-00A0C9062910}
DEFINE_GUID(GUID_MOF_RESOURCE_ADDED_NOTIFICATION,
	0xb48d49a2,
	0xe777,
	0x11d0,
	0xa5, 0xc, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0x10
);

//
// This guid id for notifications of new mof resources being added
// {B48D49A3-E777-11d0-A50C-00A0C9062910}
DEFINE_GUID(GUID_MOF_RESOURCE_REMOVED_NOTIFICATION,
	0xb48d49a3,
	0xe777,
	0x11d0,
	0xa5, 0xc, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0x10
);

#endif

ULONG
WMIAPI
WmiNotificationRegistrationA(
	LPGUID Guid,
	BOOLEAN Enable,
	PVOID DeliveryInfo,
	ULONG_PTR DeliveryContext,
	ULONG Flags
);

ULONG
WMIAPI
WmiNotificationRegistrationW(
	LPGUID Guid,
	BOOLEAN Enable,
	PVOID DeliveryInfo,
	ULONG_PTR DeliveryContext,
	ULONG Flags
);

#ifdef UNICODE
#define WmiNotificationRegistration WmiNotificationRegistrationW
#else
#define WmiNotificationRegistration WmiNotificationRegistrationA
#endif

void
WMIAPI
WmiFreeBuffer(
	PVOID Buffer
);

ULONG
WMIAPI
WmiEnumerateGuids(
	LPGUID GuidList,
	ULONG *GuidCount
);

ULONG
WMIAPI
WmiMofEnumerateResourcesW(
	MOFHANDLE MofResourceHandle,
	ULONG *MofResourceCount,
	PMOFRESOURCEINFOW *MofResourceInfo
);

ULONG
WMIAPI
WmiMofEnumerateResourcesA(
	MOFHANDLE MofResourceHandle,
	ULONG *MofResourceCount,
	PMOFRESOURCEINFOA *MofResourceInfo
);

#ifdef UNICODE
#define WmiMofEnumerateResources WmiMofEnumerateResourcesW
#else
#define WmiMofEnumerateResources WmiMofEnumerateResourcesA
#endif

ULONG
WMIAPI
WmiFileHandleToInstanceNameA(
	WMIHANDLE DataBlockHandle,
	HANDLE FileHandle,
	ULONG *NumberCharacters,
	CHAR *InstanceNames
);

ULONG
WMIAPI
WmiFileHandleToInstanceNameW(
	WMIHANDLE DataBlockHandle,
	HANDLE FileHandle,
	ULONG *NumberCharacters,
	WCHAR *InstanceNames
);

#ifdef UNICODE
#define WmiFileHandleToInstanceName WmiFileHandleToInstanceNameW
#else
#define WmiFileHandleToInstanceName WmiFileHandleToInstanceNameA
#endif

#define WmiInsertTimestamp(WnodeHeader) \
	GetSystemTimeAsFileTime((FILETIME *)&((PWNODE_HEADER)WnodeHeader)->TimeStamp)

ULONG
WMIAPI
WmiDevInstToInstanceNameA(
	PSTR InstanceName,
	ULONG InstanceNameLength,
	PSTR DevInst,
	ULONG InstanceIndex
);

ULONG
WMIAPI
WmiDevInstToInstanceNameW(
	PWSTR InstanceName,
	ULONG InstanceNameLength,
	PWSTR DevInst,
	ULONG InstanceIndex
);

#ifdef UNICODE
#define WmiDevInstToInstanceName WmiDevInstToInstanceNameW
#else
#define WmiDevInstToInstanceName WmiDevInstToInstanceNameA
#endif

typedef struct _WMIGUIDINFORMATION {
	ULONG Size;
	BOOLEAN IsExpensive;
	BOOLEAN IsEventOnly;
} WMIGUIDINFORMATION, *PWMIGUIDINFORMATION;

ULONG
WMIAPI
WmiQueryGuidInformation(
	WMIHANDLE GuidHandle,
	PWMIGUIDINFORMATION GuidInfo
);

ULONG
WMIAPI
WmiReceiveNotificationsW(
	ULONG HandleCount,
	HANDLE *HandleList,
	NOTIFICATIONCALLBACK Callback,
	ULONG_PTR DeliveryContext
);

ULONG
WMIAPI
WmiReceiveNotificationsA(
	ULONG HandleCount,
	HANDLE *HandleList,
	NOTIFICATIONCALLBACK Callback,
	ULONG_PTR DeliveryContext
);

#ifdef UNICODE
#define WmiReceiveNotifications WmiReceiveNotificationsW
#else
#define WmiReceiveNotifications WmiReceiveNotificationsA
#endif

#ifdef __cplusplus
}
#endif

#endif // _WMIUM_
