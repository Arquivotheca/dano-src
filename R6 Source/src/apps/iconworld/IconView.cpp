//--------------------------------------------------------------------
//	
//	IconView.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1994-97 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <AppFileInfo.h>
#include <Clipboard.h>
#include <Debug.h>
#include <Mime.h>
#include <OS.h>
#include <Screen.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <fs_attr.h>

#include "IconView.h"
#include "IconTools.h"
#include "IconColor.h"
#include "IconWindow.h"
#include "IconWorld.h"

const char *kLargeIconMimeType = "icon/large";
const char *kMiniIconMimeType = "icon/mini";

extern char		splash_icon32[], splash_icon16[];
extern uchar	cursor_marquee[], cursor_lasso[], cursor_pencil[],
				cursor_eraser[], cursor_dropper[], cursor_bucket[],
				cursor_shapes[];
extern pattern	marquee_pat[];

const BRect kLargeIconRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1);
const BRect kSmallIconRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1);

//====================================================================

TIconView::TIconView(BRect rect, entry_ref *ref)
		  :BView(rect, "", B_NOT_RESIZABLE, B_WILL_DRAW | B_PULSE_NEEDED)
{
  char			*bits;
  long			i;
  BRect			r;
  rgb_color		color;
  icon_info		*icon;
  
  fRef = ref;
  fFile = new BFile();
  fUndo32 = FALSE;
  fUndo32Marquee = FALSE;
  fUndo16 = FALSE;
  fUndo16Marquee = FALSE;
  fMarqueeFlag = FALSE;
  
  fDirty = FALSE;
  fTextView = NULL;
  
  fEditField = LARGEICON;
  fIconCount = 1;
  fSelectedIcon = 0;
  
  fPenSize = 1;
  fPenMode = M_MODE_COPY;
#ifdef APPI_EDIT
  fCreateAppi = FALSE;
#endif

#if 0
  fSignature = 'IWLD';
#endif
  fType = 'IRES';

  fIconList = new BList(1);
  
  SetFont(be_plain_font);
  SetFontSize(9);
  
  icon = (icon_info *)malloc(sizeof(icon_info));
  fIconList->AddItem(icon);
  icon->mimeSignature[0] = '\0';
  
  r.Set(0, 0, LARGEICON - 1, LARGEICON - 1);
  bits = splash_icon32;
  fHilite32Icon = new BBitmap(r, B_COLOR_8_BIT);
  fHilite32Icon->SetBits(bits, fHilite32Icon->BitsLength(), 0, B_COLOR_8_BIT);
  fUndo32Icon = new BBitmap(r, B_COLOR_8_BIT);
  fEdit32Icon = new BBitmap(r, B_COLOR_8_BIT);
  fDrag32Icon = new BBitmap(r, B_COLOR_8_BIT);
  fUndo32Drag = new BBitmap(r, B_COLOR_8_BIT);
  fOffScreen32 = new BBitmap(r, B_COLOR_8_BIT, TRUE);
  fOffScreen32->AddChild(fOffView32 = new BView(r, "", B_FOLLOW_ALL, B_WILL_DRAW));
  fOffScreen32->SetBits(fEdit32Icon->Bits(), fEdit32Icon->BitsLength(), 0,
			B_COLOR_8_BIT);
  
  icon->icon32 = new BBitmap(r, B_COLOR_8_BIT);
  icon->icon32->SetBits(bits, icon->icon32->BitsLength(), 0, B_COLOR_8_BIT);
  
  r.Set(0, 0, LARGEICON * FATBITS - 1, LARGEICON * FATBITS - 1);
  fFatBitScreen32 = new BBitmap(r, B_COLOR_8_BIT, TRUE);
  fFatBitScreen32->AddChild(fFatBitView32 = new BView(r, "", B_FOLLOW_ALL, B_WILL_DRAW));
  
  r.Set(0, 0, SMALLICON - 1, SMALLICON - 1);
  bits = splash_icon16;
  fHilite16Icon = new BBitmap(r, B_COLOR_8_BIT);
  fHilite16Icon->SetBits(bits, fHilite16Icon->BitsLength(), 0, B_COLOR_8_BIT);
  fUndo16Icon = new BBitmap(r, B_COLOR_8_BIT);
  fEdit16Icon = new BBitmap(r, B_COLOR_8_BIT);
  fDrag16Icon = new BBitmap(r, B_COLOR_8_BIT);
  fUndo16Drag = new BBitmap(r, B_COLOR_8_BIT);
  fOffScreen16 = new BBitmap(r, B_COLOR_8_BIT, TRUE);
  fOffScreen16->AddChild(fOffView16 = new BView(r, "", B_FOLLOW_ALL, B_WILL_DRAW));
  fOffScreen16->SetBits(fEdit16Icon->Bits(), fEdit16Icon->BitsLength(), 0,
			B_COLOR_8_BIT);
  
  icon->icon16 = new BBitmap(r, B_COLOR_8_BIT);
  icon->icon16->SetBits(bits, icon->icon16->BitsLength(), 0, B_COLOR_8_BIT);
  
  r.Set(0, 0, SMALLICON * FATBITS - 1, SMALLICON * FATBITS - 1);
  fFatBitScreen16 = new BBitmap(r, B_COLOR_8_BIT, TRUE);
  fFatBitScreen16->AddChild(fFatBitView16 = new BView(r, "", B_FOLLOW_ALL, B_WILL_DRAW));
  
	
  // Technically, this should be in AttachedToWindow, but this won't
  // matter for months/years/never.
  BScreen screen( Window() );
  const color_map *map = screen.ColorMap();
  if( map != NULL ) {
    for (i = 0; i < 256; i++) {
      color = map->color_list[i];
      color.red = (long)color.red * 2 / 3;
      color.green = (long)color.green * 2 / 3;
      color.blue = (long)color.blue * 2 / 3;
      fHiliteColor[i] = screen.IndexForColor(color);
    }
  } else {
    // not much we can do...?
  }
  
  fHiliteColor[B_TRANSPARENT_8_BIT] = B_TRANSPARENT_8_BIT;
}

//--------------------------------------------------------------------

TIconView::~TIconView(void)
{
  icon_info	*icon;

  CloseTextView(TRUE);
  delete fHilite32Icon;
  delete fUndo32Icon;
  delete fEdit32Icon;
  delete fOffScreen32;
  delete fFatBitScreen32;
  delete fDrag32Icon;
  delete fUndo32Drag;

  delete fHilite16Icon;
  delete fUndo16Icon;
  delete fEdit16Icon;
  delete fOffScreen16;
  delete fFatBitScreen16;
  delete fDrag16Icon;
  delete fUndo16Drag;

  icon = (icon_info*)fIconList->ItemAt(0);
  delete icon->icon32;
  delete icon->icon16;
  delete fIconList;
  delete fFile;
}

//--------------------------------------------------------------------

void TIconView::AttachedToWindow(void)
{
  if ((fRef) && (OpenFile(fRef) != B_NO_ERROR))
    fResult = B_ERROR;
  else
    fResult = B_NO_ERROR;
  Window()->Show();
}

//--------------------------------------------------------------------

void TIconView::Draw(BRect where)
{
	char		str[256];
	long		length;
	BRect		r, r1;
	icon_info*	icon;

	SetHighColor(0, 0, 0);
	r.Set(FAT32LEFT + 3, FAT32TOP + 3, FAT32RIGHT - 3, FAT32BOTTOM - 3);
	if (where.Intersects(r))
		StrokeRect(r);
	if (fEditField == LARGEICON) {
		r.InsetBy(-2, -2);
		StrokeRect(r);
		r.InsetBy(-1, -1);
		StrokeRect(r);
	}

	r.Set(FAT16LEFT + 3, FAT16TOP + 3, FAT16RIGHT - 3, FAT16BOTTOM - 3);
	if (where.Intersects(r))
		StrokeRect(r);
	if (fEditField == SMALLICON) {
		r.InsetBy(-2, -2);
		StrokeRect(r);
		r.InsetBy(-1, -1);
		StrokeRect(r);
	}

	r.Set(ICON32LEFT, ICON32TOP, ICON32RIGHT, ICON32BOTTOM);
	if (where.Intersects(r))
		StrokeRect(r);

	r.Set(ICON16LEFT, ICON16TOP, ICON16RIGHT, ICON16BOTTOM);
	if (where.Intersects(r))
		StrokeRect(r);

	r.Set(HILITE32LEFT, HILITE32TOP, HILITE32RIGHT, HILITE32BOTTOM);
	if (where.Intersects(r))
		StrokeRect(r);

	r.Set(HILITE16LEFT, HILITE16TOP, HILITE16RIGHT, HILITE16BOTTOM);
	if (where.Intersects(r))
		StrokeRect(r);

	r.Set(LIST32LEFT + 3, LIST32TOP + 3, LIST32RIGHT - 3, LIST32BOTTOM - 3);
	for (short i = 0; i < fIconCount; i++) {
		StrokeRect(r);
		if (i == fSelectedIcon) {
			r.InsetBy(-2, -2);
			StrokeRect(r);
			r.InsetBy(-1, -1);
			StrokeRect(r);
			r.InsetBy(3, 3);
		}
		icon = (icon_info *)fIconList->ItemAt(i);
		DrawBitmap(icon->icon32, BPoint(r.left + 2, r.top + 2));
		r1.Set(r.left, r.bottom + 4, r.right, r.bottom + 28);

		if (where.Intersects(r1))
			FillRect(r1, B_SOLID_LOW);

		if ((!fTextView) || ((fTextView) && (fTextField != i))) {
			sprintf(str, "%s", icon->mimeSignature);
			length = ((r.right - r.left) - StringWidth(str)) / 2;
			MovePenTo(BPoint(r.left/* + length + 1*/, r.bottom + 14));
			BRect eraseRect(r);
			eraseRect.top = eraseRect.bottom + 4;
			eraseRect.bottom += 14;
			eraseRect.right = eraseRect.left + 200;
			FillRect(eraseRect, B_SOLID_LOW);
			DrawString(str);
		}
		r.top = r.bottom + LISTOFFSET;
		r.bottom = r.top + LARGEICON + 3;
	}
	DrawIcon(where, LARGEICON);
	DrawIcon(where, SMALLICON);
}

