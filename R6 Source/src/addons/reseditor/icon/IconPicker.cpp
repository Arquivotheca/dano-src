#include "IconPicker.h"
#include "IconEditor.h"
#include "BitmapEditor.h"
#include "utils.h"

#include <Region.h>
#include <Autolock.h>
#include <Debug.h>
#include <Bitmap.h>

#include <Directory.h>
#include <File.h>
#include <NodeInfo.h>
#include <DataIO.h>

#include <BitmapStream.h>
#include <TranslatorRoster.h>
#include <TranslatorFormats.h>

#include <math.h>

static BLocker typesLock;
static bool typesMade = false;
static BMessage types;

static const BMessage& format_types()
{
	if( typesMade ) return types;
	
	BAutolock l(&typesLock);
	if( typesMade ) return types;
	
	typesMade = true;
	
	/* for convenience of coding, cache the default list */
	BTranslatorRoster *r = BTranslatorRoster::Default();

	/* we're going to ask each and every translator */
	translator_id * trans;
	int32 count;
	if (r->GetAllTranslators(&trans, &count)) return types;

	/* find out which of them can export for us */
	for (int phase=0; phase<2; phase++)
	{
		// 'phase' is used to put the PNG format first, since
		// that is our preferred format.
		for (int ix=0; ix<count; ix++)
		{
			/* they have to have B_TRANSLATOR_BITMAP among inputs */
			const translation_format * formats;
			int32 nInput;
			if (r->GetInputFormats(trans[ix], &formats, &nInput))
			{
				continue;
			}
			
			int iy=0;
			for (; iy<nInput; iy++)
			{
				if (formats[iy].type == B_TRANSLATOR_BITMAP) break;
			}
			if (iy >= nInput) continue; /* didn't have translator bitmap */
	
			/* figure out what the Translator can write */
			int32 nOutput;
			if (r->GetOutputFormats(trans[ix], &formats, &nOutput))
			{
				continue;
			}
			/* add everything besides B_TRANSLATOR_BITMAP to outputs */
			for (int iy=0; iy<nOutput; iy++)
			{
				if (formats[iy].type != B_TRANSLATOR_BITMAP)
				{
					bool ispng = strcmp(formats[iy].MIME, "image/png") == 0
								|| strcmp(formats[iy].MIME, "image/x-png") == 0;
					if ((ispng && phase==0) || (!ispng && phase==1)) {
						types.AddString("be:types", formats[iy].MIME);
						types.AddString("be:type_descriptions", formats[iy].name);
						types.AddInt32("be:_format", formats[iy].type);
						types.AddInt32("be:_translator", trans[ix]);
					}
				}
			}
		}
	}

	/* done with list of installed translators */
	delete[] trans;

	return types;
}

static void add_dnd_types(BMessage* intoMessage)
{
	const BMessage& types = format_types();
	const char * type;
	bool first = true;
	for (int ix=0; !types.FindString("be:types", ix, &type); ix++) {
		const char * name = "";
		types.FindString("be:type_descriptions", ix, &name);
		if (first)
			intoMessage->AddString("be:types", B_FILE_MIME_TYPE);

		intoMessage->AddString("be:types", type);
		intoMessage->AddString("be:filetypes", type);
		intoMessage->AddString("be:type_descriptions", name);
	}
}

static bool match_type(
	const char * mime_type,
	int32 * o_format,
	int32 * o_translator,
	const char *type_name)
{
	const BMessage& types = format_types();
	
	const char * str = NULL;
	for (int ix=0; !types.FindString("be:types", ix, &str); ix++) {
		const char *name = 0;
		types.FindString("be:type_descriptions", ix, &name);
		if (!strcasecmp(str, mime_type) &&
				(!type_name || !*type_name || !strcmp(name, type_name))) {
			// match by mime type and, if specified, by specific type name
			// (this provides a way to distinguish between say two image/gif translators
			if (!types.FindInt32("be:_format", ix, o_format) && 
				!types.FindInt32("be:_translator", ix, o_translator)) {
				return true;
			}
		}
	}
	return false;
}

