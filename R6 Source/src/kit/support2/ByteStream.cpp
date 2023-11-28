
#include <support2/IByteStream.h>
#include <support2/ByteStream.h>
#include <support2/Parcel.h>

#include <malloc.h>
#include <support2/StdIO.h>
#include <support2_p/BinderKeys.h>

namespace B {
namespace Support2 {

using namespace B::Private;

/*-----------------------------------------------------------------*/

class RByteInput : public RInterface<IByteInput>
{
	public:

		RByteInput(IBinder::arg o) : RInterface<IByteInput>(o) {};

		virtual ssize_t ReadV(const iovec *vector, ssize_t count)
		{
			ssize_t thisSize,size = 0;
			for (int32 i=0;i<count;i++) size += vector[i].iov_len;

			BParcel command;
			BParcel reply;
			command.Copy(&size, sizeof(size));
			ssize_t status = Remote()->Transact(B_READ_TRANSACTION, command, &reply);
			ssize_t sizeLeft = reply.Length();
			if (status >= B_OK && sizeLeft >= static_cast<ssize_t>(sizeof(ssize_t))) {
				const uint8 *ptr,*buf;
				buf = static_cast<const uint8*>(reply.Data());
				if (*reinterpret_cast<const ssize_t*>(buf) >= 0) {
					// No error; return data.
					buf += sizeof(ssize_t);
					sizeLeft -= sizeof(ssize_t);
					ptr = buf;
					for (int32 i=0;i<count && sizeLeft>0;i++) {
						thisSize = vector[i].iov_len;
						if (thisSize > sizeLeft) thisSize = sizeLeft;
						memcpy(vector[i].iov_base,ptr,thisSize);
						sizeLeft -= thisSize;
						ptr += thisSize;
					}
					return ptr-buf;
				} else {
					// Return error code.
					return *reinterpret_cast<const ssize_t*>(buf);
				}
			} else {
				// A completely empty buffer means the other side ran
				// out of memory.
				return B_NO_MEMORY;
			}
			return status;
		}
};

class RByteOutput : public RInterface<IByteOutput>
{
	public:

		RByteOutput(IBinder::arg o) : RInterface<IByteOutput>(o) {};

		virtual ssize_t WriteV(const iovec *vector, ssize_t count)
		{
			BParcel command;
			BParcel reply;
			ssize_t status(B_OK);
			
			if (count == 1) {
				command.Reference(vector->iov_base, vector->iov_len);
			} else {
				ssize_t size = 0, i;
				
				for (i=0;i<count;i++) size += vector[i].iov_len;
				uint8 *buf = static_cast<uint8*>(command.Alloc(size));
				if (buf) {
					for (i=0;i<count;i++) {
						memcpy(buf,vector[i].iov_base,vector[i].iov_len);
						buf += vector[i].iov_len;
					}
				} else {
					status = B_NO_MEMORY;
				}
			}
			
			if (status >= B_OK) {
				status = Remote()->Transact(B_WRITE_TRANSACTION, command, &reply);
				if (status >= B_OK) {
					if (reply.Length() >= static_cast<ssize_t>(sizeof(ssize_t)))
						status = *static_cast<const ssize_t*>(reply.Data());
					else
						status = B_ERROR;
				}
			}
			
			return status;
		}

		virtual status_t End()
		{
			Remote()->Put(BValue(g_keyEnd, BValue::null));
			return B_OK;
		}
		
		virtual status_t Sync()
		{
			BValue v(Remote()->Get(g_keySync));
			int32 result;
			if (v.GetInt32(&result) != B_OK) result = B_ERROR;
			return result;
		}
};


class RByteSeekable : public RInterface<IByteSeekable>
{
	public:

		RByteSeekable(IBinder::arg o) : RInterface<IByteSeekable>(o) {};

		virtual	off_t Position() const
		{
			BValue v(Remote()->Get(g_keyPosition));
			int64 off;
			if (v.GetInt64(&off) != B_OK) off = B_ERROR;
			return off;
		}
		
