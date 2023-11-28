
#ifndef _GEHMLCONTENT_H_
#define _GEHMLCONTENT_H_

#include <Content.h>
#include "GehmlObject.h"

class GehmlContent : public GehmlObject, public BinderListener
{
	public:
										GehmlContent(BStringMap &attributes);
		virtual							~GehmlContent();

		virtual	void					Acquired();

		virtual	status_t				Overheard(binder_node node, uint32 observed, BString propertyName);

		virtual	status_t				HandleMessage(BMessage *msg);
//		virtual	status_t				SetParent(const binder_node &parent);

		virtual	bool					Position(layoutbuilder_t layout, BRegion &outDirty);
		virtual	void					Clean(layout_t layout, BRegion &outDirty);
		virtual	void					Draw(layout_t layout, BDrawable &into, const BRegion &dirty);

		virtual	void					GetConstraints(int32 axis, GehmlConstraint &constraint) const;

		static	status_t				Constructor(BStringMap &attributes, gehml_obj &child, gehml_group &group);

	private:
		
		int32							m_size[2];
		atom<Wagner::ContentInstance>	m_content;
		BString							m_src;
		int32							m_id;
};

#endif
