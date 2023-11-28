
#ifndef _SUPPORT2_IVALUESTREAM_H
#define _SUPPORT2_IVALUESTREAM_H

#include <support2/IInterface.h>
#include <support2/Value.h>

namespace B {
namespace Support2 {

/**************************************************************************************/

class IValueInput : public IInterface
{
	public:

		B_DECLARE_META_INTERFACE(ValueInput)

		virtual	status_t				Read(BValue *in) = 0;
};

/**************************************************************************************/

class IValueOutput : public IInterface
{
	public:

		B_DECLARE_META_INTERFACE(ValueOutput)

		virtual	status_t				Write(const BValue &out) = 0;
		virtual	status_t				End() = 0;
};

/**************************************************************************************/

} } // namespace B::Support2

#endif /* _SUPPORT2_IVALUESTREAM_H */
