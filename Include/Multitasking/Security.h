#ifndef Security_H
#define Security_H

#define SecurityPrivilegeCount 21

//These security privileges correspond to bits within a bitmap.
//When one is added, SecurityPrivilegeCount must be updated to prevent overflows
namespace SecurityPrivilege
{
	//Does the process have the capability to perform message passing?
	const unsigned int MessagePassing = 0;
	//Can the process request and free a shared page?
	const unsigned int SharedPage = 1;
	//Controls the ability of the process to yield its timeslice
	const unsigned int Yield = 2;
	//Allows the process to move its priority from below-normal to normal
	const unsigned int ChangePriority = 3;
	//Can the process create a command line?
	const unsigned int CommandLine = 4;
	//If set, allows the process to create a graphical user interface (without full screen)
	const unsigned int GUI = 5;
	//By default, lets the process accept input from the keyboard
	const unsigned int AcceptKeyboardInput = 6;
	//When set, allows the process to receive mouse movement/click/scroll messages
	const unsigned int AcceptMouseInput = 7;
	//Controls the ability of the process to create a new thread
	const unsigned int CreateThread = 8;
	//Enables the process to create another process
	const unsigned int CreateProcess = 9;
	//A restricted function which allows a process to create a thread operating in Virtual 8086 mode
	const unsigned int CreateV86Thread = 10;
	//Can the process read from its protected storage directory?
	const unsigned int ReadFromStorage = 11;
	//Same as the above, but controls the write ability
	const unsigned int WriteToStorage = 12;
	//Lets the process access a network using TCP/IP
	const unsigned int TCPIP = 13;
	//Provides the process with the ability to send and recieve other protocols in the network stack
	const unsigned int AccessNetworkStack = 14;
	//If set, the process can read a single page from a given file
	const unsigned int ReadPageFromFile = 15;
	//Should be used sparingly. Allows the process to read the entire file into memory
	const unsigned int ReadCompleteFile = 16;
	//If set, the process can enumerate any devices in or attached to the computer
	const unsigned int EnumerateDevices = 17;
	//Must be set for most PCI drivers. Allows the process to access the memory space of a given device
	const unsigned int AccessMemoryMappedDevices = 18;
	//Should not be used for anything except a file browser. Will allow the process to view the entire file system
	const unsigned int ReadFileSystem = 19;
	//Same as above, but with write-access
	const unsigned int WriteFileSystem = 20;

	const unsigned int RestrictedPrivileges[7] =
		{
			CreateV86Thread, AccessNetworkStack, ReadCompleteFile, EnumerateDevices,
			AccessMemoryMappedDevices, ReadFileSystem, WriteFileSystem
		};
	const unsigned int DefaultPrivileges[10] =
		{
			MessagePassing, Yield, ChangePriority, CommandLine, AcceptKeyboardInput, CreateThread, CreateProcess, ReadFromStorage,
			WriteToStorage, ReadPageFromFile
		};
	//When this is taken into account, a process may request the following privileges without incurring the wrath of the kernel:
	//SharedPage, GUI, AcceptMouseInput, TCPIP
}

#endif
