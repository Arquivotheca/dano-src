
#ifndef _GEHMLEMBEDDEDROOT_H_
#define _GEHMLEMBEDDEDROOT_H_

#include <Content.h>
#include "GehmlRoot.h"

class GehmlEmbeddedRoot : public GehmlRoot
{
	public:
										GehmlEmbeddedRoot();
										GehmlEmbeddedRoot(BStringMap &attributes);
		virtual							~GehmlEmbeddedRoot();

		virtual	void					Clean(layout_t layout, BRegion &outWasDirty);
		virtual	void					MarkDirty(const BRegion &dirty);

		static	status_t				Constructor(BStringMap &attributes, gehml_obj &child, gehml_group &group);

	private:
	
		BRegion							m_dirtyRegion;
		Gehnaphore						m_dirtyRegionLock;
};

#endif
