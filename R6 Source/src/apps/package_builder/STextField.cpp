#include <Be.h>
#include "STextField.h"
#include "MyDebug.h"

const rgb_color kDisableColor = {127,127,127,0};

#define LABEL_MARGIN 9


// make a version which has a scrollbar???

STextField::STextField(BRect frame,
						const char *name,
						const char *label, 
						const char *initial_text, 
						BMessage *message,
						ulong resizeMask,
						ulong flags,
						bool scrolling)
	: BControl(frame,name,label,message,resizeMask,
				flags | B_WILL_DRAW),
	  fLabelText(label),
	  fEditText(initial_text),
	  fScrolling(scrolling)
	  // fScrollView(
{
	BRect r = Bounds();
	fModMessage = NULL;
	fTextView = NULL;
}

STextField::~STextField()
{
	delete fModMessage;
}

void STextField::AttachedToWindow()
{
	
	BRect r = Bounds();
	const BFont *lfont = be_plain_font;
	
	SetFont(lfont);
	font_height fheight;
	lfont->GetHeight(&fheight);

	// inset for border	
	r.InsetBy(2,2);	
	// inset for label
	r.top += lfont->Size() +
				fheight.descent + fheight.leading +
					LABEL_MARGIN;

	if (fScrolling) {
		// r.InsetBy(1,1);
		r.right -= B_V_SCROLL_BAR_WIDTH;
	}

	BRect tr = r;
	tr.OffsetTo(0,0);
	tr.InsetBy(2,2);
	
	fTextView = new _TextFieldView_(r,B_EMPTY_STRING,tr,
				B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE,this);
				
	if (fScrolling) {
		BScrollView	*scrollView = new BScrollView(B_EMPTY_STRING,
										fTextView,
										B_FOLLOW_ALL,
										0,
										FALSE, TRUE, B_NO_BORDER);
		AddChild(scrollView);
	}
	else {			
		AddChild(fTextView);
	}
	
	fTextView->DisallowChar(B_TAB);
	fTextView->SetWordWrap(TRUE);
	fTextView->SetFont(be_plain_font);
	if (fEditText)
		fTextView->SetText(fEditText);
	
	BControl::AttachedToWindow();
}

void STextField::DrawFocus(bool state)
{
	// Draws the focus line

	// Don't draw the line if we are disabled
	
	if (!IsEnabled())
		return;
	float width = StringWidth(fLabelText);
	BRect r;
	
	if (fScrolling)
		r = fTextView->Parent()->Frame();
	else
		r = fTextView->Frame();

	rgb_color save = HighColor();
	SetHighColor(0,0,255);	
	StrokeLine(BPoint(r.left-1,r.top - LABEL_MARGIN + 2),
				BPoint(r.left + width,r.top - LABEL_MARGIN + 2), 
			state ? B_SOLID_HIGH : B_SOLID_LOW);
	SetHighColor(save);
}

void STextField::Draw(BRect updt)
{

	BRect r;
	if (fScrolling) {
		r = fTextView->Parent()->Frame();
		r.right++;
	}
	else
		r = fTextView->Frame();
	
	if (fLabelText) {
		MovePenTo(BPoint(r.left, r.top - LABEL_MARGIN));
		DrawString(fLabelText);
		if (fTextView->IsFocus()) {
			DrawFocus(TRUE);
		}
	}

	
	if (updt.Intersects(r)) {
		rgb_color save = HighColor();
		r.left -= 2;
		r.right += 2;
		r.top -= 2;
		r.bottom++;
		
		SetHighColor(161,161,161);
		SetPenSize(1.0);
		float c = r.top;
		StrokeLine(BPoint(r.left,c),BPoint(r.right,c));
		c = r.left;
		StrokeLine(BPoint(c,r.top+1),BPoint(c,r.bottom));
		SetHighColor(240,240,240);
		c = r.bottom;
		StrokeLine(BPoint(r.left+1,c),BPoint(r.right,c));
		c = r.right;
		StrokeLine(BPoint(c,r.top+1),BPoint(c,r.bottom-1));
		
		SetHighColor(save);
	}
	// StrokeRect(Bounds());
}

void STextField::SetText(const char *text)
{
	fEditText = text;
	if (fTextView) {
		fTextView->SetText(text);
	}
}

const char	*STextField::Text() const
{
	return fTextView->Text();
}

long STextField::TextLength() const
{
	return fTextView->TextLength();
}

void STextField::SetLabel(const char *text)
{
	fLabelText = text;
	Invalidate();
}

