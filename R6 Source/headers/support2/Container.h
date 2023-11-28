
#ifndef _SUPPORT2_CONTAINER_H_
#define _SUPPORT2_CONTAINER_H_

#include <support2/Binder.h>
#include <support2/ValueStream.h>

namespace B {
namespace Support2 {

/**************************************************************************************/

class BContainer : public LValueInput, public LValueOutput
{
	public:
		B_STANDARD_ATOM_TYPEDEFS(BContainer);

								BContainer();
		virtual					~BContainer();

		virtual	status_t		Read(BValue *in);
		virtual	status_t		Write(const BValue &out);
		virtual	status_t		End();

	private:

				BValue			m_value;
};

/**************************************************************************************/

} } // namespace B::Support2

#endif	/* _SUPPORT2_BINDERCONTAINER_H_ */
