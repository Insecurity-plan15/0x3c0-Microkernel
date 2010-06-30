#ifndef DriverType_H
#define DriverType_H

namespace Drivers
{
	namespace DriverType
	{
		/* A timer has an automatic link with the scheduler. If it is the only timer device active in the current processor,
		*  it will automatically schedule before any custom interrupt handling code is run */
		const unsigned int Timer =					0x1 << 0;

		const unsigned int Bus = 					0x1 << 1;

		/* A storage device just holds data. It can be read from, and written to. No additional functions are implemented,
		*  but a contract for message passing must be adhered to */
		const unsigned int StorageDevice =	 		0x1 << 2;

		const unsigned int DisplayAdapter = 		0x1 << 3;

		const unsigned int SoundCard = 				0x1 << 4;

		const unsigned int NetworkInterface =		0x1 << 5;

		const unsigned int Keyboard = 				0x1 << 6;

		const unsigned int Mouse = 					0x1 << 7;

		//An imaging input device is something similar to a webcam, digital camera or scanner
		const unsigned int ImagingInput =			0x1 << 8;

		//An imaging output device could be a printer, projector or monitor
		const unsigned int ImagingOutput =			0x1 << 9;

		const unsigned int Battery =				0x1 << 10;

		const unsigned int Port =					0x1 << 11;

		const unsigned int CPU =					0x1 << 12;

		const unsigned int Modem = 					0x1 << 13;

		const unsigned int InterruptManagement = 	0x1 << 14;

		const unsigned int VFS = 					0x1 << 15;
	}
}

#endif // DriverType_H
