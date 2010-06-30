#ifndef Bitmap_H
#define Bitmap_H

class Bitmap
{
private:
	unsigned int *bitmap;
	unsigned int elements;
public:
	Bitmap(unsigned int count);
	~Bitmap();
	bool TestBit(unsigned int bit);
	void SetBit(unsigned int bit);
	void ClearBit(unsigned int bit);
	unsigned int GetCount();
};

#endif
