#ifndef Memory_H
#define Memory_H

#include <Typedefs.h>

class Memory
{
private:
	Memory();
	~Memory();
public:
	static void Clear(void *src, unsigned int size);
	static void Copy(void *destination, const void *src, unsigned int num);
};
#endif
