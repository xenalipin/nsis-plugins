//
// dmi_entry_type
//
typedef enum {
	DMI_ENTRY_BIOS,
	DMI_ENTRY_SYSTEM,
	DMI_ENTRY_BASEBOARD,
	DMI_ENTRY_CHASSIS,
	DMI_ENTRY_PROCESSOR,
	DMI_ENTRY_MEM_CONTROLLER,
	DMI_ENTRY_MEM_MODULE,
	DMI_ENTRY_CACHE,
	DMI_ENTRY_PORT_CONNECTOR,
	DMI_ENTRY_SYSTEM_SLOT,
	DMI_ENTRY_ONBOARD_DEVICE,
	DMI_ENTRY_OEMSTRINGS,
	DMI_ENTRY_SYSCONF,
	DMI_ENTRY_BIOS_LANG,
	DMI_ENTRY_GROUP_ASSOC,
	DMI_ENTRY_SYSTEM_EVENT_LOG,
	DMI_ENTRY_PHYS_MEM_ARRAY,
	DMI_ENTRY_MEM_DEVICE,
	DMI_ENTRY_32_MEM_ERROR,
	DMI_ENTRY_MEM_ARRAY_MAPPED_ADDR,
	DMI_ENTRY_MEM_DEV_MAPPED_ADDR,
	DMI_ENTRY_BUILTIN_POINTING_DEV,
	DMI_ENTRY_PORTABLE_BATTERY,
	DMI_ENTRY_SYSTEM_RESET,
	DMI_ENTRY_HW_SECURITY,
	DMI_ENTRY_SYSTEM_POWER_CONTROLS,
	DMI_ENTRY_VOLTAGE_PROBE,
	DMI_ENTRY_COOLING_DEV,
	DMI_ENTRY_TEMP_PROBE,
	DMI_ENTRY_ELECTRICAL_CURRENT_PROBE,
	DMI_ENTRY_OOB_REMOTE_ACCESS,
	DMI_ENTRY_BIS_ENTRY,
	DMI_ENTRY_SYSTEM_BOOT,
	DMI_ENTRY_64_MEM_ERROR,
	DMI_ENTRY_MGMT_DEV,
	DMI_ENTRY_MGMT_DEV_COMPONENT,
	DMI_ENTRY_MGMT_DEV_THRES,
	DMI_ENTRY_MEM_CHANNEL,
	DMI_ENTRY_IPMI_DEV,
	DMI_ENTRY_SYS_POWER_SUPPLY,
	DMI_ENTRY_ADDITIONAL,
	DMI_ENTRY_ONBOARD_DEV_EXT,
	DMI_ENTRY_MGMT_CONTROLLER_HOST,
	DMI_ENTRY_INACTIVE = 126,
	DMI_ENTRY_END_OF_TABLE
} DMI_ENTRY_TYPE;

#include <pshpack1.h>

typedef struct {
	BYTE        Used20CallingMethod;
	BYTE        SMBIOSMajorVersion;
	BYTE        SMBIOSMinorVersion;
	BYTE        DmiRevision;
	DWORD       Length;
	BYTE        SMBIOSTableData[];
} RawSMBIOSData;

typedef struct {
	BYTE        Type;
	BYTE        Length;
	WORD        Handle;
} DMI_HEADER;

//
// BIOS Information (Type 0)
//

typedef struct _SMBIOS_BIOS_INFORMATION {
	// 2.0+
	DMI_HEADER  Header;
	BYTE        Vendor;
	BYTE        BIOSVersion;
	WORD        BIOSStartingAddressSegment;
	BYTE        BIOSReleaseDate;
	BYTE        BIOSROMSize;
	DWORD64     BIOSCharacteristics;
	// 2.4+
	BYTE        BIOSCharacteristicsExtensionBytes[2];
	BYTE        SystemBIOSMajorRelease;
	BYTE        SystemBIOSMinorRelease;
	BYTE        EmbeddedControllerFirmwareMajorRelease;
	BYTE        EmbeddedControllerFirmwareMinorRelease;
	// 3.1+ 
	WORD        ExtendedBIOSROMSize;
} SMBIOS_BIOS_INFORMATION;

//
// SMBIOS_UUID
//

typedef struct {
	DWORD       time_low;
	WORD        time_mid;
	WORD        time_hi_and_version;
	BYTE        clock_seq_hi_and_reserved;
	BYTE        clock_seq_low;
	BYTE        Node[6];
} SMBIOS_UUID;

//
// System Information (Type 1) structure
//

typedef struct {
	// 2.0+
	DMI_HEADER  Header;
	BYTE        Manufacturer;
	BYTE        ProductName;
	BYTE        Version;
	BYTE        SerialNumber;
	// 2.1+ 
	BYTE        UUID[16];
	BYTE        WakeupType;
	// 2.4+ 
	BYTE        SKUNumber;
	BYTE        Family;
} SMBIOS_SYSTEM_INFORMATION;

