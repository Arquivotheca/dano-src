//*****************************************************************************
//
//	File:		TextControl.cpp
//
//	Written by:	Peter Potrebic
//
//	Copyright 1994-97, Be Incorporated
//
//*****************************************************************************

#include "stdlib.h"
#include "string.h"
#include "limits.h"

#include <ClassInfo.h>
#include <Debug.h>
#include <PropertyInfo.h>
#include <TextControl.h>

#ifndef TEXT_CONTROL_PRIVATE_H
#include <TextControlPrivate.h>
#endif
#ifndef _BUTTON_H
#include <Button.h>
#endif
#ifndef _DATA_IO_H
#include <DataIO.h>
#endif
#ifndef _GLYPH_H
#include <Glyph.h>
#endif
#ifndef _TEXT_VIEW_H
#include <TextView.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _INTERFACE_MISC_H
#include <interface_misc.h>
#endif
#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif
#ifndef _MENU_PRIVATE_H
#include "MenuPrivate.h"
#endif

#include <StreamIO.h>

/* -------------------------------------------------------------------- */

#define TV_MARGIN	3.0
#define TV_DIVIDER_MARGIN 6.0

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

BTextControl::BTextControl(BRect frame, const char *name,
	const char *label, const char *initial_text, BMessage *message,
	uint32 resizeMask, uint32 flags)
	: BControl(frame, name, label, message, resizeMask, flags | B_FRAME_EVENTS | B_WILL_DRAW)
{
	fText = NULL;
	InitData(label, initial_text);

	font_height	finfo;
	float		h1;
	float		h2;
	float		h;
	BRect		b = Bounds();

	GetFontHeight(&finfo);
	h1 = ceil(finfo.ascent + finfo.descent + finfo.leading);
	h2 = fText->LineHeight();

	// Height of main view must be the larger of h1 and h2+(TV_MARGIN*2)
	h = (h1 > h2 + (TV_MARGIN*2)) ? h1 : h2 + (TV_MARGIN*2);
	ResizeTo(b.Width(), h);
	b.bottom = h;

	// set height and position of text entry view
	fText->ResizeTo(fText->Bounds().Width(), h2);
	// vertically center this view
	fText->MoveBy(0, (b.Height() - (h2+(TV_MARGIN*2))) / 2);
}

/* -------------------------------------------------------------------- */

// Private constructor for use by BComboBox so that BComboBox can provide
// a special _BTextInput_.

BTextControl::BTextControl(BRect frame, const char *name,
	const char *label, const char *initial_text, BMessage *message,
	_BTextInput_ *textInput, uint32 rmask, uint32 flags)
	: BControl(frame, name, label, message, rmask, flags | B_FRAME_EVENTS | B_WILL_DRAW)
{
	fText = textInput;
	InitData(label, initial_text);

	font_height	finfo;
	float		h1;
	float		h2;
	float		h;
	BRect		b = Bounds();

	GetFontHeight(&finfo);
	h1 = ceil(finfo.ascent + finfo.descent + finfo.leading);
	h2 = fText->LineHeight();

	// Height of main view must be the larger of h1 and h2+(TV_MARGIN*2)
	h = (h1 > h2 + (TV_MARGIN*2)) ? h1 : h2 + (TV_MARGIN*2);
	ResizeTo(b.Width(), h);
	b.bottom = h;

	// set height and position of text entry view
	fText->ResizeTo(fText->Bounds().Width(), h2);
	// vertically center this view
	fText->MoveBy(0, (b.Height() - (h2+(TV_MARGIN*2))) / 2);
}


/* -------------------------------------------------------------------- */

