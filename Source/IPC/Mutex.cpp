#include <IPC/Mutex.h>

Mutex::Mutex(bool value)
	: Semaphore(value ? 1 : 0)
{
}

Mutex::~Mutex()
{
}

Mutex &Mutex::operator =(const Mutex &x)
{
	//This isn't a type initialisation. It passes through the base class' assignment operator
	Semaphore::operator =(x);
	return *this;
}

bool Mutex::operator |=(bool x)
{
	return Semaphore::operator |= (x);
}

bool Mutex::operator &=(bool x)
{
	return Semaphore::operator &= (x);
}

bool Mutex::operator ^=(bool x)
{
	return Semaphore::operator ^= (x);
}

bool Mutex::operator ==(const Mutex &x)
{
	return Semaphore::operator == (x);
}

bool Mutex::operator !=(const Mutex &x)
{
	return Semaphore::operator != (x);
}

Mutex::operator bool () const
{
	//Uses the implicit conversion operator declared in the Semaphore class
	unsigned int i = *this;

	return i == 1;
}

bool Mutex::CompareAndSwap(bool oldValue, bool newValue)
{
	return Semaphore::CompareAndSwap(oldValue ? 1 : 0, newValue ? 1 : 0);
}

void Mutex::Lock()
{
	if(this->atomic == 0)
		while(this->atomic == 0)
			asm volatile ("pause");
	else
		this->atomic = 0;
}

void Mutex::Unlock()
{
	this->atomic = 1;
}
