
#ifndef _GEHMLGROUP_H_
#define _GEHMLGROUP_H_

#include <AssociativeArray.h>
#include <SmartArray.h>
#include "GehmlObject.h"

typedef AssociativeArray<BString,gehml_obj>			ObjectMap;
typedef SmartArray<gehml_obj>						ObjectList;

class GehmlGroup : public GehmlObject
{
	public:
							GehmlGroup();
							GehmlGroup(BStringMap &attributes, int32 extraFlags = 0);
		virtual				~GehmlGroup();

		virtual	void		AddChild(const gehml_obj &child, const BString &name=BString());
		virtual	void		RemoveChild(const gehml_obj &child);

		virtual	bool		Position(layoutbuilder_t layout, BRegion &outDirty);
		virtual	void		Layout(layoutbuilder_t layout);

		virtual	bool		Constrain();
		virtual	void		GetConstraints(int32 axis, GehmlConstraint &constraint) const;

		static	status_t	Constructor(BStringMap &attributes, gehml_obj &child, gehml_group &group);

		ObjectList			VisitChildren();

	protected:

		enum {
			gfSqueezeH = 0x00000001,
			gfSqueezeV = 0x00000002
		};

		float				m_minAbsSize[2],m_maxAbsSize[2],m_prefAbsSize[2];
		int32				m_groupFlags;

	private:

		ObjectList			m_children;
		ObjectMap			m_childMap;
		BString				m_base;
		Gehnaphore			m_lock;

#if DEBUG_LAYOUT
	public:
		virtual void		DumpInfo(int32 indent);
#endif
};

#endif
