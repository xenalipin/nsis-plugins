#Unicode true

!ifdef NSIS_UNICODE
!addplugindir ".\x86-unicode"
!else
!addplugindir ".\x86-ansi"
!endif

Name Example
OutFile Example.exe

XPStyle on
InstallColors /windows
ShowInstDetails show

Section Install
	# System Information (Type 1)
	DetailPrint "========================================"

	# System Information (Type 1) - Manufacturer
	smbios::GetSystemManufacturer
	Pop $R0
	DetailPrint "System - Manufacturer: $R0"
	# System Information (Type 1) - SerialNumber
	smbios::GetSystemSerialNumber
	Pop $R0
	DetailPrint "System - SerialNumber: $R0"
	# System Information (Type 1) - ProductName
	smbios::GetSystemProductName
	Pop $R0
	DetailPrint "System - ProductName: $R0"
	# System Information (Type 1) - Family
	smbios::GetSystemFamily
	Pop $R0
	DetailPrint "System - Family: $R0"
	# System Information (Type 1) - UUID
	smbios::GetSystemUUID
	Pop $R0
	DetailPrint "System - UUID: $R0"

	# Baseboard (or Module) (Type 2)
	DetailPrint "========================================"

	# Baseboard (or Module) (Type 2) - Manufacturer
	smbios::GetModuleManufacturer
	Pop $R0
	DetailPrint "Module - Manufacturer: $R0"
	# Baseboard (or Module) (Type 2) - SerialNumber
	smbios::GetModuleSerialNumber
	Pop $R0
	DetailPrint "Module - SerialNumber: $R0"
	# Baseboard (or Module) (Type 2) - ProductName
	smbios::GetModuleProductName
	Pop $R0
	DetailPrint "Module - ProductName: $R0"

	# Processor Information (Type 4)
	DetailPrint "========================================"

	# Processor Information (Type 4) - Processor ID
	smbios::GetProcessorID
	Pop $R0
	DetailPrint "Processor - ID: $R0"

	# Physical Memory Array (Type 16)
	DetailPrint "========================================"

	# Physical Memory Array (Type 16) - MaximumCapacity
	smbios::GetMemoryArrayCapacity
	Pop $R0
	DetailPrint "Memory Array - MaximumCapacity: $R0 MB"

	# Physical Memory Array (Type 16) - NumberOfMemoryDevices
	smbios::GetMemoryArrayNumbers
	Pop $R0
	DetailPrint "Memory Array - NumberOfDevices: $R0"

	# Memory Device (Type 17)
	DetailPrint "========================================"

	StrCpy $0 0

lbl_loop:
	# Memory Device (Type 17) - Size
	smbios::GetMemorySize $0
	Pop $R0

	; no more memory devices found
	StrCmp $R0 "" lbl_done

	; not installed (0)
	; size unknown (-1)
	IntCmp $R0 0 lbl_skip lbl_skip
	; size valid
	DetailPrint "Memory Device [$0] - Size: $R0 MB"

	# Memory Device (Type 17) - Manufacturer
	smbios::GetMemoryManufacturer $0
	Pop $R0
	DetailPrint "Memory Device [$0] - Manufacturer: $R0"

	# Memory Device (Type 17) - SerialNumber
	smbios::GetMemorySerialNumber $0
	Pop $R0
	DetailPrint "Memory Device [$0] - SerialNumber: $R0"

	# Memory Device (Type 17) - Speed
	smbios::GetMemorySpeed $0
	Pop $R0
	DetailPrint "Memory Device [$0] - Speed: $R0 MHz"

lbl_skip:
	IntOp $0 $0 + 1
	Goto lbl_loop

lbl_done:
	DetailPrint "Memory Devices: $0"

SectionEnd
