#ifndef Typedefs_H
#define Typedefs_H

typedef char *				pointer;
typedef unsigned int		uint;
typedef unsigned long long	ulong;


#ifdef x86_64
	typedef ptr ulong;
#else
	typedef ptr uint;
#endif

#endif // Typedefs_H
