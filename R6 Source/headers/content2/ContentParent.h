
#ifndef _CONTENT2_CONTENTPARENT_H_
#define _CONTENT2_CONTENTPARENT_H_

#include <support2/Binder.h>
#include <content2/ContentDefs.h>
#include <interface2/InterfaceDefs.h>

namespace B {
namespace Content2 {

/**************************************************************************************/

class IContentParent : virtual public BAtom
{
	public:
	
		static	property_id					Interface() { static property_id id; if (!id) id = "IContentParent"; return id; }
		static	atom_ptr<IContentParent>	AsInterface(const binder &o);
		virtual	binder						AsBinder() = 0;

		virtual status_t					BubbleEvent(const BMessage &msg, const content &origin) = 0;
};

/**************************************************************************************/

class LContentParent : public IContentParent, virtual public BBinder { public: BINDER_CALLS; binder AsBinder(); };

/**************************************************************************************/

} } // namespace B::Content2

#endif	/* _INTERFACE2_CONTENTPARENT_H_ */
