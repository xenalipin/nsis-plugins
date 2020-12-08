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
!include MUI2.nsh

#
# DebugView Example
#
Name "DebugView"
Outfile "DebugView.exe"
Caption "$(^Name)"

InstallDir $TEMP

ShowInstDetails show
RequestExecutionLevel admin

!define MUI_CUSTOMFUNCTION_GUIINIT onGUIInit

!insertmacro MUI_PAGE_DIRECTORY
!define MUI_PAGE_CUSTOMFUNCTION_SHOW "Instfiles.Show"
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE "Instfiles.Leave"
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_LANGUAGE English

Section "-Install"
	${For} $R0 0 100
		DetailPrint "$R0"
		Sleep 100
	${Next}
SectionEnd

Function onGUIInit
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