void BTextControl::InitData(const char *label, const char *initial_text,
	BMessage *data)
{
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	SetInvalidate(true);
	
	BRect	r1 = Bounds();

	fLabelAlign = B_ALIGN_LEFT;
	fModificationMessage = NULL;
	fDivider = 0.0;		// assuming that there is no label
	fSkipSetFlags = false;

	// fPrevWidth/height are shorts to minimize the amount of reserved space
	// used in the object.
	fPrevWidth = (uint16) r1.Width();
	fPrevHeight = (uint16) r1.Height();

	uint32	mask = 0;
	BFont	aFont(be_plain_font);

	if (!data || !data->HasString(S_FONT_FAMILY_STYLE)) 
		mask |= B_FONT_FAMILY_AND_STYLE;
	if (!data || !data->HasFloat(S_FONT_FLOATS))
		mask |= B_FONT_SIZE;

	if (mask != 0)
		SetFont(&aFont, mask);

	// fDivider specifies the dividing line between label & text
	if (label)
		fDivider = r1.Width() / 2.0;

	// Need to be tricky here (???). If the user wants this control to
	// be keyboard navigable, then we really want the underlying text
	// view to be navigable, not this view.
	bool navigate = ((Flags() & B_NAVIGABLE) != 0);
	if (navigate) {
		fSkipSetFlags = true;
		SetFlags(Flags() & ~B_NAVIGABLE);	// disable navigation for this
		fSkipSetFlags = false;
	}

	// don't know where to locate the string. Can't determine until
	// AttachedToWindow().

	if (data) {
		// if there is a data msg then the _BTextInput_ view was already
		// created by the unarchiving process. SImply setup the pointer
		// to it.
		fText = (_BTextInput_ *) FindView("_input_");
		ASSERT(fText);
	} else {
		r1.top += TV_MARGIN;
		r1.left = r1.left + fDivider + TV_DIVIDER_MARGIN;
		r1.right -= TV_MARGIN;
		r1.bottom -= TV_MARGIN;
		BRect	tr = r1;
		tr.OffsetTo(B_ORIGIN);
		// note that navigation is enabled on fText if appropriate
		if (fText == NULL) {
			fText = new _BTextInput_(r1, tr, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
				B_WILL_DRAW | B_FRAME_EVENTS | (navigate ? B_NAVIGABLE : 0));
		} else {
			fText->MoveTo(r1.LeftTop());
			fText->ResizeTo(r1.Width(), r1.Height());
			if (navigate) {
				fText->SetFlags(fText->Flags() | B_NAVIGABLE);
			}
		}
		AddChild(fText);
		SetText(initial_text);
		fText->SetAlignment(B_ALIGN_LEFT);
		fText->AlignTextRect();
		fText->SetPrintWhiteBackground(false);
	}
}

/*------------------------------------------------------------*/

BTextControl::BTextControl(BMessage *data)
	: BControl(data)
{
	const char *initial_text = NULL;
	long	l;

	// Label is inherited from BControl
	InitData(Label(), initial_text, data);

	alignment a_label = B_ALIGN_LEFT;
	alignment a_text = B_ALIGN_LEFT;
	if (data->HasInt32(S_ALIGN_LABEL)) {
		data->FindInt32(S_ALIGN_LABEL, &l);
		a_label = (alignment) l;
	}
	if (data->HasInt32(S_ALIGN_TEXT)) {
		data->FindInt32(S_ALIGN_TEXT, &l);
		a_text = (alignment) l;
	}
	SetAlignment(a_label, a_text);

	if (data->HasFloat(S_DIVIDER))
		// don't call SetDivider because it will move the text_view
		data->FindFloat(S_DIVIDER, &fDivider);


	if (data->HasData(S_MOD_MESSAGE, B_RAW_TYPE)) {
		long		length;
		const void	*ptr;
		BMessage	*msg = new BMessage();
		data->FindData(S_MOD_MESSAGE, B_RAW_TYPE, &ptr, &length);
		msg->Unflatten((const char *) ptr);
		SetModificationMessage(msg);
	}
}

/*------------------------------------------------------------*/

BArchivable *BTextControl::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BTextControl"))
		return NULL;
	return new BTextControl(data);
}

/*------------------------------------------------------------*/

status_t BTextControl::Archive(BMessage *data, bool deep) const
{
	BControl::Archive(data, deep);
//+	data->AddString(B_CLASS_NAME_ENTRY, "BTextControl");

	alignment a_label;
	alignment a_text;
	GetAlignment(&a_label, &a_text);
	data->AddInt32(S_ALIGN_LABEL, a_label);
	data->AddInt32(S_ALIGN_TEXT, a_text);

	data->AddFloat(S_DIVIDER, Divider());

	BMessage *m = ModificationMessage();
	if (m) {
		BMallocIO	stream;
		m->Flatten(&stream);
		data->AddData(S_MOD_MESSAGE, B_RAW_TYPE, stream.Buffer(), stream.BufferLength());
	}
	return 0;
}

