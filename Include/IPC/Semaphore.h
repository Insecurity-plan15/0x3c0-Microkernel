#ifndef Semaphore_H
#define Semaphore_H
/*
* More information about the GCC builtin functions used for this primitive
* can be found at http://gcc.gnu.org/onlinedocs/gcc/Atomic-Builtins.html
*/

class Semaphore
{
protected:
	volatile unsigned int atomic;
public:
	Semaphore(unsigned int atom = 0);
	~Semaphore();

	Semaphore &operator =(const Semaphore &x);
	unsigned int operator +=(unsigned int x);
	unsigned int operator -=(unsigned int x);

	unsigned int operator |=(unsigned int x);
	unsigned int operator &=(unsigned int x);
	unsigned int operator ^=(unsigned int x);

	bool operator ==(const Semaphore &x);
	bool operator !=(const Semaphore &x);

	operator unsigned int () const;

	bool CompareAndSwap(unsigned int oldValue, unsigned int newValue);
};

#endif
