enum {
	BiosInfo,
	SystemInfo,
	BaseboardInfo,
	SystemEnclosureInfo,
	ProcessorInfo,
	MemoryControllerInfo,
	MemoryModuleInfo,
	CacheInfo,
	PortConnectorInfo,
	SystemSlots,
	OnBoardDevicesInfo,
	OEMStrings,
	SystemConfigOptions,
	BiosLanguageInfo,
	GroupAssociations,
	SystemEventLogInfo,
	PhysicalMemoryArrayInfo,
	MemoryDeviceInfo,
	MemoryErrorInfo32,
	MemoryArrayMappedAddress,
	MemoryDeviceMappedAddress,
	BuiltInPointingDevice,
	PortableBattery,
	SystemReset,
	HardwareSecurity,
	SystemPowerControls,
	VoltageProbe,
	CoolingDevice,
	TemperatureProbe,
	ElectricalCurrentProbe,
	OutOfBandRemoteAccess,
	BootIntegrityServicesEntryPoint, // Boot Integrity Service
	SystemBootInfo,
	MemoryErrorInfo64,
	ManagementDevice,
	ManagementDeviceComponent,
	ManagementDeviceThresholdData,
	ManagementChannel,
	IPMIDeviceInfo,
	SystemPowerSupply,
	AdditionalInfo,
	OnBoardDevicesInfoEx,
	ManagementControllerHostInterface,
	Inactive,
	EndOfTable
};

typedef struct {
	BYTE    Used20CallingMethod;
	BYTE    SMBIOSMajorVersion;
	BYTE    SMBIOSMinorVersion;
	BYTE    DmiRevision;
	DWORD   Length;
	BYTE    SMBIOSTableData[];
} RawSMBIOSData;

#include <pshpack1.h>

typedef struct {
	BYTE    Type;
	BYTE    Length;
	WORD    Handle;
} SMBIOS_HEADER;

//
// BIOS Information (Type 0)
//

typedef struct _SMBIOS_BIOS_INFORMATION {
	BYTE    Type;
	BYTE    Length;
	WORD    Handle;
	BYTE    Vendor;
	BYTE    BIOSVersion;
	WORD    BIOSStartingAddressSegment;
	BYTE    BIOSReleaseDate;
	BYTE    BIOSROMSize;
	DWORD64 BIOSCharacteristics;
	BYTE    BIOSCharacteristicsForExtensionBytes[2];
	BYTE    SystemBIOSMajorRelease;
	BYTE    SystemBIOSMinorRelease;
	BYTE    EmbeddedControllerFirmwareMajorRelease;
	BYTE    EmbeddedControllerFirmwareMinorRelease;
	WORD    ExtendedBIOSROMSize;
} SMBIOS_BIOS_INFORMATION;

#define SMBIOS_TYPE00_SIZE_0200 FIELD_OFFSET(SMBIOS_BIOS_INFORMATION, BIOSCharacteristicsForExtensionBytes)
#define SMBIOS_TYPE00_SIZE_0204 FIELD_OFFSET(SMBIOS_BIOS_INFORMATION, ExtendedBIOSROMSize)
#define SMBIOS_TYPE00_SIZE_0301 sizeof(SMBIOS_BIOS_INFORMATION)
#define SMBIOS_TYPE00_SIZE_CURR SMBIOS_TYPE00_SIZE_0301

//
// SMBIOS_UUID
//

typedef struct {
	DWORD   time_low;
	WORD    time_mid;
	WORD    time_hi_and_version;
	BYTE    clock_seq_hi_and_reserved;
	BYTE    clock_seq_low;
	BYTE    Node[6];
} SMBIOS_UUID;

//
// System Information (Type 1) structure
//

typedef struct {
	BYTE    Type;
	BYTE    Length;
	WORD    Handle;
	BYTE    Manufacturer;
	BYTE    ProductName;
	BYTE    Version;
	BYTE    SerialNumber;
	BYTE    UUID[16];
	BYTE    WakeupType;
	BYTE    SKUNumber;
	BYTE    Family;
} SMBIOS_SYSTEM_INFORMATION;