/* -------------------------------------------------------------------- */

BTextControl::~BTextControl()
{
	SetModificationMessage(NULL);
}

/* -------------------------------------------------------------------- */

void	BTextControl::AttachedToWindow()
{
	BControl::AttachedToWindow();

//+	PRINT(("text_control(%s) - nagivate=%x\n", Label(), Flags() & B_NAVIGABLE));

#define AUTO_RESIZE_CONTROL 0
#if AUTO_RESIZE_CONTROL
	/*
	 Resize the input field
	*/
	font_height	finfo;
	float		h1;
	float		h2;
	float		h;
	BRect		b = Bounds();

	GetFontHeight(&finfo);
	h1 = ceil(finfo.ascent + finfo.descent + finfo.leading);
	h2 = fText->LineHeight();
//+	PRINT(("ascent=%.1f, descent=%.1f, leading=%.1f\n", 
//+		finfo2.ascent, finfo2.descent, finfo2.leading));

	// Height of main view must be the larger of h1 and h2+(TV_MARGIN*2)
	h = (h1 > h2 + (TV_MARGIN*2)) ? h1 : h2 + (TV_MARGIN*2);
	ResizeTo(b.Width(), h);
	b.bottom = h;

	// set height and position of text entry view
	fText->ResizeTo(fText->Bounds().Width(), h2);
	// vertically center this view
//+	PRINT(("height=%.1f, h2=%.1f, ", b.Height(), h2));
	fText->MoveBy(0, (b.Height() - (h2+(TV_MARGIN*2))) / 2);
//+	PRINT_OBJECT(fText->Frame());
#endif

	bool		enabled = IsEnabled();
	rgb_color	mc = ui_color(B_DOCUMENT_TEXT_COLOR);
	rgb_color	base = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	rgb_color	bg = ViewColor();
	BFont		textFont;
		
#if _FILL_DISABLED_CONTROLS_
	if (!enabled) {
		fText->SetLowColor(bg);
		fText->SetViewColor(bg);
	} else {
		fText->SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
		fText->SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	}
#endif

	fText->GetFontAndColor(0, &textFont);
	if (!enabled) mc.disable(base);
	fText->SetFontAndColor(&textFont, B_FONT_ALL, &mc);

	fText->MakeEditable(enabled);
}

/* -------------------------------------------------------------------- */

void	BTextControl::SetModificationMessage(BMessage *msg)
{
	if (fModificationMessage)
		delete fModificationMessage;
	
	fModificationMessage = msg;
}

/* -------------------------------------------------------------------- */

BMessage *BTextControl::ModificationMessage() const
{
	return fModificationMessage;
}

/* -------------------------------------------------------------------- */

void	BTextControl::SetText(const char *text)
{
	if( InvokeKind() != B_CONTROL_INVOKED ) {
		// A hack for ObjektSynth (and any other application that may
		// be overriding Invoke() to change the control's text):  Don't
		// allow text to be replaced if this is not a full invoke.
		return;
	}
	
	fText->SetText(text);

	if (fText->IsFocus())
		fText->SetInitialText();

	fText->Invalidate();
}

/* -------------------------------------------------------------------- */

const char*	BTextControl::Text() const
{
	return(fText->Text());
}

/* -------------------------------------------------------------------- */

void BTextControl::MakeFocus(bool state)
{
//+	PRINT(("BTextControl::MakeFocus(%d, %s)\n", state, Label()));

	fText->MakeFocus(state);
	if (state)
		fText->SelectAll();
}

/* -------------------------------------------------------------------- */

void BTextControl::MouseDown(BPoint)
{
	if (!fText->IsFocus()) {
		fText->MakeFocus(TRUE);
		fText->SelectAll();
	}
}

/* -------------------------------------------------------------------- */

void BTextControl::WindowActivated(bool /*+active*/)
{
//+	BControl::WindowActivated(active);
	if (fText->IsFocus()) {
		Invalidate();
		Window()->UpdateIfNeeded();
	}
}

/* -------------------------------------------------------------------- */

