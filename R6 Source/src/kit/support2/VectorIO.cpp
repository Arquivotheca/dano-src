
#include <support2/VectorIO.h>

#include <support2/Vector.h>

#include <memory.h>
#include <stdlib.h>

namespace B {
namespace Support2 {

BVectorIO::BVectorIO()
{
	m_heapVector = NULL;
	m_vectorCount = 0;
	m_totalLength = 0;
	m_localData.size = sizeof(m_localDataBuffer);
	m_localData.next_pos = 0;
	m_heapData = NULL;
}

BVectorIO::~BVectorIO()
{
	MakeEmpty();
}

const iovec* BVectorIO::Vectors() const
{
	return m_vectorCount <= MAX_LOCAL_VECTOR
		? m_localVector
		: (&m_heapVector->ItemAt(0));
}

ssize_t BVectorIO::CountVectors() const
{
	return m_vectorCount;
}

size_t BVectorIO::DataLength() const
{
	return m_totalLength;
}

void BVectorIO::MakeEmpty()
{
	if (m_heapVector) {
		delete m_heapVector;
		m_heapVector = NULL;
	}
	if (m_heapData) {
		const int32 ND = m_heapData->CountItems();
		for (int32 i=0; i<ND; i++) free(m_heapData->ItemAt(i));
		delete m_heapData;
		m_heapData = NULL;
	}
	
	m_vectorCount = 0;
	m_totalLength = 0;
	m_localData.size = sizeof(m_localDataBuffer);
	m_localData.next_pos = 0;
}

void BVectorIO::SetError(status_t err)
{
	m_vectorCount = err;
}

ssize_t BVectorIO::AddVector(void* base, size_t length)
{
	if (m_vectorCount < B_OK) return m_vectorCount;
	
	const int32 i = m_vectorCount++;
	if (i < MAX_LOCAL_VECTOR) {
		m_localVector[i].iov_base = base;
		m_localVector[i].iov_len = length;
	} else {
		if (!m_heapVector) {
			ssize_t result = B_OK;
			m_heapVector = new BVector<iovec>;
			if (m_heapVector) {
				m_heapVector->SetCapacity(MAX_LOCAL_VECTOR*2);
				for (int32 j=0; j<MAX_LOCAL_VECTOR && result >= B_OK; j++)
					result = m_heapVector->AddItem(m_localVector[i]);
			} else {
				result = B_NO_MEMORY;
			}
			if (result < B_OK) {
				m_vectorCount = result;
				return result;
			}
		}
		iovec v;
		v.iov_base = base;
		v.iov_len = length;
		const ssize_t result = m_heapVector->AddItem(v);
		if (result < B_OK) {
			m_vectorCount = result;
			return result;
		}
	}
	
	m_totalLength += length;
	return i;
}

ssize_t BVectorIO::AddVector(const iovec* vector, size_t count)
{
	ssize_t pos = m_vectorCount;
	while (count-- > 0) {
		pos = AddVector(vector->iov_base, vector->iov_len);
		if (pos < B_OK) break;
		vector++;
	}
	return pos;
}

void* BVectorIO::Allocate(size_t length)
{
	ssize_t result = B_NO_MEMORY;
	alloc_data* ad = &m_localData;
	if ((ad->next_pos+length) > ad->size) {
		ad = NULL;
		if (!m_heapData) m_heapData = new BVector<alloc_data*>;
		if (m_heapData) {
			if (length < (MAX_LOCAL_DATA/2)) {
				const int32 N = m_heapData->CountItems();
				ad = m_heapData->ItemAt(N-1);
				if ((ad->next_pos+length) > ad->size) {
					ad = static_cast<alloc_data*>(
						malloc(sizeof(alloc_data) + MAX_LOCAL_DATA));
					if (ad) result = m_heapData->AddItem(ad);
					if (result >= B_OK) {
						ad->size = MAX_LOCAL_DATA;
						ad->next_pos = 0;
					} else {
						free(ad);
						ad = NULL;
					}
				}
			}
			if (!ad) {
				ad = static_cast<alloc_data*>(
					malloc(sizeof(alloc_data) + length));
				if (ad) result = m_heapData->AddItemAt(ad, 0);
				if (result >= B_OK) {
					ad->size = length;
					ad->next_pos = 0;
				} else {
					free(ad);
					ad = NULL;
				}
			}
		}
	}
	
	if (ad) {
		void* data = reinterpret_cast<uint8*>(ad+1)+ad->next_pos;
		result = AddVector(data, length);
		if (result >= B_OK) {
			ad->next_pos += length;
			return data;
		}
	}
	
	m_vectorCount = result;
	return NULL;
}

ssize_t BVectorIO::Write(void* buffer, size_t avail) const
{
	const iovec* vec = Vectors();
	ssize_t count = CountVectors();
	size_t total = 0;
	while (--count >= 0 && total < avail) {
		total += vec->iov_len;
		if (total <= avail) memcpy(buffer, vec->iov_base, vec->iov_len);
		else {
			memcpy(buffer, vec->iov_base, vec->iov_len-(total-avail));
			total = avail;
		}
		buffer = static_cast<uint8*>(buffer) + vec->iov_len;
		vec++;
	}
	return total;
}

ssize_t BVectorIO::Read(const void* buffer, size_t avail) const
{
	const iovec* vec = Vectors();
	ssize_t count = CountVectors();
	size_t total = 0;
	while (--count >= 0 && total < avail) {
		total += vec->iov_len;
		if (total <= avail) memcpy(vec->iov_base, buffer, vec->iov_len);
		else {
			memcpy(vec->iov_base, buffer, vec->iov_len-(total-avail));
			total = avail;
		}
		buffer = static_cast<const uint8*>(buffer) + vec->iov_len;
		vec++;
	}
	return total;
}

status_t BVectorIO::_ReservedVectorIO1()	{ return B_UNSUPPORTED; }
status_t BVectorIO::_ReservedVectorIO2()	{ return B_UNSUPPORTED; }
status_t BVectorIO::_ReservedVectorIO3()	{ return B_UNSUPPORTED; }
status_t BVectorIO::_ReservedVectorIO4()	{ return B_UNSUPPORTED; }
status_t BVectorIO::_ReservedVectorIO5()	{ return B_UNSUPPORTED; }
status_t BVectorIO::_ReservedVectorIO6()	{ return B_UNSUPPORTED; }
status_t BVectorIO::_ReservedVectorIO7()	{ return B_UNSUPPORTED; }
status_t BVectorIO::_ReservedVectorIO8()	{ return B_UNSUPPORTED; }
status_t BVectorIO::_ReservedVectorIO9()	{ return B_UNSUPPORTED; }
status_t BVectorIO::_ReservedVectorIO10()	{ return B_UNSUPPORTED; }

} }	// namespace B::Support2
