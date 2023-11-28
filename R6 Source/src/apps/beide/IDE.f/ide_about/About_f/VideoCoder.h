//========================================================================
//	VideoCoder.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Class VideoCoder
//	Implement a simple RLE animation scheme for encoding 
//	frames into a compressed stream

#ifndef _VIDEOCODER_H
#define _VIDEOCODER_H

#include "IDEConstants.h"

#include <OS.h>
#include <Debug.h>


template<class T>
class VideoCoder
{
private:

		area_id					fStorage;

protected:

		uchar *					fOldFrame;
		uchar *					fNewFrame;

		ulong					fMaxDataSize;

		uchar *					fRLEData;
		ulong					fRLESize;

		uchar *					fChangeData;
		ulong					fChangeSize;

		int						fXPad;
		int						fXSize;
		int						fYSize;
		bool					fOutput;

		T &						fFile;

		bool					CodeRLE();
		bool					CodeChange();

		bool					DecodeRLE(
									unsigned char *data,
									ulong size);
		bool					DecodeChange(
									unsigned char *data,
									ulong size);

		long					CalcRLESize();		//	given state, what's the max size of an RLE image?

		enum {
			v_HeaderTag = MW_FOUR_CHAR_CODE('head'),
			v_RLETag = MW_FOUR_CHAR_CODE('rle '),
			v_ChangeTag = MW_FOUR_CHAR_CODE('delt'),
			v_EndTag = MW_FOUR_CHAR_CODE('end ')
		};

		long					WriteData(
									const void *data,
									ulong size,
									ulong tag);

		long					ReadData(
									uchar * & data,
									ulong & size,
									ulong & tag);

public:
								VideoCoder(
									T & openFile,
									bool output = FALSE,
									int frameSizeX = 0,		//	used when output is true
									int frameSizeY = 0);
								~VideoCoder();

		void					GetParams(
									int& xSize,
									int& ySize) { xSize = fXSize; ySize = fYSize; }

		//	compression
		long					PutFrame(
									uchar *data,
									ulong rowBytes,
									bool forceKey = false);

		//	playback
		long					GetFrame(
									uchar *data,
									ulong rowBytes);
};

#ifndef V_ERROR_BASE
 #define V_ERROR_BASE 0xffd00000
#endif
#define V_END_OF_DATA	((V_ERROR_BASE)+1)
#define V_CODER_FAILED	((V_ERROR_BASE)+2)

#define VIDEO_VERSION 100


struct VideoHeader {
	void	SwapBigToHost();
	void	SwapHostToBig();

	ulong	version;
	ulong	xSize;
	ulong	ySize;
};
inline void VideoHeader::SwapBigToHost()
{
	if (B_HOST_IS_LENDIAN)
	{
		version = B_BENDIAN_TO_HOST_INT32(version);
		xSize = B_BENDIAN_TO_HOST_INT32(xSize);
		ySize = B_BENDIAN_TO_HOST_INT32(ySize);
	}
}
inline void VideoHeader::SwapHostToBig()
{
	if (B_HOST_IS_LENDIAN)
	{
		version = B_HOST_TO_BENDIAN_INT32(version);
		xSize = B_HOST_TO_BENDIAN_INT32(xSize);
		ySize = B_HOST_TO_BENDIAN_INT32(ySize);
	}
}