void BTextControl::Draw(BRect)
{
	DrawTextInputBorder();
	DrawLabel();
	if (fText->IsFocus() && Window()->IsActive()) {
		// Only throb if cursor isn't already flashing.
		int32 selStart, selEnd;
		fText->GetSelection(&selStart, &selEnd);
		if (selStart != selEnd && !IsInvalidatePending())
			InvalidateAtTime(fText->NextFocusTime());
	}
}

/* -------------------------------------------------------------------- */
void BTextControl::DrawTextInputBorder()
{
	BButtonBorderGlyph glyph;

	BRect fr(fText->Frame());
	fr.InsetBy(-TV_MARGIN, -TV_MARGIN);
	fr.bottom -= 1;
	glyph.SetFrame(fr);
	
	glyph.SetBackgroundColor(LowColor());
	glyph.SetFillColor(fText->LowColor());
	glyph.SetLabelColor(fText->HighColor());
	glyph.SetPressed(true);
	glyph.SetActive(false);
	glyph.SetFocused(fText->IsFocus() && Window()->IsActive());
	glyph.SetEnabled(IsEnabled());
	if (glyph.IsFocused()) {
		int32 selStart, selEnd;
		fText->GetSelection(&selStart, &selEnd);
		if (selStart != selEnd)
			glyph.SetBorderColor(fText->NextFocusColor());
		else
			glyph.SetBorderColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	} else {
		glyph.SetBorderColor(make_color(0, 0, 0));
	}
	
	PushState();
	glyph.Draw(this);
	PopState();
}

/* -------------------------------------------------------------------- */

void BTextControl::DrawLabel()
{
	PushState();
	BRect bounds(Bounds());
	BRect fr(fText->Frame());
	rgb_color high = HighColor();
	rgb_color low = LowColor();
	rgb_color view = ViewColor();
	rgb_color baseCol = low;
	font_height	fInfo;
	bool enabled = IsEnabled();

	// draw label text
	const char *label = Label();
	bounds.right = bounds.left + fDivider;
	if ((label != NULL) && (fDivider > 0.0)) {
		BPoint	loc;
		GetFontHeight(&fInfo);

		switch (fLabelAlign) {
			case B_ALIGN_LEFT:
				loc.x = bounds.left + TV_MARGIN;
				break;
			case B_ALIGN_CENTER:
				{
					float center = (bounds.right - bounds.left) / 2;
					loc.x = center - (StringWidth(label)/2);
				}
				break;
			case B_ALIGN_RIGHT:
				loc.x = bounds.right - StringWidth(label) - TV_MARGIN;
				break;
		}

		// Bug #9647
		// If TextControl is following TOP & BOTTOM special case and
		// keep the label drawing relative to the text view. Special cased
		// it like this to prevent subtle position changes in other cases.
		uint32 rmode = ResizingMode();
		if ((rmode & _rule_(0xf, 0, 0xf, 0)) ==
			_rule_(_VIEW_TOP_, 0, _VIEW_BOTTOM_, 0)) {
				loc.y = fr.bottom - 2;
		} else {
			loc.y = bounds.bottom - (2 + ceil(fInfo.descent));
		}
		// End of Bug #9647

		if (!enabled) {
			SetHighColor(high.disable_copy(baseCol));
		}
		SetLowColor(view);
		DrawString(label, loc);

		SetHighColor(high);
		SetLowColor(low);
	}
	PopState();
}


/* -------------------------------------------------------------------- */

void BTextControl::CommitValue(bool enterPressed)
{
	if (Message()) {
		BMessage msg(*Message());
		msg.AddBool("be:commit",enterPressed);
		Invoke(&msg);
	} else
		Invoke();
}

/* -------------------------------------------------------------------- */

void BTextControl::SetAlignment(alignment l_align, alignment t_align)
{
	fText->SetAlignment(t_align);
	fText->AlignTextRect();

	if (fLabelAlign != l_align) {
		fLabelAlign = l_align;
		Invalidate();
	}
}

/* -------------------------------------------------------------------- */

void BTextControl::GetAlignment(alignment *l_align, alignment *t_align) const
{
	*t_align = fText->Alignment();
	*l_align = fLabelAlign;
}

/* -------------------------------------------------------------------- */