static void copy_action(BMessage* request, const BBitmap* source)
{
	BBitmap* const_hack = const_cast<BBitmap*>(source);
	
	BBitmapStream strm(const_hack);
	bool handled = false;
	
	//	someone accepted our drag and requested one of the two
	//	types of data we can provide (in-message or in-file bitmap)
	const char * type = NULL;
	for (int32 i=0; !handled && !request->FindString("be:types", &type); i++) {
		const char * type_name = "";
		request->FindString("be:type_descriptions", i, &type_name);
		if (strcasecmp(type, B_FILE_MIME_TYPE) == 0) {
			const char * name;
			entry_ref dir;
			if (!request->FindString("be:filetypes", i, &type) &&
					!request->FindString("name", i, &name) &&
					!request->FindRef("directory", i, &dir)) {
				
				// get translator info
				int32 format, translator;
				if (match_type(type, &format, &translator, type_name)) {
					//	write file
					BDirectory d(&dir);
					BFile f(&d, name, O_RDWR | O_TRUNC);
					BTranslatorRoster::Default()->Translate(translator,
							&strm, NULL, &f, format);
					BNodeInfo ni(&f);
					ni.SetType(type);
					handled = true;
				}
			}
		}
		else {
			// get translator info
			int32 format, translator;
			if (match_type(type, &format, &translator, type_name)) {
				//	put in message
				BMessage msg(B_MIME_DATA);
				BMallocIO f;
				BTranslatorRoster::Default()->Translate(translator,
						&strm, NULL, &f, format);
				msg.AddData(type, B_MIME_TYPE, f.Buffer(), f.BufferLength());
				request->SendReply(&msg);
				handled = true;
			}
		}
	}
	
	strm.DetachBitmap(&const_hack);
}

TIconView::TIconView(BRect frame, const char* name, const char *mimeType,
					 bool hilite)
	: BView( frame, name, B_FOLLOW_NONE, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE)
{
	fShowing = 0;
	fHilite = hilite;
	fIconMimeType = mimeType;
	SetViewColor(B_TRANSPARENT_COLOR);
}


TIconView::~TIconView()
{
}

void
TIconView::StartShowing(TBitmapEditor* editor)
{
	if( Window() && fShowing ) StopWatchingAll(BMessenger(fShowing));
	fShowing = editor;
	fViewArea = BRect();
	Invalidate();
	if( fShowing && Window() ) StartWatchingAll(BMessenger(fShowing));
}

void
TIconView::SetView(BRect area)
{
	fViewArea = area;
	Invalidate();
}

void 
TIconView::Draw(BRect r)
{
	if( !fShowing ) {
		SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		FillRect(r);
	}
	
	BRect area(fViewArea);
	BRect bounds(Bounds());
	if( !area.IsValid() ) area = fShowing->Bounds();
	else {
		float wdiff = area.Width() - bounds.Width();
		float hdiff = area.Height() - bounds.Height();
		area.InsetBy(floor(wdiff/2+.5), floor(hdiff/2+.5));
	}
	if( area.left < 0 ) area.left = 0;
	if( area.right > fShowing->Bounds().right ) {
		area.right = fShowing->Bounds().right;
	}
	if( area.top < 0 ) area.top = 0;
	if( area.bottom > fShowing->Bounds().bottom ) {
		area.bottom = fShowing->Bounds().bottom;
	}
	
	BPoint off( floor((bounds.Width()-area.Width())/2),
				floor((bounds.Height()-area.Height())/2) );
	BRect dest(area);
	dest.OffsetTo(off);
	
	#if DEBUG
	PRINT(("Drawing icon: area=")); area.PrintToStream();
	PRINT(("Bounds=")); off.PrintToStream();
	PRINT(("Offset=")); off.PrintToStream();
	PRINT(("Destination=")); dest.PrintToStream();
	#endif
	
	if( !fHilite ) {
		DrawBitmap(fShowing->ShownBitmap(), area, dest);
	} else {
		fShowing->GetHilite(this, off, area);
	}
	
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	if( dest.left > bounds.left ) {
		FillRect(BRect(bounds.left, bounds.top, dest.left-1, bounds.bottom));
	}
	if( dest.right < bounds.right ) {
		FillRect(BRect(dest.right+1, bounds.top, bounds.right, bounds.bottom));
	}
	if( dest.top > bounds.top ) {
		FillRect(BRect(bounds.left, bounds.top, bounds.right, dest.top-1));
	}
	if( dest.bottom < bounds.bottom ) {
		FillRect(BRect(bounds.left, dest.bottom+1, bounds.right, bounds.bottom));
	}
}

