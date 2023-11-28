
#include <render2/Region.h>
#include <render2/Update.h>
#include <interface2/ViewParent.h>
#include <content2/Content.h>
#include <content2/ContentView.h>
#include <support2/MessageCodes.h>

namespace B {
namespace Content2 {

BContentView::BContentView(const content &theContent, const BMessage &attr) : BView(attr)
{
	m_content = theContent;
	m_aggregateConstraints = BView::Constraints();
}

BContentView::BContentView(const BContentView &copyFrom) : BView(copyFrom)
{
	m_content = copyFrom.m_content;
	m_aggregateConstraints = copyFrom.m_aggregateConstraints;
}

BContentView::~BContentView()
{
}

status_t
BContentView::Acquired(const void* id)
{
	return BView::Acquired(id);
}

status_t
BContentView::Released(const void* id)
{
	m_content = NULL;
	return BView::Released(id);
}

BView * 
BContentView::Copy()
{
	return new BContentView(*this);
}

view 
BContentView::PostTraversal(BRegion &outDirty)
{
	view v = BView::PostTraversal(outDirty);
	if (v.ptr() != this) {
		// mathias: REVISIT that.
		// So we create an update description for us..
		// ... what we looked like, where we were
		const BRegion old_shape = Bounds().OffsetTo(B_ORIGIN);
		// ... what we look like, where we are
		const BRegion new_shape = v->Bounds().OffsetTo(B_ORIGIN);
		// For now, we are opaque, and our 2d transformation is just an offset
		BUpdate update(	BUpdate::B_OPAQUE,
						BTranslation2d::MakeTranslate(Bounds().x, Bounds().y),			old_shape,
						BTranslation2d::MakeTranslate(v->Bounds().x, v->Bounds().y),	new_shape);
		outDirty = update;
	}	
	return v;
}
void 
BContentView::Constrain(const BLayoutConstraints &internalConstraints)
{
	m_aggregateConstraints = Constraints() << internalConstraints;
	Parent()->ConstrainChild(this,m_aggregateConstraints);
}

void 
BContentView::Invalidate(const BRegion &dirty)
{
	viewparent p = Parent();
	if (p != NULL) {
		BRegion copy;
		if (dirty.IsFull()) copy.Include(Bounds());
		else {
			copy = dirty;
			copy.OffsetBy(Bounds().LeftTop());
		}
		p->InvalidateChild(this,copy);
	}
}

void 
BContentView::Draw(const render &)
{
	// Default implementation does no drawing
}

BLayoutConstraints 
BContentView::Constraints() const
{
	return m_aggregateConstraints;
}

content 
BContentView::Content() const
{
	return m_content;
}

void BContentView::DispatchEvent(const BMessage &msg, const BPoint& where, event_dispatch_result *result)
{
	event_dispatch_result dispatch;
	switch (msg.What()) {
		case B_MOUSE_MOVED:
			dispatch = MouseMoved(msg, where, msg["be:transit"].AsInteger(), BMessage());
			break;
		case B_MOUSE_DOWN:
			dispatch = MouseDown(msg, where, msg["buttons"].AsInteger());
			break;
		case B_MOUSE_UP:
			dispatch = MouseUp(msg, where, msg["buttons"].AsInteger());
			break;
		default:
			dispatch = B_EVENT_ABSORBED;
			break;
	}
	if (result)
		*result = dispatch;
}

event_dispatch_result 
BContentView::MouseMoved(const BMessage &, const BPoint& , uint32 , const BMessage &)
{
	return B_EVENT_ABSORBED;
}

event_dispatch_result 
BContentView::MouseDown(const BMessage &, const BPoint& , int32)
{
	return B_EVENT_ABSORBED;
}

event_dispatch_result 
BContentView::MouseUp(const BMessage &, const BPoint&)
{
	return B_EVENT_ABSORBED;
}

} }	// namespace B::Content2
