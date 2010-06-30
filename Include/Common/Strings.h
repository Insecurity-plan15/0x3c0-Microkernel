#ifndef Strings_H
#define Strings_H

class Strings
{
private:
	Strings();
	~Strings();
public:
	static int Compare(char *str1, char *str2);
	static int Compare(char *str1, char *str2, unsigned int length);

	static unsigned int Length(char *str);
	static unsigned int Length(const char *str);

	static char *Copy(char *dest, const char *src);
	static char *Copy(char *dest, const char *src, unsigned int length);

	static char *CreateNumeric(int number, unsigned char base);

	static char *Reverse(char *string);
	static const char *Reverse(const char *string);
};

#endif
