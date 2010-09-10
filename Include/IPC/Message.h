#ifndef Message_H
#define Message_H

#include <Multitasking/Process.h>

/* This structure is just plain old data. It contains the stuff which needs to be transferred,
* the method which is invoked in the target process as soon as the message is received, and the endpoints
* of the communique */
struct Message
{
private:
	virtAddress data;
	unsigned int length;
	Process *source;
	Process *destination;
	unsigned int code;
	unsigned int messageChain;
public:
	Message(virtAddress d, unsigned int l, Process *src, Process *dest, unsigned int cd, unsigned int msgChain);
	~Message();
	Process *GetDestination();
	Process *GetSource();
	virtAddress GetData();
	unsigned int GetLength();
	unsigned int GetCode();
	unsigned int GetMessageChain();
};

#endif
