
#ifndef _GEHMLHVGROUP_H_
#define _GEHMLHVGROUP_H_

#include "GehmlGroup.h"

class GehmlHVGroup : public GehmlGroup
{
	public:
							GehmlHVGroup(BStringMap &attributes, int8 hOrV);
		virtual				~GehmlHVGroup();

		virtual	bool		Constrain();
		virtual	void		Layout(layoutbuilder_t layout);

		static	status_t	HConstructor(BStringMap &attributes, gehml_obj &child, gehml_group &group);
		static	status_t	VConstructor(BStringMap &attributes, gehml_obj &child, gehml_group &group);

	private:
				
		int8				m_orientation;
};

#endif
