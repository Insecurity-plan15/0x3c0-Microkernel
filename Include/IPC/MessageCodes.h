#ifndef MessageCodes_H
#define MessageCodes_H

namespace MessageCodes
{
	//These messages are sent only to drivers
	namespace Drivers
	{
		//Sent when an interrupt is received
		const unsigned int InterruptReceived = 0;
	}

	//Sent by the kernel when a shared page has been made available
	const unsigned int SharedPageAvailable = 1;
}

#endif // MessageCodes_H
