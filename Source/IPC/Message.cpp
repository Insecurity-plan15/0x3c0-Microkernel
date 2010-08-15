#include <IPC/Message.h>

Message::Message(void *d, unsigned int l, Process *src, Process *dest, unsigned int cd, unsigned int msgChain)
{
	data = d;
	length = l;
	source = src;
	destination = dest;
	code = cd;
	messageChain = msgChain;
}

Message::~Message()
{
}

Process *Message::GetDestination()
{
	return destination;
}

Process *Message::GetSource()
{
	return source;
}

void *Message::GetData()
{
	return data;
}

unsigned int Message::GetLength()
{
	return length;
}

unsigned int Message::GetCode()
{
	return code;
}

unsigned int Message::GetMessageChain()
{
    return messageChain;
}
