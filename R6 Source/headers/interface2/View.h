
#ifndef _INTERFACE2_VIEW_H_
#define _INTERFACE2_VIEW_H_

#include <support2/Binder.h>
#include <support2/Message.h>
#include <render2/Rect.h>
#include <render2/Region.h>
#include <render2/Render.h>
#include <render2/2dTransform.h>
#include <interface2/LayoutConstraints.h>

namespace B {
namespace Render2 {
	class BUpdate;
	class B2dTransform;
}}

namespace B {
namespace Interface2 {

class IViewParent;

/**************************************************************************************/

class IView : public IVisual
{
	public:

		B_DECLARE_META_INTERFACE(View)

		virtual	status_t				SetParent(const atom_ptr<IViewParent> &parent) = 0;
		virtual	void					SetShape(const BRegion& shape) = 0;
		virtual	void					SetTransform(const B2dTransform& transform) = 0;
		virtual void					SetLayoutBounds(const BRect& bounds) = 0;
		virtual	void					SetExternalConstraints(const BLayoutConstraints &c) = 0;
		virtual	void					Hide() = 0;
		virtual	void					Show() = 0;

		virtual	void					PreTraversal() = 0;
		virtual	IView::ptr				PostTraversal(BUpdate &outDirty) = 0;

		virtual	BLayoutConstraints		Constraints() const = 0;
		virtual	atom_ptr<IViewParent>	Parent() const = 0;
		virtual	BRegion					Shape() const = 0;
		virtual	B2dTransform			Transform() const = 0;

		virtual	bool					IsHidden() const = 0;

		virtual	void					DispatchEvent(	const BMessage &msg,
														const BPoint& where,
														event_dispatch_result *result = NULL) = 0;
};

/**************************************************************************************/

class LView : public LInterface<IView>
{
	public:
		virtual	BValue					Inspect(const BValue &which, uint32 flags = 0);
		
		virtual	status_t				Told(value &in);
		virtual	status_t				Asked(const BValue &outBindings, BValue &out);
		virtual	status_t				Called(	BValue &in,
												const BValue &outBindings,
												BValue &out);
};

/**************************************************************************************/

class BView : public LView
{
	public:

		/*
			Effective arguments for the BView(BValue) constructor (keys in
			attr whose values we pay attention to):
			
			"bounds"	:	BRect
			"left",		:	string, e.g.: "5px"    --  5 pixels
			"right",		              "7.1cm"  --  7.1 centimeters
			"top",			              "24pt"   --  24 points
			"bottom"		              "25%"    --  25% of parent's size
							              "1/3nm"  --  0.33333333 of parent's size
							Setting "right" places your right edge the given
							distance from the right edge of your parent.  That is,
							right="4px" means you right edge is four pixels in from
							your parent's right edge.  "bottom" works similarly.
							"left" and "top" set your distances from your parent's
							left and top edges, respectively. 
			"width",	:	string: "[all]", "[min] [max]", or "[min] [pref] [max]",
			"height"		 where [all], [min], [max], and [pref] are specified as
							 above.
			"prefw",	:	string, as above.
			"prefh"

			Values from later in this list override values from earlier.  E.g.,
			if you specify both "bounds" and "left", the value from "left" will
			override bounds.left.
		*/
										BView(const BValue &attr = BValue::undefined);
										BView(const BView &copyFrom);
		virtual							~BView();

		virtual	BView *					Copy() = 0;

		virtual	BView *					OpenLayoutTransaction();

		virtual	status_t				SetParent(const atom_ptr<IViewParent> &parent);
		virtual	void					SetShape(const BRegion& shape);
		virtual	void					SetTransform(const B2dTransform& transform);
		virtual void					SetLayoutBounds(const BRect& bounds);
		virtual	void					SetExternalConstraints(const BLayoutConstraints &c);
		virtual	void					Hide();
		virtual	void					Show();

		virtual	void					PreTraversal();
		virtual	IView::ptr				PostTraversal(BUpdate &outDirty);

		virtual	void					Display(IRender::arg into);
		virtual void					Invalidate(const BUpdate& update);

		virtual	BLayoutConstraints		Constraints() const;
		virtual	atom_ptr<IViewParent>	Parent() const;
		virtual	BRegion					Shape() const;
		virtual BRect					Bounds() const;
		virtual	B2dTransform			Transform() const;
		virtual	bool					IsHidden() const;

	protected:

		virtual	status_t				Acquired(const void* id);
		virtual	status_t				Released(const void* id);

				enum {
					lfVisible	= 0x00000001
				};
				
				BLayoutConstraints		ExternalConstraints() const;

				uint32					Flags() const;
				uint32					SetFlags(uint32 flags, bool on);
				atom_ptr<BView>			GetTransactionView();

	private:
	
				void					Init();

				atom_ref<IViewParent>	m_parent;
				BLayoutConstraints		m_externalConstraints;
				atom_ptr<BView>			m_openTransaction;
				uint32					m_flags;				
				B2dTransform			m_transform;
				BRegion					m_shape;
};

/**************************************************************************************/

} } // namespace B::Interface2

#endif	/* _INTERFACE2_VIEW_H_ */
