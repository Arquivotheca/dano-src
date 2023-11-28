// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#include <BeBuild.h>
#include <SupportDefs.h>
#include <Errors.h>
#include <Message.h>
#include <Rect.h>
#include <Bitmap.h>
#include <GraphicsDefs.h>
#include <Screen.h>
//#include <TranslationUtils.h>

#include "BMPView.h"


BMPView::BMPView(	BRect frame,
					const char *name, 
					uint32 resizingMode,
					uint32 flags)
	: 	BView(frame, name, resizingMode, flags | B_WILL_DRAW),
		fBitmap(NULL)
{
	#ifdef B_BEOS_VERSION_DANO
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	#endif
	resourcehandler = GetLibbeResources();
}

BMPView::~BMPView(void)
{
	delete fBitmap;
}

void BMPView::AttachedToWindow(void)
{
	BView *p = Parent();
	if (p) {
		if (p->Flags() & B_DRAW_ON_CHILDREN) {
			SetViewColor(B_TRANSPARENT_32_BIT);
			SetDrawingMode(B_OP_ALPHA);
		} else {
			SetViewColor(p->ViewColor());
			SetHighColor(ViewColor());
			SetDrawingMode(B_OP_OVER);
		}
	} else {
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetHighColor(ViewColor());
		SetDrawingMode(B_OP_OVER);
	}
}

void BMPView::Draw(BRect frame)
{
	if (fBitmap != NULL)
	{
		const BRect b = Bounds();
		BPoint p(	(b.Width() - fBitmap->Bounds().Width())*0.5f,
					(b.Height() - fBitmap->Bounds().Height())*0.5f );
		DrawBitmap(fBitmap, p);
	}
}

status_t BMPView::SetBitmap(int32 id)
{
	status_t result = B_OK;
	delete fBitmap;
	fBitmap = NULL;

#if 0
	// Get the bitmap
	size_t len = 0;
	const void *data = resourcehandler->LoadResource('bits', id, &len);
	BMemoryIO stream(data, len);
	fBitmap = BTranslationUtils::GetBitmap(&stream);
#endif
	if (fBitmap == NULL)
	{ // try the other way.
		result = resourcehandler->GetBitmapResource('BBMP', id, &fBitmap);
		if (result != B_OK)
			return result;		
	}
	
	if (LockLooper())
	{ // redraw the bitmap
		Invalidate();
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}

	return result;
}

void BMPView::GetPreferredSize(float *x, float *y)
{
	if (fBitmap)
	{
		*x = fBitmap->Bounds().Width()+1.0f;
		*y = fBitmap->Bounds().Height()+1.0f;
	}
	else
		BView::GetPreferredSize(x, y);
}
