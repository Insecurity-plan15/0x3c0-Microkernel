#ifndef Mutex_H
#define Mutex_H
#include <IPC/Semaphore.h>

class Mutex : public Semaphore
{
public:
	Mutex(bool value);
	~Mutex();

	Mutex &operator =(const Mutex &x);
	bool operator |=(bool x);
	bool operator &=(bool x);
	bool operator ^=(bool x);

	bool operator ==(const Mutex &x);
	bool operator !=(const Mutex &x);

	operator bool () const;

	bool CompareAndSwap(bool oldValue, bool newValue);
	void Lock();
	void Unlock();
};

#endif
