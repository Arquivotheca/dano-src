#include <support2/Parcel.h>

#include <support2/ByteOrder.h>
#include <support2/IBinder.h>
#include <support2/ITextStream.h>
#include <support2/StringIO.h>
#include <kernel/OS.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

namespace B {
namespace Support2 {

inline void BParcel::do_free()
{
	if (m_free) m_free(m_data, m_origLength, m_freeContext);
	m_data = NULL;
	m_length = m_origLength = 0;
	m_free = NULL;
	m_base = 0;
}

static void standard_free(const void* data, ssize_t, void*)
{
	free(const_cast<void*>(data));
}

BParcel::BParcel(reply_func replyFunc, void* replyContext)
	:	m_data(NULL), m_length(0), m_origLength(0),
		m_free(NULL), m_freeContext(NULL),
		m_reply(replyFunc), m_replyContext(replyContext),
		m_base(0)
{
}

BParcel::BParcel(const void* data, ssize_t len,
	free_func freeFunc, void* freeContext, reply_func replyFunc, void* replyContext)
	:	m_data(data), m_length(len), m_origLength(0),
		m_free(freeFunc), m_freeContext(freeContext),
		m_reply(replyFunc), m_replyContext(replyContext),
		m_base(0)
{
}

BParcel::~BParcel()
{
	Reply();
	do_free();
}

const void* BParcel::Data() const
{
	return m_data;
}

ssize_t BParcel::Length() const
{
	return m_length;
}

void BParcel::Reference(const void* data, ssize_t len, free_func freeFunc, void* context)
{
	do_free();
	m_data = data;
	m_length = m_origLength = len;
	m_free = freeFunc;
	m_freeContext = context;
}

status_t BParcel::Copy(const void* data, ssize_t len)
{
	void* d = Alloc(len);
	if (d) memcpy(d, data, len);
	return m_length >= 0 ? B_OK : m_length;
}

void* BParcel::Alloc(ssize_t len)
{
	do_free();
	if (len <= (ssize_t)sizeof(m_inline)) {
		m_data = m_inline;
		m_length = m_origLength = len;
		m_free = NULL;
	} else if ((m_data=malloc(len)) != NULL) {
		m_length = m_origLength = len;
		m_free = standard_free;
	} else {
		m_length = m_origLength = B_NO_MEMORY;
		m_free = NULL;
	}
	return const_cast<void*>(m_data);
}

void* BParcel::ReAlloc(ssize_t len)
{
	if (len > m_origLength) {
		debugger("Growing BParcel not yet implemented.  Sorry.");
		return NULL;
	}
	
	m_length = len;
	return const_cast<void*>(m_data);
}

void BParcel::Transfer(BParcel* src)
{
	if (src->m_data != src->m_inline) {
		// The data is stored outside of the buffer object.
		if (src->m_free || !src->m_data) {
			// The source buffer "owns" its data, so we can just
			// transfer ownership.
			m_data = src->m_data;
			m_length = src->m_length;
			m_origLength = src->m_origLength;
			m_free = src->m_free;
			m_freeContext = src->m_freeContext;
		} else {
			// The source buffer is only referencing its data, so someone
			// else will be deleting it.  In this case we must make a copy.
			Copy(src->m_data, src->m_length);
		}
	} else {
		// The data is stored inside of the buffer object -- just
		// copy it in to the new buffer.
		m_data = m_inline;
		m_length = src->m_length;
		m_origLength = src->m_origLength;
		m_free = NULL;
		memcpy(m_inline, src->m_inline, src->m_length);
	}
	m_reply = src->m_reply;
	m_offsets = src->m_offsets;
	m_binders = src->m_binders;
	m_base = src->m_base;
	
	src->m_data = NULL;
	src->m_length = src->m_origLength = 0;
	src->m_free = NULL;
	src->m_reply = NULL;
	src->m_offsets.MakeEmpty();
	src->m_binders.MakeEmpty();
	src->m_base = 0;
}

status_t BParcel::SetValues(const BValue* value1, ...)
{
	SetBinderOffsets(NULL, 0);
	do_free();
	
	static const int32 MAX_COUNT = 10;
	const BValue* values[MAX_COUNT];
	int32 count = 0, i;
	
	va_list vl;
	va_start(vl,value1);
	while (value1 && count < MAX_COUNT) {
		values[count++] = value1;
		value1 = va_arg(vl,BValue*);
	}
	va_end(vl);

	size_t size = 0;
	for (i=0;i<count;i++) size += values[i]->FlattenedSize();
	uint8* ptr = static_cast<uint8*>(Alloc(size+sizeof(int32)));
	if (!ptr) return B_NO_MEMORY;
	
	*(int32*)ptr = B_HOST_TO_LENDIAN_INT32(count);
	ptr += sizeof(int32);
	SetBase(sizeof(int32));
	
#if BINDER_DEBUG_MSGS
	BStringIO::ptr msg(new BStringIO);
	msg << "BParcel::SetValues {" << endl << indent;
#endif
	for (i=0;i<count;i++) {
		const ssize_t len = values[i]->Flatten(ptr, size, this);
		if (len < B_OK) {
			SetBinderOffsets(NULL, 0);
			do_free();
			return len;
		}
#if BINDER_DEBUG_MSGS
		msg
			<< "Flattened value to (" << ptr << "," << len << ") "
			<< indent
				<< BHexDump(ptr,len) << endl
				<< *values[i] << endl
			<< dedent;
#endif
		size -= len;
		ptr += len;
	}
#if BINDER_DEBUG_MSGS
	msg << dedent << "}" << endl;
	msg->PrintAndReset(berr);
#endif

	SetBase(0);
	
	return B_OK;
}

int32 BParcel::CountValues() const
{
	ssize_t avail = Length();
	int32 numValues = 0;
	if (Data() && avail > static_cast<ssize_t>(sizeof(int32))) {
		numValues = B_LENDIAN_TO_HOST_INT32(*reinterpret_cast<const int32*>(Data()));
	}
	return numValues;
}

int32 BParcel::GetValues(int32 maxCount, BValue* outValues) const
{
	ssize_t avail = Length();
	int32 i=0;
	if (Data() && avail > static_cast<ssize_t>(sizeof(int32))) {
		const uint8 *buf = static_cast<const uint8*>(Data());
		int32 numValues = B_LENDIAN_TO_HOST_INT32(*reinterpret_cast<const int32*>(buf));
		int32 tmp;
		
		buf += sizeof(int32);
		avail -= sizeof(int32);
		
		if (numValues > maxCount) numValues = maxCount;
		while (i < numValues && avail > 0) {
			tmp = outValues[i].Unflatten(buf,avail);
#if BINDER_DEBUG_MSGS
			BStringIO::ptr msg(new BStringIO);
			msg
				<< "Unflatten value from (" << buf << "," << tmp << ") "
				<< indent
					<< BHexDump(buf,tmp) << endl
					<< outValues[i] << endl
				<< dedent;
			msg->PrintAndReset(berr);
#endif
			buf += tmp;
			avail -= tmp;
			i++;
		}
	}
	
	return i;
}

bool BParcel::ReplyRequested() const
{
	return m_reply ? true : false;
}

status_t BParcel::Reply()
{
	const reply_func reply = m_reply;
	if (reply) {
		m_reply = NULL;
		return reply(*this, m_replyContext);
	}
	return B_NO_INIT;
}

void BParcel::Free()
{
	Reply();
	do_free();
}

void BParcel::SetBase(size_t pos)
{
	m_base = pos;
}

size_t BParcel::MoveBase(ssize_t delta)
{
	const size_t prev = m_base;
	m_base = prev + delta;
	return prev;
}

size_t BParcel::Base() const
{
	return m_base;
}

status_t BParcel::AddBinder(size_t typeOffset, size_t objectOffset)
{
	entry e;
	e.type = typeOffset + m_base;
	e.object = objectOffset + m_base;
	const ssize_t r = m_offsets.AddItem(e);
	return r >= B_OK ? B_OK : r;
}

status_t BParcel::AddBinder(const IBinder::ptr& object)
{
	const ssize_t r = m_binders.AddItem(object);
	return r >= B_OK ? B_OK : r;
}

status_t BParcel::AddBinder(size_t typeOffset, size_t objectOffset,
	const IBinder::ptr& object)
{
	entry e;
	e.type = typeOffset + m_base;
	e.object = objectOffset + m_base;
	const ssize_t r = m_offsets.AddItem(e);
	const ssize_t r2 = m_binders.AddItem(object);
	return r >= B_OK ? (r2 >= B_OK ? B_OK : r2) : r;
}

const void* BParcel::BinderOffsetsData() const
{
	return &m_offsets.ItemAt(0);
}

size_t BParcel::BinderOffsetsLength() const
{
	return m_offsets.CountItems() * sizeof(entry);
}

status_t BParcel::SetBinderOffsets(const void* offsets, size_t length)
{
	entry e;
	ssize_t s;
	
	m_offsets.MakeEmpty();
	m_binders.MakeEmpty();
	m_offsets.SetCapacity(length / sizeof(entry));
	m_base = 0;
	
	while (length >= sizeof(entry)) {
		e.type = *(static_cast<const size_t*>(offsets));
		e.object = *(static_cast<const size_t*>(offsets)+1);
		if ((s=m_offsets.AddItem(e)) < B_OK) {
			m_offsets.MakeEmpty();
			return s;
		}
		offsets = static_cast<const uint8*>(offsets) + sizeof(entry);
		length -= sizeof(entry);
	}
	
	return B_OK;
}

void BParcel::PrintToStream(ITextOutput::arg io, uint32 flags) const
{
	if (flags&B_PRINT_STREAM_HEADER) io << "BParcel(";
	
	if (Data() && Length() > 0) {
		io << indent << BHexDump(Data(), Length());
		const int32 N = m_offsets.CountItems();
		for (int32 i=0; i<N; i++) {
			io << endl << "Binder #" << i << " @ (" << (void*)(m_offsets[i].type)
				<< "," << (void*)(m_offsets[i].object) << "): "
				<< BTypeCode(*(uint32*)(((uint8*)m_data)+m_offsets[i].type))
				<< " = " << (*(void**)(((uint8*)m_data)+m_offsets[i].object));
		}
		io << dedent;
	} else if (Data()) {
		io << "Data=" << Data() << ", Length=" << Length();
	} else {
		io << "NULL";
	}
	
	if (flags&B_PRINT_STREAM_HEADER) io << ")";
}

ITextOutput::arg operator<<(ITextOutput::arg io, const BParcel& buffer)
{
	buffer.PrintToStream(io, B_PRINT_STREAM_HEADER);
	return io;
}

} } // namespace B::Support2
