#ifndef Message_H
#define Message_H

#include <Multitasking/Process.h>

/* This structure is just plain old data. It contains the stuff which needs to be transferred,
* the method which is invoked in the target process as soon as the message is received, and the endpoints
* of the communique */
struct Message
{
private:
	void *data;
	unsigned int length;
	Process *source;
	Process *destination;
	unsigned int code;
public:
	Message(void *d, unsigned int l, Process *src, Process *dest, unsigned int cd);
	~Message();
	Process *GetDestination();
	Process *GetSource();
	void *GetData();
	unsigned int GetLength();
	unsigned int GetCode();
};

#endif
