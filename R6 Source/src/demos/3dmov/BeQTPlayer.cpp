// ===========================================================================
//	BeMovieWindow.cpp
// 	©1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "BeQTPlayer.h"
#include "MoviePlay.h"

#include <Path.h>

// ============================================================================

BeQTPlayer::BeQTPlayer(Store *store) : QTPlayer(store), mBBitmap(0), mWidth(0), mHeight(0)
{
	mQTStore = store;	// Fancy buffered store, we own it, we should delete it
}

BeQTPlayer::~BeQTPlayer()
{
	delete mBBitmap;
	delete mQTStore;
}

//	Resize the video buffer
			
Boolean	BeQTPlayer::SetVideoBufferSize(long width,long height,long depth)
{
	if (mBBitmap)
		delete mBBitmap;
	mWidth  = width;
	mHeight = height;
	
//	screen_info info;
//	get_screen_info(&info);
	
//	if (info.mode == B_RGB_32_BIT) {
		mDepth = 32;
		mBBitmap = new BBitmap(BRect(0,0,width,height),B_RGB_32_BIT);	// 32 bits
	//} else {
	//	mDepth = 8;
	//	mBBitmap = new BBitmap(BRect(0,0,width,height),B_COLOR_8_BIT);	// 8 bits
	//}

	return mBBitmap != 0;
}

Boolean	BeQTPlayer::GetVideoBuffer(Byte **dst, long *dstRowBytes, long *depth)
{
	*dst = (Byte *)mBBitmap->Bits();
	*dstRowBytes = mBBitmap->BytesPerRow();
	*depth = mDepth;
	return true;
}

Boolean	BeQTPlayer::DrawVideo()
{
	return true;
}

BBitmap* BeQTPlayer::GetBitmap(BRect& r)
{
	r.Set(0,0,mWidth-1,mHeight-1);
	return mBBitmap;
}



// ============================================================================
//	QTStore buffers large chunks of data in memory

class QTStore : public Store {
public:
						QTStore(Store *store);
	virtual				~QTStore();
					
			void		ReadBlob();

	virtual	long		GetLength();
	virtual	void*		GetData(long pos, long size);		// Cached read methods
	virtual	long		ReleaseData(void *data, long size);

protected:
		Store*	mStore;
		long	mBlobCount;
		long	mBlobSize;
		long	mHead;
		long	mTail;
		Byte*	mBuffer;
		long	mBufferSize;
		Byte*	mHugeRead;
};

QTStore::QTStore(Store *store) : mStore(store), mBlobCount(10), mBlobSize(32*1024L), mHead(0), mTail(0), mHugeRead(0)
{
	mBufferSize = mBlobCount*mBlobSize;
	mBuffer = (Byte *)MALLOC(mBufferSize + mBlobSize); // big buffer per movie!, extra blob for makeup
}

QTStore::~QTStore()
{
	if (mBuffer)
		FREE(mBuffer);
	if (mHugeRead)
		FREE(mHugeRead);
}

long QTStore::GetLength()
{
	return mStore->GetLength();
}

//	Read a blob, push data out the back end if req

void QTStore::ReadBlob()
{
	long dataLeft = mStore->GetLength() - mHead;
	if (dataLeft <= 0) return;
	
	if ((mHead - mTail) < mBufferSize) {				// Space to read
		Byte *dst = mBuffer + (mHead % mBufferSize);	// Place to read to
		mStore->Seek(mHead);
		mStore->Read(dst,MIN(dataLeft,mBlobSize));
		mHead += mBlobSize;
		if ((mHead - mTail) >= mBufferSize)
			mTail += mBlobSize;
	}
}

void*	QTStore::GetData(long pos, long size)		// Cached read methods
{
	if (pos < mTail) {	// Reading backwards .. reset
		mHead = 0;
		mTail = 0;
	}
	
	if (size > mBlobSize || (pos - mHead) > mBufferSize) {	// out of band request, requires special handling...
		mHugeRead = (Byte *)MALLOC(size);
		if (mHugeRead) {
			mStore->Seek(pos);
			mStore->Read(mHugeRead,size);
		}
		return mHugeRead;
	}
	
	while (mHead < (pos + size))		// Read until request is in buffer
		ReadBlob();
	
	long offset = pos % mBufferSize;	// Start of data
	if (offset + size > mBufferSize)
		memcpy(mBuffer + mBufferSize,mBuffer,offset + size - mBufferSize);	// Make wrapped data contig...
	return mBuffer + offset;
}