//--------------------------------------------------------------------

void TIconView::KeyDown(const char *key, int32 count)
{
#pragma unused(count)
	uchar		*src_bits;
	uchar		*dst_bits;
	uchar		bit;
	short		next;
	long		x;
	long		y;
	ulong		mods;
	icon_info	*icon;

	Window()->CurrentMessage()->FindInt32("modifiers", (long *)&mods);
	icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
	switch (key[0]) {
		case B_TAB:
			if (mods & B_CONTROL_KEY) {
				(mods & B_SHIFT_KEY) ? next = fSelectedIcon - 1 :
									   next = fSelectedIcon + 1;
				if ((next >= 0) && (next < fIconCount)) {
					icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
					PrepareUndo(LARGEICON);
					MakeComposite(LARGEICON, icon->icon32->Bits());
					PrepareUndo(SMALLICON);
					MakeComposite(SMALLICON, icon->icon16->Bits());
					SetSelected(next);
				}
			}
			else if (fEditField == LARGEICON)
				SetEditField(SMALLICON);
			else
				SetEditField(LARGEICON);
			break;

		case B_BACKSPACE:
			if (fMarqueeFlag)
				Clear();
			break;

		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
		case B_UP_ARROW:
		case B_DOWN_ARROW:
			if (!fMarqueeFlag) {
				PrepareUndo(fEditField);
				if (fEditField == LARGEICON) {
					src_bits = (uchar *)icon->icon32->Bits();
					dst_bits = (uchar *)fOffScreen32->Bits();
				}
				else {
					src_bits = (uchar *)icon->icon16->Bits();
					dst_bits = (uchar *)fOffScreen16->Bits();
				}
				memcpy(dst_bits, src_bits, fEditField * fEditField);
			}

			switch (key[0]) {
				case B_LEFT_ARROW:
					if (fMarqueeFlag) {
						if (fMarqueeRect.right >= 0) {
							fMarqueeRect.left--;
							fMarqueeRect.right--;
						}
					}
					else {
						for (y = 0; y < fEditField; y++) {
							bit = src_bits[y * fEditField];
							for (x = 0; x < (fEditField - 1); x++)
								src_bits[(y * fEditField) + x] =
											src_bits[(y * fEditField) + x + 1];
							src_bits[(y * fEditField) + x] = bit;
						}
					}
					break;

				case B_RIGHT_ARROW:
					if (fMarqueeFlag) {
						if (fMarqueeRect.left < fEditField) {
							fMarqueeRect.left++;
							fMarqueeRect.right++;
						}
					}
					else {
						for (y = 0; y < fEditField; y++) {
							bit = src_bits[y * fEditField + (fEditField - 1)];
							for (x = fEditField - 2; x >= 0; x--)
								src_bits[(y * fEditField) + x + 1] =
											src_bits[(y * fEditField) + x];
							src_bits[(y * fEditField)] = bit;
						}
					}
					break;

				case B_UP_ARROW:
					if (fMarqueeFlag) {
						if (fMarqueeRect.bottom >= 0) {
							fMarqueeRect.top--;
							fMarqueeRect.bottom--;
						}
					}
					else {
						for (y = 0; y < (fEditField - 1); y++)
							for (x = 0; x < fEditField; x++)
								src_bits[(y * fEditField) + x] =
										src_bits[((y + 1) * fEditField) + x];
						for (x = 0; x < fEditField; x++)
							src_bits[(y * fEditField) + x] = dst_bits[x];
					}
					break;

				case B_DOWN_ARROW:
					if (fMarqueeFlag) {
						if (fMarqueeRect.top < fEditField) {
							fMarqueeRect.top++;
							fMarqueeRect.bottom++;
						}
					}
					else {
						for (y = 0; y < (fEditField - 1); y++)
							for (x = 0; x < fEditField; x++)
								src_bits[((y + 1) * fEditField) + x] =
											dst_bits[(y * fEditField) + x];
						for (x = 0; x < fEditField; x++)
							src_bits[x] = dst_bits[(y * fEditField) + x];
					}
					break;
			}
			DrawIcon(Bounds(), fEditField);
			fDirty = TRUE;
			break;
	}
}

//--------------------------------------------------------------------

void TIconView::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped()) {
		HandleIconDrop(msg);
		return;	
	}
	
	bool		save;
	entry_ref	ref;

	switch(msg->what) {
		case M_ICON_DROPPED:
			msg->PrintToStream();
			IconDropped(msg);
			break;

		case M_CLOSE_TEXT_VIEW:
			msg->FindBool("save", &save);
			CloseTextView(save);
			break;

		case B_SIMPLE_DATA:
			be_app->PostMessage(msg);
			break;

#ifdef APPI_EDIT
		case M_CREATE_APPI:
			if (fCreateAppi)
				fCreateAppi = FALSE;
			else
				fCreateAppi = TRUE;
			fDirty = TRUE;
			break;
#endif

		case M_SINGLE_LAUNCH:
			fAppFlags = (fAppFlags & ~B_LAUNCH_MASK) | B_SINGLE_LAUNCH;
			fDirty = TRUE;
			break;

		case M_MULTI_LAUNCH:
			fAppFlags = (fAppFlags & ~B_LAUNCH_MASK) | B_MULTIPLE_LAUNCH;
			fDirty = TRUE;
			break;

		case M_EXCLUSIVE_LAUNCH:
			fAppFlags = (fAppFlags & ~B_LAUNCH_MASK) | B_EXCLUSIVE_LAUNCH;
			fDirty = TRUE;
			break;

		case M_BACKGROUND_APP:
			fAppFlags = fAppFlags ^ B_BACKGROUND_APP;
			fDirty = TRUE;
			break;

		case M_ARGV_ONLY:
			fAppFlags = fAppFlags ^ B_ARGV_ONLY;
			fDirty = TRUE;
			break;
	}
}

//--------------------------------------------------------------------

void TIconView::MouseDown(BPoint point)
{
	short		field = 0;
	short		i;
	ulong		buttons;
	ulong		modifiers;
	float		x, y;
	BMessage	msg;
	BRect		r, text;
	BPoint		where;
	icon_info	*icon;

	CloseTextView(TRUE);
	Window()->CurrentMessage()->FindInt32("buttons", (long *)&buttons);
	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		// bug# 7163
		/*be_app->SetCursor(B_HAND_CURSOR);
		where = point;
		ConvertToScreen(&where);
		((TIconApp *)be_app)->fMenu->Go(where, TRUE);*/
		return;
	}
	else if (buttons & B_TERTIARY_MOUSE_BUTTON) {
		return;
	}

	Window()->CurrentMessage()->FindInt32("modifiers", (long *)&modifiers);
	r.Set(FAT32LEFT + 4, FAT32TOP + 4, FAT32RIGHT -5, FAT32BOTTOM - 5);
	if (r.Contains(point)) {
		TrackMouse(LARGEICON, modifiers, point, r);
		return;
	}
	r.Set(FAT16LEFT + 4, FAT16TOP + 4, FAT16RIGHT -5, FAT16BOTTOM - 5);
	if (r.Contains(point)) {
		TrackMouse(SMALLICON, modifiers, point, r);
		return;
	}

	r.Set(LIST32LEFT + 3, LIST32TOP + 3, LIST32RIGHT - 3, LIST32BOTTOM - 3);
	for (i = 0; i < fIconCount; i++) {
		icon = (icon_info *)fIconList->ItemAt(i);
		if (r.Contains(point)) {
			SetHighColor(127, 127, 127);
			r.InsetBy(-3, -3);
			StrokeRect(r);
			r.InsetBy(1, 1);
			StrokeRect(r);
			where = point;
			do {
				x = abs(where.x - point.x);
				y = abs(where.y - point.y);
				if ((x > 3.0) || (y > 3.0)) {
					BMessage iconContainer;
					BMessage	*msgPart;
					BBitmap 	*currBitmap;
					BRect		r;
										
					msgPart = new BMessage();
					currBitmap = icon->icon32;
					currBitmap->Archive(msgPart);
					iconContainer.AddMessage(kLargeIconMimeType, msgPart);
					
					msgPart = new BMessage();
					currBitmap = icon->icon16;
					currBitmap->Archive(msgPart);
					iconContainer.AddMessage(kMiniIconMimeType, msgPart);
					
					be_app->SetCursor(B_HAND_CURSOR);
					//
					//		calculate the correct location for the large icon to drag
					//
    				r.left = LIST32LEFT;
			    	r.right = r.left + 32;
			    	r.top = LIST32TOP + ((LARGEICON + LISTOFFSET + 3) * (i));
			    	r.bottom = r.top + 32;
					DragMessage(&iconContainer, r);
				}
				snooze(50000);
				GetMouse(&where, &buttons);
			} while (buttons);
			if (i == fSelectedIcon) {
				SetHighColor(0, 0, 0);
				StrokeRect(r);
				r.InsetBy(-1, -1);
				StrokeRect(r);
			}
			else {
				icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
				PrepareUndo(LARGEICON);
				MakeComposite(LARGEICON, icon->icon32->Bits());
				PrepareUndo(SMALLICON);
				MakeComposite(SMALLICON, icon->icon16->Bits());
				SetSelected(i);
			}
			return;
		}
		text = r;
		text.top = text.bottom + 3;
		text.bottom = text.top + 13;
		text.right += 100;
		if (text.Contains(point)) {
			fTextField = i;
			r.left = 1;
			r.top = 1;
			r.right = (text.right - text.left) - 1;
			r.bottom = (text.bottom - text.top) - 1;
			fTextView = new TEditTextView(text, r);
			fScrollView = new BScrollView("", fTextView, 0, 0);
			AddChild(fScrollView);
			fTextView->SetFont(be_plain_font);
			fTextView->SetFontSize(9);
			//fTextView->SetAlignment(B_ALIGN_CENTER);
			fTextView->SetMaxBytes(B_MIME_TYPE_LENGTH);	// bug#7167, limit length to mime length
			fTextView->SetText(icon->mimeSignature);
			fDirty = TRUE;  // !!!!! if the text changes, make sure it gets saved
			fTextView->MakeResizable(fScrollView);	
			fTextView->SelectAll();
			fTextView->MakeFocus();
			return;
		}
		r.top = r.bottom + LISTOFFSET;
		r.bottom = r.top + LARGEICON + 3;
	}
}

