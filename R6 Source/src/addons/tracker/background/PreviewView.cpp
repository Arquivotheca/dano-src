//*****************************************************************************
//
//	File:		 PreviewView.cpp
//
//	Description: Preview view class for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************


#include "PreviewView.h"
#include "Scale.h"
#include <Application.h>
#include <Bitmap.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <BitmapStream.h>
#include <Beep.h>
#include <Debug.h>
#include <File.h>
#include <Mime.h>
#include <Window.h>
#include <string.h>

#include <stdio.h>


const uint32 kGoodLoad = 'load';
const unsigned char kGrabberHand[] = {
16,1,4,0,
0x00, 0x00, 0x07, 0x80, 0x3d, 0x70, 0x25, 0x28, 0x24, 0xa8, 0x12, 0x94, 0x12, 0x54,
0x09, 0x2a, 0x08, 0x01, 0x3c, 0x01, 0x4c, 0x01, 0x42, 0x01, 0x30, 0x01, 0x0c, 0x01,
0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x80, 0x3f, 0xf0, 0x3f, 0xf8, 0x3f, 0xf8,
0x1f, 0xfc, 0x1f, 0xfc, 0x0f, 0xfe, 0x0f, 0xff, 0x3f, 0xff, 0x7f, 0xff, 0x7f, 0xff,
0x3f, 0xff, 0x0f, 0xff, 0x03, 0xfe, 0x01, 0xf8
};

int32 Loader(void *data);

PreviewView::PreviewView(BRect frame, const char *name, float ratio, BMessage *badload, uint32 resizingMode, uint32 flags)
 : BView(frame, name, resizingMode, flags), BInvoker(badload, this), loader(B_ERROR),
	scale(ratio), bitmap(0), bigbitmap(0), offset(0, 0), mode(0),
	aspectconstrain(false), notificmessage(0)
{
}

PreviewView::~PreviewView()
{
	delete bitmap;
	delete bigbitmap;
	delete notificmessage;
}

void PreviewView::DetachedFromWindow()
{
	// interrupt image loader
	terminate = true;
	status_t	junk;
	wait_for_thread(loader, &junk);
	terminate = false;
}

bool PreviewView::LoadBitmap(const char *path, BBitmap **saveithere)
{
	bool ok = false;

	if(! Window())
		DEBUGGER("PreviewView must be attached to looper before calling LoadBitmap");

	BFile *file = new BFile(path, O_RDONLY);
	if(file->InitCheck() == B_OK)
	{
		BTranslatorRoster *roster = BTranslatorRoster::Default();
		const char		*mime = 0;
		status_t		err = B_ERROR;
		char			str[B_MIME_TYPE_LENGTH + 30];

		if(B_OK <= file->ReadAttr("BEOS:TYPE", B_STRING_TYPE, 0, str, B_FILE_NAME_LENGTH))
			mime = str;

		// First try to identify the data using the hint, if any
		if(mime)
			err = roster->Identify(file, 0, &info, 0, mime);

		// If not identified, try without a hint
		if(err)
			err = roster->Identify(file, 0, &info);

		if(err == B_OK && info.group == B_TRANSLATOR_BITMAP)
		{
			// interrupt previous image loader
			terminate = true;
			status_t	junk;
			wait_for_thread(loader, &junk);
			terminate = false;

			bigrect = Bounds();
			loadfile = file;
			savelocation = saveithere;

			// start loader
			if((loader = spawn_thread(LoaderStub, "ImgLoader", B_LOW_PRIORITY, (void *)this)) >= B_OK)
			{
				resume_thread(loader);
				ok = true;
			}
			else
				beep();
		}
	}

	if(! ok)
		delete file;

	return ok;
}

int32 PreviewView::Loader()
{
	BTranslatorRoster	*roster = BTranslatorRoster::Default();
	BBitmapStream		bms;
	BMessage			msg(-1);
	BBitmap				*newbitmap = 0;

	// load bitmap
	if(roster->Translate(loadfile, &info, &msg, &bms, B_TRANSLATOR_BITMAP) != B_OK ||
		bms.DetachBitmap(&newbitmap) != B_OK)
		newbitmap = 0;

	delete loadfile;

	if(terminate)
	{
		delete newbitmap;
		return 0;
	}

	if(newbitmap)
	{
		if(notificmessage)
		{
			BMessage	notification(*notificmessage);
			char		desc[B_MIME_TYPE_LENGTH + 1];
			*desc = 0;
			BMimeType(info.MIME).GetShortDescription(desc);
			notification.AddString("type", desc);
			notification.AddInt32("sizex", newbitmap->Bounds().IntegerWidth() + 1);
			notification.AddInt32("sizey", newbitmap->Bounds().IntegerHeight() + 1);
			notificmessenger.SendMessage(&notification);
		}

		BMessenger me(this);
		float w;
		float h;
	
		// scale bitmap for stretched mode
		w = ceil(bigrect.Width());
		h = ceil(bigrect.Height());
		if(aspectconstrain)
		{
			float	nh = newbitmap->Bounds().Height();	
			float	nw = newbitmap->Bounds().Width();	
			if(nh < nw)
				h = w * nh / nw;
			else
				w = h * nw / nh;
		}
		BBitmap *big = new BBitmap(BRect(0, 0, w, h), newbitmap->ColorSpace());
		ddascale32(newbitmap, big, w / newbitmap->Bounds().Width(), h / newbitmap->Bounds().Height(), &terminate);

		// scale bitmap for normal modes
		w = ceil(newbitmap->Bounds().Width() * scale);
		h = ceil(newbitmap->Bounds().Height() * scale);
		BBitmap *norm = new BBitmap(BRect(0, 0, w, h), newbitmap->ColorSpace());
		ddascale32(newbitmap, norm, w / newbitmap->Bounds().Width(), h / newbitmap->Bounds().Height(), &terminate);

		// send bitmaps
		BMessage result(kGoodLoad);
		result.AddPointer("normal", norm);
		result.AddPointer("big", big);
		if(terminate || me.SendMessage(&result) != B_OK)
		{
			delete big;
			delete norm;
		}

		// delete original image, if not needed
		if(savelocation)
		{
			if(*savelocation)
				delete *savelocation;
			*savelocation = newbitmap;
		}
		else
			delete newbitmap;
	}
	else
	{
		BMessage	copy(*Message());
		Invoke(&copy);	// error condition
	}

	return 0;
}

