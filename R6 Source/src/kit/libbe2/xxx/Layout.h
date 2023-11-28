
#ifndef _INTERFACE2_LAYOUT_H_
#define _INTERFACE2_LAYOUT_H_

#include <RenderDefs.h>
#include <InterfaceDefs.h>
#include <AssociativeArray.h>
#include <Rect.h>

namespace B {
namespace Interface2 {

typedef AssociativeArray<int32,layout_ptr>	BLayoutCollection;
typedef const class BLayoutData &			layout_t;
typedef class BLayoutBuilder &				layoutbuilder_t;
typedef class BLayoutTop &					layouttop_t;

class BLayout {
	public:
								BLayout();
								BLayout(const view_ptr &root);

		void					SetRoot(view_ptr root);

		void					SetBounds(const BRect &bounds);
		bool					Layout(BRegion &outDirty);
		void					Draw(BDrawable &into, const BRegion &dirty);

		int32					SetRootNeeds(int32 needs);

	private:
	
		BLocker					m_lock;
		int32					m_rootNeeds;
		layout_ptr				m_layout;
};

class BLayoutData
{
	public:

		const BRect &			Bounds() const;

	protected:

								BLayoutData();

		BView *					Base() const;

		view_ptr				m_base;
		layout_ptr				m_copy;
		BLayoutCollection *		m_children;
		BRect					m_bounds;
		int32					m_flags;
		int32					m_refs;
};

class BLayoutBuilder : public BLayoutData
{
	public:
		const BRect &			OldBounds() const;
		status_t				PlaceChild(view_ptr child, BRect bounds);
		
	protected:
								BLayoutBuilder(const view_ptr &base);
								BLayoutBuilder(BLayoutBuilder &copyFrom);
								~BLayoutBuilder();

		BLayoutTop *			This();
		bool					AssertWritable();
		
		int32					FirstPass(int32 flags);
		layout_ptr				SecondPass(BRegion &dirty);
};

class BLayoutTop : public BLayoutBuilder
{
	public:
								BLayoutTop(const view_ptr &base);

		int32					Constrain(int32 rootNeeds);
		layout_ptr				Layout(int32 rootNeeds, BRegion &outDirty);
		void					SetBounds(const BRect &bounds);
		void					Position(const BRect &bounds);

	private:
	
		friend					BLayout;
		friend 					layout_ptr;
		void					Draw(BDrawable &into, const BRegion &dirty) const;
		void					DrawChildren(BDrawable &into, const BRegion &dirty) const;
		void					Acquire(void *) { m_refs++; };
		void					Release(void *) { if (!--m_refs) delete this; };

};

} } // namespace B::Interface2

using namespace B::Interface2;

#endif
