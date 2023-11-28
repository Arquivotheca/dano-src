#include "CopyFile.h"
#include "MyDebug.h"

#include <File.h>

long	CopyBytes(BFile *dst, BFile *src, long count);
// assume files are open
// for copying small chunks
long	CopyBytes(BFile *dst, BFile *src, long count)
{
	const long bufsiz = 2048;
	char	*buf = new char[bufsiz];
	
	//ASSERT(src->IsOpen());
	//ASSERT(dst->IsOpen());
	
	long bytes, res;
	
	while( count ) {
		bytes = min_c(bufsiz,count);
		res = src->Read(buf,bytes);
		if (res < B_NO_ERROR || res != bytes)
			goto bail;
		res = dst->Write(buf,bytes);
		if (res < B_NO_ERROR || res != bytes)
			goto bail;
		count -= res;
	}
	
bail:
	delete buf;
	return res;
}

long	CopyFile(BEntry *dstEnt, BEntry *srcEnt)
{
	status_t err;

	BFile	src(srcEnt,O_RDONLY);
	err = src.InitCheck();
	ASSERT(err == B_NO_ERROR);
	PRINT(("source file opened read only\n"));
	if (err < B_OK)
		return err;
		
	BFile	dst(dstEnt,O_RDWR);
	err = dst.InitCheck();
	ASSERT(err == B_NO_ERROR);
	PRINT(("dest file opened read/write\n"));
	if (err < B_OK)
		return err;

	const int32		bufsiz = 16384;
	size_t			bytes;
	off_t			tot;
	char 			*buf = new char[bufsiz];	
	//long 			upSize, bucket;

	//BMessage tempMsg(M_UPDATE_TEMP);
	//int64 lsize;
	//src.GetSize(&lsize);
	//int32 ssize = lsize;
	//tempMsg.AddInt32("bytes",ssize);
	//updateMess.SendMessage(&tempMsg);
	//bucket = ssize/20.0;
	//upSize = max_c(1,bucket);
	//bucket = upSize;
	
	tot = 0;

	while( (bytes = src.Read(buf,bufsiz)) > 0 ) {
		err = dst.Write(buf,bytes);
		if (err < B_OK)
			break;
		tot += bytes;
		//if ((bucket -= bytes) <= 0) {
		//	BMessage upMsg(M_UPDATE_PROGRESS);
		//	upMsg.AddInt32("bytes",max_c(upSize,bytes));
		//	updateMess.SendMessage(&upMsg);
		//	bucket = upSize;
		//}
	}
	
	dst.SetSize(tot);
	delete buf;
	
	//updateMess.SendMessage(M_DONE);
	if (err < B_OK)
		return err;
	else
		return B_NO_ERROR;
}
