/*
	BmpUtils.cpp
	Implementation.
*/

#include <TranslatorRoster.h>
#include <BitmapStream.h>
#include <File.h>
#include <NodeInfo.h>
#include <View.h>
#include "BmpUtils.h"

BBitmap *LoadBitmap(const char *pathname)
{ // given a pathname, loads an image
	// return NULL on failure
	BBitmap				*bm = NULL;

	BFile				file(pathname, B_READ_ONLY);
	translator_info		info;
	char				str[B_MIME_TYPE_LENGTH+1];
	const char			*mimeStr = NULL;
	status_t			err = B_ERROR;
	BTranslatorRoster	*roster = BTranslatorRoster::Default();
	BBitmapStream		stream;
	ssize_t				length = file.ReadAttr("BEOS:TYPE", B_MIME_TYPE, 
								0, str, B_MIME_TYPE_LENGTH);
	if (length > 0)
		mimeStr = str;
	if (mimeStr)
		err = roster->Identify(&file, NULL, &info, 0, mimeStr, B_TRANSLATOR_BITMAP);
	if (err != B_NO_ERROR)
		err = roster->Identify(&file, NULL, &info, 0, NULL, B_TRANSLATOR_BITMAP);
	if (roster->Translate(&file, &info, NULL, &stream, B_TRANSLATOR_BITMAP, 0, mimeStr) != B_NO_ERROR)
		return NULL;
	stream.DetachBitmap(&bm);

	return bm;
}

void SetImageIcons(const char *pathname)
{ // given a pathname to an image, loads it and creates Tracker icons
	BBitmap		*img = LoadBitmap(pathname);

	if (img == NULL)
		return;

	BBitmap		*icon;
	BView		*view;
	BNode		node(pathname);
	BNodeInfo	ni(&node);
	BRect		r;
	float		aspect;

	icon = new BBitmap(BRect(0, 0, 31, 31), B_COLOR_8_BIT, true);
	view = new BView(icon->Bounds(), NULL, B_FOLLOW_ALL, 0);
	icon->AddChild(view);
	icon->Lock();
	view->SetHighColor(B_TRANSPARENT_32_BIT);
	r = icon->Bounds();
	view->FillRect(r);
	r = img->Bounds();
	aspect = r.Width() / r.Height();
	if (aspect > 1.0f)
	{
		r.left = 0; r.right = 31;
		r.top = 16 - 16 / aspect;
		r.bottom = 16 + 16 / aspect;
	}
	else
	{
		r.top = 0; r.bottom = 31;
		r.left = 16 - 16 / aspect;
		r.right = 16 + 16 / aspect;
	}
	view->DrawBitmap(img, r);
	view->Sync();
	icon->Unlock();
	ni.SetIcon(icon, B_LARGE_ICON);
	delete icon;

	icon = new BBitmap(BRect(0, 0, 15, 15), B_COLOR_8_BIT, true);
	view = new BView(icon->Bounds(), NULL, B_FOLLOW_ALL, 0);
	icon->AddChild(view);
	icon->Lock();
	view->SetHighColor(B_TRANSPARENT_32_BIT);
	r = icon->Bounds();
	view->FillRect(r);
	r = img->Bounds();
	aspect = r.Width() / r.Height();
	if (aspect > 1.0f)
	{
		r.left = 0; r.right = 15;
		r.top = 8 - 8 / aspect;
		r.bottom = 8 + 8 / aspect;
	}
	else
	{
		r.top = 0; r.bottom = 15;
		r.left = 8 - 8 / aspect;
		r.right = 8 + 8 / aspect;
	}
	view->DrawBitmap(img, r);
	view->Sync();
	icon->Unlock();
	ni.SetIcon(icon, B_MINI_ICON);
	delete icon;

	delete img;
}
