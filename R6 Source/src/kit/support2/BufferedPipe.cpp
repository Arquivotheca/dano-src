
#include <support2/BufferedPipe.h>

#include <kernel/OS.h>
#include <support2/Parcel.h>
#include <support2/String.h>
#include <support2/Value.h>

namespace B {
namespace Support2 {

BBufferedPipe::BBufferedPipe()
{
	m_buffer = 0;
	m_pos = 0;
	m_size = 0;
	m_error = 0;
}

BBufferedPipe::~BBufferedPipe()
{
	if (m_buffer) FreeBuffer(m_buffer,m_size);
}

status_t 
BBufferedPipe::Status()
{
	return m_error;
}

void
BBufferedPipe::SetStatus(status_t err)
{
	m_error = err;
}

void 
BBufferedPipe::FreeBuffer(uint8 *, int32 )
{
}

/******************************************************************/

BBufferedInputPipe::BBufferedInputPipe()
{
}


BBufferedInputPipe::~BBufferedInputPipe()
{
}

void 
BBufferedInputPipe::read(void *_buffer, int32 size)
{
	void *ptr;
	uint8 *buffer = (uint8*)_buffer;
	int32 outSize;
	
	while (size) {
		ptr = Reserve(size,outSize);
		if (!outSize) {
			Renew();
			ptr = Reserve(size,outSize);
			if (!outSize) debugger("BBufferedInputPipe::read got zero-length buffer!");
		}
		memcpy(buffer,ptr,outSize);
		size -= outSize;
		buffer += outSize;
	}
}

int64
BBufferedInputPipe::read64()
{
	return *((int64*)Assert(sizeof(int64)));
}

int32
BBufferedInputPipe::read32()
{
	return *((int32*)Assert(sizeof(int32)));
}

int16
BBufferedInputPipe::read16()
{
	return *((int16*)Assert(sizeof(int16)));
}

int8
BBufferedInputPipe::read8()
{
	return *((int8*)Assert(sizeof(int8)));
}

double
BBufferedInputPipe::readf64()
{
	return *((double*)Assert(sizeof(double)));
}

float
BBufferedInputPipe::readf32()
{
	return *((float*)Assert(sizeof(float)));
}

void
BBufferedInputPipe::readstr(char *str)
{
	int32 len = read32();
	read(str,len);
	str[len] = 0;
}

BString
BBufferedInputPipe::readstr()
{
	BString str;
	int32 len = read32();
	read(str.LockBuffer(len+1),len);
	str.UnlockBuffer(len);
	return str;
}

void
BBufferedInputPipe::drain(int32 size)
{
	int32 outSize;
	
	while (size) {
		Reserve(size,outSize);
		if (!outSize) {
			Renew();
			Reserve(size,outSize);
			if (!outSize) debugger("BBufferedInputPipe::drain got zero-length buffer!");
		}
		size -= outSize;
	}
}

/******************************************************************/

BBufferedOutputPipe::BBufferedOutputPipe()
{
}


BBufferedOutputPipe::~BBufferedOutputPipe()
{
}

void 
BBufferedOutputPipe::write(const void *_buffer, int32 size)
{
	void *ptr;
	const uint8 *buffer = (const uint8*)_buffer;
	int32 outSize;
	
	while (size) {
		ptr = Reserve(size,outSize);
		if (!outSize) {
			Renew();
			ptr = Reserve(size,outSize);
			if (!outSize) debugger("BBufferedOutputPipe::write got zero-length buffer!");
		}
		memcpy(ptr,buffer,outSize);
		size -= outSize;
		buffer += outSize;
	}
}

void 
BBufferedOutputPipe::write64(int64 value)
{
	*((int64*)Assert(sizeof(int64))) = value;
}

void 
BBufferedOutputPipe::write32(int32 value)
{
	*((int32*)Assert(sizeof(int32))) = value;
}

void 
BBufferedOutputPipe::write16(int16 value)
{
	*((int16*)Assert(sizeof(int16))) = value;
}

void 
BBufferedOutputPipe::write8(int8 value)
{
	*((int8*)Assert(sizeof(int8))) = value;
}

void 
BBufferedOutputPipe::writef64(double value)
{
	*((double*)Assert(sizeof(double))) = value;
}

void 
BBufferedOutputPipe::writef32(float value)
{
	*((float*)Assert(sizeof(float))) = value;
}

void 
BBufferedOutputPipe::writestr(const char *str)
{
	int32 len = strlen(str)+1;
	write32(len);
	write(str,len);
}

void 
BBufferedOutputPipe::writebinder(const IBinder::ptr& binder, BParcel *offset)
{
	BValue v(binder);
	const size_t size = v.FlattenedSize();
	write32(size);
	void *buffer = Assert(size);
	offset->SetBase(Position());
	v.Flatten(buffer, size, offset);
	offset->AddBinder(binder);
}

void 
BBufferedOutputPipe::flush()
{
	Renew();
}

} }	// namespace B::Support2
