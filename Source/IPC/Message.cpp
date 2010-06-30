#include <IPC/Message.h>

Message::Message(void *d, unsigned int l, Process *src, Process *dest, unsigned int cd)
{
	data = d;
	length = l;
	source = src;
	destination = dest;
	code = cd;
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