template<class T>
VideoCoder<T>::VideoCoder(
	T & openFile,
	bool output,
	int xSize,
	int ySize) :
	fFile(openFile)
{
	long err = B_NO_ERROR;
	VideoHeader hdr;
	fStorage = -1;

	fOutput = output;
	if (output) {
		hdr.version = VIDEO_VERSION;
		hdr.xSize = xSize;
		hdr.ySize = ySize;
		if (xSize < 1 || ySize < 1)
			throw (long)V_CODER_FAILED;
		hdr.SwapHostToBig();
		err = WriteData(&hdr, sizeof(hdr), v_HeaderTag);
	} else {

		ulong size = 0;
		ulong tag = 0;
		err = fFile.Read(&tag, sizeof(tag));
		if (err >= 0) err = fFile.Read(&size, sizeof(size));
		if (err >= 0) err = fFile.Read(&hdr, sizeof(hdr));
		if (err > 0) err = 0;
		
		if (B_HOST_IS_LENDIAN)
		{
			hdr.SwapBigToHost();
			size = B_BENDIAN_TO_HOST_INT32(size);
		}

		if (tag != v_HeaderTag)
			throw (long)V_CODER_FAILED;
		if (hdr.version != VIDEO_VERSION)
			throw (long)V_CODER_FAILED;
		if (hdr.xSize < 1 || hdr.ySize < 1)
			throw (long)V_CODER_FAILED;
	}

	if (err)
		throw (long)err;
	void *data = NULL;
	fXSize = hdr.xSize;
	fYSize = hdr.ySize;
	fXPad = (fXSize+7)&~7;
	fMaxDataSize = (CalcRLESize()+7)&~7;
	//	the change stuff eats 1.5x RLE size as worst case, so we give it two times fMaxDataSize.
	int32 size = fMaxDataSize*3+(fXPad*fYSize)*2;
	size = (size+4095)&~4095;		//	page size is 4096
	fStorage = create_area("VideoCoder storage", &data, B_ANY_ADDRESS, size,
			B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (fStorage < 0)
		throw (long)fStorage;
	fOldFrame = (uchar *)data;
	fNewFrame = fOldFrame+fXPad*fYSize;
	fRLEData = fNewFrame+fXPad*fYSize;
	fChangeData = fRLEData+fMaxDataSize;
	fRLESize = fChangeSize = 0;
	memset(fNewFrame, 0, fXPad*fYSize);
}


template<class T>
VideoCoder<T>::~VideoCoder()
{
	if (fOutput)
		long err = WriteData(NULL, 0, v_EndTag);
	if (fStorage >= 0)
		delete_area(fStorage);
}


template<class T>
long
VideoCoder<T>::PutFrame(
	uchar *data,
	ulong rowBytes,
	bool forceKey)
{
	memcpy(fOldFrame, fNewFrame, fXPad*fYSize);
	if (rowBytes == fXPad) {
		memcpy(fNewFrame, data, fXPad*fYSize);
	} else {
		for (int ix=0; ix<fYSize; ix++) {
			memcpy(fNewFrame+fXPad*ix, data+rowBytes*ix, fXSize);
		}
	}
	if (!CodeRLE()) {
		return V_CODER_FAILED;
	}
	bool writeChange = false;
	if (!forceKey && CodeChange()) {
		if (fChangeSize <= fRLESize)
			writeChange = true;
	}
	long err;
	if (writeChange) {
		err = WriteData(fChangeData, fChangeSize, v_ChangeTag);
#if DEBUG
		printf("ChangeData (size %d : %d)\n", fChangeSize, fRLESize);
#endif
	} else {
		err = WriteData(fRLEData, fRLESize, v_RLETag);
#if DEBUG
		printf("RLEData (size %d : %d)\n", fChangeSize, fRLESize);
#endif
	}
	return err;
}


template<class T>
long
VideoCoder<T>::GetFrame(
	uchar *outData,
	ulong rowBytes)
{
	uchar *data;
	ulong size;
	ulong key;
	long err = ReadData(data, size, key);
	if (err) return err;
	if (key == v_EndTag) {
		return V_END_OF_DATA;
	}
	bool done = false;
	if (key == v_RLETag) {
		done = DecodeRLE(data, size);
	}
	else if (key == v_ChangeTag) {
		done = DecodeChange(data, size);
	}
#if DEBUG
	if (!done)
		puts("Corrupt data!");
#endif
	if (!done)
		return V_CODER_FAILED;
	if (rowBytes == fXPad) {
		memcpy(outData, fNewFrame, fXPad*fYSize);
	} else {
		int copy = fXPad;
		if (copy > rowBytes)
			copy = rowBytes;
		for (int ix=0; ix<fYSize; ix++) {
			memcpy(outData+rowBytes*ix, fNewFrame+fXPad*ix, copy);
		}
	}
	return B_NO_ERROR;
}


struct ItemHeader {
	void	SwapBigToHost();
	void	SwapHostToBig();

	ulong	tag;
	ulong	size;
};

inline void ItemHeader::SwapBigToHost()
{
	if (B_HOST_IS_LENDIAN)
	{
		// tag is bigendian so don't swap
		size = B_BENDIAN_TO_HOST_INT32(size);
	}
}
inline void ItemHeader::SwapHostToBig()
{
	if (B_HOST_IS_LENDIAN)
	{
		size = B_HOST_TO_BENDIAN_INT32(size);
	}
}

template<class T>
long
VideoCoder<T>::WriteData(
	const void * data,
	ulong size,
	ulong tag)
{
	ItemHeader hdr;
	hdr.size = size;
	hdr.tag = tag;
	hdr.SwapHostToBig();
	long err = fFile.Write(&hdr, sizeof(hdr));
	if (err < 0)
		return err;
	if (size > 0)
		err = fFile.Write(data, size);
	if (err < 0)
		return err;
	return 0;
}


template<class T>
long
VideoCoder<T>::ReadData(
	uchar * & data,
	ulong & size,
	ulong & tag)
{
	ItemHeader hdr;
	long err = fFile.Read(&hdr, sizeof(hdr));
	if (err < 0)
		return err;
	
	hdr.SwapBigToHost();
	size = hdr.size;
	tag = hdr.tag;
	if (hdr.size) {
		data = fRLEData;
		err = fFile.Read(fRLEData, hdr.size);
	} else {
		data = NULL;
	}
	if (err < 0)
		return err;
	return 0;
}


//	This is the max amount of data that can be taken by an 
//	RLE-coded frame (worst case; like vertical hairlines)
template<class T>
long
VideoCoder<T>::CalcRLESize()
{
	int size = fXSize+(fXSize+126)/127+1;	//	max expansion plus termination
	size *= fYSize;
	return size;
}


//	Code fNewFrame into fRLEData
//	Format of RLE coding, for each scan line:
//	<control byte> <data>
//	== 0 means no more data (just 0) until end of line
//	< 0 means repeat <data> byte 2-<control> times
//	> 0 means <data> consists of <control> literal bytes
//	each line is always terminated by a 0
template<class T>
bool
VideoCoder<T>::CodeRLE()
{
	unsigned char *out = fRLEData;
	for (int ix=0; ix<fYSize; ix++) {
		int nliteral = 0;
		unsigned char *literalstart = fNewFrame+ix*fXPad;
		unsigned char *ptr = literalstart;
		unsigned char *end = ptr+fXSize;
#if DEBUG
		unsigned char *dataStart = ptr;
#endif
		while (ptr < end) {
			int rlen = 1;
			for (; ptr+rlen<end; rlen++)
				if (ptr[rlen] != *ptr)
					break;
			ASSERT(ptr+rlen==end || ptr[rlen]!=*ptr);	//	make sure we found true end
			if (rlen > 2) {	//	 long repeat?
#if DEBUG
				unsigned char *save = ptr;
#endif
				if (literalstart < ptr) {	//	literals to go?
					while (ptr-literalstart > 0) {	//	emit literals
						int32 len = ptr-literalstart;
						if (len > 127)
							len = 127;
						*(out++) = len;
						memcpy(out, literalstart, len);
						out += len;
						literalstart += len;
					}
				}
				ASSERT(literalstart == ptr);	//	make sure we've caught up with it all
				if (*ptr == 0 && ptr+rlen == end) {	//	end of line with all 0?
					//	terminating 0 will take kind of this
					ptr = end;
					literalstart = ptr;
				} else {
					if (rlen > 130)
						rlen = 130;
					*(out++) = 2-rlen;
					ASSERT(((char)out[-1]) < 0);	//	make sure coding is OK
					*(out++) = *ptr;
					ASSERT(ptr[rlen-1] == *ptr);	//	make sure it's a real repeat
					ptr += rlen;
					literalstart = ptr;
				}
				ASSERT(ptr > save);	//	make sure we're making progress
			} else {
				ptr++;
			}
		}
		if (literalstart < ptr) {	//	literals to go?
			while (ptr-literalstart > 0) {	//	emit literals
				int32 len = ptr-literalstart;
				if (len > 127)
					len = 127;
				*(out++) = len;
				memcpy(out, literalstart, len);
				out += len;
				literalstart += len;
			}
		}
		ASSERT(literalstart == ptr);	//	make sure we've caught up with it all
		*(out++) = 0;	//	terminating 0
		ASSERT(ptr == end);	//	make sure we covered it all (exactly)
		ASSERT(out-fRLEData <= fMaxDataSize);	//	make sure we're not overwriting
#if DEBUG
		{	//	validate that the number of pixels coded is the right number
			
		}
#endif
	}
	fRLESize = out-fRLEData;
	if (fRLESize > fMaxDataSize)	//	this should not happen!!!
		return false;
	return true;
}


//	Format of change data:
//	<control> <data>
//	== 0 means no change to end of line (no data)
//	< 0 means no change for 1-<control> pixels (no data)
//	> 0 means <data> is <control> literal pixels
//	every line is terminated by a 0
//	Note that worst case for change is to use 1.5x data buffer size, so we have 
//	extra buffer space for that circumstance.
template<class T>
bool
VideoCoder<T>::CodeChange()
{
	fChangeSize = 0;
	unsigned char *out = fChangeData;
	for (int ix=0; ix<fYSize; ix++) {
		unsigned char *oldptr = fOldFrame+fXPad*ix;
		unsigned char *newptr = fNewFrame+fXPad*ix;
		unsigned char *oldend = oldptr+fXSize;
		unsigned char *literalstart = newptr;
		while (oldptr < oldend) {
#if DEBUG
			unsigned char *save = oldptr;
#endif
			int32 mlen = 0;
			for (; oldptr+mlen<oldend; mlen++)	//	find match for no-change
				if (oldptr[mlen] != newptr[mlen])
					break;
			ASSERT((oldptr+mlen == oldend) || (oldptr[mlen] != newptr[mlen]));
			if (mlen > 1) {
				while (literalstart < newptr) {	//	emit literals
					int len = newptr-literalstart;
					if (len > 127)
						len = 127;
					*(out++) = len;
					memcpy(out, literalstart, len);
					out += len;
					literalstart += len;
				}
				ASSERT(literalstart == newptr);
				if (mlen+oldptr == oldend) {	//	no more change
					//	terminating 0 takes care of this
					newptr += (oldend-oldptr);
					oldptr = oldend;
					literalstart = newptr;
				} else {
					newptr += mlen;
					oldptr += mlen;
					literalstart = newptr;
					while (mlen > 129) {
						if (mlen == 130) {	//	avoid 0 value
							*(out++) = -127;
							mlen -= 128;
						} else {
							*(out++) = -128;
							mlen -= 129;
						}
						ASSERT(((char)out[-1])<0);
					}
					*(out++) = 1-mlen;	//	code no change
					ASSERT(((char)out[-1])<0);
				}
			} else {
				oldptr++;
				newptr++;
			}
			ASSERT(oldptr > save); 	//	make sure we have progress
		}
		ASSERT(oldptr == oldend);
		while (literalstart < newptr) {	//	emit literals if needed
			int32 len = newptr-literalstart;
			if (len > 127)
				len = 127;
			*(out++) = len;
			memcpy(out, literalstart, len);
			out += len;
			literalstart += len;
		}
		ASSERT(literalstart == newptr);
		*(out++) = 0;	//	terminating 0
		ASSERT(out-fChangeData < fMaxDataSize*1.5);	//	make sure we don't overwrite
		if (out-fChangeData > fRLESize)
			break;	//	no idea coding more when we know it's too large
	}
	fChangeSize = out-fChangeData;
	ASSERT(fChangeSize <= fMaxDataSize*1.5);	//	make sure we haven't overrun
	if (fChangeSize <= fRLESize)
		return true;
	return false;
}


//	Given RLE data in handed buffer, decode it into fNewFrame
template<class T>
bool
VideoCoder<T>::DecodeRLE(
	unsigned char *data,
	ulong
#if DEBUG
	size
#endif
	)
{
#if DEBUG
	unsigned char *save = data+size;
#endif
	for (int ix=0; ix<fYSize; ix++) {	//	repeat for each scan line
		unsigned char *out = fNewFrame+ix*fXPad;
		unsigned char *end = out+fXSize;
		for (char control = *(data++); control; control=*(data++)) {
			if (control < 0) {
				char fill = *(data++);	//	repeat this byte
				memset(out, fill, 2-(int)control);
				out += 2-control;
			} else {
				ASSERT(control > 0);
				memcpy(out, data, control);	//	literal data
				out += control;
				data += control;
			}
		}
		ASSERT(out <= end);
		if (out < end) {
			memset(out, 0, end-out);	//	black to end of line
			out = end;
		}
		ASSERT(out == end);	//	make sure we terminate exactly correctly
		ASSERT(data <= save);	//	make sure we don't read too far
	}
	ASSERT(save == data);	//	make sure we use exactly all data
	return true;	//	assume coder always works
}


//	Given delta data in handed buffer, decode it into fNewBuffer
//	which contains the previous buffer on entry
template<class T>
bool
VideoCoder<T>::DecodeChange(
	unsigned char *data,
	ulong
#if DEBUG
	size
#endif
	)
{
#if DEBUG
	unsigned char *save = data+size;
#endif
	for (int ix=0; ix<fYSize; ix++) {	//	repeat for each scan line
		unsigned char *out = fNewFrame+ix*fXPad;
		unsigned char *end = out+fXSize;
		for (char control = *(data++); control; control=*(data++)) {
			if (control < 0) {
				//	no change for 1-control bytes
				out += 1-control;
			} else {
				ASSERT(control > 0);	//	literal data
				memcpy(out, data, control);
				out += control;
				data += control;
			}
		}
		if (out < end) {
			out = end;	//	ignore rest of line
		}
		ASSERT(out == end);	//	make sure we terminate exactly correctly
		ASSERT(data <= save);	//	make sure we don't read too far
	}
	ASSERT(save == data);	//	make sure we use exactly all data
	return true;	//	assume coder always works
}

#endif
