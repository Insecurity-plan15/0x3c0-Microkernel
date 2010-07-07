#include <ELF/ELFHeaders.h>
#include <Common/Strings.h>

using namespace ELF;
using namespace ELF::FileStructures;

ELFObject::ELFObject()
{
	valid = false;
}

ELFObject::~ELFObject()
{
}

bool ELFObject::IsValid()
{
	return valid;
}

unsigned int ELFObject::GetBase()
{
	return base;
}

unsigned int ELFObject::GetEnd()
{
	return elfEnd;
}
