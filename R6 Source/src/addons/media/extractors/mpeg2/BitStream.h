#ifndef C_BIT_STREAM_H

#define C_BIT_STREAM_H

#include <SupportDefs.h>
#include <Debug.h>

class CBitStream
{
	static const size_t kBufferSize=8192;
	
	CBitStream (const CBitStream &);
	CBitStream &operator= (const CBitStream &);

	uint32 fRemainder;
	int32 fRemainderBitsLeft;
	uint8 *fBuffer;
	size_t fByteOffset;
	size_t fBufferLength;
	
	protected:
		virtual ssize_t Read (void *bytes, size_t size) = 0;
		virtual ssize_t Skip (size_t size) = 0;

	public:
		CBitStream();
		virtual ~CBitStream();
				
		uint32 PeekBits (int32 count);
		uint32 GetBits (int32 count);
		void SkipBits (int32 count);
		void GetBytes (void *data, size_t size);
				
		void SkipBytes (size_t size);
		
		void SeekByteBoundary();
		
		virtual void MakeEmpty();
		
		size_t BufferedBitsLeft() const;
		
		struct eof_exception {};
};

inline uint32 
CBitStream::PeekBits(int32 count)
{
	if (fRemainderBitsLeft>=count)
		return fRemainder>>(32-count);
	
	count-=fRemainderBitsLeft;
	uint32 val=fRemainderBitsLeft ? (fRemainder>>(32-fRemainderBitsLeft))<<count : 0;
	
	if (fByteOffset+3>=fBufferLength)
	{
		ssize_t nbytes=Read(fBuffer,kBufferSize);
		
		if (nbytes<=0)
			throw eof_exception();
			
		ASSERT(nbytes>0);

		fBufferLength=nbytes;
		fByteOffset=0;
	}
	
	uint32 temp=fBuffer[fByteOffset]<<24 | fBuffer[fByteOffset+1]<<16
				| fBuffer[fByteOffset+2]<<8 | fBuffer[fByteOffset+3];
	
	return val | (temp>>(32-count));
}

inline uint32 
CBitStream::GetBits(int32 count)
{
	uint32 val=0;

	do
	{
		if (fRemainderBitsLeft)
		{	
			size_t cut=min_c(fRemainderBitsLeft,count);
			
			val=(val<<cut)|(fRemainder>>(32-cut));
			
			fRemainder<<=cut;
			fRemainderBitsLeft-=cut;
			count-=cut;
			
			if (count==0)
				return val;
		}
		
		if (fByteOffset+3>=fBufferLength)
		{
			ssize_t nbytes=Read(fBuffer,kBufferSize);

			if (nbytes<=0)
				throw eof_exception();
	
			ASSERT(nbytes>0);
			
			fBufferLength=nbytes;
			fByteOffset=0;
		}

		fRemainder=fBuffer[fByteOffset]<<24 | fBuffer[fByteOffset+1]<<16
					| fBuffer[fByteOffset+2]<<8 | fBuffer[fByteOffset+3];
		
		fByteOffset+=4;
		fRemainderBitsLeft=32;				
	}
	while (count>0);
	
	return val;
}

inline void 
CBitStream::SkipBits(int32 count)
{
	(void)GetBits(count);
}

#endif
