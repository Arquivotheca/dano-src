#include <Bitmap.h>
#include <TranslationUtils.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <Application.h>
#include <Screen.h>
#include <DataIO.h>
#include <TranslatorRoster.h>
#include <Resources.h>

typedef struct tagBITMAPINFOHEADER{
        uint32     biSize;
        long       biWidth;
        long       biHeight;
        uint16     biPlanes;
        uint16     biBitCount;
        uint32     biCompression;
        uint32     biSizeImage;
        long       biXPelsPerMeter;
        long       biYPelsPerMeter;
        uint32     biClrUsed;
        uint32     biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER;

#if 0

extern "C"
void *getbitmap(const char *filename)
{
	LPBITMAPINFOHEADER bh;
	const int width = 16;
	const int height = 16;
	size_t bmsize = width*height*4;
	bh = (LPBITMAPINFOHEADER)malloc(bmsize+sizeof(BITMAPINFOHEADER));
	if(bh == NULL) {
		//delete bm;
		return NULL;
	}
	bh->biSize = sizeof(BITMAPINFOHEADER);
	//bh->biWidth = (int)bm->Bounds().Width()+1;
	//bh->biHeight = (int)bm->Bounds().Height()+1;
	bh->biWidth = width;
	bh->biHeight = height;
	bh->biPlanes = 1;
	bh->biBitCount = 32;
	bh->biCompression = 0;
	bh->biSizeImage = bmsize;
	bh->biXPelsPerMeter = 1;
	bh->biYPelsPerMeter = 1;
	bh->biClrUsed = 0;
	bh->biClrImportant = 0;
	memset(bh+1, 0, bmsize);
	printf("created fake %dx%dx%d size bitmap\n", bh->biWidth, bh->biHeight, bh->biBitCount);
	return bh;
}

#else
extern "C"
void *getbitmap(const char *filename)
{
	status_t err;
	//BApplication app("application/x-test");
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	size_t bmsize;
	int bits = 0;
	LPBITMAPINFOHEADER bh;
	BMallocIO out;
	//BFile in(filename, O_RDONLY);
	
	image_info info;
	int32 cookie = 0;
	char *imagefilename = NULL;
	while(get_next_image_info(0, &cookie, &info) == B_OK) {
		//printf("image %s\n", info.name);
		if(info.text <= (void*)getbitmap &&
		   (uint8*)getbitmap < (uint8*)info.text+info.text_size) {
			//printf("found image %s\n", info.name);
			imagefilename = info.name;
			break;
		}
	}
	if(imagefilename == NULL) {
		printf("getbitmap(%s): image not found\n", filename);
		return NULL;
	}
	BFile resfile(imagefilename, O_RDONLY);
	err = resfile.InitCheck();
	if(err != B_NO_ERROR) {
		printf("getbitmap(%s): open failed, %s\n", filename, strerror(err));
		return NULL;
	}
	BResources res(&resfile);
	size_t ressize;
	void *resdata = res.FindResource('data', filename, &ressize);
	if(resdata == NULL) {
		printf("getbitmap(%s): FindResource failed\n", filename);
		return NULL;
	}
	BMemoryIO in(resdata, ressize);
	
	err = roster->Translate(&in, NULL, NULL, &out, B_TRANSLATOR_BITMAP);
	if(err != B_NO_ERROR) {
		printf("getbitmap(%s): translate failed, %s\n", filename, strerror(err));
		return NULL;
	}
	
	//BBitmap *bm = BTranslationUtils::GetBitmapFile(filename);
	TranslatorBitmap *bm = (TranslatorBitmap *)out.Buffer();
	//printf("size %d\n", out.BufferLength());
	if(bm == NULL) {
		printf("BTranslationUtils::GetBitmap(%s) failed\n", filename);
		return NULL;
	}
	if(B_BENDIAN_TO_HOST_INT32(bm->magic) != B_TRANSLATOR_BITMAP) {
		printf("getbitmap(%s): wrong output\n", filename);
		return NULL;
	}
#if 0
	if(bm->ColorSpace() == B_CMAP8)
		bits = 8;
	if(bm->ColorSpace() == B_RGB15)
		bits = 15;
	if(bm->ColorSpace() == B_RGB16)
		bits = 16;
	if(bm->ColorSpace() == B_RGB32)
		bits = 32;
#endif
	if(B_BENDIAN_TO_HOST_INT32(bm->colors) == B_CMAP8)
		bits = 8;
	if(B_BENDIAN_TO_HOST_INT32(bm->colors) == B_RGB15)
		bits = 15;
	if(B_BENDIAN_TO_HOST_INT32(bm->colors) == B_RGB16)
		bits = 16;
	if(B_BENDIAN_TO_HOST_INT32(bm->colors) == B_RGB32)
		bits = 32;
	if(bits == 0) {
		printf("BTranslationUtils::GetBitmap(%s) failed, "
		       "wrong colorspace (0x%08x)\n", filename, bm->colors);
		//delete bm;
		return NULL;
	}
	//bmsize = bm->BitsLength();
	bmsize = B_BENDIAN_TO_HOST_INT32(bm->dataSize);
	if(bmsize + sizeof(TranslatorBitmap) != out.BufferLength()) {
		printf("size %d\n", out.BufferLength());
		printf("bmsize %d\n", bmsize);
		return NULL;
	}

	if(bits == 8)
		bh = (LPBITMAPINFOHEADER)malloc(bmsize+sizeof(BITMAPINFOHEADER)+256*4);
	else
		bh = (LPBITMAPINFOHEADER)malloc(bmsize+sizeof(BITMAPINFOHEADER));
	if(bh == NULL) {
		//delete bm;
		return NULL;
	}
	bh->biSize = sizeof(BITMAPINFOHEADER);
	//bh->biWidth = (int)bm->Bounds().Width()+1;
	//bh->biHeight = (int)bm->Bounds().Height()+1;
	bh->biWidth = (int)B_BENDIAN_TO_HOST_FLOAT(bm->bounds.Width())+1;
	bh->biHeight = (int)B_BENDIAN_TO_HOST_FLOAT(bm->bounds.Height())+1;
	bh->biPlanes = 1;
	bh->biBitCount = bits;
	bh->biCompression = 0;
	bh->biSizeImage = bmsize;
	bh->biXPelsPerMeter = 1;
	bh->biYPelsPerMeter = 1;
	bh->biClrUsed = 255;
	bh->biClrImportant = 255;
	uint8 *srcbits = (uint8*)(bm+1);
	uint8 *destbits;
	if(bits == 8) {
		BScreen screen;
		memcpy(bh+1, screen.ColorMap()->color_list, 256*4);
		//memcpy(((uint8*)(bh+1))+256*4, bm->Bits(), bmsize);
		destbits = ((uint8*)(bh+1))+256*4;
	}
	else {
		destbits = (uint8*)(bh+1);
	}
	int linesize = bmsize / bh->biHeight;
	for(int y = 0; y < bh->biHeight; y++) {
		memcpy(destbits+bmsize-(y+1)*linesize, srcbits+y*linesize, linesize);
	}
	//delete bm;
	//printf("got %dx%dx%d size bitmap\n", bh->biWidth, bh->biHeight, bh->biBitCount);
	return bh;
}
#endif