//--------------------------------------------------------------------

void
TIconView::DoSetCursor(BPoint point)
{
	uchar	*cursor;
	short	tool;
	BRect	r1, r2;

	if (Window()->IsActive()) {
		tool = ((TIconApp *)be_app)->fToolWind->fView->GetToolType();
		if ((modifiers() & B_CONTROL_KEY) && (tool != T_MARQUEE))
			tool = T_DROPPER;

		r1.Set(FAT32LEFT + 4, FAT32TOP + 4, FAT32RIGHT -5, FAT32BOTTOM - 5);
		r2.Set(FAT16LEFT + 4, FAT16TOP + 4, FAT16RIGHT -5, FAT16BOTTOM - 5);
		if ((r1.Contains(point)) || (r2.Contains(point))) {
			switch (tool) {
				case T_MARQUEE:
					if (fEditField == LARGEICON) {
						r1.right = r1.left + (fMarqueeRect.right * FATBITS) + FATBITS;
						r1.bottom = r1.top + (fMarqueeRect.bottom * FATBITS) + FATBITS;
						r1.left += (fMarqueeRect.left * FATBITS);
						r1.top += (fMarqueeRect.top * FATBITS);
						if ((r1.Contains(point)) && (fMarqueeFlag))
							cursor = (uchar *)B_HAND_CURSOR;
						else
							cursor = cursor_marquee;
					}
					else {
						r2.right = r2.left + (fMarqueeRect.right * FATBITS) + FATBITS;
						r2.bottom = r2.top + (fMarqueeRect.bottom * FATBITS) + FATBITS;
						r2.left += (fMarqueeRect.left * FATBITS);
						r2.top += (fMarqueeRect.top * FATBITS);
						if ((r2.Contains(point)) && (fMarqueeFlag))
							cursor = (uchar *)B_HAND_CURSOR;
						else
							cursor = cursor_marquee;
					}
					break;

				case T_LASSO:
					cursor = cursor_lasso;
					break;

				case T_PENCIL:
					cursor = cursor_pencil;
					break;

				case T_ERASER:
					cursor = cursor_eraser;
					break;

				case T_DROPPER:
					cursor = cursor_dropper;
					break;

				case T_BUCKET:
					cursor = cursor_bucket;
					break;

				default:
					cursor = cursor_shapes;
					break;
			}
			be_app->SetCursor(cursor);
		}
		else
			be_app->SetCursor(B_HAND_CURSOR);
	}
}

void TIconView::MouseMoved(BPoint point, ulong code, const BMessage *msg)
{
	// watch for actual mouse moved states and act accordingly
	switch (code) {
		case B_ENTERED_VIEW:
		case B_INSIDE_VIEW:
			DoSetCursor(point);
			break;
			
		case B_EXITED_VIEW:
			if ( (Window()->IsActive()) && (msg == NULL) )
				be_app->SetCursor(B_HAND_CURSOR);
			break;
			
		default:
			BView::MouseMoved(point, code, msg);
			break;
	}
	
}

//--------------------------------------------------------------------

void TIconView::Pulse(void)
{
	uchar*	pat;
	uchar	byte;
	long	i;

	if ((fMarqueeFlag) && (Window()->IsActive())) {
		pat = (uchar *)marquee_pat;
		byte = pat[7];
		for (i = 6; i >= 0; i--)
			pat[i + 1] = pat[i];
		pat[0] = byte;

		DrawIcon(Bounds(), fEditField);
	}
}

//--------------------------------------------------------------------

void TIconView::Clear(void)
{
	uchar		*src_bits;
	uchar		*dst_bits;
	BRect		r;
	BView		*view;
	const color_map*	cmap;
	TColorView	*color_view;
	icon_info	*icon;

	PrepareUndo(fEditField);
	if (fMarqueeFlag)
		fMarqueeFlag = FALSE;
	else {
		// If Window() returns NULL, screen will apply to the main screen.
		BScreen screen( Window() );
		cmap = screen.ColorMap();

		color_view = ((TIconApp *)be_app)->fColorWind->fView;
		icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
		if (fEditField == LARGEICON) {
			src_bits = (uchar *)icon->icon32->Bits();
			dst_bits = (uchar *)fOffScreen32->Bits();
			r.Set(0, 0, LARGEICON - 1, LARGEICON - 1);
			view = fOffView32;
		}
		else {
			src_bits = (uchar *)icon->icon16->Bits();
			dst_bits = (uchar *)fOffScreen16->Bits();
			r.Set(0, 0, SMALLICON - 1, SMALLICON - 1);
			view = fOffView16;
		}
		memcpy(dst_bits, src_bits, fEditField * fEditField);

		if (view->Window()->Lock()) {
			view->SetHighColor(0, 0, 0);
			if (color_view->GetTheLowColor() == B_TRANSPARENT_8_BIT)
				view->SetLowColor(0x77, 0x74, 0x73, 0x72);
			else
				view->SetLowColor(cmap->color_list[color_view->GetTheLowColor()]);
			view->SetPenSize(1);
			view->SetDrawingMode(B_OP_COPY);
			view->FillRect(r, B_SOLID_LOW);
			view->Sync();
			view->Window()->Unlock();
		}
		memcpy(src_bits, dst_bits, fEditField * fEditField);
	}
	DrawIcon(Bounds(), fEditField);
	fDirty = TRUE;
}

//--------------------------------------------------------------------

void TIconView::CloseTextView(bool save)
{
	icon_info	*icon;

	if (fTextView) {
		icon = (icon_info *)fIconList->ItemAt(fTextField);
		if (save) {
			strcpy(icon->mimeSignature, fTextView->Text());
			fDirty = true;
		}
		RemoveChild(fScrollView);
		delete fScrollView;
		fTextView = NULL;
		MakeFocus();
	}
}

//--------------------------------------------------------------------

void TIconView::Copy(void)
{
	BBitmap		*bitmap;
	BRect		r;
	TIconApp	*app = (TIconApp *)be_app;
	icon_info	*icon;

	icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
	app->fClipFlag = TRUE;
	if (fEditField == LARGEICON) {
		if (fMarqueeFlag) {
			bitmap = fDrag32Icon;
			app->fClipRect = fMarqueeRect;
		}
		else {
			bitmap = icon->icon32;
			app->fClipRect.Set(0, 0, LARGEICON - 1, LARGEICON - 1);
		}
	}
	else {
		if (fMarqueeFlag) {
			bitmap = fDrag16Icon;
			app->fClipRect = fMarqueeRect;
		}
		else {
			bitmap = icon->icon16;
			app->fClipRect.Set(0, 0, SMALLICON - 1, SMALLICON - 1);
		}
	}

	if (fOffView32->Window()->Lock()) {
		r.Set(0, 0, fEditField - 1, fEditField - 1);
		fOffView32->DrawBitmap(bitmap, r, r);
		fOffView32->Window()->Unlock();
		memcpy(app->fClipIcon->Bits(), fOffScreen32->Bits(),
								LARGEICON * LARGEICON);
	}
	
	
	// set up the systemwide copy/paste buffer
	be_clipboard->Lock();
	be_clipboard->Clear();
	
	BMessage *message = be_clipboard->Data();

	bitmap = icon->icon32;
	if (bitmap) {
		BMessage *embeddedIcon = new BMessage();

		bitmap->Archive(embeddedIcon);
		status_t err = message->AddMessage(kLargeIconMimeType, embeddedIcon);
		ASSERT(err == B_OK);
	}

	bitmap = icon->icon16;
	if (bitmap) {
		BMessage *embeddedIcon = new BMessage();

		bitmap->Archive(embeddedIcon);
		status_t err = message->AddMessage(kMiniIconMimeType, embeddedIcon);
		ASSERT(err == B_OK);
	}

	be_clipboard->Commit();
	be_clipboard->Unlock();

}

//--------------------------------------------------------------------

void TIconView::Cut(void)
{
	Copy();
	Clear();
}

//--------------------------------------------------------------------

void TIconView::DeleteIcon(void)
{
	BRect		r;
	icon_info	*icon;

	if (fSelectedIcon) {

		icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
		fIconList->RemoveItem(fSelectedIcon);
		delete icon->icon32;
		delete icon->icon16;
		free(icon);

		r = Bounds();
		r.left = LIST32LEFT;
		r.right = LIST32RIGHT;
		r.top = LIST32TOP + ((LARGEICON + LISTOFFSET + 3) * fSelectedIcon);
		FillRect(r, B_SOLID_LOW);

		if (fSelectedIcon != (fIconCount - 1))
			Invalidate(r);

		fIconCount -= 1;
		SetSelected(fSelectedIcon - 1);
		fDirty = TRUE;
	}
}

//--------------------------------------------------------------------

