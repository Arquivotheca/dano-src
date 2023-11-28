
#ifndef _INTERFACE2_VIEWLAYOUT_H_
#define _INTERFACE2_VIEWLAYOUT_H_

#include <interface2/InterfaceDefs.h>
#include <interface2/ViewList.h>
#include <interface2/ViewParent.h>
#include <interface2/View.h>
#include <render2/Render.h>

namespace B {
namespace Render2 {
	class BUpdate;
}}

namespace B {
namespace Interface2 {

/**************************************************************************************/

class BViewLayout : public LViewParent, public BViewList, public BView
{
	public:
										BViewLayout(const BValue &attr = BValue::undefined);
										BViewLayout(const BViewLayout &copyFrom);
		virtual							~BViewLayout();

		virtual	status_t				AddView(const IView::ptr &child, const BMessage &attr);
		virtual	status_t				RemoveView(const IView::ptr &child);

		virtual	status_t				ConstrainChild(	const IView::ptr &child,
														const BLayoutConstraints &constraints);
		virtual	status_t				InvalidateChild(const IView::ptr &child,
														const BUpdate &update);
		virtual	void					MarkTraversalPath(int32 type);

		virtual	void					Draw(const IRender::ptr &into);
				void					SetShape(const BRegion& shape);
		virtual	status_t				SetParent(const atom_ptr<IViewParent> &parent);

		virtual	void					PreTraversal();
		virtual	IView::ptr				PostTraversal(BUpdate &outDirty);
		virtual void					Layout() = 0;
		
		virtual	BLayoutConstraints		Constraints() const;
		virtual	BLayoutConstraints		InternalConstraints() const = 0;

		virtual	void					DispatchEvent(	const BMessage &msg,
														const BPoint& where,
														event_dispatch_result *result);
				
	protected:
	
		virtual	status_t				Acquired(const void* id);
		virtual	status_t				Released(const void* id);
		virtual	void					SetExternalConstraints(const BLayoutConstraints &c);
				int32					GetTraversalFlags(int32 which_bits);

	private:
	
				int32					m_traversalFlags;
				BLayoutConstraints		m_aggregateConstraints;
};

/**************************************************************************************/

} } // namespace B::Interface2

#endif	/* _INTERFACE2_VIEWLAYOUT_H_ */
