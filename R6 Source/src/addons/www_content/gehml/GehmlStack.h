
#ifndef _GEHMLSTACK_H_
#define _GEHMLSTACK_H_

#include "GehmlGroup.h"

class GehmlStack : public GehmlGroup
{
	public:
							GehmlStack(BStringMap &attributes);
		virtual				~GehmlStack();

		virtual	bool		Constrain();
		virtual	void		Layout(layoutbuilder_t layout);

		static	status_t	Constructor(BStringMap &attributes, gehml_obj &child, gehml_group &group);
};

#endif
