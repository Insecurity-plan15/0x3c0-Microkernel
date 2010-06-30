#ifndef Message_H
#define Message_H

struct Message
{
	void *Data;
	unsigned int Length;
	unsigned int Code;
	//When a message is being sent, Source is ignored by the kernel
	unsigned int Source;
} __attribute__((packed));

#endif
