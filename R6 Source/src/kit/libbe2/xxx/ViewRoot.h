
#ifndef _INTERFACE2_VIEWROOT_H_
#define _INTERFACE2_VIEWROOT_H_

#include <Handler.h>
#include <ViewGroup.h>
#include <Layout.h>

namespace B {
namespace Interface2 {

class BViewRoot : public BViewGroup, public BHandler
{
	public:
										BViewRoot();
										BViewRoot(const BMessage &attributes);
		virtual							~BViewRoot();

		BPoint							Size();
		virtual	void					SetSize(BPoint size);

		BPoint							Position();
		virtual	void					SetPosition(BPoint location);

		virtual	bool					Position(layoutbuilder_t layout, BRegion &outDirty);
		virtual	void					Draw(layout_t layout, BDrawable &into, const BRegion &dirty);
		virtual	int32					PropagateNeeds(int32 flags, const view_ptr &from);

		virtual	bool					Traverse(BRegion &outDirty);
		virtual	void					MarkDirty(const BRegion &dirty) = 0;
		BLayout	&						GetLayout();

		virtual	status_t				HandleMessage(BMessage *msg);

		virtual void					Acquired();
		virtual	void					Released();

	private:
	
		BLocker							m_lock;
		BLayout							m_layout;
		BPoint							m_size;
		BPoint							m_position;
		BLocker							m_traversalLock;
};

} } // namespace B::Interface2

using namespace B::Interface2;

#endif	/* _INTERFACE2_VIEWROOT_H_ */
