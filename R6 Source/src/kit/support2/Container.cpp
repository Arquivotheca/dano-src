
#include <support2/Container.h>

#include <support2/StdIO.h>

namespace B {
namespace Support2 {

BContainer::BContainer()
{
}

BContainer::~BContainer()
{
}

status_t 
BContainer::Read(BValue *in)
{
	*in = m_value;
	return B_OK;
}

status_t 
BContainer::Write(const BValue &inValue)
{
	berr << "BContainer::Write " << inValue << endl;
	m_value.Overlay(inValue);
	return B_OK;
}

status_t 
BContainer::End()
{
	return B_OK;
}

} }	// namespace B::Support2