void TIconView::DrawIcon(BRect where, short size)
{
	uchar		*src_bits;
	uchar		*dst_bits;
	uchar		*hilite_bits;
	long		i;
	BBitmap		*bitmap;
	BRect		icon_rect;
	BRect		fat_rect;
	BRect		r;
	BView		*fat_view, *off_view;
	rgb_color	c;	
	icon_info	*icon;

	c.red = 255;
	c.green = 255;
	c.blue = 255;

	icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
	icon_rect.Set(0, 0, size - 1, size - 1);
	fat_rect.Set(0, 0, FATBITS * size - 1, FATBITS * size - 1);
	if (size == LARGEICON) {
		r.Set(FAT32LEFT + 4, FAT32TOP + 4, FAT32RIGHT - 5, FAT32BOTTOM - 5);
		src_bits = (uchar *)icon->icon32->Bits();
		bitmap = fOffScreen32;
		off_view = fOffView32;
		fat_view = fFatBitView32;
	}
	else {
		r.Set(FAT16LEFT + 4, FAT16TOP + 4, FAT16RIGHT - 5, FAT16BOTTOM - 5);
		src_bits = (uchar *)icon->icon16->Bits();
		bitmap = fOffScreen16;
		off_view = fOffView16;
		fat_view = fFatBitView16;
	}
	dst_bits = (uchar *)bitmap->Bits();

	MakeComposite(size, dst_bits);
	if ((fMarqueeFlag) && (off_view->Window()->Lock())) {
		off_view->SetHighColor(0, 0, 0);
		off_view->SetLowColor(255, 255, 255);
		off_view->SetPenSize(1);
		off_view->SetDrawingMode(B_OP_COPY);
		off_view->StrokeRect(fMarqueeRect, *marquee_pat);
		off_view->Sync();
		off_view->Window()->Unlock();
	}

	if (fat_view->Window()->Lock()) {
		if (where.Intersects(r)) {
			fat_view->DrawBitmap(bitmap, icon_rect, fat_rect);
			fat_view->SetHighColor(255, 255, 255);
			fat_view->BeginLineArray(size * 2 + 2);
			for (i = 0; i <= size; i++)
				fat_view->AddLine(BPoint(i * FATBITS, 0),
							  BPoint(i * FATBITS, FATBITS * size), c);
			for (i = 0; i <= size; i++)
				fat_view->AddLine(BPoint(0, i * FATBITS),
							  BPoint(FATBITS * size, i * FATBITS), c);
			fat_view->EndLineArray();
			fat_view->SetHighColor(0, 0, 0);
			fat_view->Sync();

			if (size == LARGEICON)
				DrawBitmap(fFatBitScreen32, fat_rect, r);
			else
				DrawBitmap(fFatBitScreen16, fat_rect, r);
		}
		fat_view->Window()->Unlock();
	}

	MakeComposite(size, dst_bits);
	if (size == LARGEICON) {
		DrawBitmap(bitmap, BPoint(ICON32LEFT + 2, ICON32TOP + 2));
		DrawBitmap(bitmap, BPoint(LIST32LEFT + 5,
								  LIST32TOP + 5 + ((LARGEICON + LISTOFFSET + 3)
															* fSelectedIcon)));
		bitmap = fHilite32Icon;
	}
	else {
		DrawBitmap(bitmap, BPoint(ICON16LEFT + 2, ICON16TOP + 2));
		bitmap = fHilite16Icon;
	}
	hilite_bits = (uchar *)bitmap->Bits();
	for (i = 0; i < size * size; i++)
		hilite_bits[i] = fHiliteColor[dst_bits[i]];

	if (size == LARGEICON)
		DrawBitmap(bitmap, BPoint(HILITE32LEFT + 2, HILITE32TOP + 2));
	else
		DrawBitmap(bitmap, BPoint(HILITE16LEFT + 2, HILITE16TOP + 2));
}

//--------------------------------------------------------------------

void TIconView::DrawSignature(short index)
{
#pragma unused(index)
}

//--------------------------------------------------------------------

void TIconView::IconDropped(BMessage *msg)
{
	uchar		*src_bits;
	uchar		*dst_bits;
	long		count;
	icon_info	*icon;

	icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
	if (msg->HasData("icon32", B_UINT8_TYPE)) {
		if (msg->FindData("icon32", B_UINT8_TYPE, &src_bits, &count) != B_NO_ERROR)
			return;
		PrepareUndo(LARGEICON);
		dst_bits = (uchar *)icon->icon32->Bits();
		memcpy(dst_bits, src_bits, count);
		fMarqueeFlag = FALSE;
		DrawIcon(Bounds(), LARGEICON);
		fDirty = TRUE;
	}
	if (msg->HasData("icon16", B_UINT8_TYPE)) {
		if (msg->FindData("icon16", B_UINT8_TYPE, &src_bits, &count) != B_NO_ERROR)
			return;
		PrepareUndo(SMALLICON);
		dst_bits = (uchar *)icon->icon16->Bits();
		memcpy(dst_bits, src_bits, count);
		fMarqueeFlag = FALSE;
		DrawIcon(Bounds(), SMALLICON);
		fDirty = TRUE;
	}
//	if (msg->HasInt32("signature")) {
//		msg->FindInt32("signature", &icon->signature);
//		DrawSignature(fSelectedIcon);
//		fDirty = TRUE;
//	}
}


void TIconView::HandleIconDrop(BMessage *message)
{
	BBitmap *largeIcon = NULL;
	BBitmap *miniIcon = NULL;
	BMessage iconMessage;
	icon_info	*icon;
	status_t err;
	BRect r;
	bool continueDrop=false;
	BPoint p,tempP;
	int iconIndx=0;
		
	tempP = ConvertFromScreen(message->DropPoint());

	p = message->DropPoint();
	ConvertFromScreen(&p);
	//
	//	This is a bit of a hack, but then again, check out how it works
	//	in the old app, it doesn't
	//
	p.y += 68;
	//
	//	try and see if the drop point is in one of the rects
	//	of the existing icons
	//	
	r.left = LIST32LEFT;
	r.right = r.left + 32;
	for (iconIndx=0 ; iconIndx < fIconCount ; iconIndx++) {
		r.top = LIST32TOP + ((LARGEICON + LISTOFFSET + 3) * (iconIndx+1));
		r.bottom = r.top + 32;
		if (r.Contains(p)) {
			continueDrop = true;
			break;
		}
	}
	
	if (continueDrop == false)
		return;
		
	icon = (icon_info *)fIconList->ItemAt(iconIndx);
	//
	//	extract the bitmaps as packaged in the filetypes compatible
	//	format
	//
	err = message->FindMessage(kLargeIconMimeType, &iconMessage);
	if (!err) {
		largeIcon = (BBitmap*) BBitmap::Instantiate(&iconMessage);
		icon->icon32 = largeIcon;
		
		fMarqueeFlag = FALSE;
		DrawIcon(Bounds(), LARGEICON);
		fDirty = TRUE;
	}

	err = message->FindMessage(kMiniIconMimeType, &iconMessage);
	if (!err) {
		miniIcon = (BBitmap*) BBitmap::Instantiate(&iconMessage);
		icon->icon16 = miniIcon;
		
		fMarqueeFlag = FALSE;
		DrawIcon(Bounds(), SMALLICON);
		fDirty = TRUE;
	}
	//
	// change the selection
	//	
	SetSelected(iconIndx);
}

//--------------------------------------------------------------------

void TIconView::MakeComposite(short size, void *bits)
{
	uchar		*src_bits;
	uchar		*dst_bits;
	BBitmap		*bitmap;
	BView		*view;
	icon_info	*icon;

	icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
	if (size == LARGEICON) {
		src_bits = (uchar *)icon->icon32->Bits();
		dst_bits = (uchar *)fOffScreen32->Bits();
		bitmap = fDrag32Icon;
		view = fOffView32;
	}
	else {
		src_bits = (uchar *)icon->icon16->Bits();
		dst_bits = (uchar *)fOffScreen16->Bits();
		bitmap = fDrag16Icon;
		view = fOffView16;
	}

	memcpy(dst_bits, src_bits, size * size);
	if ((fEditField == size) && (fMarqueeFlag) && (view->Window()->Lock())) {
		view->DrawBitmap(bitmap, fDragRect, fMarqueeRect);
		view->Sync();
		view->Window()->Unlock();
	}
	memcpy(bits, dst_bits, size * size);
}

//--------------------------------------------------------------------

void TIconView::NewIcon(void)
{
  uchar		*src_bits;
  BRect		r;
  icon_info	*icon;

  if (fIconCount < 256) {
    icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
    MakeComposite(LARGEICON, icon->icon32->Bits());
    MakeComposite(SMALLICON, icon->icon16->Bits());

    icon = (icon_info *)malloc(sizeof(icon_info));
    r.Set(0, 0, LARGEICON - 1, LARGEICON - 1);
    icon->icon32 = new BBitmap(r, B_COLOR_8_BIT);
    src_bits = (uchar *)icon->icon32->Bits();
    memset(src_bits, B_TRANSPARENT_8_BIT, LARGEICON * LARGEICON);
    
    r.Set(0, 0, SMALLICON - 1, SMALLICON - 1);
    icon->icon16 = new BBitmap(r, B_COLOR_8_BIT);
    src_bits = (uchar *)icon->icon16->Bits();
    memset(src_bits, B_TRANSPARENT_8_BIT, SMALLICON * SMALLICON);
    icon->mimeSignature[0] = '\0';
    icon->id = -1;
    fIconList->AddItem(icon);
    fIconCount++;
    SetSelected(fIconCount - 1);
    fDirty = TRUE;
    
    r = Bounds();
    r.left = LIST32LEFT;
    r.right = LIST32RIGHT;
    r.top = LIST32TOP + ((LARGEICON + LISTOFFSET + 3) * (fIconCount - 1));
    Draw(r);
  }
}

//--------------------------------------------------------------------

