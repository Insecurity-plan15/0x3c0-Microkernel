#ifndef Ports_H
#define Ports_H

class Ports
{
private:
	Ports();
	~Ports();
public:
	static unsigned int ReadInt(unsigned short port);
	static unsigned short ReadShort(unsigned short  port);
	static unsigned char ReadByte(unsigned short  port);
	
	static void WriteInt(unsigned short  port, unsigned int value);
	static void WriteShort(unsigned short  port, unsigned short value);
	static void WriteByte(unsigned short  port, unsigned char value);
};
#endif
