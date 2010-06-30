#ifndef Multiboot_H
#define Multiboot_H

struct ELFHeaderTable
{
	unsigned int num;		//Number of section headers
	unsigned int size;		//Size of each section header
	unsigned int addr;		//The address of the first section header
	unsigned int shndx;		//Section header string table index
} __attribute__((packed));

struct AOUTSymbolTable
{
	unsigned int tabsize;
	unsigned int strsize;
	unsigned int addr;
	unsigned int reserved;
} __attribute__((packed));

//Bochs 2.4 doesn't provide a ROM configuration table. Virtual PC does.
struct ROMConfigurationTable
{
	unsigned short Length;
	unsigned char Model;
	unsigned char Submodel;
	unsigned char BiosRevision;
	bool DualBus : 1;
	bool MicroChannelBus : 1;
	bool EBDAExists : 1;
	bool WaitForExternalEventSupported : 1;
	bool Reserved0 : 1;
	bool HasRTC : 1;
	bool MultipleInterruptControllers : 1;
	bool BiosUsesDMA3 : 1;
	bool Reserved1 : 1;
	bool DataStreamingSupported : 1;
	bool NonStandardKeyboard : 1;
	bool BiosControlCpu : 1;
	bool BiosGetMemoryMap : 1;
	bool BiosGetPosData : 1;
	bool BiosKeyboard : 1;
	bool DMA32Supported : 1;
	bool ImlSCSISupported : 1;
	bool ImlLoad : 1;
	bool InformationPanelInstalled : 1;
	bool SCSISupported : 1;
	bool RomToRamSupported : 1;
	bool Reserved2 : 3;
	bool ExtendedPOST : 1;
	bool MemorySplit16MB : 1;
	unsigned char Reserved3 : 1;
	unsigned char AdvancedBIOSPresence : 3;
	bool EEPROMPresent : 1;
	bool Reserved4 : 1;
	bool FlashEPROMPresent : 1;
	bool EnhancedMouseMode : 1;
	unsigned char Reserved5 : 6;
} __attribute__((packed));

struct Module
{
	unsigned int Start;
	unsigned int End;
	char *Name;
	unsigned int Reserved;
} __attribute__((packed));

struct MemoryMapElement
{
	unsigned int Size;
	unsigned int BaseAddressLow;
	unsigned int BaseAddressHigh;
	unsigned int LengthLow;
	unsigned int LengthHigh;
	unsigned int Type;
} __attribute__((packed));

struct DriveStructure
{
	unsigned int Size;
	unsigned char DriveNumber;
	unsigned char Mode;
	unsigned short Cylinders;
	unsigned char Heads;
	unsigned char Sectors;	//Following this field is a list of ports used by the drive, terminating in zero. This means that the Size field has to be used, not sizeof()
} __attribute__((packed));

struct APMTable
{
	unsigned short Version;
	unsigned short CS;
	unsigned int Offset;
	unsigned short CS16Bit;	//This is the 16-bit protected mode code segment
	unsigned short DS;
	unsigned short Flags;
	unsigned short CSLength;
	unsigned short CS16BitLength;
	unsigned short DSLength;
} __attribute__((packed));

struct VbeInfoBlock
{
	char Signature[4];
	unsigned short Version;
	short OEMString[2];
	unsigned char Capabilities[4];
	short VideoModes[2];
	short TotalMemory;
} __attribute__((packed));

struct VbeModeInfo
{
	unsigned short Attributes;
	unsigned char WinA;
	unsigned char WinB;
	unsigned short Granularity;
	unsigned short WinSize;
	unsigned short SegmentA;
	unsigned short SegmentB;
	unsigned short WindowFunctionPointer[2];
	unsigned short Pitch;
	unsigned short XResolution;
	unsigned short YResolution;
	unsigned char CharacterWidth;
	unsigned char CharacterHeight;
	unsigned char Planes;
	unsigned char BitsPerPixel;
	unsigned char Banks;
	unsigned char MemoryModel;
	unsigned char BankSize;
	unsigned char ImagePages;
	unsigned char Reserved;

	unsigned char RedMask;
	unsigned char RedPosition;

	unsigned char GreenMask;
	unsigned char GreenPosition;

	unsigned char BlueMask;
	unsigned char BluePosition;

	unsigned char ReservedMask;
	unsigned char ReservedPosition;

	unsigned char DirectColorAttributes;

	unsigned int FrameBuffer;
} __attribute__((packed));

struct MultibootInfo
{
	unsigned int Flags;
	unsigned int MemoryLow;
	unsigned int MemoryHigh;
	unsigned int BootDevice;
	char *CommandLine;
	unsigned int ModuleCount;
	Module *Modules;
	union
	{
		AOUTSymbolTable AOUTTable;
		ELFHeaderTable ELFTable;
	} SymbolTables;
	unsigned int MemoryMapLength;
	MemoryMapElement *MemoryMap;
	unsigned int DrivesLength;
	DriveStructure *DrivesAddress;
	ROMConfigurationTable *ConfigTable;
	char *BootloaderName;
	APMTable *ApmTable;
	VbeInfoBlock *VBEControlInformation;
	VbeModeInfo *VBEModeInformation;
	unsigned short VBEMode;
	unsigned int VBEInterfaceAddress;
	unsigned short VBEInterfaceLength;
} __attribute__((packed));

#endif