long	QTStore::ReleaseData(void *data, long size)
{
	if (mHugeRead && data == mHugeRead) {
		FREE(mHugeRead);
		mHugeRead = NULL;
	}
}

// ============================================================================
// ============================================================================

class BFileStore : public Store {
public:		
						BFileStore(entry_ref *fileref);
						~BFileStore();
						
	virtual	long		Delete();
					
	virtual	long		Open(Boolean readOnly);
	virtual	long		Close();
	
	virtual	long		GetLength();
	virtual	long		GetPos();
	virtual	long		Seek(long pos);
	
	virtual	long		Read(void *data, long count);
	virtual	long		Write(void *data, long count);

protected:
	FILE		*fp;
	char		fpath[256];
	char		fname[256];
	entry_ref	*fref;
};


BFileStore::BFileStore(entry_ref *fileref)
{
	BEntry	*myentry;
	BPath		mypath;
	int16		off;
	
		// fp = pointer to open file
		// fpath = char string containing path to the file (does not include the file)
		// fname = char string containing the name of the file + path
		// fref = original entry_ref (contains name without path!)
	
	fref = fileref;
	myentry = new BEntry(fref);
	myentry->GetPath(&mypath);
	
	strcpy(fpath, mypath.Path());		// fpath currently contains the filename, we must trim this...
	strcpy(fname, fpath);
	
	off = strlen(fpath);
	while(fpath[off] != '/')
	{
		off--;
		if(off<=0)
		{
			off = strlen(fpath);
			break;
		}
	}
	
	fpath[off] = 0;	// terminate it where the first '/' was located, trancating the string
	fp = NULL; // don't open yet!
}

BFileStore::~BFileStore()
{
	Close();
}

long BFileStore::Delete()
{
	Close();
	remove(fname);		// POSIX RULES FOREVER!!!!!!!!!!!!!
}

long BFileStore::Open(Boolean readOnly)
{
	if (!fp)
		fp = fopen(fname, (readOnly) ? "rb" : "rwb");
	else
		fp = freopen(fname, (readOnly) ? "rb" : "rwb", fp); // in case we want to change modes
	
	return (fp)?1:0;
}

long BFileStore::Close()
{
	if(fp)
	{
		fclose(fp);
		fp = NULL;
		return 1;
	}
	
	return 0;
}

long BFileStore::GetLength()
{
	fpos_t now, size=0;
	bool	isopen = TRUE;
	
	if(!fp)
	{
		fp = fopen(fname, "rb");
		isopen = FALSE;
	}
	
	now = ftell(fp);
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, now, SEEK_SET);		// return to our original location
	
	if(!isopen)
	{
		fclose(fp);		// leave things as they were...
		fp = NULL;
	}
	
	return (long)size;
}

long BFileStore::GetPos()
{
	return (long)ftell(fp);	// cast to long... fpos_t is long long for BeOS
}

long BFileStore::Seek(long pos)
{
	return (long)fseek(fp, pos, SEEK_SET);
}

long BFileStore::Read(void *data, long count)
{
	return (long)fread(data, count, 1, fp);
}

long BFileStore::Write(void *data, long count)
{
	return (long)fwrite(data, count, 1, fp);
}

// ============================================================================
//	Create a thing that draws a quicktime movie

QTPlayer* NewBeQTPlayer(entry_ref *fileref)
{
	Store* store = new BufferedStore(new BFileStore(fileref));	 // Create a store from an entry_ref
	store->Open(true);
	
	Store* qtStore = new QTStore(store);			// Fancy buffered qt store
	BeQTPlayer *player = new BeQTPlayer(qtStore);	// Quicktime player
	
//	Bail if the movie isn't valid
	
	if (player->MovieIsValid() == false) {
		delete player;
		return NULL;
	}
	return player;
}

// ============================================================================