void
TIconView::AllAttached()
{
	if( fShowing ) StartWatchingAll(BMessenger(fShowing));
}

void 
TIconView::MessageReceived(BMessage *msg)
{
	bool handled = false;
	
	if (msg->WasDropped()) {
		// distinguish the target with the icon view name
		msg->AddString("drop", Name());
		BView::MessageReceived(msg);
		return;
	}
	
	switch(msg->what) {
		case B_COPY_TARGET: {
			if (fShowing && fShowing->RealBitmap()) {
				copy_action(msg, fShowing->RealBitmap());
			}
		} break;
		
		case B_OBSERVER_NOTICE_CHANGE: {
			#if DEBUG
			PRINT(("Received notification: ")); msg->PrintToStream();
			#endif
			uint32 what = 0;
			msg->FindInt32(B_OBSERVE_WHAT_CHANGE, (int32*)&what);
			switch( what ) {
				case T_BITMAP_INVALIDATED: {
					BRect region;
					if( msg->FindRect("be:region", &region) != B_OK ) {
						region = Bounds();
					}
					if( region.IsValid() ) {
						Invalidate();
					}
					handled = true;
				} break;
				case T_ICON_VIEW_MOVED: {
					msg->FindRect("be:region", &fViewArea);
					Invalidate();
				} break;
			}
		}
	}
	
	if( !handled ) BView::MessageReceived(msg);
}

void 
TIconView::MouseDown(BPoint pt)
{
	BView::MouseDown(pt);

	bool doingDrag=false;
	if (Window()->Lock()) {
		BPoint 		where = pt;
		BMessage 	dragData;
		ulong		buttons;

		// name from picker is large or mini
		dragData.AddString("icon", Name());

		do {
			if (fabs(where.x - pt.x) > 3.0
				|| fabs(where.y - pt.y) > 3.0) {
				
				((TIconPicker*)Parent())->BuildDragData(&dragData,
					strcmp("mini",Name()) == 0 ? kDragSourceMini : kDragSourceLarge);
				
				if (!Frame().Contains(where)) {
					add_dnd_types(&dragData);
					dragData.AddInt32("be:actions", B_COPY_TARGET);
					dragData.AddString("be:clip_name", "Icon Clip");
					
					const BBitmap* show = fShowing ? fShowing->RealBitmap() : 0;
					
					/* We throttle size of dragged bitmap */
					if ((!show) ||
							(show->Bounds().Width() < 1) ||
							(show->Bounds().Height() < 1) ||
							(show->Bounds().Height()*show->BytesPerRow() > 100000)) {
						DragMessage(&dragData, Bounds(), this);
					} else {
						BBitmap* copy = new BBitmap(show);
						DragMessage(&dragData, copy, B_OP_BLEND, pt, this);
					}
					doingDrag = true;
				}
			}
			snooze(50000);
			GetMouse(&where, &buttons);
		} while (!doingDrag && buttons);

		Window()->Unlock();
	}
}

//
//

enum {
	MIN_SPACE = 4
};

TIconPicker::TIconPicker(BRect frame)
	: BControl(frame, "", "icon picker", NULL, B_FOLLOW_RIGHT,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	  fPrimaryBitmap(0), fSecondaryBitmap(0),
	  fLargeIcon(0), fLargeHiliteIcon(0),
	  fSmallIcon(0), fSmallHiliteIcon(0)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}