long TIconView::OpenFile(entry_ref *ref)
{
  char			str[256];
  char			name[256];
  long			num_icons = 0;
  icon_info		*icon;
  
  BEntry 			entry(ref);
  
  entry.SetTo(ref);
  if (entry.InitCheck() != B_NO_ERROR) {
    sprintf(str, "Error trying to open file");
    return B_ERROR;
  }
  else
    entry.GetName(name);

  fFile->SetTo(&entry, O_RDWR);
  if (fFile->InitCheck() != B_NO_ERROR) {
    sprintf(str, "Error: Could not open '%s'", name); 
    (new BAlert("", str, "Sorry"))->Go();
    return B_ERROR;
  }
  
  PRINT(("opening %s\n", name));
  
  BBitmap *largeBitmap = new BBitmap(kLargeIconRect, B_COLOR_8_BIT);
  BBitmap *smallBitmap = new BBitmap(kSmallIconRect, B_COLOR_8_BIT);
  
  status_t error;  
  BAppFileInfo *mimeType = new BAppFileInfo(fFile);
	char appSignature[255];
	  
  error = mimeType->GetSignature(appSignature);
  if (error != B_OK) {  
  		strcpy(appSignature,"");
	}    
	
	//printf("signature %s\n", appSignature);	  

  error = mimeType->GetIcon(largeBitmap, B_LARGE_ICON);
  error |= mimeType->GetIcon(smallBitmap, B_MINI_ICON);
  
  if (error == B_OK) {
    icon = (icon_info *)fIconList->ItemAt(num_icons++);
    icon->icon32->SetBits(largeBitmap->Bits(), largeBitmap->BitsLength(),0, B_COLOR_8_BIT);
    icon->icon16->SetBits(smallBitmap->Bits(), smallBitmap->BitsLength(),0, B_COLOR_8_BIT);
    icon->id = 0;
    strcpy(icon->mimeSignature,appSignature);
  } /*else
    printf("no icon data found, %x\n", error);*/

  /*const char *B_SUPPORTED_MIME_ENTRY = "types";
  
  BMessage message;
  error = mimeType->GetSupportedTypes(&message);
  if (error)
    printf("no supported types found, %x\n", error);
  else
  		message.PrintToStream();*/
  
  // supported type signatures are embedded as a sequence of C strings;
  // find them all and extract their icons
  int32 index=0;
	// !!!! so, if a file does not have a file types attribute
	//	the remaining icons won't be found
	// 	follow the next section as a hack to read in all icons
	//	change this later to open of an app OR a file
#if 0
	for (index = 0; ; index++) {
		printf("looking for another\n");
		const char *mimeSignature;
		int32 bufferLength;
		
		error = message.FindData("types", 'CSTR', index, &mimeSignature,&bufferLength);
		if (error != B_OK) {
			printf(" none found, bailing\n");
			break;
		}
	
		error = mimeType->GetIconForType(mimeSignature, largeBitmap, B_LARGE_ICON);
		//		ASSERT(error == B_OK);
		error |= mimeType->GetIconForType(mimeSignature, smallBitmap, B_MINI_ICON);
		//		ASSERT(error == B_OK);
	
		if (error == B_OK) {
			NewIcon();
			icon = (icon_info *)fIconList->ItemAt(num_icons++);
			if (icon) {
				strcpy(icon->mimeSignature, mimeSignature);
				icon->id = index + 1;
	
				icon->icon32->SetBits(largeBitmap->Bits(), largeBitmap->BitsLength(),0, B_COLOR_8_BIT);
				icon->icon16->SetBits(smallBitmap->Bits(), smallBitmap->BitsLength(),0, B_COLOR_8_BIT);
			}
		}
	}
#endif
	//
	//	scan the file for specific attributes of type ICON
	//	make a BIG assumption that they exist in pairs with MICNs
	//	
	char currattrname[B_ATTR_NAME_LENGTH];
	attr_info attrinfo;
	
	fFile->RewindAttrs();
	while (fFile->GetNextAttrName(currattrname) == B_NO_ERROR) {
		fFile->GetAttrInfo(currattrname,&attrinfo);
		if (	(strcmp(currattrname,"BEOS:M:STD_ICON") != 0) &&
				(strcmp(currattrname,"BEOS:L:STD_ICON") != 0)) {
			
			if ((attrinfo.type == 'ICON') /*|| (attrinfo.type == 'MICN')*/){
				char *temp;
				temp = currattrname;
				temp += 7;	// !!!!! skip the header, this is a hack!
				error = mimeType->GetIconForType(temp, largeBitmap, B_LARGE_ICON);
				error |= mimeType->GetIconForType(temp, smallBitmap, B_MINI_ICON);

				if (error == B_OK) {
					NewIcon();
					icon = (icon_info *)fIconList->ItemAt(num_icons++);
					if (icon) {
						strcpy(icon->mimeSignature, temp);
						icon->id = index++;
			
						icon->icon32->SetBits(largeBitmap->Bits(), largeBitmap->BitsLength(),0, B_COLOR_8_BIT);
						icon->icon16->SetBits(smallBitmap->Bits(), smallBitmap->BitsLength(),0, B_COLOR_8_BIT);
					}
				} else
					printf("! problem occurred getting icon - %s\n",currattrname);
			}
		}
	}	

	delete mimeType;
	delete largeBitmap;
	delete smallBitmap;
	
	// we have no new style icons, try to find the old ones
	//  !!!!! making another big assumption that files are not going
	//	to have icons stored as resources
#if 0
	if (!num_icons) {
		unsigned long len;
	
		fFile->Unset();
	
		fFile->SetTo(ref, O_RDWR);
		if (fFile->InitCheck() != B_NO_ERROR) {
			sprintf(str, "Error: Could not open '%s'", name);
			(new BAlert("", str, "Sorry"))->Go();
			return B_ERROR;
		}
		// we have no new style icons, try to find the old ones
		BResources *res = new BResources(fFile);
		while (	(res->GetResourceInfo('ICON', num_icons, &icon_id,&icon_name, &len) &&
						(num_icons < 7))) {
			if (num_icons)
				NewIcon();
			
			icon = (icon_info *)fIconList->ItemAt(num_icons);
	
			if (!num_icons && strlen(appSignature))
				strcpy(icon->mimeSignature, appSignature);
			else {
				strcpy(icon->mimeSignature, icon_name);
				icon->mimeSignature[4] = '\0';
			}
	
			icon->id = icon_id;
			bits = (char *)res->FindResource('ICON', icon_id, &len);
			icon->icon32->SetBits(bits, len, 0, B_COLOR_8_BIT);
			free(bits);
	
			bits = (char *)res->FindResource('MICN', icon_name, &len);
			icon->icon16->SetBits(bits, len, 0, B_COLOR_8_BIT);
			free(bits);
			num_icons++;
		}
		
		delete res;
	}
#endif  

  fDirty = FALSE;
  SetSelected(0);
  Window()->SetTitle(name);
  if (!Window()->IsHidden())
    Draw(Bounds());
  return B_NO_ERROR;
}

//--------------------------------------------------------------------

void TIconView::Paste(void)
{
	uchar		*src_bits;
	uchar		*dst_bits;
	uchar		*drag_bits;
	ulong		buttons;
	float		width, height;
	BPoint		point;
	BRect		r;
	BView		*view;
	TIconApp	*app = (TIconApp *)be_app;
	icon_info	*icon;

	if (!app->fClipFlag)
		return;
	icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
	if (fEditField == LARGEICON) {
		src_bits = (uchar *)icon->icon32->Bits();
		dst_bits = (uchar *)fOffScreen32->Bits();
		drag_bits = (uchar *)fDrag32Icon->Bits();
		r.Set(0, 0, LARGEICON - 1, LARGEICON - 1);
		view = fOffView32;
	}
	else {
		src_bits = (uchar *)icon->icon16->Bits();
		dst_bits = (uchar *)fOffScreen16->Bits();
		drag_bits = (uchar *)fDrag16Icon->Bits();
		r.Set(0, 0, SMALLICON - 1, SMALLICON - 1);
		view = fOffView16;
	}

	PrepareUndo(fEditField);
	MakeComposite(fEditField, src_bits);
	fDragRect = app->fClipRect;
	fDragRect.OffsetTo(0, 0);
	fDragRect = fDragRect & r;
	
	if (view->Window()->Lock()) {
		view->DrawBitmap(app->fClipIcon, app->fClipRect, fDragRect);
		view->Window()->Unlock();
		memcpy(drag_bits, dst_bits, fEditField * fEditField);
	}

	if (!fMarqueeFlag) {
		width = fDragRect.Width();
		height = fDragRect.Height();
		fMarqueeRect.left = ((fEditField - 1) - width) / 2;
		fMarqueeRect.right = fMarqueeRect.left + width;
		fMarqueeRect.top = ((fEditField - 1) - height) / 2;
		fMarqueeRect.bottom = fMarqueeRect.top + height;
		fMarqueeRect = fMarqueeRect & r;
		fMarqueeFlag = TRUE;
	}

	app->fToolWind->fView->SetTool(T_MARQUEE);
	DrawIcon(Bounds(), fEditField);
	GetMouse(&point, &buttons);
	ConvertToScreen(&point);
	MouseMoved(point, 0, NULL);
	fDirty = TRUE;
}

//--------------------------------------------------------------------

void TIconView::PrepareUndo(short size)
{
	uchar		*src_bits;
	uchar		*dst_bits;
	icon_info	*icon;

	icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
	if (fMarqueeFlag) {
		fUndoMarqueeRect = fMarqueeRect;
		fUndoDragRect = fDragRect;
		if (size == LARGEICON) {
			fUndo32Marquee = TRUE;
			memcpy(fUndo32Drag->Bits(), fDrag32Icon->Bits(), size * size);
		}
		else {
			fUndo16Marquee = TRUE;
			memcpy(fUndo16Drag->Bits(), fDrag16Icon->Bits(), size * size);
		}
	}
	else
		(size == LARGEICON) ? fUndo32Marquee = FALSE :
							  fUndo16Marquee = FALSE;

	if (size == LARGEICON) {
		src_bits = (uchar *)icon->icon32->Bits();
		dst_bits = (uchar *)fUndo32Icon->Bits();
		fUndo32 = TRUE;
	}
	else {
		src_bits = (uchar *)icon->icon16->Bits();
		dst_bits = (uchar *)fUndo16Icon->Bits();
		fUndo16 = TRUE;
	}
	memcpy(dst_bits, src_bits, fEditField * fEditField);
	if (fMarqueeFlag)
		fUndoTool = T_MARQUEE;
	else
		fUndoTool = ((TIconApp *)be_app)->fToolWind->fView->GetTool();
}