//
// Baseboard (or Module) Information (Type 2)
//

typedef struct {
	// 2.0+
	DMI_HEADER  Header;
	BYTE        Manufacturer;       // Number of null-terminated string
	BYTE        Product;            // Number of null-terminated string
	BYTE        Version;            // Number of null-terminated string
	BYTE        SerialNumber;       // Number of null-terminated string
	BYTE        AssetTag;           // Number of null-terminated string
	BYTE        FeatureFlags;
	BYTE        LocationInChassis;  // Number of null-terminated string
	WORD        ChassisHandle;
	BYTE        BoardType;
	BYTE        NumberOfContainedObjectHandles;
	WORD        ContainedObjectHandles[];
} SMBIOS_BASEBOARD_INFORMATION;

//
// System Enclosure or Chassis (Type 3)
//

typedef struct {
	BYTE        ContainedElementType;
	BYTE        ContainedElementMinimum;
	BYTE        ContainedElementMaximum;
} SMBIOS_SYSTEM_ENCLOSURE_CONTAINED_ELEMENTS;

typedef struct {
	// 2.0+
	DMI_HEADER  Header;
	BYTE        Manufacturer;       // Number of null-terminated string
	BYTE        EnclosureType;
	BYTE        Version;            // Number of null-terminated string
	BYTE        SerialNumber;       // Number of null-terminated string
	BYTE        AssetTagNumber;     // Number of null-terminated string
	// 2.1+ 
	BYTE        BootupState;
	BYTE        PowerSupplyState;
	BYTE        ThermalState;
	BYTE        SecurityStatus;
	// 2.3+ 
	DWORD       VendorSpecific;
	BYTE        Height;
	BYTE        NumberOfPowerCords;
	BYTE        ContainedElementCount;
	BYTE        ContainedElementRecordLength;
//	BYTE        ContainedElements[];
	// 2.7+ 
//	BYTE        SKUNumber;
} SMBIOS_SYSTEM_ENCLOSURE_INFORMATION;

//
//  Processor Information (Type 4)
//

typedef struct {
	// 2.0+
	DMI_HEADER  Header;
	BYTE        SocketDesignation;      // Number of null-terminated string
	BYTE        ProcessorType;
	BYTE        ProcessorFamily;
	BYTE        ProcessorManufacturer;  // Number of null-terminated string
	DWORD64     ProcessorID;
	BYTE        ProcessorVersion;       // Number of null-terminated string
	BYTE        Voltage;
	WORD        ExternalClock;
	WORD        MaxSpeed;
	WORD        CurrentSpeed;
	BYTE        Status;
	BYTE        ProcessorUpgrade;
	// 2.1+
	WORD        L1CacheHandle;
	WORD        L2CacheHandle;
	WORD        L3CacheHandle;
	// 2.3+ 
	BYTE        SerialNumber;           // Number of null-terminated string
	BYTE        AssetTag;               // Number of null-terminated string
	BYTE        PartNumber;             // Number of null-terminated string
	// 2.5+ 
	BYTE        CoreCount;
	BYTE        CoreEnabled;
	BYTE        ThreadCount;
	WORD        ProcessorCharacteristics;
	// 2.6+ 
	WORD        ProcessorFamily2;
	// 3.0+ 
	WORD        CoreCount2;
	WORD        CoreEnabled2;
	WORD        ThreadCount2;
	// 3.6+ 
	WORD        ThreadEnabled;
} SMBIOS_PROCESSOR_INFORMATION;

//
// Memory Controller Information (Type 5, Obsolete)
//
typedef struct {
	// 2.0+
	DMI_HEADER  Header;
	BYTE        ErrorDetectingMethod;
	BYTE        ErrorCorrectingCapability;
	BYTE        SupportedInterleave;
	BYTE        CurrentInterleave;
	BYTE        MaximumMemoryModuleSize;
	WORD        SupportedSpeeds;
	WORD        SupportedMemoryTypes;
	BYTE        MemoryModuleVoltage;
	BYTE        NumberOfAssociatedMemorySlots;
//	WORD        MemoryModuleConfigurationHandles[];     // NumberOfAssociatedMemorySlots
	// 2.1+ 
//	BYTE        EnabledErrorCorrectingCapabilities;
} SMBIOS_MEMORY_CONTROLLER_INFORMATION;

//
// Memory Module Information (Type 6, Obsolete)
//
typedef struct {
	// 2.0+
	DMI_HEADER  Header;
	BYTE        SocketDesignation;
	BYTE        BankConnections;
	BYTE        CurrentSpeed;
	WORD        CurrentMemoryType;
	BYTE        InstalledSize;
	BYTE        EnabledSize;
	BYTE        ErrorStatus;
} SMBIOS_MEMORY_MODULE_INFORMATION;