		virtual off_t Seek(off_t position, uint32 seek_mode)
		{
			BValue v(Remote()->Invoke(
				BValue(g_keyPosition, BValue::Int64(position))
					.Overlay(g_keyMode, BValue::Int32(seek_mode)),
				g_keySeek));
			int64 off;
			if (v.GetInt64(&off) != B_OK) off = B_ERROR;
			return off;
		}
};


/*-----------------------------------------------------------------*/

B_IMPLEMENT_META_INTERFACE(ByteInput)
B_IMPLEMENT_META_INTERFACE(ByteOutput)
B_IMPLEMENT_META_INTERFACE(ByteSeekable)

/*-----------------------------------------------------------------*/

status_t
LByteInput::Link(const IBinder::ptr &to, const BValue &bindings)
{
	return BBinder::Link(to, bindings);
}

status_t
LByteInput::Unlink(const IBinder::ptr &from, const BValue &bindings)
{
	return BBinder::Unlink(from, bindings);
}

status_t
LByteInput::Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out)
{
	return BBinder::Effect(in, inBindings, outBindings, out);
}

status_t
LByteInput::Transact(uint32 code, BParcel& data, BParcel* reply, uint32 flags)
{
	if (code == B_READ_TRANSACTION) {
		if (data.Length() >= static_cast<ssize_t>(sizeof(ssize_t))) {
			ssize_t size = *static_cast<const ssize_t*>(data.Data());
			void* out;
			
			// Try to allocate a buffer to read data in to (plus room for
			// a status code at the front).
			// If an error occurred trying to allocate the buffer,
			// just return.  The caller will intepret an empty buffer
			// as being a B_NO_MEMORY error.
			
			if (size > 0 && reply && (out=reply->Alloc(size+sizeof(ssize_t))) != NULL) {
				const ssize_t amt = Read(static_cast<uint8*>(out)+sizeof(ssize_t), size);
				*static_cast<ssize_t*>(out) = amt;
				if (amt >= 0) reply->ReAlloc(amt+sizeof(ssize_t));
				else reply->ReAlloc(sizeof(ssize_t));
			}
		}
		
		return B_OK;
	} else {
		return BBinder::Transact(code, data, reply, flags);
	}
}

status_t 
LByteInput::Told(BValue &/*in*/)
{
	return B_OK;
}

status_t 
LByteInput::Asked(const BValue &/*outBindings*/, BValue &/*out*/)
{
	return B_OK;
}

status_t
LByteInput::Called(BValue &in, const BValue &outBindings, BValue &out)
{
	BValue val;
	int32 ival;
	if ((val=in[g_keyRead]).GetInt32(&ival) == B_OK) {
		BValue result;
		void* data = result.BeginEditBytes(B_RAW_TYPE, ival);
		if (data) {
			ssize_t s = Read(data, ival);
			result.EndEditBytes(s);
			if (s < B_OK) result = BValue::Int32(s);
		} else {
			result = BValue::Int32(B_NO_MEMORY);
		}
		out += outBindings * BValue(g_keyRead, result);
	}
	return B_OK;
}

/*-----------------------------------------------------------------*/

status_t
LByteOutput::Link(const IBinder::ptr &to, const BValue &bindings)
{
	return BBinder::Link(to, bindings);
}

status_t
LByteOutput::Unlink(const IBinder::ptr &from, const BValue &bindings)
{
	return BBinder::Unlink(from, bindings);
}

status_t
LByteOutput::Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out)
{
	return BBinder::Effect(in, inBindings, outBindings, out);
}

status_t
LByteOutput::Transact(uint32 code, BParcel& data, BParcel* reply, uint32 flags)
{
	if (code == B_WRITE_TRANSACTION) {
		iovec iov;
		iov.iov_base = const_cast<void*>(data.Data());
		iov.iov_len = data.Length();
		ssize_t result = B_ERROR;
		if (iov.iov_base && iov.iov_len > 0) result = WriteV(&iov,1);
		if (reply) reply->Copy(&result, sizeof(result));
		return B_OK;
	} else {
		return BBinder::Transact(code, data, reply, flags);
	}
}

status_t 
LByteOutput::Told(BValue &val)
{
	BValue v;
	if ((v = val[g_keyWrite])) {
		iovec iov;
		iov.iov_base = const_cast<void*>(v.Data());
		iov.iov_len = v.Length();
		WriteV(&iov,1);
	} else if ((v = val[g_keyEnd])) {
		End();
	}
	return B_OK;
}

status_t 
LByteOutput::Asked(const BValue &outBindings, BValue &out)
{
	if (outBindings * BValue(g_keySync, BValue::null)) {
		out += outBindings * BValue(g_keySync, BValue::Int32(Sync()));
	}

	return B_OK;
}

status_t
LByteOutput::Called(BValue &/*in*/, const BValue &/*outBindings*/, BValue &/*out*/)
{
	return B_OK;
}

/*-----------------------------------------------------------------*/

status_t
LByteSeekable::Link(const IBinder::ptr &to, const BValue &bindings)
{
	return BBinder::Link(to, bindings);
}

status_t
LByteSeekable::Unlink(const IBinder::ptr &from, const BValue &bindings)
{
	return BBinder::Unlink(from, bindings);
}

status_t
LByteSeekable::Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out)
{
	return BBinder::Effect(in, inBindings, outBindings, out);
}

status_t
LByteSeekable::Transact(uint32 code, BParcel& data, BParcel* reply, uint32 flags)
{
	return BBinder::Transact(code, data, reply, flags);
}

status_t 
LByteSeekable::Told(BValue &)
{
	return B_OK;
}

status_t 
LByteSeekable::Asked(const BValue &outBindings, BValue &out)
{
	out += outBindings * BValue(g_keyPosition, BValue::Int64(Position()));
	return B_OK;
}

status_t
LByteSeekable::Called(BValue &in, const BValue &outBindings, BValue &out)
{
	BValue val;
	if ( (val=in[g_keySeek]) ) {
		int64 pos;
		int32 mode;
		if (val[g_keyPosition].GetInt64(&pos) == B_OK &&
				val[g_keyMode].GetInt32(&mode) == B_OK) {
			out += outBindings * BValue(g_keySeek, BValue::Int64(Seek(pos, mode)));
		}
	}
	return B_OK;
}

} }	// namespace B::Support2