TIconPicker::~TIconPicker()
{
}

void
TIconPicker::StartShowing(TBitmapEditor* primary,
						  TBitmapEditor* secondary)
{
	fPrimaryBitmap = primary;
	fSecondaryBitmap = secondary;
	AddParts(Bounds().Width(), Bounds().Height());
	Invalidate();
}

void
TIconPicker::AttachedToWindow()
{
	BControl::AttachedToWindow();
	
	if( Parent() ) SetLowColor(Parent()->ViewColor());
	else SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	AddParts(Bounds().Width(), Bounds().Height());
	Invalidate();
}

void
TIconPicker::GetPreferredSize(float *width, float *height)
{
	float pw = fPrimaryBitmap ? fPrimaryBitmap->Width() : 0;
	if( pw > 32 ) pw = 32;
	float ph = fPrimaryBitmap ? fPrimaryBitmap->Height() : 0;
	if( ph > 32 ) ph = 32;
	float sw = fSecondaryBitmap ? fSecondaryBitmap->Width() : 0;
	if( sw > 16 ) sw = 16;
	float sh = fSecondaryBitmap ? fSecondaryBitmap->Height() : 0;
	if( sh > 16 ) sh = 16;
	
	if( width ) *width = MIN_SPACE*2 + pw + sw + (pw>0&&sw>0 ? MIN_SPACE:0);
	if( height ) *height = MIN_SPACE*3 + (ph>sh?ph:sh)*2;
}

void
TIconPicker::Draw(BRect)
{
	PushState();
	
	BRegion region(Bounds().InsetBySelf(2, 2));
	if( fLargeIcon ) region.Exclude(fLargeIcon->Frame().InsetBySelf(-2, -2));
	if( fLargeHiliteIcon ) region.Exclude(fLargeHiliteIcon->Frame().InsetBySelf(-2, -2));
	if( fSmallIcon ) region.Exclude(fSmallIcon->Frame().InsetBySelf(-2, -2));
	if( fSmallHiliteIcon ) region.Exclude(fSmallHiliteIcon->Frame().InsetBySelf(-2, -2));
	
	ConstrainClippingRegion(&region);
	FillRect(Bounds(), B_SOLID_LOW);
	ConstrainClippingRegion(0);

	DrawFancyBorder(this);

	if( fLargeIcon ) AddBevel(this, fLargeIcon->Frame(), true);
	if( fLargeHiliteIcon ) AddBevel(this, fLargeHiliteIcon->Frame(), true);
	if( fSmallIcon ) AddBevel(this, fSmallIcon->Frame(), true);
	if( fSmallHiliteIcon ) AddBevel(this, fSmallHiliteIcon->Frame(), true);

	if (Window()->IsActive()) {
		// focus mark for picker
		DrawFocusMark(IsFocus());
	} else {
		// focus mark for picker - clear
		DrawFocusMark(false);
	}
	
	PopState();
}

void
TIconPicker::FrameResized(float width, float height)
{
	AddParts(width, height);
	Invalidate();
}

void
TIconPicker::DrawFocusMark(bool state)
{
	BRect b(Bounds());
	b.InsetBy(3,3);
	if (state)
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	else
		SetHighColor(ViewColor());
	StrokeRect(b);
}

void 
TIconPicker::KeyDown(const char *bytes, int32 n)
{
	if (!IsFocus())
		return;
		
	switch (bytes[0]) {
		case B_DOWN_ARROW:
		case B_LEFT_ARROW:
		case B_UP_ARROW:
		case B_RIGHT_ARROW:
			//SwapSelection();
			break;
		case B_ENTER:
		case B_SPACE:
			break;
		default:
			BControl::KeyDown(bytes, n);
			break;
	}
}

