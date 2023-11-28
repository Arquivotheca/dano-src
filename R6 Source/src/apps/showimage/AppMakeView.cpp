/*	AppMakeView.cpp
 */

#include "TranslationKit.h"
#include "AppMakeView.h"
#include <TextView.h>
#include <DataIO.h>
#include "BitmapView.h"
/* enable later stuff
#include "PictureView.h"
#include "MediaPlayerView.h"
 */
#include <string.h>
#include <Bitmap.h>
#include "prefix.h"



long
AppMakeView(
	BPositionIO *		inStream,
	BView * &		outView,
	BRect &			outExtent,
	const char *	hintMIME,
	BString &		outDataInfo,
	translator_id &	outTranslator,
	uint32 &		outFormat)
{
	outDataInfo = NULL;
	outView = NULL;
	long err = B_ERROR;
	translator_info info;

	/*	First try to identify the data using the hint, if any
	 */
	if (hintMIME)
	{
		err = DATA->Identify(inStream, NULL, &info, 0, hintMIME);
	}
	/*	If not identified, try without a hint
	 */
	if (err)
	{
		err = DATA->Identify(inStream, NULL, &info);
	}
	/*	Bail out if we can't find anything
	 */
	if (err)
		return err;

	outDataInfo = info.name;
	outTranslator = info.translator;
	outFormat = info.type;
	
	/*	Call through to helper functions for the type found
	 */
	switch (info.group)
	{
	case B_TRANSLATOR_TEXT:
		err = AppMakeTextView(outView, outExtent, *inStream, info);
		if (!err && outView)
			delete inStream;
		break;
	case B_TRANSLATOR_BITMAP:
		err = AppMakeBitmapView(outView, outExtent, *inStream, info);
		if (!err && outView)
			delete inStream;
		break;
/* enable later stuff 
	case DATA_PICTURE:
		err = AppMakePictureView(outView, outExtent, *inStream, info);
		if (!err && outView)
			delete inStream;
		break;
	case DATA_MEDIA:
		err = AppMakeMediaView(outView, outExtent, *inStream, info);
		break;	/*	saves stream for later use	*/
	default:
		err = B_ERROR;	/*	A format that's not text, bitmap or picture */
		break;
	}
	return err;
}


long
AppMakeTextView(
	BView * &		outView,
	BRect &			outExtent,
	BPositionIO &		stream,
	translator_info &		info)
{
	/*	Create view */
	outExtent.left = outExtent.top = 0;
	outExtent.right = 400;
	outExtent.bottom = 300;
	outView = new BTextView(outExtent, "text", outExtent,
			B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED);

	/*	Translate */
	return BTranslationUtils::GetStyledText(&stream, (BTextView *)outView);

#if 0
	/*	Translate */
	BMallocIO mall;
	long err = DATA->Translate(&stream, &info, NULL, &mall, B_TRANSLATOR_TEXT);
	if (err)
		return err;

	/*	Create view */
	outExtent.left = outExtent.top = 0;
	outExtent.right = 400;
	outExtent.bottom = 300;
	outView = new BTextView(outExtent, "text", outExtent,
			B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED);

	/*	Insert data */
	long size = mall.BufferLength();
	long offset = 0;
	const char *text = (const char *)mall.Buffer();
	((BTextView *)outView)->SetText(text+offset, size);

	/*	Declare success */
	return B_NO_ERROR;
#endif
}


long
AppMakeBitmapView(
	BView * &		outView,
	BRect &			outExtent,
	BPositionIO &		stream,
	translator_info &		info)
{
	BBitmapStream bms;
	BMessage msg(-1);
	long err = DATA->Translate(&stream, &info, &msg, &bms, B_TRANSLATOR_BITMAP);
	if (err)
		return err;

	/*	Get data */
	BBitmap *map = NULL;
	err = bms.DetachBitmap(&map);
	if (err)
		return err;

	/*	Create view */
	const char *comment = NULL;
	if (msg.FindString(B_TRANSLATOR_EXT_COMMENT, &comment) < B_OK)
		comment = NULL;
	outExtent = map->Bounds();
	BitmapView *view = new BitmapView(map, comment);
	outView = view;

	/*	Declare success */
	return B_NO_ERROR;
}


#if 0
/* enable later stuff */
long
AppMakePictureView(
	BView * &		outView,
	BRect &			outExtent,
	BPositionIO &		stream,
	translator_info &		info)
{
	BMallocIO mall;
	long err = DATATranslate(stream, &info, NULL, mall, DATA_PICTURE);
	if (err)
		return err;

	/*	Get data */
	long offset = 0;
	long size = mall.BufferLength();
	const void *data = mall.Buffer();
	BPicture *pic = new BPicture((char *)data+offset, size);

	/*	Create view */
	/*	outExtent = pic->Bounds();	//	Can't get bounds of picture!
	 *	so we just fake some
	 */
	outExtent.left = outExtent.top = 0;
	outExtent.right = 512;
	outExtent.bottom = 384;
	PictureView *view = new PictureView(outExtent, pic);
	outView = view;

	/*	Declare success */
	return B_NO_ERROR;
}



long
AppMakeMediaView(
	BView * &		outView,
	BRect &			outExtent,
	BPositionIO &		stream,
	translator_info &		info)
{
	outExtent.Set(0,0, 320,240);
	MediaPlayerView *view = new MediaPlayerView(outExtent, &stream, info);
	outView = view;
	return B_OK;
}

#endif