#define SMBIOS_TYPE01_SIZE_0200 FIELD_OFFSET(SMBIOS_SYSTEM_INFORMATION, UUID)
#define SMBIOS_TYPE01_SIZE_0201 FIELD_OFFSET(SMBIOS_SYSTEM_INFORMATION, SKUNumber)
#define SMBIOS_TYPE01_SIZE_0204 sizeof(SMBIOS_SYSTEM_INFORMATION)
#define SMBIOS_TYPE01_SIZE_CURR SMBIOS_TYPE01_SIZE_0204

//
// Baseboard (or Module) Information (Type 2)
//

typedef struct {
	BYTE    Type;               // Baseboard Information indicator
	BYTE    Length;             // Length of the structure, at least 08h
	WORD    Handle;
	BYTE    Manufacturer;       // Number of null-terminated string
	BYTE    Product;            // Number of null-terminated string
	BYTE    Version;            // Number of null-terminated string
	BYTE    SerialNumber;       // Number of null-terminated string
	BYTE    AssetTag;           // Number of null-terminated string
	BYTE    FeatureFlags;
	BYTE    LocationInChassis;  // Number of null-terminated string
	WORD    ChassisHandle;
	BYTE    BoardType;
	BYTE    NumberOfContainedObjectHandles;
	WORD    ContainedObjectHandles[];
} SMBIOS_BASEBOARD_INFORMATION;

//
// System Enclosure or Chassis (Type 3)
//

typedef struct {
	BYTE    ContainedElementType;
	BYTE    ContainedElementMinimum;
	BYTE    ContainedElementMaximum;
} SMBIOS_SYSTEM_ENCLOSURE_CONTAINED_ELEMENTS;

typedef struct {
	BYTE    Type;               // System Enclosure indicator
	BYTE    Length;
	WORD    Handle;
	BYTE    Manufacturer;       // Number of null-terminated string
	BYTE    EnclosureType;
	BYTE    Version;            // Number of null-terminated string
	BYTE    SerialNumber;       // Number of null-terminated string
	BYTE    AssetTagNumber;     // Number of null-terminated string
	BYTE    BootupState;
	BYTE    PowerSupplyState;
	BYTE    ThermalState;
	BYTE    SecurityStatus;
	DWORD   VendorSpecific;
	BYTE    Height;
	BYTE    NumberOfPowerCords;
	BYTE    ContainedElementCount;
	BYTE    ContainedElementRecordLength;
	BYTE    ExtraData[];
//	BYTE    ContainedElements[];
//	BYTE    SKUNumber;
} SMBIOS_SYSTEM_ENCLOSURE_INFORMATION;

//
//  Processor Information (Type 4)
//

typedef struct {
	BYTE    Type;                   // Processor Information indicator
	BYTE    Length;
	WORD    Handle;
	BYTE    SocketDesignation;      // Number of null-terminated string
	BYTE    ProcessorType;
	BYTE    ProcessorFamily;
	BYTE    ProcessorManufacturer;  // Number of null-terminated string
	DWORD64 ProcessorID;
	BYTE    ProcessorVersion;       // Number of null-terminated string
	BYTE    Voltage;
	WORD    ExternalClock;
	WORD    MaxSpeed;
	WORD    CurrentSpeed;
	BYTE    Status;
	BYTE    ProcessorUpgrade;
	WORD    L1CacheHandle;
	WORD    L2CacheHandle;
	WORD    L3CacheHandle;
	BYTE    SerialNumber;           // Number of null-terminated string
	BYTE    AssetTag;               // Number of null-terminated string
	BYTE    PartNumber;             // Number of null-terminated string
	BYTE    CoreCount;
	BYTE    CoreEnabled;
	BYTE    ThreadCount;
	WORD    ProcessorCharacteristics;
	WORD    ProcessorFamily2;
	WORD    CoreCount2;
	WORD    CoreEnabled2;
	WORD    ThreadCount2;
	WORD    ThreadEnabled;
} SMBIOS_PROCESSOR_INFORMATION;