void BTextControl::SetDivider(float divide)
{
	float	diff = fDivider - divide;

	fDivider = divide;

	fText->MoveBy(-diff, 0);
	fText->ResizeBy(diff, 0);

	if (Window()) {
		fText->Invalidate();
		Invalidate();
	}
}

/* -------------------------------------------------------------------- */

float BTextControl::Divider() const
{
	return fDivider;
}

/* -------------------------------------------------------------------- */

void BTextControl::SetFlags(uint32 flags)
{
	if (!fSkipSetFlags) {
		uint32	te_flags = fText->Flags();
		bool	te_nav = ((te_flags & B_NAVIGABLE) != 0);
		bool	wants_nav = ((flags & B_NAVIGABLE) != 0);

		ASSERT((Flags() & B_NAVIGABLE) == 0);

		if (!te_nav && wants_nav) {
			// The text control wants to be navigable. Pass that along to
			// the text view
			fText->SetFlags(te_flags | B_NAVIGABLE);
		} else if (te_nav && !wants_nav) {
			// Caller wants to end NAV on the text view;
			fText->SetFlags(te_flags & ~B_NAVIGABLE);
		}

		flags = flags & ~B_NAVIGABLE;	// never want NAV for this view
	}

	BControl::SetFlags(flags);
}

/* -------------------------------------------------------------------- */

void BTextControl::SetEnabled(bool state)
{
	if (state == IsEnabled())
		return;

	if (Window()) {
		fText->MakeEditable(state);
		rgb_color	mc = ui_color(B_DOCUMENT_TEXT_COLOR);
		rgb_color	base = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
		rgb_color	bg = ViewColor();
		
#if _FILL_DISABLED_CONTROLS_
		if (!state) {
			fText->SetLowColor(bg);
			fText->SetViewColor(bg);
		} else {
			fText->SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
			fText->SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
		}
#endif

		if (!state) mc.disable(base);
		BFont textFont;
		fText->GetFontAndColor(0, &textFont);
		fText->SetFontAndColor(&textFont, B_FONT_ALL, &mc);

		fText->Invalidate();
		Window()->UpdateIfNeeded();
	}

	fSkipSetFlags = true;
	BControl::SetEnabled(state);
	fSkipSetFlags = false;

//+	// Want the sub_view to be the navigable one. We always want to be able
//+	// to navigate to that view, even if disabled since Copy still works.
//+	fText->SetFlags(fText->Flags() | B_NAVIGABLE);
//+	SetFlags(Flags() & ~B_NAVIGABLE);
}

/*-------------------------------------------------------------*/

void
BTextControl::GetPreferredSize(
	float	*width,
	float	*height)
{
	BFont theFont;
	GetFont(&theFont);

	*width = Bounds().Width();
	if (Label() != NULL) {
		float strWidth = theFont.StringWidth(Label());
		*width = ceil(TV_MARGIN + strWidth + TV_DIVIDER_MARGIN + 
					  (strWidth * 1.50) + TV_MARGIN);
	}

	font_height	finfo;
	float		h1;
	float		h2;

	theFont.GetHeight(&finfo);
	h1 = ceil(finfo.ascent + finfo.descent + finfo.leading);
	h2 = fText->LineHeight();
//+	PRINT(("ascent=%.1f, descent=%.1f, leading=%.1f\n", 
//+		finfo2.ascent, finfo2.descent, finfo2.leading));

	// Height of main view must be the larger of h1 and h2+(TV_MARGIN*2)
	*height = ceil((h1 > h2 + (TV_MARGIN*2)) ? h1 : h2 + (TV_MARGIN*2));
}

/*-------------------------------------------------------------*/

void
BTextControl::ResizeToPreferred()
{
	BControl::ResizeToPreferred();

	fDivider = 0.0;
	if (Label() != NULL) {
		BFont theFont;
		GetFont(&theFont);
		fDivider = ceil(TV_MARGIN + theFont.StringWidth(Label()));
	}

	BRect tvRect = Bounds();
	tvRect.left += (Label() != NULL) ? (fDivider + TV_DIVIDER_MARGIN) : 
									   TV_MARGIN;
	tvRect.top += TV_MARGIN;
	tvRect.right -= TV_MARGIN;
	tvRect.bottom -= TV_MARGIN;
	fText->MoveTo(tvRect.LeftTop());
	fText->ResizeTo(tvRect.Width(), tvRect.Height());
	tvRect.OffsetTo(B_ORIGIN);
	fText->SetTextRect(tvRect);
}

