
#include <support2_p/BinderKeys.h>

#include <support2/Autolock.h>
#include <support2/String.h>
#include <support2/StdIO.h>
#include <render2/Region.h>
#include <render2/Update.h>
#include <interface2/InterfaceUtils.h>
#include <interface2/View.h>
#include <render2/Render.h>
#include <interface2/ViewParent.h>

using namespace B::Private;

namespace B {
namespace Interface2 {

// Binder properties/commands.
static const BValue g_keyParent			("Parent");
static const BValue g_keyShape			("Shape");
static const BValue g_keyTransform		("Transform");
static const BValue g_keyLayoutBounds	("LayoutBounds");
static const BValue g_keyHidden			("IsHidden");
static const BValue g_keyPreTraversal	("PreTraversal");
static const BValue g_keyPostTraversal	("PostTraversal");
static const BValue g_keyConstraints	("Constraints");
static const BValue g_keyEvent			("Event");

// Data fields.
static const BValue g_keyWhere	("b:where");
static const BValue g_keyUpdate	("b:upd");
static const BValue g_keyView	("b:view");
static const BValue g_keyMsg	("b:msg");

/**************************************************************************************/

BValue 
LView::Inspect(const BValue &which, uint32)
{
	return (which * BValue(IView::descriptor,BValue::Binder(this)))
		+ (which * BValue(IVisual::descriptor,BValue::Binder(this)));
}

/**************************************************************************************/

class RView : public RInterface<IView>
{
	public:

										RView(IBinder::arg remote) : RInterface<IView>(remote) {};

		virtual	status_t				SetParent(const IViewParent::ptr &parent);
		virtual	void					SetShape(const BRegion& shape);
		virtual	void					SetTransform(const B2dTransform& transform);
		virtual void					SetLayoutBounds(const BRect& bounds);
		virtual	void					SetExternalConstraints(const BLayoutConstraints &c);
		virtual	void					Hide();
		virtual	void					Show();

		virtual	void					PreTraversal();
		virtual	IView::ptr				PostTraversal(BUpdate &outDirty);

		virtual	void					Display(IRender::arg into);
		virtual	void					Draw(IRender::arg into);
		virtual void					Invalidate(const BUpdate& update);

		virtual	BLayoutConstraints		Constraints() const;
		virtual	IViewParent::ptr		Parent() const;
		virtual	BRegion					Shape() const;
		virtual BRect					Bounds() const;
		virtual	B2dTransform			Transform() const;
		virtual	bool					IsHidden() const;

