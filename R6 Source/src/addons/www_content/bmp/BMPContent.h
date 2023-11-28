//
//	Browser interface
//
#include <OS.h>
#include <Bitmap.h>
#include <View.h>
#include <Debug.h>
#include <stdio.h>
#include <ByteOrder.h>
#include <Message.h>
#include <Screen.h>
#include <GraphicsDefs.h>

#include "Content.h"

#include "FXFP_2_30.h"
#include "BMP_Definitions.h"

using namespace Wagner;

bigtime_t kMinUpdateInterval = 10000;

class BBitmap;

/*********************************************************************
 *   BMPContentInstance
 *
 */
class BMPContentInstance : public ContentInstance {
public:
	BMPContentInstance(Content *content, GHandler *handler);
	virtual status_t Draw(BView *into, BRect exposed);
	virtual status_t GetSize(int32 *width, int32 *height, uint32 *flags);
	virtual status_t ContentNotification(BMessage *msg);
};

/*********************************************************************
 *   BMPContent
 *
 */
 
enum BMPContentState {
	BMP_CONTENT_INIT = 0,
	BMP_FILEHEADER_REC = 1,
	BMP_COLORMAP_REC = 2,
	BMP_DATA_REC = 3,
	BMP_FINALSTATE = 4
};

class BMPContent : public Content
{
	friend class BMPContentInstance;

public:
	BMPContent(void* handle);
	virtual ~BMPContent();
	
	virtual ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	BBitmap* GetBitmap();
	virtual size_t GetMemoryUsage();
	virtual	bool IsInitialized();
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage&);

private:
	//Control Data
	bool mValid;
	int mState;
	int mVersion;
	bigtime_t 	mLastUpdate;

	//Content Control Data
	BBitmap*	mBitmap;
			
	//Content Format Data
	BMP_FileHeader mFileHeader;
	
	BMP_PV5BitmapInfo mBitmapInfo;
	unsigned long mHeaderSize;
	
	RGB_Color *mColorArray;
	unsigned long mColorCount;
	
	void *mPixelData;
	unsigned long mPixelDataSize;
	
	//Decode Control Data
	const void *mBuffer;
	unsigned long mBufferUsed;
	unsigned long mBufferUsedPrev;
	color_space mColorSpace;
	int32 mLastRow;
	
	void (*mDecodeFunction)(BMPContent *);
	
	//Decode Functions
	void RefreshBitmap();
	void SetupFileHeader(uchar *fheaderptr);
	void SetupBitmapInfo(uchar *bmpinfo);
	void SetupColorArray(uchar *colorarray);
	void SetupDecode();
		
	static void DecodeNotSupported(BMPContent *tc);

	static void Decode4Bit_Tar16Bit(BMPContent *tc);
	static void Decode8Bit_Tar16Bit(BMPContent *tc);
	static void Decode24Bit_Tar16Bit(BMPContent *tc);
	
	static void Decode4Bit_Tar24Bit(BMPContent *tc);
	static void Decode8Bit_Tar24Bit(BMPContent *tc);
	static void Decode24Bit_Tar24Bit(BMPContent *tc);
};


/*********************************************************************
 *   BMPContentInstance
 *
 */
class BMPContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "image/bmp");
		into->AddString(S_CONTENT_EXTENSIONS, "bmp");
	}
	
	virtual Content* CreateContent(void* handle,
								   const char* mime,
								   const char* extension)
	{
		(void)mime;
		(void)extension;
		return new BMPContent(handle);
	}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...)
{
	if( n == 0 ) return new BMPContentFactory;
	return 0;
}

