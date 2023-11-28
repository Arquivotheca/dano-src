
#ifndef _INTERFACE2_VIEWGROUP_H_
#define _INTERFACE2_VIEWGROUP_H_

#include <AssociativeArray.h>
#include <SmartArray.h>
#include <View.h>

namespace B {
namespace Interface2 {

class BViewGroup : public BView
{
	public:
							BViewGroup();
							BViewGroup(const BMessage &attributes, int32 extraFlags = 0);
		virtual				~BViewGroup();

		virtual	void		Released();

		virtual	void		AddChild(const view_ptr &child, const BString &name=BString());
		virtual	void		RemoveChild(const view_ptr &child);

		virtual	bool		Position(layoutbuilder_t layout, BRegion &outDirty);
		virtual	void		Layout(layoutbuilder_t layout);

		virtual	bool		Constrain();
		virtual	void		GetConstraints(int32 axis, BLayoutConstraint &constraint) const;

		BViewList			VisitChildren();

	protected:

		enum {
			gfSqueezeH = 0x00000001,
			gfSqueezeV = 0x00000002
		};

		float				m_minAbsSize[2],m_maxAbsSize[2],m_prefAbsSize[2];
		int32				m_groupFlags;

	private:

		BViewList			m_children;
		BViewMap			m_childMap;
		BLocker				m_lock;

#if DEBUG_LAYOUT
	public:
		virtual void		DumpInfo(int32 indent);
#endif
};

} } // namespace B::Interface2

using namespace B::Interface2;

#endif	/* _INTERFACE2_VIEWGROUP_H_ */
