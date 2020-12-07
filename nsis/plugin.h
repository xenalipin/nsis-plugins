/*
 * This file is a part of NSIS.
 *
 * Copyright (C) 1999-2020 Nullsoft and Contributors
 *
 * Licensed under the zlib/libpng license (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Licence details can be found in the file COPYING.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.
 */

//
// Copyright (C) 1999-2020 Nullsoft and Contributors
//
// modified by jiake (137729898@qq.com)
// support for Windows style name
// support for nullptr keyword
//

#ifndef _NSIS_PLUGIN_H
#define _NSIS_PLUGIN_H

#pragma once

#if defined(__cplusplus)
#	if defined(_MSC_VER) && (_MSC_VER < 1600) && !defined(_NATIVE_NULLPTR_SUPPORTED)
#		define nullptr NULL
#	endif
#else
#	if !defined(nullptr)
#		define nullptr ((void *)0)
#	endif
#endif

// Starting with NSIS 2.42, you can check the version of the plugin API in exec_flags->plugin_api_version
// The format is 0xXXXXYYYY where X is the major version and Y is the minor version (MAKELONG(y,x))
// When doing version checks, always remember to use >=, ex: if (pX->exec_flags->plugin_api_version >= NSISPIAPIVER_1_0) {}

#define NSISPIAPIVER_1_0            0x00010000
#define NSISPIAPIVER_CURR           NSISPIAPIVER_1_0

//
// Messages
//

// Definitions for page showing plug-ins

// sent to the outer window to tell it to go to the next inner window
#define WM_NOTIFY_OUTER_NEXT        (WM_USER + 0x8)
// custom pages should send this message to let NSIS know they're ready
#define WM_NOTIFY_CUSTOM_READY      (WM_USER + 0xd)
// sent as wParam with WM_NOTIFY_OUTER_NEXT when user cancels - heed its warning
#define NOTIFY_BYE_BYE              ((WPARAM)'x')

// Internal messages

// sent to the last child window to tell it that the install thread is done
#define WM_NOTIFY_INSTPROC_DONE     (WM_USER + 0x4)
// sent to every child window to tell it it can start executing NSIS code
#define WM_NOTIFY_START             (WM_USER + 0x5)
// sent to every child window to tell it it is closing soon
#define WM_NOTIFY_INIGO_MONTOYA     (WM_USER + 0xB)
// update message used by DirProc and SelProc for space display
#define WM_IN_UPDATEMSG             (WM_USER + 0xF)
// simulates clicking on the tree
#define WM_TREEVIEW_KEYHACK         (WM_USER + 0x13)
// notifies a component selection change (.onMouseOverSection)
#define WM_NOTIFY_SELCHANGE         (WM_USER + 0x19)
// Notifies the installation type has changed by the user
#define WM_NOTIFY_INSTTYPE_CHANGED  (WM_USER + 0x20)

//
// NSPIM
//

// NSIS Plug-In Callback Messages
typedef enum {
	NSPIM_UNLOAD,    // This is the last message a plugin gets, do final cleanup
	NSPIM_GUIUNLOAD  // Called after .onGUIEnd
} NSPIM;

// Prototype for callbacks registered with extra_parameters->RegisterPluginCallback()
// Return NULL for unknown messages
// Should always be __cdecl for future expansion possibilities
typedef UINT_PTR (*NSISPLUGINCALLBACK)(NSPIM);

//
// exec_flags_t
//

// extra_parameters data structures containing other interesting stuff
// but the stack, variables and HWND passed on to plug-ins.
typedef struct _exec_flags_t {
	int autoclose;
	int all_user_var;
	int exec_error;
	int abort;
	int exec_reboot;        // NSIS_SUPPORT_REBOOT
	int reboot_called;      // NSIS_SUPPORT_REBOOT
	int cur_insttype;       // depreacted
	int plugin_api_version; // see NSISPIAPIVER_CURR
	int silent;             // NSIS_CONFIG_SILENT_SUPPORT
	int instdir_error;
	int rtl;
	int errlvl;
	int alter_reg_view;
	int status_update;
} exec_flags_t;

//
// stack_t
//

#pragma warning(push)
#pragma warning(disable: 4200)

typedef struct _stack_t {
	struct _stack_t *next;
	TCHAR text[];
} stack_t;

#pragma warning(pop)

//
// extra_parameters
//

typedef struct _extra_parameters {
	exec_flags_t *exec_flags;
	int  (CALLBACK *ExecuteCodeSegment)(int, HWND);
	void (CALLBACK *ValidateFilename)(LPTSTR);
	int  (CALLBACK *RegisterPluginCallback)(HMODULE, NSISPLUGINCALLBACK);
} extra_parameters;

//
// Variable Offset Enumration
//

enum {
	INST_0,       // $0
	INST_1,       // $1
	INST_2,       // $2
	INST_3,       // $3
	INST_4,       // $4
	INST_5,       // $5
	INST_6,       // $6
	INST_7,       // $7
	INST_8,       // $8
	INST_9,       // $9
	INST_R0,      // $R0
	INST_R1,      // $R1
	INST_R2,      // $R2
	INST_R3,      // $R3
	INST_R4,      // $R4
	INST_R5,      // $R5
	INST_R6,      // $R6
	INST_R7,      // $R7
	INST_R8,      // $R8
	INST_R9,      // $R9
	INST_CMDLINE, // $CMDLINE
	INST_INSTDIR, // $INSTDIR
	INST_OUTDIR,  // $OUTDIR
	INST_EXEDIR,  // $EXEDIR
	INST_LANG,    // $LANGUAGE
	__INST_LAST
};

#endif