		virtual	void					DispatchEvent(	const BMessage &msg,
														const BPoint& where,
														event_dispatch_result *result);
};

/**************************************************************************************/

status_t RView::SetParent(const IViewParent::ptr &parent) {
	return Remote()->Put(BValue(g_keyParent, parent->AsBinder()));
}

void RView::SetShape(const BRegion& shape) {
	Remote()->Put(BValue(g_keyShape, shape.AsValue()));
}

void RView::SetTransform(const B2dTransform& transform) {
	Remote()->Put(BValue(g_keyTransform, transform.AsValue()));
}

void RView::SetLayoutBounds(const BRect& bounds) {
	Remote()->Put(BValue(g_keyLayoutBounds, bounds));
}

void RView::SetExternalConstraints(const BLayoutConstraints &c) {
#warning need implementation of RView::SetExternalConstraints()
}

void RView::Hide() {
	Remote()->Put(BValue(g_keyHidden, BValue::Bool(true)));
}

void  RView::Show() {
	Remote()->Put(BValue(g_keyHidden, BValue::Bool(false)));
}

void RView::PreTraversal() {
	Remote()->Put(BValue(g_keyPreTraversal, BValue::Bool(true)));
}

IView::ptr RView::PostTraversal(BUpdate &outDirty)
{
	const BValue out(Remote()->Invoke(BValue::null, g_keyPostTraversal));
	const IBinder::ptr bnd(out[g_keyView].AsBinder());
	outDirty = BUpdate(out[g_keyUpdate]);
	return bnd != NULL ? (new RView(bnd)) : NULL;
}

void RView::Display(IRender:: arg into) {
	Remote()->Put(BValue(g_keyDisplay, into->AsBinder()));
}

void RView::Draw(IRender:: arg into) {
	Remote()->Put(BValue(g_keyDraw, into->AsBinder()));
}

void RView::Invalidate(const BUpdate& update) {
	Remote()->Put(BValue(g_keyInvalidate, update.AsValue()));
}

BLayoutConstraints RView::Constraints() const
{
	BValue val = Remote()->Get(g_keyConstraints);
	#warning Unflatten constraints here...
}

IViewParent::ptr RView::Parent() const {
	return IViewParent::AsInterface(Remote()->Get(g_keyParent));
}

BRegion RView::Shape() const {
	return BRegion(Remote()->Get(g_keyShape));
}

B2dTransform RView::Transform() const {
	return B2dTransform(Remote()->Get(g_keyTransform));
}

bool RView::IsHidden() const {
	return Remote()->Get(g_keyHidden).AsBool();
}

void RView::DispatchEvent(const BMessage &msg, const BPoint& where, event_dispatch_result *result)
{
	BValue value(g_keyMsg, msg);
	value.Overlay(g_keyWhere, where);
	if (result == NULL) {
		Remote()->Put(BValue(g_keyEvent, value));
	} else {
		*result = (event_dispatch_result)Remote()->Invoke(value, g_keyEvent).AsInteger();
	}
}

BRect RView::Bounds() const {
	return BRect(Remote()->Get(BValue(g_keyBounds)));
}

/**************************************************************************************/

B_IMPLEMENT_META_INTERFACE(View)

; // semicolon to placate Eddie

/**************************************************************************************/

status_t 
LView::Told(BValue &map)
{
	BValue val;
	if (val = map[g_keyDisplay]) {
		IRender::ptr into = IRender::AsInterface(val);
		Display(into);
	}
	if (val = map[g_keyDraw]) {
		IRender::ptr into = IRender::AsInterface(val);
		Draw(into);
	}
	if (val = map[g_keyInvalidate])		Invalidate(BUpdate(val));
	if (val = map[g_keyParent])			SetParent(IViewParent::AsInterface(val));
	if (val = map[g_keyShape])			SetShape(BRegion(val));
	if (val = map[g_keyLayoutBounds])	SetLayoutBounds(BRect(val));
	if (val = map[g_keyTransform])		SetTransform(B2dTransform(val));
	if (val = map[g_keyPreTraversal])	PreTraversal();
	if (val = map[g_keyHidden])			val.AsBool() ? Hide() : Show();
	if (val = map[g_keyEvent]) {
		BMessage msg(val[g_keyMsg]);
		DispatchEvent(msg, BPoint(val[g_keyWhere]));
	}
	return B_OK;
}

status_t
LView::Called(BValue &in, const BValue &outBindings, BValue &out)
{
	BValue val;
	if (in[g_keyPostTraversal].IsDefined()) {
		BUpdate outDirty;
		IView::ptr v = PostTraversal(outDirty);
		BValue value;
		value.Overlay(g_keyView, BValue::Binder(v->AsBinder()));
		value.Overlay(g_keyUpdate, outDirty.AsValue());
		out += outBindings * BValue(g_keyPostTraversal, value);
	}
	
	val = in[g_keyEvent];
	if (val.IsDefined()) {
		event_dispatch_result result;
		BMessage msg(val[g_keyMsg]);
		DispatchEvent(msg, BPoint(val[g_keyWhere]), &result);
		out += outBindings * BValue(g_keyEvent, BValue::Int32(result));
	}

	return B_OK;
}

status_t 
LView::Asked(const BValue &outBindings, BValue &out)
{
	BValue props;
	props.
		Overlay(g_keyBounds,		Bounds().AsValue()).
		Overlay(g_keyShape,			Shape().AsValue()).
		Overlay(g_keyTransform,		Transform().AsValue()).
		Overlay(g_keyHidden,		BValue::Bool(IsHidden()));
		
	IViewParent::ptr parent = Parent();
	if (parent != NULL) props.Overlay(g_keyParent, parent->AsBinder());
	
	out += outBindings * props;

	return B_OK;
}

/**************************************************************************************/
// #pragma mark -

void BView::Init()
{
	m_parent = NULL;
	m_flags = 0;
}

BView::BView(const BView &copyFrom) : LView()
{
	m_parent = copyFrom.m_parent;
	m_externalConstraints = copyFrom.m_externalConstraints;
	m_flags = copyFrom.m_flags;
	m_transform = copyFrom.m_transform;
	m_shape = copyFrom.m_shape;
}

BView::BView(const BValue &attr)
{
	int32 count;
	status_t result;
	BString s;
	
	Init();

	BRect bounds(attr.ValueFor("bounds"), &result);
	if (result == B_OK) {
		m_externalConstraints.SetLeft(bounds.left);
		m_externalConstraints.SetTop(bounds.top);
		m_externalConstraints.SetWidth(bounds.right - bounds.left);
		m_externalConstraints.SetHeight(bounds.bottom - bounds.top);
	}

	if (attr.ValueFor("left").GetString(&s) == B_OK) 		m_externalConstraints.SetLeft(parse_dimth(s));
	if (attr.ValueFor("right").GetString(&s) == B_OK)		m_externalConstraints.SetRight(parse_dimth(s));
	if (attr.ValueFor("top").GetString(&s) == B_OK)			m_externalConstraints.SetTop(parse_dimth(s));
	if (attr.ValueFor("bottom").GetString(&s) == B_OK)		m_externalConstraints.SetBottom(parse_dimth(s));

	if (attr.ValueFor("width").GetString(&s) == B_OK) {
		dimth values[3];
		count = parse_dimths(s,values,3);
		if (count == 1) {
			m_externalConstraints.SetWidth(values[0],true);
		} else if (count == 2) {
			m_externalConstraints.axis[B_HORIZONTAL].min = values[0];
			m_externalConstraints.axis[B_HORIZONTAL].max = values[1];
		} else if (count == 3) {
			m_externalConstraints.axis[B_HORIZONTAL].min = values[0];
			m_externalConstraints.axis[B_HORIZONTAL].pref = values[1];
			m_externalConstraints.axis[B_HORIZONTAL].max = values[2];
		}
	}

	if (attr.ValueFor("height").GetString(&s) == B_OK) {
		dimth values[3];
		count = parse_dimths(s,values,3);
		if (count == 1) {
			m_externalConstraints.SetHeight(values[0],true);
		} else if (count == 2) {
			m_externalConstraints.axis[B_VERTICAL].min = values[0];
			m_externalConstraints.axis[B_VERTICAL].max = values[1];
		} else if (count == 3) {
			m_externalConstraints.axis[B_VERTICAL].min = values[0];
			m_externalConstraints.axis[B_VERTICAL].pref = values[1];
			m_externalConstraints.axis[B_VERTICAL].max = values[2];
		}
	}

	if (attr.ValueFor("prefw").GetString(&s) == B_OK) {
		dimth value;
		parse_dimths(s,&value,1);
		m_externalConstraints.SetWidth(value,false);
	}

	if (attr.ValueFor("prefh").GetString(&s) == B_OK) {
		dimth value;
		parse_dimths(s,&value,1);
		m_externalConstraints.SetHeight(value,false);
	}
}

BView::~BView()
{
}

status_t
BView::Acquired(const void* id)
{
	return LView::Acquired(id);
}

status_t
BView::Released(const void* id)
{
	m_openTransaction = NULL;
	m_parent = NULL;
	return LView::Released(id);
}

BView *
BView::OpenLayoutTransaction()
{
	if (m_openTransaction == NULL) {
		m_openTransaction = Copy();
		IViewParent::ptr parent = Parent();
		if (parent != NULL) {
			parent->MarkTraversalPath(IViewParent::posttraversal);
		}
	}
	return m_openTransaction.ptr();
}

atom_ptr<BView> BView::GetTransactionView()
{
	if (m_openTransaction != NULL) {
		return m_openTransaction;
	}
	else {
		return this;
	}
}

status_t 
BView::SetParent(const IViewParent::ptr &parent)
{
	if (m_parent != parent) m_parent = parent;
	return B_OK;
}

void BView::SetTransform(const B2dTransform& transform)
{
	if (m_transform != transform)
		OpenLayoutTransaction()->m_transform = transform;
}

void BView::SetShape(const BRegion& shape)
{
	if (m_shape != shape)
		OpenLayoutTransaction()->m_shape = shape;
}

void BView::SetLayoutBounds(const BRect& bounds)
{
	if (m_shape.IsRect() || (m_shape.IsEmpty())) {
		// The shape is just a rect.
		SetShape(bounds.OffsetToCopy(B_ORIGIN));
	} else {
		// The shape is a complex region
		if (	(bounds.Width() == 0) ||
				(bounds.Height() == 0))
		{
			SetShape(BRegion::empty);
		} else {
			// We have to compute the scale factor from the old
			// shape to the new shape
			BRegion shape = m_shape;
			shape.Transform(m_transform);
			const coord x_scale = bounds.Width() / shape.Bounds().Width();
			const coord y_scale = bounds.Height() / shape.Bounds().Height();
			const B2dTransform scaller = B2dTransform::MakeScale(x_scale, y_scale);
			shape = m_shape;
			shape.Transform(scaller);
			SetShape(shape);
		}
	}

	if (!(m_transform.Operations() & ~B_TRANSFORM_TRANSLATE)) {
		// The current transformation is just a translation
		SetTransform(B2dTransform::MakeTranslate(bounds.LeftTop()));	
	} else {
		// The current transformation is complex. Just change the Origin
		B2dTransform tr = m_transform;
		tr.Translate(bounds.LeftTop() - m_transform.Origin());
		SetTransform(tr);
	}
}

void 
BView::Hide()
{
	if (m_flags & lfVisible)
		OpenLayoutTransaction()->SetFlags(lfVisible,false);
}

void 
BView::Show()
{
	if (!(m_flags & lfVisible))
		OpenLayoutTransaction()->SetFlags(lfVisible,true);
}

void 
BView::PreTraversal()
{
}

IView::ptr 
BView::PostTraversal(BUpdate &update)
{
	if (m_openTransaction == NULL)
		return this;
	IView::ptr v = m_openTransaction;
	m_openTransaction = NULL;
	update = BUpdate(BUpdate::B_OPAQUE, m_transform, m_shape, v->Transform(), v->Shape());
	return v;
}

void 
BView::Display(IRender:: arg into)
{
	Draw(into);
}

void
BView::Invalidate(const BUpdate& update)
{
	IViewParent::ptr p = Parent();
	if (p != NULL) {
		#warning B_OPAQUE hardcoded
		p->InvalidateChild(this, update);
	}
}

BLayoutConstraints 
BView::ExternalConstraints() const
{
	return m_externalConstraints;
}

void 
BView::SetExternalConstraints(const BLayoutConstraints &c)
{
	if (c != m_externalConstraints) {
		m_externalConstraints = c;
		IViewParent::ptr parent = Parent();
		if (parent != NULL) {
			parent->ConstrainChild(this, c);
		}
	}
}

BLayoutConstraints
BView::Constraints() const
{
	return ExternalConstraints();
}

IViewParent::ptr 
BView::Parent() const
{
	return m_parent.promote();
}

B2dTransform BView::Transform() const
{
	return m_transform;
}

BRegion BView::Shape() const
{
	return m_shape;
}

BRect BView::Bounds() const
{
	return m_shape.Bounds();
}

bool 
BView::IsHidden() const
{
	return !(Flags() & lfVisible);
}

uint32 
BView::Flags() const
{
	return m_flags;
}

uint32 
BView::SetFlags(uint32 flags, bool on)
{
	uint32 oldFlags = m_flags;
	if (on) m_flags |= flags;
	else m_flags &= ~flags;
	return oldFlags;
}

} }	// namespace B::Interface2
