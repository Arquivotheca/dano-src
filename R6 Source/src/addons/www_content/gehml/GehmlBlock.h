
#ifndef _GEHMLBLOCK_H_
#define _GEHMLBLOCK_H_

#include <Content.h>
#include "GehmlObject.h"

class GehmlBlock : public GehmlObject
{
	public:
										GehmlBlock(BStringMap &attributes);
		virtual							~GehmlBlock();

		virtual	bool					Position(layoutbuilder_t layout, BRegion &outDirty);
		virtual	void					Draw(layout_t layout, BDrawable &into, const BRegion &dirty);

		static	status_t				Constructor(BStringMap &attributes, gehml_obj &child, gehml_group &group);

	private:
		
		rgb_color						m_color;
};

#endif