//
// Cache Information (Type 7)
//
typedef struct {
	// 2.0+
	DMI_HEADER  Header;
	BYTE        SocketDesignation;
	WORD        CacheConfiguration;
	BYTE        MaximumCacheSize;
	BYTE        InstalledSize;
	WORD        SupportedSRAMType;
	WORD        CurrentSRAMType;
	// 2.1+ 
	BYTE        CacheSpeed;
	BYTE        ErrorCorrectionType;
	BYTE        SystemCacheType;
	BYTE        Associativity;
	// 3.1+ 
	DWORD       MaximumCacheSize2;
	DWORD       InstalledCacheSize2;
} SMBIOS_CACHE_INFORMATION;

//
// Port Connector Information (Type 8)
//
typedef struct {
	// 2.0+
	DMI_HEADER  Header;
	BYTE        InternalReferenceDesignator;
	BYTE        InternalConnectorType;
	BYTE        ExternalReferenceDesignator;
	BYTE        ExternalConnectorType;
	BYTE        PortType;
} SMBIOS_PORT_CONNECTOR_INFORMATION;

//
// System Slots (Type 9)
//
typedef struct {
	// 2.0+
	DMI_HEADER  Header;
	BYTE        SlotDesignation;
	BYTE        SlotType;
	BYTE        SlotDataBusWidth;
	BYTE        CurrentUsage;
	BYTE        SlotLength;
	WORD        SlotID;
	BYTE        SlotCharacteristics1;
	// 2.1+ 
	BYTE        SlotCharacteristics2;
	// 2.6+ 
	WORD        SegmentGroupNumber;
	BYTE        BusNumber;
	BYTE        DeviceNumber;
	// 3.2+ 
	BYTE        DataBusWidth;
	BYTE        PeerGroupingCount;
	BYTE        PeerGroups[];
} SMBIOS_SYSTEM_SLOTS_INFORMATION;

//
// On Board Devices Information (Type 10, Obsolete)
//
typedef struct {
	// 2.0+
	DMI_HEADER  Header;
	BYTE        DeviceType[];
} SMBIOS_ON_BOARD_DEVICES_INFORMATION;

//
// OEM Strings (Type 11)
//
typedef struct {
	// 2.0+
	DMI_HEADER  Header;
	BYTE        Count;
} SMBIOS_OEM_STRINGS_INFORMATION;

//
// Physical Memory Array (Type 16)
//
typedef struct {
	// 2.1+
	DMI_HEADER  Header;
	BYTE        Location;
	BYTE        Use;
	BYTE        MemoryErrorCorrection;
	DWORD       MaximumCapacity;
	WORD        MemoryErrorInformationHandle;
	WORD        NumberOfMemoryDevices;
	// 2.7+ 
	DWORD64     ExtendedMaximumCapacity;
} PHYSICAL_MEMORY_ARRAY_INFORMATION;

//
// Memory Device (Type 17)
//
typedef struct {
	// 2.1+
	DMI_HEADER  Header;
	WORD        PhysicalMemoryArrayHandle;
	WORD        MemoryErrorInformationHandle;
	WORD        TotalWidth;
	WORD        DataWidth;
	WORD        Size;
	BYTE        FormFactor;
	BYTE        DeviceSet;
	BYTE        DeviceLocator;
	BYTE        BankLocator;
	BYTE        MemoryType;
	WORD        TypeDetail;
	// 2.3+
	WORD        Speed;
	BYTE        Manufacturer;
	BYTE        SerialNumber;
	BYTE        AssetTag;
	BYTE        PartNumber;
	// 2.6+ 
	BYTE        Attributes;
	// 2.7+ 
	DWORD       ExtendedSize;
	WORD        ConfiguredMemoryClockSpeed;
	// 2.8+ 
	WORD        MinimumVoltage;
	WORD        MaximumVoltage;
	WORD        ConfiguredVoltage;
	// 3.2+ 
	BYTE        MemoryTechnology;
	WORD        MemoryOperatingModeCapability;
	BYTE        FirmwareVersion;
	WORD        ModuleManufacturerID;
	WORD        ModuleProductID;
	WORD        MemorySubsystemControllerManufacturerID;
	WORD        MemorySubsystemControllerProductID;
	DWORD64     NonVolatileSize;
	DWORD64     VolatileSize;
	DWORD64     CacheSize;
	DWORD64     LogicalSize;
	// 3.3+ 
	DWORD       ExtendedSpeed;
	DWORD       ExtendedConfiguredMemorySpeed;
} MEMORY_DEVICE_INFORMATION;

//
// Memory Array Mapped Address (Type 19)
//
typedef struct {
	// 2.0+
	DMI_HEADER  Header;
	DWORD       StartingAddress;
	DWORD       EndingAddress;
	WORD        MemoryArrayHandle;
	BYTE        PartitionWidth;
	// 2.7+ 
	DWORD64     ExtendedStartingAddress;
	DWORD64     ExtendedEndingAddress;
} MEMORY_ARRAY_MAPPED_ADDRESS_INFORMATION;

#include <poppack.h>
