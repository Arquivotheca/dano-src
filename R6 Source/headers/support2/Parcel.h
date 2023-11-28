/***************************************************************************
//
//	File:			support2/Parcel.h
//
//	Description:	Container for a raw block of data.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef	_SUPPORT2_PARCEL_H
#define	_SUPPORT2_PARCEL_H

#include <support2/SupportDefs.h>
#include <support2/ITextStream.h>
#include <support2/Vector.h>

namespace B {
namespace Support2 {

class IBinder;

/*-------------------------------------------------------------*/
/*------- BParcel Class ---------------------------------------*/

class BParcel
{
	public:
	
		typedef	void			(*free_func)(const void* data, ssize_t len, void* context);
		typedef	status_t		(*reply_func)(const BParcel& buffer, void* context);
	
								BParcel(reply_func replyFunc = NULL,
										void* replyContext = NULL);
								BParcel(const void* data, ssize_t len,
										free_func freeFunc = NULL,
										void* freeContext = NULL,
										reply_func replyFunc = NULL,
										void* replyContext = NULL);
								~BParcel();
	
				// Retrieving data.
				const void*		Data() const;
				ssize_t			Length() const;
				
				// Adding data.
				void			Reference(	const void* data, ssize_t len,
											free_func freeFunc = NULL,
											void* context = NULL);
				status_t		Copy(const void* data, ssize_t len);
				void*			Alloc(ssize_t len);
				void*			ReAlloc(ssize_t len);
				
				// This transfers ownership of the data from the given
				// BParcel to this one, leaving 'src' empty.
				void			Transfer(BParcel* src);
				
				// Adding/retrieving data as values.
				status_t		SetValues(const BValue* value1, ...);
				int32			CountValues() const;
				int32			GetValues(int32 maxCount, BValue* outValues) const;
				
				// Sending replies.
				bool			ReplyRequested() const;
				status_t		Reply();
				
				// Deallocating data.
				void			Free();
				
				// Describing binders in the data.
				void			SetBase(size_t pos);
				size_t			MoveBase(ssize_t delta);
				size_t			Base() const;
				
				//!	Add information about a binder object in the buffer.
				/*!	This form does \e not keep a reference on the binder
					object; some external entity must hold a reference
					while the BParcel exists.
				*/
				status_t		AddBinder(size_t typeOffset, size_t objectOffset);
				
				//!	Add a reference to a binder object in the buffer.
				/*!	This form only maintains a reference, it does not
					actually include information about where the binder
					appears in the buffer.
				*/
				status_t		AddBinder(const atom_ptr<IBinder>& object);
				
				//!	Add information about and a reference to a binder object in the buffer.
				status_t		AddBinder(	size_t typeOffset, size_t objectOffset,
											const atom_ptr<IBinder>& object);
	
				// Retrieving and restoring the binder information.
				const void*		BinderOffsetsData() const;
				size_t			BinderOffsetsLength() const;
				status_t		SetBinderOffsets(const void* offsets, size_t length);
				
				void			PrintToStream(ITextOutput::arg io, uint32 flags = 0) const;
				
	private:
								BParcel(const BParcel& o);
				BParcel&		operator=(const BParcel& o);
				
				void			do_free();
				
				const void*		m_data;
				ssize_t			m_length;
				ssize_t			m_origLength;
				
				free_func		m_free;
				void*			m_freeContext;
				
				reply_func		m_reply;
				void*			m_replyContext;
				
				uint8			m_inline[256];
				
				struct entry {
					size_t type;
					size_t object;
				};
				B_IMPLEMENT_BASIC_TYPE_FUNCS(entry);
		
				BVector<entry>				m_offsets;
				BVector<atom_ptr<IBinder> >	m_binders;
				size_t						m_base;
};

ITextOutput::arg	operator<<(ITextOutput::arg io, const BParcel& value);

/*-------------------------------------------------------------*/

} } // namespace B::Support2

#endif	// _SUPPORT2_PARCEL_H
