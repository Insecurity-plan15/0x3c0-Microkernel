#include <Common/Strings.h>

Strings::Strings()
{
}

Strings::~Strings()
{
}

int Strings::Compare(char *str1, char *str2)
{
	for(; *str1 == *str2; ++str1, ++str2)
		if(*str1 == 0)
			return 0;
	return (*(unsigned char *)str1 < *(unsigned char *)str2) ? -1 : 1;
}

int Strings::Compare(char *str1, char *str2, unsigned int length)
{
	for(unsigned int i = 0; i < length && *str1 == *str2; ++str1, ++str2, i++)
		if(*str1 == 0)
			return 0;
	return (*(unsigned char *)str1 < *(unsigned char *)str2) ? -1 : 1;
}

unsigned int Strings::Length(char *str)
{
	unsigned int length = 0;

	for(; str[length] != 0; length++)
		;
	return length;
}

unsigned int Strings::Length(const char *str)
{
	return Strings::Length((char *)str);
}

char *Strings::Copy(char *dest, const char *src)
{
	return Strings::Copy(dest, src, Strings::Length(src));
}

char *Strings::Copy(char *dest, const char *src, unsigned int length)
{
	unsigned int i = 0;

	for(; i < length; i++)
		dest[i] = src[i];
	dest[i] = 0;
	return dest;
}

//Note: when this number is finished with, it needs deleting
char *Strings::CreateNumeric(int number, unsigned char base)
{
	//The highest number possible is 10 characters long. Add one for the NULL byte, and another for the minus
	char *c = new char[12];
	long tmpValue;
	//A big list of possible characters, backwards to allow for negative numbers
	char values[72];

	Strings::Copy(values, "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz");
	if(base < 2 || base > 36)
		return c;
	do
	{
		tmpValue = number;
		number /= base;
		*c++ = values[35 + (tmpValue - number * base)];
	} while (number != 0);
	if(tmpValue < 0)
		*c++ = '-';
	*c-- = 0;
	return Strings::Reverse(c);
}

char *Strings::Reverse(char *string)
{
	char *reversed = string;
	unsigned int length = Strings::Length(string);

	for(unsigned int i = length; i >= (length / 2) + 1; i--)
	{
		char c = reversed[length - i];

		reversed[length - i] = reversed[i - 1];
		reversed[i - 1] = c;
	}
	return reversed;
}

const char *Strings::Reverse(const char *string)
{
	return Strings::Reverse((char *)string);
}
