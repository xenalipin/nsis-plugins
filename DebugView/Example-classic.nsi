#
# NSIS 3.x 支持
#

#Unicode true

!ifdef NSIS_PACKEDVERSION
!define /math NSIS_VERSON_MAJOR ${NSIS_PACKEDVERSION} >> 24
!else
!define NSIS_VERSON_MAJOR 2
!endif

!if ${NSIS_VERSON_MAJOR} > 2

#
# NSIS 3.x 方式引用插件
#
!addplugindir "/x86-unicode" ".\x86-unicode"
!addplugindir "/x86-ansi" ".\x86-ansi"

!else

#
# NSIS 2.x 方式引用插件
#
!ifdef NSIS_UNICODE
!addplugindir ".\x86-unicode"
!else
!addplugindir ".\x86-ansi"
!endif

!endif

#
# Required header files
#
!include LogicLib.nsh

#
# DebugView Example
#
Name "DebugView"
Outfile "DebugView.exe"
Caption "$(^Name)"

InstallDir $TEMP

XPStyle on
ShowInstDetails show
RequestExecutionLevel admin
InstallColors /windows

Page directory
Page instfiles "" "Instfiles.Show" "Instfiles.Leave"

Section "-Install"
	${For} $R0 0 100
		DetailPrint "$R0"
		Sleep 100
	${Next}
SectionEnd

Function .onGUIInit
	DebugView::Start
FunctionEnd

Function Instfiles.Show
	DebugView::Attach
FunctionEnd

Function Instfiles.Leave
	DebugView::Detach
FunctionEnd

Function .onGUIEnd
	DebugView::Stop
FunctionEnd