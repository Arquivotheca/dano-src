
#include <support2/Debug.h>
#include <support2/Autolock.h>
#include <render2/Region.h>
#include <render2/Update.h>
#include <interface2/ViewLayoutRoot.h>
#include <interface2/View.h>

namespace B {
namespace Interface2 {

enum {
	bmsgGo = 'LYGO'
};

BViewLayoutRoot::BViewLayoutRoot(const IView::ptr &child, const BValue &)
 : m_child(child),
   m_traversalFlags(0)
{
}

BViewLayoutRoot::BViewLayoutRoot(const BViewLayoutRoot &copyFrom)
 : LViewParent(),
   BView(copyFrom),
   BHandler(),
   m_child(copyFrom.Child()),
   m_traversalFlags(copyFrom.m_traversalFlags)
{
}

BViewLayoutRoot::~BViewLayoutRoot()
{
}

IView::ptr 
BViewLayoutRoot::Child() const
{
	BAutolock _autolock(m_lock.Lock());
	return m_child;
}

status_t
BViewLayoutRoot::Acquired(const void* id)
{
	LViewParent::Acquired(id);
	BHandler::Acquired(id);
	if (m_traversalFlags) {
		PostMessage(BMessage(bmsgGo));
	}
	
	{
		BAutolock _autolock(m_lock.Lock());
		if (m_child != NULL) m_child->SetParent(this);
	}

	return B_OK;
}

status_t
BViewLayoutRoot::Released(const void* id)
{
	BHandler::Released(id);
	LViewParent::Released(id);
	return B_OK;
}

status_t 
BViewLayoutRoot::HandleMessage(const BMessage &msg)
{
	switch (msg.What()) {
		case bmsgGo: {
//			printf("\n\n%s at %p: GO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n", typeid(*this).name(), this);
			int32 flags = atomic_and(&m_traversalFlags, 0);

			if (flags & pretraversal) m_child->PreTraversal();
			if (flags & constrain) DoConstrain();
			if (flags & layout) DoLayout();
			if (flags & (layout | posttraversal)) {
				TRACE();
				const BRect bounds = Shape().Bounds();
				const BPoint size(bounds.Width(), bounds.Height());
				const BRect child_bounds = m_child->Constraints().Resolve(size);
				m_child->SetLayoutBounds(child_bounds);

				BUpdate dirty;
				IView::ptr new_child = m_child->PostTraversal(dirty);
				{
					BAutolock _autolock(m_lock.Lock());
					m_child = new_child;
				}
				if (!dirty.IsEmpty())
					InvalidateChild(m_child, dirty);
			}
		} break;

		default:
			return BHandler::HandleMessage(msg);
	}
	
	return B_OK;
}

void 
BViewLayoutRoot::TriggerTraversal(int32 flags)
{
	if (flags && (atomic_or(&m_traversalFlags, flags) == 0)) {
		PostMessage(BMessage(bmsgGo));
	}
}

status_t 
BViewLayoutRoot::ConstrainChild(const IView::ptr &, const BLayoutConstraints &)
{
	MarkTraversalPath(constrain | layout);
	return B_OK;
}

void 
BViewLayoutRoot::MarkTraversalPath(int32 type)
{
	TriggerTraversal(type & (pretraversal | posttraversal | constrain | layout));
}

IView::ptr
BViewLayoutRoot::PostTraversal(BUpdate &update)
{
	IView::ptr v = BView::PostTraversal(update);
	if (v != this) {
		int32 flags = posttraversal;
		BViewLayoutRoot *inheritor = static_cast<BViewLayoutRoot *>(v.ptr());
		inheritor->MarkTraversalPath(m_traversalFlags | flags);
		RedirectMessagesTo(inheritor);
	}
	return v;
}

void
BViewLayoutRoot::SetShape(const BRegion& shape)
{
	if (Shape() != shape) {
		m_lock.Lock();
		IView::ptr child = m_child;
		m_lock.Unlock();
		
		BView::SetShape(shape);
		BRect bounds(shape.Bounds());
		BLayoutConstraints child_constraints;
		child_constraints.axis[B_HORIZONTAL].pref = dimth(bounds.Width(), dimth::pixels);
		child_constraints.axis[B_VERTICAL].pref = dimth(bounds.Height(), dimth::pixels);
		child->SetExternalConstraints(child_constraints << m_child->Constraints());
		MarkTraversalPath(layout);
	}
}

status_t 
BViewLayoutRoot::SetParent(const IViewParent::ptr &parent)
{
	if (Parent() != parent) {
		BView::SetParent(parent);
		parent->MarkTraversalPath(m_traversalFlags);
	}
	return B_OK;
}

} }	// namespace B::Interface2