//--------------------------------------------------------------------

bool TIconView::SaveChanges(void)
{
  char		str[256];
  short		i;
  long		result;
  BAlert		*alert;
  BFilePanel	*panel;
  BMessenger	window(Window());
  
  be_app->SetCursor(B_HAND_CURSOR);
  if (fDirty) {
    sprintf(str, "Save changes to '%s'?", Window()->Title());
    alert = new BAlert("", str, "Cancel", "No", "OK");
    alert->SetShortcut(0, 'c');
    alert->SetShortcut(1, 'n');
    alert->SetShortcut(2, 'o');
    result = alert->Go();
    
    switch (result) {
    case 0:
      return FALSE;
      
    case 1:
      break;
      
    case 2:
      //
      // change from  !initcheck to == B_NO_INIT, bug #06496
      //
      if (fFile->InitCheck() == B_NO_INIT) {
			panel = new BFilePanel(B_SAVE_PANEL, &window);
			panel->Window()->Show();
			return FALSE;
      }
      if (SaveFile() != B_NO_ERROR)
			return FALSE;
      break;
    }
  }
  
  if (fFile->InitCheck()) {
    for (i = 1; i < fIconCount; i++) {
      SetSelected(1);
      DeleteIcon();
    }
  }
  return TRUE;
}

//--------------------------------------------------------------------

//
//	Remove all of the old icons in the file,
//	The only relevant ones are ICON and MICN
//
void TIconView::RemoveAllIcons(void)
{
	char currattrname[B_ATTR_NAME_LENGTH];
	attr_info attrinfo;
	
	fFile->RewindAttrs();
	while (fFile->GetNextAttrName(currattrname) == B_NO_ERROR) {
		fFile->GetAttrInfo(currattrname,&attrinfo);
		
		if (	(strcmp(currattrname,"BEOS:M:STD_ICON") != 0) &&
				(strcmp(currattrname,"BEOS:L:STD_ICON") != 0)) {
			
			if ((attrinfo.type == 'ICON') || (attrinfo.type == 'MICN')){
				fFile->RemoveAttr(currattrname);
			}
		}
	}	
}

long TIconView::SaveFile(void)
{
  char		str[256];
  long		index = 0;
#ifdef APPI_EDIT
  long		*appi;
#endif
  long		result;
  icon_info	*icon;
  
  if (fMarqueeFlag) {
    icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
    MakeComposite(LARGEICON, icon->icon32->Bits());
    MakeComposite(SMALLICON, icon->icon16->Bits());
    fUndo16 = fUndo32 = FALSE;
    fMarqueeFlag = FALSE;
    DrawIcon(Bounds(), fEditField);
  }
  
#define WRITE_ICONS_AS_MIME
#ifdef WRITE_ICONS_AS_MIME
  int32 error;
  BBitmap *largeBitmap = new BBitmap(kLargeIconRect, B_COLOR_8_BIT);
  BBitmap *smallBitmap = new BBitmap(kSmallIconRect, B_COLOR_8_BIT);
  BAppFileInfo *mimeType = new BAppFileInfo(fFile);
	
	RemoveAllIcons();
	
	char name[32];
	int32 uindx=1;
/*
	//
	// !!!!! the icon will not be set, all icons are written to disk as ICON/MICN pairs
	//
  	// write out the app icon
  	if (fIconCount > 0) {
    	icon = (icon_info *)fIconList->ItemAt(0);
    
	if (strlen(icon->mimeSignature) <= 0) {
		sprintf(name,"Untitled-%i",uindx);
		strcpy(icon->mimeSignature,name);
		uindx++;
	}
    largeBitmap->SetBits(icon->icon32->Bits(),icon->icon32->BitsLength(), 0, B_COLOR_8_BIT);
    error = mimeType->SetIcon(largeBitmap, B_LARGE_ICON);
    if (error < 0)
      PRINT(("error %x saving large icon data for signature %s\n",error, icon->mimeSignature));
    
    smallBitmap->SetBits(icon->icon16->Bits(),icon->icon16->BitsLength(), 0, B_COLOR_8_BIT);
    error = mimeType->SetIcon(smallBitmap, B_MINI_ICON);
    if (error)
     	PRINT(("error %x saving small icon data for signature %s\n", error, icon->mimeSignature));
  }
*/
  // write out all the icons
  for (int32 index = 0; index < fIconCount; index++) {
    icon = (icon_info *)fIconList->ItemAt(index);
		
		if (strlen(icon->mimeSignature) <= 0) {
			sprintf(name,"Untitled-%i",uindx);
			strcpy(icon->mimeSignature,name);
			uindx++;
		} else if (strlen(icon->mimeSignature) > B_MIME_TYPE_LENGTH) {
			// bug #7167, limit sig length to mime limit
			icon->mimeSignature[B_MIME_TYPE_LENGTH] = 0;
		}
			    
    largeBitmap->SetBits(icon->icon32->Bits(),icon->icon32->BitsLength(), 0, B_COLOR_8_BIT);
    error = mimeType->SetIconForType(icon->mimeSignature,largeBitmap, B_LARGE_ICON);
    if (error)
      PRINT(("error %x saving large icon data for signature %s\n", 
	     error, icon->mimeSignature));
  
    smallBitmap->SetBits(icon->icon16->Bits(),icon->icon16->BitsLength(), 0, B_COLOR_8_BIT);
    error = mimeType->SetIconForType(icon->mimeSignature,smallBitmap, B_MINI_ICON);
    if (error)
      PRINT(("error %x saving small icon data for signature %s\n", 
	     error, icon->mimeSignature));
  }
  
  delete largeBitmap;
  delete smallBitmap;
  delete mimeType;
  
#endif
  
//#define WRITE_ICONS_INTO_RESOURCES
#ifdef WRITE_ICONS_INTO_RESOURCES
	BResources *rFile;
  	rFile = new BResources();  	// 	try opening it, file was blown away when BFile was created

  	if ((result = rFile->SetTo(fFile,true)) != B_NO_ERROR) {
 		goto err_exit;
  	}
  			
  // update and add new icons
  for (i = 0; i < fIconCount; i++) {
    icon = (icon_info *)fIconList->ItemAt(i);
    name[0] = icon->mimeSignature;
 printf("icon %i, name %s\n",i,icon->mimeSignature);
    //if ((icon->id != -1) && (fFile->HasResource(icon->id, 'ICON')))
    if ((icon->id != -1) && (rFile->HasResource(icon->id, 'ICON'))) {
      //result = fFile->WriteResource('ICON', icon->id, icon->icon32->Bits(),
      result = rFile->WriteResource('ICON', icon->id, icon->icon32->Bits(),
				    0, icon->icon32->BitsLength());
    } else {
      //result = fFile->AddResource('ICON', i, icon->icon32->Bits(),
			struct stat st;
      	if (fFile->GetStat(&st) != B_NO_ERROR)
      		printf("unable to stat\n");
      	result = rFile->AddResource('ICON', i, 
										      			icon->icon32->Bits(),
														  icon->icon32->BitsLength(),
														  name[0]);
		}
    //if (result < 0) {
    if (result != B_NO_ERROR) {
      goto err_exit;
     }
    
    //if ((icon->id != -1) && (fFile->HasResource(icon->id, 'MICN')))
    if ((icon->id != -1) && (rFile->HasResource(icon->id, 'MICN')))
      //result = fFile->WriteResource('MICN', icon->id, icon->icon16->Bits(),
      result = rFile->WriteResource('MICN', icon->id, icon->icon16->Bits(),
				    0, icon->icon16->BitsLength());
    else
      //result = fFile->AddResource('MICN', i, icon->icon16->Bits(),
      result = rFile->AddResource('MICN', i, icon->icon16->Bits(),
				  icon->icon16->BitsLength(),
				  name[1]);
    if (result != B_NO_ERROR) {
      goto err_exit;
     }
  }
  
  // remove obsolite icons
  while (rFile->HasResource(i, 'ICON')) {
   	rFile->RemoveResource('ICON', i);
    	rFile->RemoveResource('MICN', i);
    i++;
  }
  
  delete rFile;
  
#ifdef APPI_EDIT
  if (fCreateAppi) {
    size = (sizeof(long) * 3) + (fIconCount * 4);
    appi = (long *)malloc(size);
    appi[index++] = fSignature;
    appi[index++] = fAppFlags;
    appi[index++] = fIconCount - 1;
    for (i = 0; i < fIconCount; i++) {
      icon = (icon_info*)fIconList->ItemAt(i);
      if (icon->signature != 'BAPP')
	appi[index++] = icon->signature;
    }
    if (fFile->HasResource((long)0, 'APPI'))
      result = fFile->WriteResource('APPI', 0, appi, 0, size);
    else
      result = fFile->AddResource('APPI', 0, appi, size, "app_info");
    free(appi);
    if (result < 0)
      goto err_exit;
  }
#endif
  
#endif
  fDirty = FALSE;
  return B_NO_ERROR;
  
err_exit:;
  //sprintf(str, "Error: Could not save '%s' (0x%.8x)", Window()->Title(), result);
  sprintf(str, "Error: Could not save '%s' (%i)", Window()->Title(), result);
  (new BAlert("", str, "Sorry"))->Go();
  return B_ERROR;
}

//--------------------------------------------------------------------

void TIconView::SaveRequested(BMessage *msg)
{
	char			str[256];
	const char		*name = NULL;
	entry_ref		dir;
	BDirectory		directory;
	BFile			*file;

	if (msg->FindRef("directory", &dir) == B_NO_ERROR) {
		msg->FindString("name", &name);
		directory.SetTo(&dir);
		if (directory.InitCheck() == B_NO_ERROR) {
			file = new BFile();
			directory.CreateFile(name, file);
			if (file->InitCheck() == B_NO_ERROR) {
				fFile = file;
				Window()->SetTitle(name);
				SaveFile();
			}
			else {
				sprintf(str, "Could not create %s.", name);
				(new BAlert("", str, "Sorry"))->Go();
				delete file;
			}
		}
	}
}

