
#include <support2/MemoryStore.h>

#include <malloc.h>
#include <string.h>	// *cmp() functions

namespace B {
namespace Support2 {

// ----------------------------------------------------------------- //

BMemoryStore::BMemoryStore()
{
	fData = NULL;
	fLength = 0;
}

// ----------------------------------------------------------------- //

BMemoryStore::BMemoryStore(const BMemoryStore &o) : IStorage()
{
	fData = NULL;
	fLength = 0;
	Copy(o);
}

// ----------------------------------------------------------------- //

BMemoryStore::BMemoryStore(void *data, size_t size)
{
	fData = (char*)data;
	fLength = size;
}

// ----------------------------------------------------------------- //

BMemoryStore::~BMemoryStore()
{
	if (fData) FreeCore(fData);
}

/*-------------------------------------------------------------*/

BMemoryStore &BMemoryStore::operator=(const BMemoryStore &o)
{
	if (this != &o) Copy(o);
	return *this;
}

// ----------------------------------------------------------------- //

const void * BMemoryStore::Buffer() const
{
	return fData;
}

// ----------------------------------------------------------------- //

status_t 
BMemoryStore::AssertSpace(size_t newSize)
{
	void *p = MoreCore(fData,newSize);
	if (!p) return ENOMEM;
	fData = (char*)p;
	return B_OK;
}

// ----------------------------------------------------------------- //

int32 BMemoryStore::Compare(const BMemoryStore &o) const
{
	const void *obuf = o.Buffer();
	const size_t osize = o.Size();

	if (this == &o || fData == obuf)
		return 0;
	if (fData == NULL)
		return -1;
	if (o.fData == NULL)
		return 1;
	const int cmp = memcmp(fData, obuf, fLength <osize ? fLength : osize);
	if (cmp == 0 && fLength !=osize)
		return (fLength < osize ? -1 : 1);
	return cmp;
}

// ----------------------------------------------------------------- //

off_t 
BMemoryStore::Size() const
{
	return fLength;
}

// ----------------------------------------------------------------- //

ssize_t 
BMemoryStore::ReadAtV(off_t pos, const struct iovec *vector, ssize_t count)
{
	size_t size,totalSize = 0;

	while (--count >= 0) {
		if (pos >= fLength) break;
		size = vector->iov_len;
		if (pos + size > fLength) size = fLength - pos;
		memcpy(vector->iov_base, fData + pos, size);
		totalSize += size;
		pos += size;
		vector++;
	}

	return totalSize;
}

// ----------------------------------------------------------------- //

ssize_t 
BMemoryStore::WriteAtV(off_t pos, const struct iovec *vector, ssize_t count)
{
	size_t size,totalSize = 0;

	while (--count >= 0) {
		size = vector->iov_len;
		if (pos + size > fLength) {
			if (AssertSpace(pos + size)) {
				return totalSize;
			}
			fLength = pos + size;
		}
		memcpy(fData + pos, vector->iov_base, size);
		totalSize += size;
		pos += size;
		vector++;
	}

	return totalSize;
}

// ----------------------------------------------------------------- //

status_t BMemoryStore::Sync()
{
	return B_OK;
}

// ----------------------------------------------------------------- //

status_t BMemoryStore::SetSize(off_t size)
{
	const status_t result = AssertSpace(size);
	if (result == B_OK) fLength = size;
	return result;
}
		
// ----------------------------------------------------------------- //

status_t BMemoryStore::Copy(const BMemoryStore &o)
{
	const void *obuf = o.Buffer();
	const size_t osize = o.Size();
	if (obuf && osize) {
		if (AssertSpace(osize)) return ENOMEM;
		fLength = osize;
		memcpy(fData,obuf,osize);
	}
	return B_OK;
}

// ----------------------------------------------------------------- //

void * BMemoryStore::MoreCore(void *oldBuf, size_t newsize)
{
	if (newsize <= fLength) return oldBuf;
	return NULL;
}

void 
BMemoryStore::FreeCore(void *)
{
}

// ----------------------------------------------------------------- //

BMallocStore::BMallocStore()
{
	fBlockSize = 256;
	fMallocSize = 0;
}

BMallocStore::BMallocStore(const BMemoryStore &o) : BMemoryStore(o)
{
	fBlockSize = 256;
	fMallocSize = 0;
	Copy(o);
}

void BMallocStore::SetBlockSize(size_t newsize)
{
	fBlockSize = newsize;
}

void *BMallocStore::MoreCore(void *oldBuf, size_t size)
{
	if (size <= fMallocSize) return oldBuf;

	char	*newdata;
	size_t	newsize;
	newsize = ((size + (fBlockSize-1)) / fBlockSize) * fBlockSize;
	newdata = (char *) realloc(oldBuf, newsize);
	if (!newdata) return NULL;
	fMallocSize = newsize;
	return newdata;
}

void 
BMallocStore::FreeCore(void *oldBuf)
{
	free(oldBuf);
}

} }	// namespace B::Support2