void PreviewView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case kGoodLoad :
			{
				BBitmap *bmp = 0;
				BBitmap *bigbmp = 0;
				if(msg->FindPointer("normal", (void **)&bmp) == B_OK && bmp &&
					msg->FindPointer("big", (void **)&bigbmp) == B_OK && bigbmp)
				{
					delete bitmap;
					delete bigbitmap;
					bitmap = bmp;
					bigbitmap = bigbmp;
					Redisplay();
				}
			}
			break;

		default :
			BView::MessageReceived(msg);
			break;
	}
}

void PreviewView::Redisplay()
{
	if(bitmap && bigbitmap)
	{
		BBitmap	*paint = bitmap;
		BRect	src = paint->Bounds();
		BRect	dest = src;
		uint32	options = 0;

		switch(mode)
		{
			case 0 :	// at offset
				dest.OffsetBy(offset.x * scale, offset.y * scale);
				break;

			case 1 :	// center
				dest.OffsetBy((Bounds().Width() - src.Width()) / 2, (Bounds().Height() - src.Height()) / 2);
				break;

			case 2 :	// scaletofit
				paint = bigbitmap;
				src = bigbitmap->Bounds();
				if(aspectconstrain)
				{
					dest = src;
					dest.OffsetBy((Bounds().Width() - src.Width()) / 2, (Bounds().Height() - src.Height()) / 2);
				}
				else
					dest = Bounds();
				break;

			case 3 :	// tile
				options = B_TILE_BITMAP;
				dest.OffsetBy((Bounds().Width() - src.Width()) / 2, (Bounds().Height() - src.Height()) / 2);
				break;
		}

		SetViewBitmap(paint, src, dest, B_FOLLOW_TOP | B_FOLLOW_LEFT, options);
		Invalidate();

		BPoint	mouse;
		uint32	buttons;
		GetMouse(&mouse, &buttons);
		if(mode == 0 && buttons == 0 && Bounds().Contains(mouse))
			be_app->SetCursor(kGrabberHand);
	}
}

void PreviewView::ClearBitmap()
{
	// interrupt previous image loader
	terminate = true;
	status_t	junk;
	wait_for_thread(loader, &junk);
	terminate = false;

	delete bitmap;
	delete bigbitmap;
	bitmap = 0;
	bigbitmap = 0;
	ClearViewBitmap();	
	Invalidate();

	BPoint	mouse;
	uint32	buttons;
	GetMouse(&mouse, &buttons);
	if(Bounds().Contains(mouse))
		be_app->SetCursor(B_HAND_CURSOR);
}

void PreviewView::MouseMoved(BPoint /*point*/, uint32 transit, const BMessage *message)
{
	switch(transit)
	{
		case B_ENTERED_VIEW :
			if(bitmap && bigbitmap && message == 0 && mode == 0)
				be_app->SetCursor(kGrabberHand);
			break;

		case B_EXITED_VIEW :
			be_app->SetCursor(B_HAND_CURSOR);
			break;
	}
}

void PreviewView::MouseDown(BPoint p)
{
	if(bitmap && bigbitmap && mode == 0)
	{
		BBitmap	*paint = bitmap;
		BRect	restriction = Bounds();
		BRect	src = paint->Bounds();
		BRect	dest;
		uint32	buttons = 0;
		BPoint	lastp = BPoint(-1, -1);
		BPoint	delta = p - BPoint(offset.x * scale, offset.y * scale);

		// correctly place restriction
		restriction = restriction | BRect(-src.right, -src.bottom, src.left, src.top);
		restriction.OffsetBy(delta);

		Window()->CurrentMessage()->FindInt32("buttons", (int32 *)&buttons);
		while(buttons)
		{
			snooze(20 * 1000);
			GetMouse(&p, &buttons, true);

			// constrain image movement
			if(p.x < restriction.left) p.x = restriction.left;
			if(p.x > restriction.right) p.x = restriction.right;
			if(p.y < restriction.top) p.y = restriction.top;
			if(p.y > restriction.bottom) p.y = restriction.bottom;

			if(p != lastp)
			{
				// coordinates are converted and reconverted because
				// offset is in the real image coordinate system
				offset = BPoint((p.x - delta.x) / scale, (p.y - delta.y) / scale);
				dest = src;
				dest.OffsetBy(offset.x * scale, offset.y * scale);
				SetViewBitmap(paint, src, dest, B_FOLLOW_TOP | B_FOLLOW_LEFT, 0);
				Invalidate();
				Move(offset);
				lastp = p;
			}
		}

		GetMouse(&p, &buttons, true);
		if(! Bounds().Contains(p))
			be_app->SetCursor(B_HAND_CURSOR);
	}
}

// emtpy stub for callback method
void PreviewView::Move(BPoint /*p*/)
{
}

void PreviewView::SetAspectConstrain(bool constrain)
{
	aspectconstrain = constrain;
}

void PreviewView::SetLoadNotification(BMessenger m, BMessage *notification)
{
	notificmessenger = m;
	notificmessage = notification;
}

void PreviewView::SetScale(float ratio)
{
	scale = ratio;
}
