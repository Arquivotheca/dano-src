
#include <support2/Message.h>

#include <support2/ITextStream.h>

namespace B {
namespace Support2 {

// Standard message fields.
static const BValue g_keyWhat("b:what");
static const BValue g_keyWhen("b:when");
static const BValue g_keySender("b:snder");
static const BValue g_keyReplyTo("b:rpyto");
static const BValue g_keyData("b:data");

BMessage::BMessage()
	:	m_what(0), m_when(0),
		m_next(NULL), m_prev(NULL)
{
}

BMessage::BMessage(uint32 what)
	:	m_what(what), m_when(0),
		m_next(NULL), m_prev(NULL)
{
}

BMessage::BMessage(const BMessage &o)
	:	m_what(o.m_what), m_when(o.m_when), m_data(o.m_data),
		m_next(NULL), m_prev(NULL)
{
}

BMessage::BMessage(const BValue &o)
	:	m_what(o[g_keyWhat].AsInteger()), m_when(o[g_keyWhen].AsTime()),
		m_sender(IHandler::AsInterface(o[g_keySender])),
		m_replyTo(IHandler::AsInterface(o[g_keyReplyTo])),
		m_data(o[g_keyData]),
		m_next(NULL), m_prev(NULL)
{
}

BMessage::~BMessage()
{
}
	
uint32 BMessage::What() const
{
	return m_what;
}

BMessage& BMessage::SetWhat(uint32 val)
{
	m_what = val;
	return *this;
}

bigtime_t BMessage::When() const
{
	return m_when;
}

BMessage& BMessage::SetWhen(bigtime_t val)
{
	m_when = val;
	return *this;
}

IHandler::ptr BMessage::Sender() const
{
	return m_sender.promote();
}

BMessage& BMessage::SetSender(IHandler::ref val)
{
	m_sender = val;
	return *this;
}
			
IHandler::ptr BMessage::ReplyTo() const
{
	return m_replyTo.promote();
}

BMessage& BMessage::SetReplyTo(IHandler::ref val)
{
	m_replyTo = val;
	return *this;
}
			
const BValue& BMessage::Data() const
{
	return m_data;
}

BValue& BMessage::Data()
{
	return m_data;
}

BMessage& BMessage::SetData(const BValue& data)
{
	m_data = data;
	return *this;
}

BValue BMessage::AsValue() const
{
	BValue result;
	if (m_what) result.Overlay(g_keyWhat, BValue::Int32(m_what));
	if (m_when) result.Overlay(g_keyWhen, BValue::Time(m_when));
	if (m_sender != NULL) {
		atom_ptr<IHandler> h(m_sender.promote());
		if (h != NULL) {
			IBinder::ptr b(h->AsBinder());
			if (b != NULL) result.Overlay(g_keySender, b);
		}
	}
	if (m_replyTo != NULL) {
		atom_ptr<IHandler> h(m_replyTo.promote());
		if (h != NULL) {
			IBinder::ptr b(h->AsBinder());
			if (b != NULL) result.Overlay(g_keyReplyTo, b);
		}
	}
	result.Overlay(g_keyData, m_data);
	return result;
}

void BMessage::PrintToStream(ITextOutput::arg io, uint32 flags) const
{
	if (flags&B_PRINT_STREAM_HEADER) io << "BMessage ";
	
	io << "{" << endl << indent
		<< "what = " << BTypeCode(m_what) << " (" << (void*)m_what << ")" << endl
		<< "when = " << m_when << endl
		<< "data = ";
	m_data.PrintToStream(io);
	io << endl << dedent << "}";
}

ITextOutput::arg operator<<(ITextOutput::arg io, const BMessage& message)
{
	message.PrintToStream(io, B_PRINT_STREAM_HEADER);
	return io;
}

} }	// namespace B::Support2