#define SMBIOS_TYPE04_SIZE_0200 FIELD_OFFSET(SMBIOS_PROCESSOR_INFORMATION, L1CacheHandle)
#define SMBIOS_TYPE04_SIZE_0201 FIELD_OFFSET(SMBIOS_PROCESSOR_INFORMATION, SerialNumber)
#define SMBIOS_TYPE04_SIZE_0203 FIELD_OFFSET(SMBIOS_PROCESSOR_INFORMATION, CoreCount)
#define SMBIOS_TYPE04_SIZE_0205 FIELD_OFFSET(SMBIOS_PROCESSOR_INFORMATION, ProcessorFamily2)
#define SMBIOS_TYPE04_SIZE_0206 FIELD_OFFSET(SMBIOS_PROCESSOR_INFORMATION, CoreCount2)
#define SMBIOS_TYPE04_SIZE_0300 FIELD_OFFSET(SMBIOS_PROCESSOR_INFORMATION, ThreadEnabled)
#define SMBIOS_TYPE04_SIZE_0306 sizeof(SMBIOS_PROCESSOR_INFORMATION)
#define SMBIOS_TYPE04_SIZE_CURR SMBIOS_TYPE04_SIZE_0306

//
// Memory Controller Information (Type 5, Obsolete)
//
typedef struct {
	BYTE    Type;                                   // Memory Controller indicator
	BYTE    Length;
	WORD    Handle;
	BYTE    ErrorDetectingMethod;
	BYTE    ErrorCorrectingCapability;
	BYTE    SupportedInterleave;
	BYTE    CurrentInterleave;
	BYTE    MaximumMemoryModuleSize;
	WORD    SupportedSpeeds;
	WORD    SupportedMemoryTypes;
	BYTE    MemoryModuleVoltage;
	BYTE    NumberOfAssociatedMemorySlots;
//	WORD    MemoryModuleConfigurationHandles[];     // NumberOfAssociatedMemorySlots
//	BYTE    EnabledErrorCorrectingCapabilities;
} SMBIOS_MEMORY_CONTROLLER_INFORMATION;


//
// Memory Module Information (Type 6, Obsolete)
//
typedef struct {
	BYTE    Type;                                   // Memory Module Configuration indicator
	BYTE    Length;
	WORD    Handle;
	BYTE    SocketDesignation;
	BYTE    BankConnections;
	BYTE    CurrentSpeed;
	WORD    CurrentMemoryType;
	BYTE    InstalledSize;
	BYTE    EnabledSize;
	BYTE    ErrorStatus;
} SMBIOS_MEMORY_MODULE_INFORMATION;

//
// Cache Information (Type 7)
//
typedef struct {
	BYTE    Type;                                   // Cache Information indicator
	BYTE    Length;
	WORD    Handle;
	BYTE    SocketDesignation;
	WORD    CacheConfiguration;
	BYTE    MaximumCacheSize;
	BYTE    InstalledSize;
	WORD    SupportedSRAMType;
	WORD    CurrentSRAMType;
	BYTE    CacheSpeed;
	BYTE    ErrorCorrectionType;
	BYTE    SystemCacheType;
	BYTE    Associativity;
	DWORD   MaximumCacheSize2;
	DWORD   InstalledCacheSize2;
} SMBIOS_CACHE_INFORMATION;

//
// Port Connector Information (Type 8)
//
typedef struct {
	BYTE    Type;                                   // Connector Information indicator
	BYTE    Length;
	WORD    Handle;
	BYTE    InternalReferenceDesignator;
	BYTE    InternalConnectorType;
	BYTE    ExternalReferenceDesignator;
	BYTE    ExternalConnectorType;
	BYTE    PortType;
} SMBIOS_PORT_CONNECTOR_INFORMATION;

//
// System Slots (Type 9)
//
typedef struct {
	BYTE    Type;                                   // System Slot Structure indicator
	BYTE    Length;
	WORD    Handle;
	BYTE    SlotDesignation;
	BYTE    SlotType;
	BYTE    SlotDataBusWidth;
	BYTE    CurrentUsage;
	BYTE    SlotLength;
	WORD    SlotID;
	BYTE    SlotCharacteristics1;
	BYTE    SlotCharacteristics2;
	WORD    SegmentGroupNumber;
	BYTE    BusNumber;
	BYTE    DeviceNumber;
	BYTE    DataBusWidth;
	BYTE    PeerGroupingCount;
	BYTE    PeerGroups[];
} SMBIOS_SYSTEM_SLOTS_INFORMATION;

//
// On Board Devices Information (Type 10, Obsolete)
//
typedef struct {
	BYTE    Type;                                   // On Board Devices Information indicator
	BYTE    Length;
	WORD    Handle;
	BYTE    DeviceType[];
} SMBIOS_ON_BOARD_DEVICES_INFORMATION;