void 
TIconPicker::MouseDown(BPoint pt)
{
	BView::MouseDown(pt);

	if (Window()->Lock()) {
		BPoint 		where = pt;
		BMessage 	dragData(B_SIMPLE_DATA);
		ulong		buttons;
		float		x, y;
		bool		doingDrag = false;

		do {
			x = fabs(where.x - pt.x);
			y = fabs(where.y - pt.y);
			if ((x > 3.0) || (y > 3.0)) {
				
				BuildDragData(&dragData);
				
				if (!Frame().Contains(where)) {
					DragMessage(&dragData, Bounds());
					doingDrag = true;
				}
			}
			snooze(50000);
			GetMouse(&where, &buttons);
		} while (!doingDrag && buttons);

		Window()->Unlock();
	}
}

status_t
TIconPicker::Invoke(BMessage*)
{
	return BControl::Invoke();
}

void
TIconPicker::BuildDragData(BMessage* dragMsg, DragSource source)
{
	if( (source&kDragSourceLarge) && fPrimaryBitmap ) {
		// make sure any active selection is applied and finished.
		fPrimaryBitmap->DeSelect();
		BMessage msgPart;
		const BBitmap* icon = fPrimaryBitmap->RealBitmap();
		if( icon ) {
			icon->Archive(&msgPart,false);
			dragMsg->AddMessage(kLargeIconMimeType, &msgPart);
		}
	}
	
	if( (source&kDragSourceMini) && fSecondaryBitmap ) {
		// make sure any active selection is applied and finished.
		fSecondaryBitmap->DeSelect();
		BMessage msgPart;
		const BBitmap* icon = fSecondaryBitmap->RealBitmap();
		if( icon ) {
			icon->Archive(&msgPart,false);
			dragMsg->AddMessage(kMiniIconMimeType, &msgPart);
		}
	}
}

TIconView*
TIconPicker::LargeIcon()
{
	return fLargeIcon;
}

TIconView*
TIconPicker::SmallIcon()
{
	return fSmallIcon;
}

TIconView*
TIconPicker::LargeHiliteIcon()
{
	return fLargeHiliteIcon;
}

TIconView*
TIconPicker::SmallHiliteIcon()
{
	return fSmallHiliteIcon;
}

