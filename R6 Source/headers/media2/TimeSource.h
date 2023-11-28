#ifndef C_TIMESOURCE_H

#define C_TIMESOURCE_H

#include "ITimeSource.h"

#include <support2/Binder.h>

namespace B {

namespace Media2 {

class LTimeSource : public B::Support2::LInterface<B::Media2::ITimeSource>
{
	protected:
		virtual ~LTimeSource() {};
		
	public:
		virtual	status_t Called (BValue &in,
									const BValue &outBindings,
									BValue &out);
};

} } // B::Media2

#endif