/*-------------------------------------------------------------*/

void BTextControl::MessageReceived(BMessage *msg)
{
//+	PRINT(("what = %.4s\n", (char *) &(msg->what)));
	bool 		handled = false;
	BMessage	reply(B_REPLY);

#if _SUPPORTS_FEATURE_SCRIPTING
	status_t	err;

	switch (msg->what) {
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		{
			BMessage	specifier;
			int32		form;
			const char	*prop;
			int32		cur;
			err = msg->GetCurrentSpecifier(&cur, &specifier, &form, &prop);
			if (!err && ((strcmp(prop, "Text") == 0) ||
						(strcmp(prop, "Value") == 0))) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddString("result", Text());
					handled = true;
				} else {
					const char *text;
					err = msg->FindString("data", &text);
					handled = true;
					if (!err) {
						const char	*old = Text();
						bool		changed = (strcmp(old, text) != 0);
						SetText(text);
						if (changed) {
							CommitValue(false);
						}
					} else {
						reply.AddString("message", "incorrect type of data");
						err = B_BAD_VALUE;
					}
					reply.AddInt32("error", err);
				}
			}
			break;
		}
	}
#endif

	if (handled)
		msg->SendReply(&reply);
	else
		BControl::MessageReceived(msg);
}


/*-------------------------------------------------------------*/
	/*
	 A generic BControl supports the following:
	 	GET/SET		"Text"		DIRECT form only
	*/
#if _SUPPORTS_FEATURE_SCRIPTING
static property_info	prop_list[] = {
	{"Text",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, 0,
		{},
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};
#endif

/*-------------------------------------------------------------*/

BHandler *BTextControl::ResolveSpecifier(BMessage *_SCRIPTING_ONLY(msg), int32 _SCRIPTING_ONLY(index),
	BMessage *_SCRIPTING_ONLY(spec), int32 _SCRIPTING_ONLY(form), const char *_SCRIPTING_ONLY(prop))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	BHandler		*target = NULL;
	BPropertyInfo	pi(prop_list);
	int32			i;

//+	PRINT(("BTextControl::Resolve: what=%.4s, index=%d, form=0x%x, prop=%s\n",
//+		(char*) &(msg->what), index, spec->what, prop));

	if ((i = pi.FindMatch(msg, index, spec, form, prop)) >= 0) {
//+		PRINT(("prop=%s, i=%d\n", prop, i));
		target = this;
	} else {
		target = BControl::ResolveSpecifier(msg, index, spec, form, prop);
	}

	return target;
#else
	return NULL;
#endif
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

_BTextInput_::_BTextInput_(BRect rect, BRect text_r, ulong rMask, ulong flags)
	:BTextView(rect, "_input_", text_r, be_plain_font, NULL, rMask, flags)
{
	MakeResizable(TRUE);
	fInitialText = NULL;
	fClean = FALSE;
}

/* -------------------------------------------------------------------- */

_BTextInput_::~_BTextInput_()
{
	if (fInitialText) {
		free(fInitialText);
		fInitialText = NULL;
	}
}

/* ---------------------------------------------------------------- */

_BTextInput_::_BTextInput_(BMessage *data)
	: BTextView(data)
{
	MakeResizable(TRUE);
	fInitialText = NULL;
	fClean = FALSE;
}

/* ---------------------------------------------------------------- */

status_t _BTextInput_::Archive(BMessage *data, bool) const
{
	BTextView::Archive(data);
	return 0;
}

/* ---------------------------------------------------------------- */

BArchivable *_BTextInput_::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "_BTextInput_"))
		return NULL;
	return new _BTextInput_(data);
}

/* -------------------------------------------------------------------- */

void _BTextInput_::SetInitialText()
{
	if (fInitialText) {
		free(fInitialText);
		fInitialText = NULL;
	}
	if (Text()) {
		fInitialText = strdup(Text());
	}
}

/* -------------------------------------------------------------------- */

