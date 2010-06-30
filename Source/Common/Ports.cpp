#include <Common/Ports.h>

Ports::Ports()
{
}

Ports::~Ports()
{
}

unsigned int Ports::ReadInt(unsigned short port)
{
	unsigned int value;

	asm volatile ("inl %1, %0" : "=a"(value) : "dN"(port));
	return value;
}

unsigned short Ports::ReadShort(unsigned short port)
{
	unsigned short value;

	asm volatile ("inw %1, %0" : "=a"(value) : "dN"(port));
	return value;
}

unsigned char Ports::ReadByte(unsigned short port)
{
	unsigned char value;

	asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

void Ports::WriteInt(unsigned short port, unsigned int value)
{
	asm volatile ("outl %1, %0" : : "dN"(port), "a"(value));
}

void Ports::WriteShort(unsigned short port, unsigned short value)
{
	asm volatile ("outw %1, %0" : : "dN"(port), "a"(value));
}

void Ports::WriteByte(unsigned short port, unsigned char value)
{
	asm volatile ("outb %1, %0" : : "dN"(port), "a"(value));
}