//
// OEM Strings (Type 11)
//
typedef struct {
	BYTE    Type;                                   // OEM Strings indicator
	BYTE    Length;
	WORD    Handle;
	BYTE    Count;
} SMBIOS_OEM_STRINGS_INFORMATION;

//
// Physical Memory Array (Type 16)
//
typedef struct {
	BYTE    Type;
	BYTE    Length;
	WORD    Handle;
	BYTE    Location;
	BYTE    Use;
	BYTE    MemoryErrorCorrection;
	DWORD   MaximumCapacity;
	WORD    MemoryErrorInformationHandle;
	WORD    NumberOfMemoryDevices;
	DWORD64 ExtendedMaximumCapacity;
} PHYSICAL_MEMORY_ARRAY_INFORMATION;

#define SMBIOS_TYPE16_SIZE_0201 FIELD_OFFSET(PHYSICAL_MEMORY_ARRAY_INFORMATION, ExtendedMaximumCapacity)
#define SMBIOS_TYPE16_SIZE_0207 sizeof(PHYSICAL_MEMORY_ARRAY_INFORMATION)
#define SMBIOS_TYPE16_SIZE_CURR SMBIOS_TYPE16_SIZE_0207

//
// Memory Device (Type 17)
//
typedef struct {
	BYTE    Type;
	BYTE    Length;
	WORD    Handle;
	WORD    PhysicalMemoryArrayHandle;
	WORD    MemoryErrorInformationHandle;
	WORD    TotalWidth;
	WORD    DataWidth;
	WORD    Size;
	BYTE    FormFactor;
	BYTE    DeviceSet;
	BYTE    DeviceLocator;
	BYTE    BankLocator;
	BYTE    MemoryType;
	WORD    TypeDetail;
	WORD    Speed;
	BYTE    Manufacturer;
	BYTE    SerialNumber;
	BYTE    AssetTag;
	BYTE    PartNumber;
	BYTE    Attributes;
	DWORD   ExtendedSize;
	WORD    ConfiguredMemoryClockSpeed;
	WORD    MinimumVoltage;
	WORD    MaximumVoltage;
	WORD    ConfiguredVoltage;
	BYTE    MemoryTechnology;
	WORD    MemoryOperatingModeCapability;
	BYTE    FirmwareVersion;
	WORD    ModuleManufacturerID;
	WORD    ModuleProductID;
	WORD    MemorySubsystemControllerManufacturerID;
	WORD    MemorySubsystemControllerProductID;
	DWORD64 NonVolatileSize;
	DWORD64 VolatileSize;
	DWORD64 CacheSize;
	DWORD64 LogicalSize;
	DWORD   ExtendedSpeed;
	DWORD   ExtendedConfiguredMemorySpeed;
} MEMORY_DEVICE_INFORMATION;

#define SMBIOS_TYPE17_SIZE_0201 FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, Speed)
#define SMBIOS_TYPE17_SIZE_0203 FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, Attributes)
#define SMBIOS_TYPE17_SIZE_0206 FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, ExtendedSize)
#define SMBIOS_TYPE17_SIZE_0207 FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, MinimumVoltage)
#define SMBIOS_TYPE17_SIZE_0208 FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, MemoryTechnology)
#define SMBIOS_TYPE17_SIZE_0302 FIELD_OFFSET(MEMORY_DEVICE_INFORMATION, ExtendedSpeed)
#define SMBIOS_TYPE17_SIZE_0303 sizeof(MEMORY_DEVICE_INFORMATION)
#define SMBIOS_TYPE17_SIZE_CURR SMBIOS_TYPE17_SIZE_0303

//
// Memory Array Mapped Address (Type 19)
//
typedef struct {
	BYTE    Type;
	BYTE    Length;
	WORD    Handle;
	DWORD   StartingAddress;
	DWORD   EndingAddress;
	WORD    MemoryArrayHandle;
	BYTE    PartitionWidth;
	DWORD64 ExtendedStartingAddress;
	DWORD64 ExtendedEndingAddress;
} MEMORY_ARRAY_MAPPED_ADDRESS_INFORMATION;

#include <poppack.h>
