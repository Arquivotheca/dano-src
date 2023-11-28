
#ifndef _GEHMLLAYOUT_H_
#define _GEHMLLAYOUT_H_

#include <AssociativeArray.h>
#include "GehmlDefs.h"

typedef AssociativeArray<int32,gehml_layout>	GehmlLayoutCollection;
typedef const class GehmlLayoutData &			layout_t;
typedef class GehmlLayoutBuilder &				layoutbuilder_t;
typedef class GehmlLayoutTop &					layouttop_t;

class GehmlLayout {
	public:
								GehmlLayout();
								GehmlLayout(const gehml_obj &root);

		void					SetRoot(gehml_obj root);

		void					SetBounds(const BRect &bounds);
		bool					Layout(BRegion &outDirty);
		void					Draw(BDrawable &into, const BRegion &dirty);

		int32					SetRootNeeds(int32 needs);

	private:
	
		Gehnaphore				m_lock;
		int32					m_rootNeeds;
		gehml_layout			m_layout;
};

class GehmlLayoutData
{
	public:

		const BRect &			Bounds() const;

	protected:

								GehmlLayoutData();

		GehmlObject *			Base() const;

		gehml_obj				m_base;
		GehmlLayoutCollection *	m_children;
		gehml_layout			m_copy;
		BRect					m_bounds;
		int32					m_flags;
		int32					m_refs;
};

class GehmlLayoutBuilder : public GehmlLayoutData
{
	public:
		const BRect &			OldBounds() const;
		status_t				PlaceChild(gehml_obj child, BRect bounds);
		
	protected:
								GehmlLayoutBuilder(const gehml_obj &base);
								GehmlLayoutBuilder(GehmlLayoutBuilder &copyFrom);
								~GehmlLayoutBuilder();

		GehmlLayoutTop *		This();
		bool					AssertWritable();
		
		int32					FirstPass(int32 flags);
		gehml_layout			SecondPass(BRegion &dirty);
};

class GehmlLayoutTop : public GehmlLayoutBuilder
{
	public:
								GehmlLayoutTop(const gehml_obj &base);

		int32					Constrain(int32 rootNeeds);
		gehml_layout			Layout(int32 rootNeeds, BRegion &outDirty);
		void					SetBounds(const BRect &bounds);
		void					Position(const BRect &bounds);

	private:
	
		friend					GehmlLayout;
		friend 					gehml_layout;
		void					Draw(BDrawable &into, const BRegion &dirty) const;
		void					DrawChildren(BDrawable &into, const BRegion &dirty) const;
		void					Acquire(void *) { m_refs++; };
		void					Release(void *) { if (!--m_refs) delete this; };

};

#endif
