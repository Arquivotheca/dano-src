#ifndef C_TIMESOURCE_INTERFACE_H

#define C_TIMESOURCE_INTERFACE_H

#include <support2/IInterface.h>

namespace B {
namespace Media2 {

using B::Support2::atom_ptr;
using B::Support2::atom_ref;
using B::Support2::BValue;
using B::Support2::IBinder;

class ITimeSource : public ::B::Support2::IInterface
{
	public:
		B_DECLARE_META_INTERFACE(TimeSource)
		
		virtual bigtime_t Now() = 0;
		virtual status_t SnoozeUntil (bigtime_t when) = 0;		
};

} } // B::Media2

#endif
