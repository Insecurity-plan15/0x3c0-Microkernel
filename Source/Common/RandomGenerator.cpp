#include <Common/RandomGenerator.h>

//These are the starting values recommended by George Marsaglia
unsigned int RandomGenerator::w = 521288629;
unsigned int RandomGenerator::z = 362436069;

//This generates a random number in the range of an unsigned int
unsigned int RandomGenerator::mwcRandom()
{
	z = 36969 * (z & 0xFFFF) + (z >> 16);
	w = 18000 * (w & 0xFFFF) + (w >> 16);

	return (z << 16) + (w & 0xFFFF);
}

RandomGenerator::RandomGenerator()
{
}

RandomGenerator::~RandomGenerator()
{
}

//The return value of this function will initially be min. When the FPU is activated by the bootstrap, it'll work
unsigned int RandomGenerator::Next(unsigned int min, unsigned int max)
{
	unsigned int rnd = mwcRandom();
	double scalar = (double)rnd / (double)0xFFFFFFFF;
	double range = max - min;

	return (unsigned int)(range * scalar) + min;
}