//--------------------------------------------------------------------

void TIconView::SelectAll(void)
{
	uchar		*src_bits;
	uchar		*drag_bits;
	uchar		*dst_bits;
	ulong		buttons;
	BPoint		point;
	BView		*view;
	icon_info	*icon;

	icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
	if (fEditField == LARGEICON) {
		src_bits = (uchar *)icon->icon32->Bits();
		dst_bits = (uchar *)fOffScreen32->Bits();
		drag_bits = (uchar *)fDrag32Icon->Bits();
		view = fOffView32;
	}
	else {
		src_bits = (uchar *)icon->icon16->Bits();
		dst_bits = (uchar *)fOffScreen16->Bits();
		drag_bits = (uchar *)fDrag16Icon->Bits();
		view = fOffView16;
	}

	PrepareUndo(fEditField);
	MakeComposite(fEditField, src_bits);
	memcpy(drag_bits, src_bits, fEditField * fEditField);
	fMarqueeRect.Set(0, 0, fEditField - 1, fEditField - 1);
	fDragRect = fMarqueeRect;
	fMarqueeFlag = TRUE;

	if (view->Window()->Lock()) {
		view->SetHighColor(0, 0, 0);
		view->SetLowColor(0x77, 0x74, 0x73, 0x72);
		view->SetPenSize(1);
		view->SetDrawingMode(B_OP_COPY);
		view->FillRect(fMarqueeRect, B_SOLID_LOW);
		view->Sync();
		view->Window()->Unlock();
	}
	memcpy(src_bits, dst_bits, fEditField * fEditField);

	((TIconApp *)be_app)->fToolWind->fView->SetTool(T_MARQUEE);
	GetMouse(&point, &buttons);
	ConvertToScreen(&point);
	MouseMoved(point, 0, NULL);
}

//--------------------------------------------------------------------

void TIconView::SetEditField(short field)
{
	BRect		r, r1, r2;
	icon_info	*icon;

	if (field != fEditField) {
		if (fMarqueeFlag) {
			PrepareUndo(fEditField);
			icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
			if (fEditField == LARGEICON)
				MakeComposite(LARGEICON, icon->icon32->Bits());
			else
				MakeComposite(SMALLICON, icon->icon16->Bits());
			fMarqueeFlag = FALSE;
			DrawIcon(Bounds(), fEditField);
		}

		fEditField = field;
		r1.Set(FAT32LEFT, FAT32TOP, FAT32RIGHT, FAT32BOTTOM);
		r2.Set(FAT16LEFT, FAT16TOP, FAT16RIGHT, FAT16BOTTOM);
		if (field != LARGEICON) {
			r = r1;
			r1 = r2;
			r2 = r;
		}
		SetHighColor(255, 255, 255);
		StrokeRect(r2);
		r2.InsetBy(1, 1);
		StrokeRect(r2);

		SetHighColor(0, 0, 0);
		StrokeRect(r1);
		r1.InsetBy(1, 1);
		StrokeRect(r1);
	}
}

//--------------------------------------------------------------------

void TIconView::SetMode(long theMode)
{
	fPenMode = theMode;
}

//--------------------------------------------------------------------

void TIconView::SetPen(long theSize)
{
	fPenSize = theSize;
}

//--------------------------------------------------------------------

void TIconView::SetSelected(short field)
{
	BRect		r;

	if ((field != fSelectedIcon) && (field >= 0) && (field < fIconCount)) {
		fMarqueeFlag = FALSE;

		r.left = LIST32LEFT;
		r.top = LIST32TOP + ((LARGEICON + LISTOFFSET + 3) * fSelectedIcon);
		r.right = LIST32RIGHT;
		r.bottom = r.top + LARGEICON + 9;
		SetHighColor(255, 255, 255);
		StrokeRect(r);
		r.InsetBy(1, 1);
		StrokeRect(r);

		fSelectedIcon = field;
		SetHighColor(0, 0, 0);
		r.left = LIST32LEFT;
		r.top = LIST32TOP + ((LARGEICON + LISTOFFSET + 3) * fSelectedIcon);
		r.right = LIST32RIGHT;
		r.bottom = r.top + LARGEICON + 9;
		StrokeRect(r);
		r.InsetBy(1, 1);
		StrokeRect(r);
		r.InsetBy(2, 2);
		StrokeRect(r);

		DrawIcon(Bounds(), LARGEICON);
		DrawIcon(Bounds(), SMALLICON);

		fUndo32 = FALSE;
		fUndo16 = FALSE;
	}
}

//--------------------------------------------------------------------

