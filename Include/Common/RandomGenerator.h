#ifndef RandomGenerator_H
#define RandomGenerator_H

class RandomGenerator
{
private:
	static unsigned int w;
	static unsigned int z;

	//This method uses George Marsaglia's MWC algorithm
	unsigned int mwcRandom();
public:
	RandomGenerator();
	~RandomGenerator();

	unsigned int Next(unsigned int min, unsigned int max);
};

#endif // RandomGenerator_H