void _BTextInput_::KeyDown(const char *bytes, int32 numBytes)
{
	BTextControl	*tc;
	uchar			aKey = bytes[0];

	switch (aKey) {
		case B_RETURN:
			tc = cast_as(Parent(), BTextControl);

			ASSERT(tc);

			if (!tc->IsEnabled())
				break;

			ASSERT(fInitialText);
			if (strcmp(fInitialText, Text()) != 0)
				tc->CommitValue(true);
			free(fInitialText);
			fInitialText = strdup(Text());
			SelectAll();
			
			// Invoking clears the explicit focus state, so that
			// someone else can now move focus.
			SetExplicitFocus(false);
			
			break;
		case B_TAB:
			BView::KeyDown(bytes, numBytes);
			break;
		default:
			BTextView::KeyDown(bytes, numBytes);
			break;
	}
}

/* -------------------------------------------------------------------- */

void _BTextInput_::MakeFocus(bool state)
{
//+	PRINT(("_BTextInput_::MakeFocus(state=%d, view=%s)\n", state,
//+		Parent()->Name()));

	if (state == IsFocus())
		return;

	BTextControl	*parent = cast_as(Parent(), BTextControl);
	ASSERT(parent);

	if (state && parent && !parent->IsEnabled() && IsNavigating()) {
		// if this control is disabled and the user is trying to
		// tab-navigate into it, don't take focus.
		return;
	}
	
	BTextView::MakeFocus(state);

	if (state) {
		SetInitialText();
		fClean = TRUE;			// text hasn't been dirtied yet.

		BMessage *m;
		if (Window() && (m = Window()->CurrentMessage()) != 0 &&
			m->what == B_KEY_DOWN) {
				// we're being focused by a keyboard event, so
				// select all...
				SelectAll();
		}
	} else {
		ASSERT(fInitialText);
		if (strcmp(fInitialText, Text()) != 0)
			parent->CommitValue(false);
		free(fInitialText);
		fInitialText = NULL;
		fClean = FALSE;
		BMessage *m;
		if (Window() && (m = Window()->CurrentMessage()) != 0 &&
			m->what == B_MOUSE_DOWN)
				Select(0,0);
	}

	if (Window()) {
		BRect invalRect(Frame());
		invalRect.InsetBy(-TV_MARGIN, -TV_MARGIN);
		invalRect.bottom -= 1;
		parent->Invalidate();
		Window()->UpdateIfNeeded();
	}
}

/* -------------------------------------------------------------------- */

void _BTextInput_::FrameResized(float x, float y)
{
	BTextView::FrameResized(x, y);

	AlignTextRect();
}


void _BTextInput_::Select(int32 startOffset, int32 endOffset)
{
	int32 curStart, curEnd;
	GetSelection(&curStart, &curEnd);
	BTextView::Select(startOffset, endOffset);
	
	// Figure out whether to start/stop focus throbbing.
	if (IsFocus() && (curStart==curEnd) != (startOffset==endOffset)) {
		BTextControl	*parent = cast_as(Parent(), BTextControl);
		ASSERT(parent);
		if (parent->IsEnabled()) parent->Invalidate();
	}
}

void _BTextInput_::Paste(BClipboard *clipboard)
{
	BTextView::Paste(clipboard);
	
	Invalidate();
}

/* -------------------------------------------------------------------- */

// What a hack...

void
_BTextInput_::AlignTextRect()
{
	BRect bounds = Bounds();
	BRect textRect = TextRect();

	switch (Alignment()) {
		case B_ALIGN_LEFT:
			textRect.OffsetTo(B_ORIGIN);
			break;

		case B_ALIGN_CENTER:
			textRect.OffsetTo((bounds.Width() - textRect.Width()) / 2,
				textRect.top);
			break;

		case B_ALIGN_RIGHT:
			textRect.OffsetTo(bounds.Width() - textRect.Width(), textRect.top);
			break;
	}

	SetTextRect(textRect);
}

/* -------------------------------------------------------------------- */

