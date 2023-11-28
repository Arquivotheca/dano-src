
#include <render2/Region.h>
#include <render2/2dTransform.h>
#include <render2/Update.h>
#include <interface2/ViewLayout.h>

namespace B {
namespace Interface2 {

BViewLayout::BViewLayout(const BValue &attr)
 : LViewParent(),
   BViewList(),
   BView(attr),
   m_traversalFlags(0),
   m_aggregateConstraints(ExternalConstraints())
{
}

BViewLayout::BViewLayout(const BViewLayout &copyFrom)
 : LViewParent(),
   BViewList(copyFrom),
   BView(copyFrom),
   m_traversalFlags(copyFrom.m_traversalFlags),
   m_aggregateConstraints(copyFrom.m_aggregateConstraints)
{
}

BViewLayout::~BViewLayout()
{
}

status_t
BViewLayout::Acquired(const void* id)
{
	LViewParent::Acquired(id);
	BViewList::Acquired(id);
	BView::Acquired(id);
	for (int32 i = _Count() - 1; i >= 0; i--) {
		_ChildAt(i)->SetParent(this);
	}
	return B_OK;
}

status_t
BViewLayout::Released(const void* id)
{
	BView::Released(id);
	BViewList::Released(id);
	LViewParent::Released(id);
	return B_OK;
}

status_t 
BViewLayout::AddView(const IView::ptr &child, const BMessage &attr)
{
	return static_cast<BViewLayout *>(OpenLayoutTransaction())->_AddChild(child, attr);
}

status_t 
BViewLayout::RemoveView(const IView::ptr &child)
{
	return static_cast<BViewLayout *>(OpenLayoutTransaction())->_RemoveChild(child);
}

status_t 
BViewLayout::ConstrainChild(const IView::ptr &, const BLayoutConstraints &)
{
	MarkTraversalPath(constrain | layout);
	return B_OK;
}

status_t 
BViewLayout::InvalidateChild(const IView::ptr &, const BUpdate &update)
{
	IViewParent::ptr p = Parent();
	if (p != NULL) {	
		BUpdate my_update(BUpdate::B_OPAQUE, Transform(), Shape());
		my_update.ComposeWithChildren(update);
		return p->InvalidateChild(this, my_update);
	}
	return B_OK;
}

void 
BViewLayout::Draw(const IRender::ptr &into)
{
	const int32 count = _Count();
	for (int32 i = 0; i < count; i++) {
		IView::ptr child = _ChildAt(i);
		into->PushState();
			into->Transform(child->Transform());
			child->Display(into);
		into->PopState();
//		snooze(500000); // Watch the order of drawing.
	}
}

void 
BViewLayout::DispatchEvent(const BMessage &msg, const BPoint& where, event_dispatch_result *result)
{
	for (int32 i = _Count() - 1; i >= 0; i--) {
		IView::ptr child = _ChildAt(i);
		BPoint child_where = child->Transform().Invert().Transform(where);
		if (child->Shape().Contains(child_where)) {
			event_dispatch_result dispatch;
			child->DispatchEvent(msg, child_where, &dispatch);
			// if one of the children absorbs the event, then we stop here
			if (dispatch == B_EVENT_ABSORBED) {
				if (result) *result = B_EVENT_ABSORBED;
				return;
			}
		}
	}
	if (result) *result = B_EVENT_FALLTHROUGH;
}

void 
BViewLayout::SetExternalConstraints(const BLayoutConstraints &c)
{
	MarkTraversalPath(pretraversal | constrain);
	BView::SetExternalConstraints(c);
}

void 
BViewLayout::PreTraversal()
{
	int32 oldFlags = GetTraversalFlags(pretraversal | constrain);
	bool do_pretraversal = oldFlags & pretraversal;
	bool do_constrain = oldFlags & constrain;

	if (do_pretraversal) {
		for (int32 i = _Count() - 1; i >= 0; i--)
			_ChildAt(i)->PreTraversal();
	}

	if (do_constrain) {
		BLayoutConstraints newConstraints = InternalConstraints() << ExternalConstraints();
//		BLayoutConstraints newConstraints = ExternalConstraints() << InternalConstraints();
		if (newConstraints != m_aggregateConstraints) {
			m_aggregateConstraints = newConstraints;
			Parent()->ConstrainChild(this, m_aggregateConstraints);
		}
	}
}

BLayoutConstraints 
BViewLayout::Constraints() const
{
	return m_aggregateConstraints;
}

void
BViewLayout::SetShape(const BRegion& shape)
{
	if (Shape() != shape) {
		BView::SetShape(shape);
		MarkTraversalPath(layout);
	}
}

status_t 
BViewLayout::SetParent(const IViewParent::ptr &parent)
{
	if (Parent() != parent) {
		BView::SetParent(parent);
		parent->MarkTraversalPath(m_traversalFlags);
	}
	return B_OK;
}

IView::ptr 
BViewLayout::PostTraversal(BUpdate &outDirty)
{
	int32 oldFlags = GetTraversalFlags(posttraversal | layout);
	const bool do_posttraversal = (bool)oldFlags;
	const bool do_layout = oldFlags & layout;

	BUpdate childrenDirty;	// update for all our children
	outDirty.MakeEmpty();	// our final update
	
	// Get our own update
	IView::ptr v = BView::PostTraversal(outDirty);
	BViewLayout *new_this = static_cast<BViewLayout *>(v.ptr());

	if (do_layout) {
		new_this->Layout();
	}
	
	if (do_posttraversal || new_this != this) {
		const int32 count = new_this->_Count();
		for (int32 i = 0; i < count; i++) {
			IView::ptr child = new_this->_ChildAt(i);
			// Compose all BUpdates from our children
			BUpdate childUpdate;
			IView::ptr newChild = child->PostTraversal(childUpdate);
			childrenDirty.ComposeBehind(childUpdate);
			if (child != newChild) {
				if (new_this == this) {
					/*
					OpenLayoutTransaction();
					v = BView::PostTraversal(outDirty);
					*/
					v = Copy();
					outDirty = BUpdate(BUpdate::B_OPAQUE, Transform(), Shape(), v->Transform(), v->Shape());
					new_this = static_cast<BViewLayout *>(v.ptr());
				}
				new_this->_ReplaceChild(i, newChild);
				newChild->SetParent(new_this);
			}
		}
	}

	// Compose our own update with our children's
	outDirty.ComposeWithChildren(childrenDirty);
	return new_this;
}

int32
BViewLayout::GetTraversalFlags(int32 which_flags)
{
	return atomic_and(&m_traversalFlags, ~which_flags) & which_flags;
}

void 
BViewLayout::MarkTraversalPath(int32 type)
{
	type &= ~(atomic_or(&(m_traversalFlags), type) & type);
	// type now contains only those bits that were set in type originally
	// and weren't set in m_traversalFlags originally.  So we mark
	// our Parent's traversal path with only those bits in type that we
	// just set for the first time.
	if (type) {
		IViewParent::ptr parent = Parent();
		if (parent != NULL){
			parent->MarkTraversalPath(type);
		}
	}
}

} }	// namespace B::Interface2
