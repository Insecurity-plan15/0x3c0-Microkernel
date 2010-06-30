#include <Bitmap.h>

//count is the number of bits designed to be held
Bitmap::Bitmap(unsigned int count)
{
	elements = count;
	//There are 32 bits in a byte, and integer division rounds down
	if((count % 32) == 0)
		bitmap = new unsigned int[count / 32];
	else
		bitmap = new unsigned int[count / 32 + 1];
}

Bitmap::~Bitmap()
{
	delete[] bitmap;
}

bool Bitmap::TestBit(unsigned int bit)
{
	return (bitmap[bit / 32] & (1 << (bit % 32))) != 0;
}

void Bitmap::SetBit(unsigned int bit)
{
	bitmap[bit / 32] |= (1 << (bit % 32));
}

void Bitmap::ClearBit(unsigned int bit)
{
	bitmap[bit / 32] &= ~(1 << (bit % 32));
}

unsigned int Bitmap::GetCount()
{
	return elements;
}