void _BTextInput_::InsertText(const char *inText, int32 inLength, 
	   int32 inOffset, const text_run_array *inRuns)
{
	char	*ptr = NULL;

	// strip out any return characters
	if (strpbrk(inText, "\r\n")) {
		int32	len = inLength;
		ptr = (char *) malloc(len+1);
		if (ptr) {
			strncpy(ptr, inText, len);
			ptr[len] = '\0';

			char *p = ptr;

			while (len--) {
				if ((*p == '\n') || (*p == '\r'))
					*p = ' ';
				p++;
			}
//+			PRINT(("a:ptr = %s\n", ptr));
		}
	}

	BTextView::InsertText(ptr ? ptr : inText, inLength, inOffset, inRuns);

	BTextControl	*parent = cast_as(Parent(), BTextControl);
	parent->InvokeNotify(parent->fModificationMessage, B_CONTROL_MODIFIED);

	if (ptr)
		free(ptr);
}

void _BTextInput_::DeleteText(int32 fromOffset, int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);
	BTextControl	*parent = cast_as(Parent(), BTextControl);
	parent->InvokeNotify(parent->fModificationMessage, B_CONTROL_MODIFIED);
}

/* -------------------------------------------------------------------- */

BTextView *BTextControl::TextView() const
{
	return fText;
}

/*-------------------------------------------------------------*/

BTextControl &BTextControl::operator=(const BTextControl &) { return *this; }

/*----------------------------------------------------------------*/

status_t BTextControl::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}

/*----------------------------------------------------------------*/

void BTextControl::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BTextControl::MouseUp(BPoint pt)
{
	BControl::MouseUp(pt);
}

/*------------------------------------------------------------*/

void BTextControl::SetValue(int32 value)
{
	BControl::SetValue(value);
}

/*------------------------------------------------------------*/

status_t BTextControl::Invoke(BMessage *msg)
{
	return BControl::Invoke(msg);
}

/* ---------------------------------------------------------------- */

void BTextControl::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BControl::MouseMoved(pt, code, msg);
}

/*---------------------------------------------------------------*/

void	BTextControl::FrameMoved(BPoint new_position)
{
	BControl::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void	BTextControl::FrameResized(float new_width, float new_height)
{
	// ??? This func and BScrollView::FrameResized are exactly the same
	// perhaps this could be factored out.
	// The code in BBox::FrameResized isn't all that different either.

	// if the frame changed size then we need to force an extra update
	// event to get the frame area to draw correctly.
	BRect	b = Bounds();

	float	prev_width = (float) fPrevWidth;
	float	prev_height = (float) fPrevHeight;

	// can't trust new_width and new_height because Update events are
	// handled out of turn. This means that the Update event corresponding
	// to this FrameResized event might have already been handled. This
	// gets the redraw code out of synch.

	new_width = b.Width();
	new_height = b.Height();

	if (new_width > prev_width) {
		// need to redraw the ALL the area between the new right side and
		// 2 pixels to the right of the previous location of the right border
		BRect	inval = b;
		inval.right -= 1;
		inval.left = inval.right - (new_width - prev_width) - 3;
		Invalidate(inval);
	} else if (new_width < prev_width) {
		// need to force update in the area that contains the new right border
		BRect inval = b;
		inval.left = inval.right - 1;
		Invalidate(inval);
	}
	if (new_height > prev_height) {
		// need to redraw the ALL the area between the new bottom and the
		// previous location of the bottom border
		BRect	inval = b;
		inval.bottom -= 1;
		inval.top = inval.bottom - (new_height - prev_height);
		Invalidate(inval);
	} else if (new_height < prev_height) {
		// need to redraw the area the should contain the new bottom border
		BRect inval = b;
		inval.top = inval.bottom - 1;
		Invalidate(inval);
	}

	fPrevWidth = (uint16) new_width;
	fPrevHeight = (uint16) new_height;
}

/*---------------------------------------------------------------*/

void BTextControl::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::AllAttached();
}

/*---------------------------------------------------------------*/

void BTextControl::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::AllDetached();
}

/*---------------------------------------------------------------*/

status_t BTextControl::GetSupportedSuites(BMessage *data)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	return BControl::GetSupportedSuites(data);
}

/* ---------------------------------------------------------------- */

void BTextControl::_ReservedTextControl1() {}
void BTextControl::_ReservedTextControl2() {}
void BTextControl::_ReservedTextControl3() {}
void BTextControl::_ReservedTextControl4() {}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
