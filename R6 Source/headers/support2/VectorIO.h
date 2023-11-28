/***************************************************************************
//
//	File:			support2/VectorIO.h
//
//	Description:	A specialized vector for storing an iovec array.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _SUPPORT2_VECTORIO_H
#define _SUPPORT2_VECTORIO_H

#include <support2/SupportDefs.h>

#include <sys/uio.h>

namespace B {
namespace Support2 {

/*--------------------------------------------------------*/
/*----- BVectorIO class ----------------------------------*/

class BVectorIO
{
public:
						BVectorIO();
virtual					~BVectorIO();

		const iovec*	Vectors() const;
inline					operator const iovec*() const			{ return Vectors(); }

		ssize_t			CountVectors() const;
		size_t			DataLength() const;
		
		ssize_t			AddVector(void* base, size_t length);
		ssize_t			AddVector(const iovec* vector, size_t count = 1);
		void*			Allocate(size_t length);
		
		void			MakeEmpty();
		void			SetError(status_t err);
		
		ssize_t			Write(void* buffer, size_t avail) const;
		ssize_t			Read(const void* buffer, size_t avail) const;
		
private:

virtual	status_t		_ReservedVectorIO1();
virtual	status_t		_ReservedVectorIO2();
virtual	status_t		_ReservedVectorIO3();
virtual	status_t		_ReservedVectorIO4();
virtual	status_t		_ReservedVectorIO5();
virtual	status_t		_ReservedVectorIO6();
virtual	status_t		_ReservedVectorIO7();
virtual	status_t		_ReservedVectorIO8();
virtual	status_t		_ReservedVectorIO9();
virtual	status_t		_ReservedVectorIO10();

						BVectorIO(const BVectorIO& o);
		BVectorIO&		operator=(const BVectorIO& o);

		enum {
			MAX_LOCAL_VECTOR	= 16,
			MAX_LOCAL_DATA		= 256
		};
		
		struct alloc_data {
			size_t	size;
			size_t	next_pos;
		};
		
		iovec					m_localVector[MAX_LOCAL_VECTOR];
		BVector<iovec>*			m_heapVector;
		ssize_t					m_vectorCount;
		size_t					m_totalLength;
		
		alloc_data				m_localData;
		uint8					m_localDataBuffer[MAX_LOCAL_DATA];
		BVector<alloc_data*>*	m_heapData;
};

} }	// namespace B::Support2

#endif
