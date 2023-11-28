
#include <support2/StdIO.h>

#include <unistd.h>
#include <support2/Autolock.h>
#include <support2/KernelStreams.h>
#include <support2/TextStream.h>

extern "C" void _kdprintf_(const char *str, ...);

namespace B {
namespace Support2 {

class SerialOStr : public LByteOutput {
public:
	SerialOStr() : fPos(0)
	{
	}
	
	virtual ~SerialOStr()
	{
		flush_buffer();
	}
	
	virtual ssize_t Write(const void *buffer, size_t size);
	virtual ssize_t WriteV(const struct iovec *vector, ssize_t count);
	virtual	status_t End();
	virtual	status_t Sync();
			ssize_t WriteOnce(const void *buffer, size_t size);

private:
	void append_buffer(const void* data, size_t size);
	void flush_buffer();
	
	BLocker fLineSync;
	BLocker fAccess;
	char fBuffer[1024];
	size_t fPos;
};

status_t SerialOStr::End()
{
	return B_OK;
}

status_t SerialOStr::Sync()
{
	return B_OK;
}

ssize_t SerialOStr::WriteOnce(const void *buffer, size_t size)
{
	if (!buffer || size == 0) return 0;
	
	#if SYNCHRONIZE_LINES
	if (fPos == 0 || fBuffer[fPos-1] == '\n') {
		fLineSync.Lock();
		while (fPos != 0 && fBuffer[fPos-1] != '\n') {
			fLineSync.Unlock();
			fLineSync.Lock();
		}
	}
	#endif
	
	BAutolock l(fAccess.Lock());
	
	const size_t origSize = size;
	
	char* c = ((char*)buffer) + size - 1;
	while (c >= buffer && *c != '\n') c--;
	if (c >= buffer) {
		size_t len = (size_t)(c-(char*)buffer) + 1;
		append_buffer(buffer, len);
		size -= len;
		buffer = c + 1;
		flush_buffer();
	}
	
	if (size > 0) append_buffer(buffer, size);
	
	#if SYNCHRONIZE_LINES
	if (fPos == 0 || fBuffer[fPos-1] == '\n') {
		fLineSync.Unlock();
	}
	#endif
	
	return origSize;
}

ssize_t SerialOStr::Write(const void *buffer, size_t size)
{
	return WriteOnce(buffer, size);
}

ssize_t SerialOStr::WriteV(const struct iovec *vector, ssize_t count)
{
	size_t total = 0;
	for (int32 i=0;i<count;i++)
		total += WriteOnce(vector[i].iov_base,vector[i].iov_len);
	return total;
}

void SerialOStr::append_buffer(const void* data, size_t size)
{
	while (size > 0) {
		if (size+fPos < sizeof(fBuffer)-1) {
			memcpy(fBuffer+fPos, data, size);
			fPos += size;
			size = 0;
		} else {
			size_t avail = sizeof(fBuffer)-fPos-1;
			memcpy(fBuffer+fPos, data, avail);
			fPos += avail;
			data = ((char*)data) + avail;
			size -= avail;
		}
		
		if (fPos >= sizeof(fBuffer)-1) flush_buffer();
	}
}

void SerialOStr::flush_buffer()
{
	if (fPos == 0) return;
	if (fPos >= sizeof(fBuffer)) fPos = sizeof(fBuffer)-1;
	fBuffer[fPos] = 0;
	_kdprintf_("%s", fBuffer);
	fPos = 0;
}

const IByteOutput::ptr SerialOutput(new SerialOStr);

} } // namespace B::Support2
