#include <IPC/Semaphore.h>

Semaphore::Semaphore(unsigned int atom)
{
	atomic = atom;
}

Semaphore::~Semaphore()
{
}

Semaphore &Semaphore::operator =(const Semaphore &x)
{
	atomic = x.atomic;
	return *this;
}

unsigned int Semaphore::operator +=(unsigned int x)
{
	return __sync_add_and_fetch(&atomic, x);
}

unsigned int Semaphore::operator -=(unsigned int x)
{
	return __sync_sub_and_fetch(&atomic, x);
}

unsigned int Semaphore::operator |=(unsigned int x)
{
	return __sync_or_and_fetch(&atomic, x);
}

unsigned int Semaphore::operator &=(unsigned int x)
{
	return __sync_and_and_fetch(&atomic, x);
}

unsigned int Semaphore::operator ^=(unsigned int x)
{
	return __sync_xor_and_fetch(&atomic, x);
}

bool Semaphore::operator ==(const Semaphore &x)
{
	return (x.atomic == this->atomic);
}

bool Semaphore::operator !=(const Semaphore &x)
{
	return !(*this == x);
}

Semaphore::operator unsigned int () const
{
	return atomic;
}

bool Semaphore::CompareAndSwap(unsigned int oldValue, unsigned int newValue)
{
	return __sync_bool_compare_and_swap(&atomic, oldValue, newValue);
}
