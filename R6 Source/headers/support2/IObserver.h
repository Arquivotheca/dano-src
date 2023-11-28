
#ifndef _SUPPORT2_IOBSERVER_H_
#define _SUPPORT2_IOBSERVER_H_

#include <support2/SupportDefs.h>
#include <support2/IBinder.h>

namespace B {
namespace Support2 {

class IObserver : virtual public BAtom
{
		virtual	status_t Observed(
							binder object,
							const property_list &changed,
							const property_list &added,
							const property_list &removed);
};

} } // namespace B::Support2

#endif	/* _SUPPORT2_IOBSERVER_H_ */