void TIconView::TrackMouse(short size, ulong mods, BPoint where, BRect fat_rect)
{
	bool		changed = FALSE;
	bool		color_changed = FALSE;
	bool		constrain, x_constrain = FALSE, y_constrain = FALSE;
	bool		drag = FALSE;
	bool		first_pass = TRUE;
	bool		frame;
	bool		out = FALSE;
	uchar		*src_bits, *dst_bits, *undo_bits, *drag_bits;
	short		high_color;
	short		low_color;
	short		tool;
	short		x, y, constrain_x, constrain_y, start_x, start_y, last_x, last_y;
	ulong		buttons;
	float		width, height;
	BBitmap		*bitmap, *drag_bitmap;
	BPoint		point;
	BRect		r;
	BView		*view;
	TIconApp	*app = (TIconApp *)be_app;
	TColorView	*color_view;
	TToolView	*tool_view;
	const color_map	*cmap;
	icon_info	*icon;

	SetEditField(size);

	icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);

	color_view = app->fColorWind->fView;
	high_color = color_view->GetTheHighColor();
	low_color = color_view->GetTheLowColor();
	
	BScreen screen( Window() );
	cmap = screen.ColorMap();

	tool_view = app->fToolWind->fView;
	tool = tool_view->GetToolType();
	if ((mods & B_CONTROL_KEY) && (tool != T_MARQUEE))
		tool = T_DROPPER;

	constrain = mods & B_SHIFT_KEY;
	frame = mods & B_COMMAND_KEY;

	if (size == LARGEICON) {
		src_bits = (uchar *)icon->icon32->Bits();
		bitmap = fOffScreen32;
		dst_bits = (uchar *)fOffScreen32->Bits();
		undo_bits = (uchar *)fUndo32Icon->Bits();
		drag_bitmap = fDrag32Icon;
		drag_bits = (uchar *)fDrag32Icon->Bits();
		view = fOffView32;
	}
	else {
		src_bits = (uchar *)icon->icon16->Bits();
		bitmap = fOffScreen16;
		dst_bits = (uchar *)fOffScreen16->Bits();
		undo_bits = (uchar *)fUndo16Icon->Bits();
		drag_bitmap = fDrag16Icon;
		drag_bits = (uchar *)fDrag16Icon->Bits();
		view = fOffView16;
	}

	if (fMarqueeFlag) {
		fat_rect.right = fat_rect.left + (fMarqueeRect.right * FATBITS) + FATBITS;
		fat_rect.bottom = fat_rect.top + (fMarqueeRect.bottom * FATBITS) + FATBITS;
		fat_rect.left += (fMarqueeRect.left * FATBITS);
		fat_rect.top += (fMarqueeRect.top * FATBITS);
		if ((tool == T_MARQUEE) && (fat_rect.Contains(where))) {
			if (frame) {
				PrepareUndo(size);
				MakeComposite(size, src_bits);
				fDirty = TRUE;
			}
			drag = TRUE;
		}
	}

	if (size == LARGEICON) {
		x = last_x = start_x = (where.x - (FAT32LEFT + 4)) / FATBITS;
		y = last_y = start_y = (where.y - (FAT32TOP + 4)) / FATBITS;
	}
	else {
		x = last_x = start_x = (where.x - (FAT16LEFT + 4)) / FATBITS;
		y = last_y = start_y = (where.y - (FAT16TOP + 4)) / FATBITS;
	}

	do {
		if (frame != (mods & B_COMMAND_KEY)) {
			frame = mods & B_COMMAND_KEY;
			changed = TRUE;
		}
		if (constrain != (mods & B_SHIFT_KEY)) {
			constrain = mods & B_SHIFT_KEY;
			x_constrain = FALSE;
			y_constrain = FALSE;
			changed = TRUE;
		}

		if (x < 0) {
			x = 0;
			out = TRUE;
		}
		if (x >= size) {
			x = size - 1;
			out = TRUE;
		}
		if (y < 0) {
			y = 0;
			out = TRUE;
		}
		if (y >= size) {
			y = size - 1;
			out = TRUE;
		}

		if ((constrain) && (!x_constrain) && (!y_constrain) &&
				((x != last_x) || (y != last_y))) {
			if (x != last_x) {
				x_constrain = TRUE;
				constrain_y = last_y;
			}
			else {
				y_constrain = TRUE;
				constrain_x = last_x;
			}
		}

		switch (tool) {
			case T_MARQUEE:
				if (drag) {
					width = fMarqueeRect.Width();
					height = fMarqueeRect.Height();
					fMarqueeRect.left += (x - last_x);
					if (fMarqueeRect.left > (size - 1))
						fMarqueeRect.left = size - 1;
					fMarqueeRect.right = fMarqueeRect.left + width;
					if (fMarqueeRect.right < 0) {
						fMarqueeRect.right = 0;
						fMarqueeRect.left = -width;
					}
					fMarqueeRect.top += (y - last_y);
					if (fMarqueeRect.top > (size - 1))
						fMarqueeRect.top = size - 1;
					fMarqueeRect.bottom = fMarqueeRect.top + height;
					if (fMarqueeRect.bottom < 0) {
						fMarqueeRect.bottom = 0;
						fMarqueeRect.top = -height;
					}
					if (first_pass)
						fDirty = TRUE;
					memcpy(dst_bits, src_bits, size * size);
					if (view->Window()->Lock()) {
						view->SetDrawingMode(B_OP_COPY);
						view->DrawBitmap(drag_bitmap, fDragRect, fMarqueeRect);
						view->Sync();
						view->Window()->Unlock();
					}
				}
				else {
					if (first_pass) {
						PrepareUndo(size);
						MakeComposite(size, src_bits);
						memcpy(drag_bits, src_bits, size * size);
					}
					if (constrain) {
						last_x = abs(x - start_x);
						last_y = abs(y - start_y);
						if (last_x < last_y)
							(y < start_y) ? y = start_y - last_x :
											y = start_y + last_x;
						else
							(x < start_x) ? x = start_x - last_y :
											x = start_x + last_y;
					}
					if ((x != last_x) || (y != last_y) ||
						(changed)) {
						if (start_x < x) {
							fMarqueeRect.left = start_x;
							fMarqueeRect.right = x;
						}
						else {
							fMarqueeRect.left = x;
							fMarqueeRect.right = start_x;
						}
						if (start_y < y) {
							fMarqueeRect.top = start_y;
							fMarqueeRect.bottom = y;
						}
						else {
							fMarqueeRect.top = y;
							fMarqueeRect.bottom = start_y;
						}
						fDragRect = fMarqueeRect;
						fMarqueeFlag = TRUE;
					}
					else if ((x == start_x) && (y == start_y)) {
						fMarqueeFlag = FALSE;
						DrawIcon(Bounds(), size);
					}
				}
				if (fMarqueeFlag)
					Pulse();
				break;

			case T_LASSO:
				break;

			case T_ERASER:
			case T_PENCIL:
				if (x_constrain)
					y = last_y = constrain_y;
				if (y_constrain)
					x = last_x = constrain_x;

				if ((first_pass) || (x != last_x) || (y != last_y)) {
					if ((first_pass) && (tool == T_PENCIL) &&
							(src_bits[y * size + x] == high_color)) {
						if (color_view->Window()->Lock()) {
							color_view->SetTheHighColor(255);
							color_view->Window()->Unlock();
							color_changed = TRUE;
						}
					}
					if (first_pass) {
						PrepareUndo(size);
						MakeComposite(size, src_bits);
						fMarqueeFlag = FALSE;
						fDirty = TRUE;
					}
					memcpy(dst_bits, src_bits, size * size);
					tool_view->Pen(x, y, last_x, last_y, view, tool);
					memcpy(src_bits, dst_bits, size * size);
					DrawIcon(Bounds(), size);
					fDirty = TRUE;
					Sync();
				}
				break;

			case T_DROPPER:
				if ((first_pass) && (fMarqueeFlag)) {
					PrepareUndo(size);
					MakeComposite(size, src_bits);
					fMarqueeFlag = FALSE;
					DrawIcon(Bounds(), size);
				}
				if (out) {
					if (color_view->Window()->Lock()) {
						if (frame) {
							if (color_view->GetTheLowColor() != low_color)
								color_view->SetTheLowColor(low_color);
						}
						else {
							if (color_view->GetTheHighColor() != high_color)
								color_view->SetTheHighColor(high_color);
						}
						color_view->Window()->Unlock();
					}
				}
				else if (size == LARGEICON)
					tool_view->Dropper(x, y, icon->icon32);
				else
					tool_view->Dropper(x, y, icon->icon16);
				break;

			case T_BUCKET:
				PrepareUndo(size);
				MakeComposite(size, dst_bits);
				fMarqueeFlag = FALSE;
				fDirty = TRUE;
				tool_view->Fill(start_x, start_y, fPenMode, view, dst_bits,
																  src_bits);
				DrawIcon(Bounds(), size);
				Sync();
				return;

			case T_MAGNIFY:
				break;

			case T_LINE:
			case T_FILL_RECT:
			case T_RECT:
			case T_FILL_RRECT:
			case T_RRECT:
			case T_FILL_OVAL:
			case T_OVAL:
			case T_FILL_ARC:
			case T_ARC:
			case T_FILL_TRIANGLE:
			case T_TRIANGLE:
				if ((constrain) && (tool != T_LINE)) {
					last_x = abs(x - start_x);
					last_y = abs(y - start_y);
					if (last_x < last_y)
						(y < start_y) ? y = start_y - last_x :
										y = start_y + last_x;
					else
						(x < start_x) ? x = start_x - last_y :
										x = start_x + last_y;
				}
				else if ((constrain) && (tool == T_LINE)) {
					last_x = abs(x - start_x);
					last_y = abs(y - start_y);
					if (last_y <= (last_x / 2))
						y = start_y;
					else if (last_x <= (last_y / 2))
						x = start_x;
					else {
						if (last_x < last_y)
							(y < start_y) ? y = start_y - last_x :
											y = start_y + last_x;
						else
							(x < start_x) ? x = start_x - last_y :
											x = start_x + last_y;
					}
				}

				if ((first_pass) || (x != last_x) || (y != last_y) || (changed)) {
					if (first_pass) {
						PrepareUndo(size);
						MakeComposite(size, src_bits);
						fMarqueeFlag = FALSE;
						fDirty = TRUE;
					}
					memcpy(dst_bits, undo_bits, size * size);
					tool_view->Shape(start_x, start_y, x, y, view, tool,
									 fPenSize, fPenMode, !frame);
					memcpy(src_bits, dst_bits, size * size);
					DrawIcon(Bounds(), size);
					fDirty = TRUE;
					Sync();
				}
				break;
		}
		out = FALSE;
		changed = FALSE;
		first_pass = FALSE;
		last_x = x;
		last_y = y;

		GetMouse(&point, &buttons);
		mods = modifiers();
		if (size == LARGEICON) {
			x = (point.x - (FAT32LEFT + 4)) / FATBITS;
			y = (point.y - (FAT32TOP + 4)) / FATBITS;
		}
		else {
			x = (point.x - (FAT16LEFT + 4)) / FATBITS;
			y = (point.y - (FAT16TOP + 4)) / FATBITS;
		}
		snooze(3000);
	} while(buttons);

	ConvertToScreen(&point);
	MouseMoved(point, 0, NULL);

	if ((fMarqueeFlag) && (!drag)) {
		fDragRect = fMarqueeRect;
		memcpy(drag_bits, src_bits, size * size);
		if (view->Window()->Lock()) {
			view->SetHighColor(0, 0, 0);
			view->SetLowColor(0x77, 0x74, 0x73, 0x72);
			view->SetPenSize(1);
			view->SetDrawingMode(B_OP_COPY);
			view->FillRect(fMarqueeRect, B_SOLID_LOW);
			view->Sync();
			view->Window()->Unlock();
		}
		memcpy(src_bits, dst_bits, size * size);
	}

	if (color_changed) {
		if (color_view->Window()->Lock()) {
			color_view->SetTheHighColor(high_color);
			color_view->Window()->Unlock();
		}
	}
}

//--------------------------------------------------------------------

void TIconView::Undo(void)
{
	bool		undo;
	bool		flag;
	short		tool;
	uchar		*src_bits;
	uchar		*dst_bits;
	uchar		*drag_bits;
	uchar		*undo_drag_bits;
	uchar		byte;
	ulong		buttons;
	BPoint		point;
	BRect		r;
	icon_info	*icon;
	TToolView	*tool_view;

	icon = (icon_info *)fIconList->ItemAt(fSelectedIcon);
	undo = FALSE;
	if ((fEditField == LARGEICON) && (fUndo32)) {
		src_bits = (uchar *)fUndo32Icon->Bits();
		dst_bits = (uchar *)icon->icon32->Bits();
		drag_bits = (uchar *)fDrag32Icon->Bits();
		undo_drag_bits = (uchar *)fUndo32Drag->Bits();
		undo = TRUE;
	}
	else if ((fEditField == SMALLICON) && (fUndo16)) {
		src_bits = (uchar *)fUndo16Icon->Bits();
		dst_bits = (uchar *)icon->icon16->Bits();
		drag_bits = (uchar *)fDrag16Icon->Bits();
		undo_drag_bits = (uchar *)fUndo16Drag->Bits();
		undo = TRUE;
	}
	if (undo) {
		for (short i = 0; i < fEditField * fEditField; i++) {
			byte = src_bits[i];
			src_bits[i] = dst_bits[i];
			dst_bits[i] = byte;

			byte = drag_bits[i];
			drag_bits[i] = undo_drag_bits[i];
			undo_drag_bits[i] = byte;
		}
		r = fMarqueeRect;
		fMarqueeRect = fUndoMarqueeRect;
		fUndoMarqueeRect = r;

		r = fDragRect;
		fDragRect = fUndoDragRect;
		fUndoDragRect = r;

		flag = fMarqueeFlag;
		if (fEditField == LARGEICON) {
			fMarqueeFlag = fUndo32Marquee;
			fUndo32Marquee = flag;
		}
		else {
			fMarqueeFlag = fUndo16Marquee;
			fUndo16Marquee = flag;
		}

		tool_view = ((TIconApp *)be_app)->fToolWind->fView;
		tool = tool_view->GetTool();
		tool_view->SetTool(fUndoTool);
		fUndoTool = tool;
		GetMouse(&point, &buttons);
		ConvertToScreen(&point);
		MouseMoved(point, 0, NULL);

		DrawIcon(Bounds(), fEditField);
	}
}


//====================================================================

TEditTextView::TEditTextView(BRect frame, BRect text_rect)
			  :BTextView(frame, "", text_rect, B_FOLLOW_NONE, B_WILL_DRAW)
{
}

//--------------------------------------------------------------------

bool TEditTextView::AcceptsChar(ulong key) const
{
	BMessage	msg;

	if ((key == B_RETURN) || (key == B_ESCAPE)) {
		msg.what = M_CLOSE_TEXT_VIEW;
		msg.AddBool("save", key == B_RETURN);
		Window()->PostMessage(&msg, Parent());
		return FALSE;
	}
	return TRUE;
}

