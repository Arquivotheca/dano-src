#include <Be.h>
// SplashView.cpp

#include "SplashWindow.h"
#include "PackWindow.h"
#include "Util.h"

//#if (!__INTEL__)
//#include "DatatypesAddOn.h"
//#include <DataStreams.h>
//#include <BitmapStream.h>
//#else
#include <TranslationUtils.h>
//#endif

#include <Debug.h>

SplashWindow::SplashWindow(const char *title, BBitmap **splashBmap, PackWindow *parW)
	: 	ChildWindow(BRect(0,0,220,100),title,B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE, parW)
{
	Lock();
	//pw = (PackWindow *)parW;

	BBitmap *bmap = *splashBmap;
	if (bmap) {
		ResizeTo(bmap->Bounds().Width(),bmap->Bounds().Height());
		PositionWindow(this,0.5,0.4);
	}
	AddChild(new SplashView(Bounds(),splashBmap,parW));
		
	Show();
	Unlock();
}


/*********************************************************/

SplashView::SplashView(BRect r,BBitmap **_splash, PackWindow *_parW)
	:	BView(r,B_EMPTY_STRING,B_FOLLOW_ALL,B_WILL_DRAW),
		splashBitmap(_splash),
		parW(_parW)
{
}

void SplashView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetFont(be_plain_font);
	MakeFocus(TRUE);
}

SplashView::~SplashView()
{
	//if (bitmap)
	//	delete bitmap;
}

void 	SwapComponents(ulong *data, long size);

void SplashView::KeyDown(const char *bytes, int32 numBytes)
{
	switch(*bytes) {
		case B_DELETE:
		case B_BACKSPACE:
			if (*splashBitmap) {
				delete *splashBitmap;
				*splashBitmap = NULL;
				Window()->ResizeTo(220,100);
				Invalidate();
				parW->attribDirty = true;
			}
			break;
		default:
			BView::KeyDown(bytes,numBytes);
			break;
	}
}

void SplashView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case B_REFS_RECEIVED:
		case 'DATA': {
			long err;
			entry_ref		fileRef;
			
			msg->FindRef("refs",&fileRef);
			BFile file(&fileRef,O_RDONLY);
			err = file.InitCheck();
			if (err < B_OK) {
				doError("Could not open file. (%s)",err);
				break;
			}
			BBitmap *bitmap = NULL;
			
//#if __INTEL__
			bitmap = BTranslationUtils::GetBitmap(&file);
//#else
//			if (DATACallbacks.image < 0) {
//				doError("Could not translate the file because the Datatypes library was not installed.");
//				break;	
//			}
//			DATAInfo	outInfo;
//			DATACallbacks.DATAIdentify(file,NULL,outInfo);
//			if (outInfo.formatGroup == DATA_BITMAP) {
//				
//				PRINT(("format type %4s\n",&outInfo.formatType));
//				PRINT(("format group %4s\n",&outInfo.formatGroup));
//				PRINT(("format name %s\n",outInfo.formatName));
//				
//				BitmapStream	outStream;
//				
//				err = DATACallbacks.DATATranslate(file,&outInfo,NULL,outStream,DATA_BITMAP);
//				if (err == DATA_NO_HANDLER)
//					PRINT(("data no handler\n"));
//				else if (err == DATA_ILLEGAL_DATA)
//					PRINT(("data illegal data\n"));
//					
//				if (!err) {
//					outStream.DetachBitmap(bitmap);
//				}
//			}
//#endif
			if (bitmap) {
				if (*splashBitmap)
					delete *splashBitmap;
				*splashBitmap = bitmap;
				BRect oldR = Window()->Frame();
				BRect newR = bitmap->Bounds();
				Window()->ResizeTo(newR.Width(),newR.Height());
				Window()->MoveBy((oldR.Width() - newR.Width())/2.0, 
								 (oldR.Height() - newR.Height())/2.0);
				Invalidate();
				parW->attribDirty = true;
			}
			else {
				doError("Could not translate data. Please be sure the appropriate translator is installed.");
			}	
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


void SplashView::Draw(BRect up)
{
	if (*splashBitmap) {
		DrawBitmap(*splashBitmap,up,up);	
	}
	else {
		DrawString("Drag and drop the desired image file...",BPoint(10,40));
		DrawString("(splash screen only displayed when",BPoint(10,58));
		DrawString(" installed via SoftwareValet)",BPoint(10,72));
	}
}

// change bgra to abgr
void 	SwapComponents(ulong *data, long size)
{
	ulong	*p, *max;
	ulong	pixel;	
	long	siz = size/sizeof(pixel);
	
	for (p = data, max = data+siz; p < max; ++p) {
		
		pixel = *p;
		pixel = ((0xFFFFFF00 & pixel) >> 8) |
				((0x000000FF & pixel) << 24);
		*p = pixel;
	}
}