const char *STextField::Label() const
{
	return fLabelText;
}

void STextField::SetModificationMessage(BMessage *msg)
{
	delete fModMessage;
	fModMessage = msg;
}

BMessage *STextField::ModificationMessage() const
{
	return fModMessage;
}

const BView	*STextField::TextView()
{
	return fTextView;
}

void STextField::MakeFocus(bool state)
{
	fTextView->MakeFocus(state);
}

void STextField::SetEnabled(bool state)
{
	BControl::SetEnabled(state);
	if (state) {
		SetHighColor(0,0,0);
		fTextView->SetHighColor(0,0,0);
		fTextView->MakeEditable(TRUE);
	}
	else {
		SetHighColor(kDisableColor);
		fTextView->SetHighColor(kDisableColor);
	}
	fTextView->MakeEditable(state);
	fTextView->MakeNaviagable(state);
	fTextView->Invalidate();
	/**
	if (fScrolling) {
		BScrollView *sc = (BScrollView *)fTextView->Parent();
		sc->ScrollBar(B_VERTICAL)->SetEnabled(state);
	}**/
	
	Invalidate();
}

void STextField::Show()
{
	if (IsHidden())
		BControl::Show();
}

void STextField::Hide()
{
	if (!IsHidden())
		BControl::Hide();
}


_TextFieldView_::_TextFieldView_(BRect frame,
						const char *name,
						BRect textRect,
						ulong resizeMask,
						ulong flags,
						STextField *_parControl)
	: BTextView(frame,name,textRect,resizeMask,flags),
	  parControl(_parControl)
{
	fModified = FALSE;
}

void _TextFieldView_::KeyDown(const char *bytes,
								int32 byteCount)
{
	long		textLength = TextLength();
	int32 v = *bytes;
			
	switch (*bytes) {	
		case B_HOME:		// Ignore these
		case B_END:
		case B_PAGE_UP:
		case B_PAGE_DOWN:
			break;
		case B_DELETE:{		// Forward delete
			long	selStart;
			long	selEnd;
			
			GetSelection(&selStart, &selEnd);
			if (selStart == selEnd)
				Select(selStart, selStart + 1);
			BTextView::Delete();
			break;
		}
		case B_TAB: {		// View navigation
							// backtab??
			BView::KeyDown(bytes,byteCount);
			break;
		}
		default:
			BTextView::KeyDown(bytes,byteCount);
			break;
	}
	
	// if any chars have been added or removed on this 
	// keydown then set dirty
	if (textLength != TextLength())
		SetDirty();
}

void _TextFieldView_::Paste(BClipboard *clip)
{
	long		textLength = TextLength();
	BTextView::Paste(clip);
	if (textLength != TextLength())
		SetDirty();
}

void _TextFieldView_::Cut(BClipboard *clip)
{
	long		textLength = TextLength();
	
	BTextView::Cut(clip);
	
	if (textLength != TextLength())
		SetDirty();
}

void _TextFieldView_::MakeFocus(bool state)
{
	PRINT(("text field view Make Focus state %s\n",state ? "true" : "false"));
	BTextView::MakeFocus(state);
	
	if (!state) {
		parControl->Target()->Looper()->PostMessage(parControl->Message(),
														parControl->Target());
	}
	else {
		fModified = FALSE;
	}
	parControl->DrawFocus(state);
}

void _TextFieldView_::SetDirty(bool dirty)
{
	fModified = dirty;
			
	if (fModified) {	
		if (parControl->fModMessage) {
			BHandler *targ = parControl->Target();
			targ->Looper()->PostMessage(parControl->fModMessage,targ);	
		}
	}
}

// void _TextFieldView_::SetEnabled(bool state)


void _TextFieldView_::MouseDown(BPoint where)
{
	if (parControl->IsEnabled())
		BTextView::MouseDown(where);
}

void _TextFieldView_::Draw(BRect up)
{
	BTextView::Draw(up);
}

void _TextFieldView_::MakeNaviagable(bool state)
{
	if (state)
		SetFlags(Flags() | B_NAVIGABLE);
	else
		SetFlags(Flags() & ~B_NAVIGABLE);
}

void _TextFieldView_::MouseMoved(BPoint where,
								ulong code,
								const BMessage *msg)
{
	if (code == B_ENTERED_VIEW) {
		if (!parControl->IsEnabled())
			return;
	}
	
	BTextView::MouseMoved(where,code,msg);
}
