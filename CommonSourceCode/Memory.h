#ifndef Memory_H
#define Memory_H

#include <Typedefs.h>

class Memory
{
private:
	Memory();
	~Memory();
public:
	static void Clear(void *src, uint64 size);
	static void Copy(void *destination, const void *src, uint64 num);
};
#endif