void
TIconPicker::AddParts(float width, float height)
{
	if( fPrimaryBitmap ) {
		if( !fLargeIcon ) {
			fLargeIcon = new TIconView(BRect(-10, -10, -5, -5), "large", kLargeIconMimeType);
			AddChild(fLargeIcon);
		}
		fLargeIcon->StartShowing(fPrimaryBitmap);
		if( !fLargeHiliteIcon ) {
			fLargeHiliteIcon = new TIconView(BRect(-10, -10, -5, -5), "large", kLargeIconMimeType, true);
			AddChild(fLargeHiliteIcon);
		}
		fLargeHiliteIcon->StartShowing(fPrimaryBitmap);
	} else {
		if( fLargeIcon ) {
			fLargeIcon->RemoveSelf();
			delete fLargeIcon;
			fLargeIcon = 0;
		}
		if( fLargeHiliteIcon ) {
			fLargeHiliteIcon->RemoveSelf();
			delete fLargeHiliteIcon;
			fLargeHiliteIcon = 0;
		}
	}
	
	if( fSecondaryBitmap ) {
		if( !fSmallIcon ) {
			fSmallIcon = new TIconView(BRect(-10, -10, -5, -5), "mini", kMiniIconMimeType);
			AddChild(fSmallIcon);
		}
		fSmallIcon->StartShowing(fSecondaryBitmap);
		if( !fSmallHiliteIcon ) {
			fSmallHiliteIcon = new TIconView(BRect(-10, -10, -5, -5), "mini", kMiniIconMimeType, true);
			AddChild(fSmallHiliteIcon);
		}
		fSmallHiliteIcon->StartShowing(fSecondaryBitmap);
	} else {
		if( fSmallIcon ) {
			fSmallIcon->RemoveSelf();
			delete fSmallIcon;
			fSmallIcon = 0;
		}
		if( fSmallHiliteIcon ) {
			fSmallHiliteIcon->RemoveSelf();
			delete fSmallHiliteIcon;
			fSmallHiliteIcon = 0;
		}
	}
	
	if( fPrimaryBitmap && fSecondaryBitmap ) {
		float extraw = (width+1-fPrimaryBitmap->Width()-fSecondaryBitmap->Width())/3;
		float compressw = 0;
		if( extraw < MIN_SPACE ) {
			compressw = MIN_SPACE*3 - extraw*3;
			extraw = MIN_SPACE;
		}
		float extrah = fPrimaryBitmap->Height();
		if( extrah < fSecondaryBitmap->Height() ) extrah = fSecondaryBitmap->Height();
		extrah = (height+1-(extrah*2))/3;
		float compressh = 0;
		if( extrah < MIN_SPACE ) {
			compressh = MIN_SPACE*3 - extrah*3;
			extrah = MIN_SPACE;
		}
		
		float pleft = extraw;
		float pright = pleft+fPrimaryBitmap->Width()-1
					 - (compressw*fPrimaryBitmap->Width())
						/(fPrimaryBitmap->Width()+fSecondaryBitmap->Width());
		float sleft = pright+extraw;
		float sright = width-extraw;
		
		float ntop = extrah;
		float nbottom = ntop+fPrimaryBitmap->Height()-1 - compressh/2;
		float htop = nbottom+extrah;
		float hbottom = height-extrah;
		
		fLargeIcon->MoveTo(floor(pleft), floor(ntop));
		fLargeIcon->ResizeTo(floor(pright-pleft), floor(nbottom-ntop));
		fSmallIcon->MoveTo(floor(sleft), floor(ntop));
		fSmallIcon->ResizeTo(floor(sright-sleft), floor(nbottom-ntop));
		fLargeHiliteIcon->MoveTo(floor(pleft), floor(htop));
		fLargeHiliteIcon->ResizeTo(floor(pright-pleft), floor(hbottom-htop));
		fSmallHiliteIcon->MoveTo(floor(sleft), floor(htop));
		fSmallHiliteIcon->ResizeTo(floor(sright-sleft), floor(hbottom-htop));
		
	} else {
		TBitmapEditor* editor = 0;
		BView* normal = 0;
		BView* hilite = 0;
		if( fPrimaryBitmap ) {
			editor = fPrimaryBitmap;
			normal = fLargeIcon;
			hilite = fLargeHiliteIcon;
		} else if( fSecondaryBitmap ) {
			editor = fSecondaryBitmap;
			normal = fSmallIcon;
			hilite = fSmallHiliteIcon;
		}		
		
		if( editor ) {
			float vspace = - (width-editor->Width()) + (height-editor->Height()*2);
			float hspace = (width-editor->Width()*2) - (height-editor->Height());
			if( vspace > hspace ) {
				float extraw = (width+1-editor->Width())/2;
				if( extraw < MIN_SPACE ) extraw = MIN_SPACE;
				float extrah = (height+1-editor->Height()*2)/3;
				if( extrah < MIN_SPACE ) extrah = MIN_SPACE;
				normal->MoveTo(floor(extraw), floor(extrah));
				normal->ResizeTo(floor(width-extraw*2), floor((height-extrah*3)/2));
				hilite->MoveTo(floor(extraw), floor(height/2+extrah/2));
				hilite->ResizeTo(floor(width-extraw*2), floor((height-extrah*3)/2));
			} else {
				float extraw = (width+1-editor->Width()*2)/3;
				if( extraw < MIN_SPACE ) extraw = MIN_SPACE;
				float extrah = (height+1-editor->Height())/2;
				if( extrah < MIN_SPACE ) extrah = MIN_SPACE;
				normal->MoveTo(floor(extraw), floor(extrah));
				normal->ResizeTo(floor((width-extraw*3)/2), floor(height-extrah*2));
				hilite->MoveTo(floor(width/2+extraw/2), floor(extrah));
				hilite->ResizeTo(floor((width-extraw*3)/2), floor(height-extrah*2));
			}
		}
	}
}
