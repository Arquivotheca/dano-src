
#include <support2/StdIO.h>
#include <support2/String.h>
#include <support2/MessageCodes.h>
#include <content2/Content.h>

namespace B {
namespace Content2 {

const BValue IContent::descriptor(BValue::TypeInfo(typeid(IContent)));

static const BValue g_keyParent("b:mom");
static const BValue g_keyEvent("b:evnt");
static const BValue g_keyCreateView("b:crtv");
static const BValue g_keyMessage("b:msg");
static const BValue g_keyView("b:view");

/**************************************************************************************/

class RContent : public RInterface<IContent>
{
	public:
								RContent(const IBinder::ptr remote) : RInterface<IContent>(remote) {};
		virtual	IContent::ptr	Parent() const;
		virtual	status_t		SetParent(const IContent::ptr& parent);
		virtual	void			DispatchEvent(const BMessage &msg, const IView::ptr& view = NULL);
		virtual	IView::ptr		CreateView(const BMessage &attr);
};

/**************************************************************************************/

IContent::ptr RContent::Parent() const
{
	#warning check implementation
	return IContent::AsInterface(Remote()->Get(g_keyParent));
}

status_t RContent::SetParent(const IContent::ptr& parent)
{
	#warning check implementation
	return Remote()->Put(BValue(g_keyParent, parent->AsBinder()));
}

void RContent::DispatchEvent(const BMessage &msg, const IView::ptr& view = NULL)
{
	#warning check implementation
	Remote()->Put(BValue(g_keyEvent,
		BValue()
		.Overlay(g_keyMessage, msg)
		.Overlay(g_keyView, view->AsBinder())
	));
}

IView::ptr RContent::CreateView(const BMessage &attr)
{
	#warning check implementation
	BValue v = Remote()->Invoke(attr, g_keyCreateView);
	return IView::AsInterface(v);
}

/**************************************************************************************/
// #pragma mark -

status_t LContent::Told(value &in)
{
	#warning check implementation
	BValue val;	
		debugger("gfdg");
	if ((val = in[g_keyParent])) {
		SetParent(IContent::AsInterface(val));
	}
	if ((val = in[g_keyEvent]))	{
		const BMessage msg(val[g_keyMessage]);
		const IView::ptr view = IView::AsInterface(val[g_keyView]);
		DispatchEvent(msg, view);
	}	
	if ((val = in[g_keyCreateView])) {
		const BMessage attr(val);		
		IView::ptr view = CreateView(attr);
		Push(BValue(g_keyCreateView, view->AsBinder()));
	}
	return B_OK;
}

status_t LContent::Asked(const value &outBindings, value &out)
{
	berr << "LContent::Asked" << indent << endl << outBindings << dedent <<endl;
	#warning check implementation

	if (outBindings[g_keyCreateView])
		out += outBindings * BValue(g_keyCreateView, CreateView(BMessage())->AsBinder());

	if (outBindings[g_keyParent] && (Parent() != NULL))
		out += outBindings * BValue(g_keyParent, BValue::Binder(Parent()->AsBinder()));

	return B_OK;
}


/**************************************************************************************/
// #pragma mark -

B_IMPLEMENT_META_INTERFACE(Content)


IContent::ptr BContent::Parent() const
{
	return m_parent.promote();
}

status_t BContent::SetParent(const IContent::ptr& parent)
{
	#warning Method needs implementation!
	if (m_parent != parent) {
		m_parent = parent;
	}
	return B_OK;
}

void BContent::DispatchEvent(const BMessage &msg, const IView::ptr& view)
{
	#warning Method needs implementation!
	switch(msg.What()) {
		case B_KEY_DOWN:
			do_key_dn(msg, view);
			break;
		case B_KEY_UP:
			do_key_up(msg, view);
			break;
		case B_MOUSE_WHEEL_CHANGED:
		default:
			break;
	}
}

void BContent::do_key_up(const BMessage &msg, const IView::ptr& /*view*/)
{
	BString s(msg.Data()["bytes"].AsString());
	KeyUp(msg, static_cast<const char *>(s.String()), s.Length()+1);
}

void BContent::do_key_dn(const BMessage &msg, const IView::ptr& /*view*/)
{
	BString s(msg.Data()["bytes"].AsString());
	KeyDown(msg, static_cast<const char *>(s.String()), s.Length()+1);
}


void BContent::WheelMoved(const BMessage&, coord, coord)
{
}

void BContent::KeyDown(const BMessage&, const char *, int32)
{
}

void BContent::KeyUp(const BMessage&, const char *, int32)
{
}

/**************************************************************************************/

} }	// namespace B::Content2
