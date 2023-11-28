
#ifndef _GEHMLROOT_H_
#define _GEHMLROOT_H_

#include "GehmlGroup.h"
#include "GehmlLayout.h"

class GehmlRoot : public GehmlGroup/*, public GHandler*/
{
	public:
										GehmlRoot();
										GehmlRoot(BStringMap &attributes);
		virtual							~GehmlRoot();

		BPoint							Size();
		virtual	void					SetSize(BPoint size);

		BPoint							Position();
		virtual	void					SetPosition(BPoint location);

		virtual	bool					Position(layoutbuilder_t layout, BRegion &outDirty);
		virtual	void					Draw(layout_t layout, BDrawable &into, const BRegion &dirty);
		virtual	int32					PropagateNeeds(int32 flags, const gehml_obj &from);

		virtual	bool					Traverse(BRegion &outDirty);
		virtual	void					MarkDirty(const BRegion &dirty) = 0;
		GehmlLayout	&					GetLayout();

		virtual	status_t				HandleMessage(BMessage *msg);

		virtual void					Acquired();
		virtual	void					Cleanup();

	private:
	
		Gehnaphore						m_lock;
		GehmlLayout						m_layout;
		BPoint							m_size;
		BPoint							m_position;
		Gehnaphore						m_traversalLock;
};

#endif
