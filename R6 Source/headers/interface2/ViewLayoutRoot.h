
#ifndef _INTERFACE2_VIEWLAYOUTROOT_H_
#define _INTERFACE2_VIEWLAYOUTROOT_H_

#include <support2/Handler.h>
#include <interface2/InterfaceDefs.h>
#include <interface2/ViewParent.h>
#include <interface2/View.h>

namespace B {
namespace Interface2 {

/**************************************************************************************/

class BViewLayoutRoot : public LViewParent, public BView, public BHandler
{
	public:
								BViewLayoutRoot(const IView::ptr &child, const BValue &attr);
								BViewLayoutRoot(const BViewLayoutRoot &copyFrom);
		virtual					~BViewLayoutRoot();

		virtual	status_t		ConstrainChild(const IView::ptr &child, const BLayoutConstraints &constraints);
		virtual	void			MarkTraversalPath(int32 type);

		virtual	status_t		HandleMessage(const BMessage &message);

		virtual	void			DoConstrain() = 0;
		virtual	void			DoLayout() = 0;

		virtual	IView::ptr		PostTraversal(BUpdate &outDirty);
				void			SetShape(const BRegion& shape);
		virtual	status_t		SetParent(const atom_ptr<IViewParent> &parent);

	protected:

		virtual	status_t		Acquired(const void* id);
		virtual	status_t		Released(const void* id);

				void			TriggerTraversal(int32 flags);
				IView::ptr		Child() const;

	private:

				IView::ptr		m_child;
				int32			m_traversalFlags;
		mutable	BLocker			m_lock;
};

/**************************************************************************************/

} } // namespace B::Interface2

#endif	/* _INTERFACE2_VIEWLAYOUTROOT_H_ */
