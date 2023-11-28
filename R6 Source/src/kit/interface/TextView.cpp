// ============================================================
//  TextView.cpp	by Hiroshi Lockheimer
// ============================================================

#include <TextView.h>

#include <ctype.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>

#include <new>

#include <Debug.h>
#include <AppDefs.h>
#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <byteorder.h>
#include <Clipboard.h>
#include <Font.h>
#include <Input.h>
#include <List.h>
#include <Region.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <String.h>
#include "TextViewSupport.h"
#include <Window.h>
#include <archive_defs.h>
#include <interface_misc.h>
#include <fbc.h>
#include <PropertyInfo.h>
#include <MessageRunner.h>
#include <FindDirectory.h>
#include <Roster.h>
#include <Path.h>
#include <Entry.h>

#if _SUPPORTS_SOFT_KEYBOARD

const char kWagnerAppSignature[]		= "application/x-vnd.Web";

#if _SUPPORTS_SOFT_KEYBOARD_APP
const char kSoftKeyboardAppSignature[]	= "application/x-vnd.Be.SoftKeyboardApp";
const char kSoftKeyboardAppName[]		= "SoftKeyboard";
#endif

static void
OpenSoftKeyboard()
{
	BMessenger wagner(kWagnerAppSignature);
    if(wagner.IsValid())
    {
        // wagner is running, send it the "show" message
        wagner.SendMessage('shwk');
    }
    
#if _SUPPORTS_SOFT_KEYBOARD_APP
	else
	{
		BMessenger softKbdApp(kSoftKeyboardAppSignature);
		if (!softKbdApp.IsValid())
		{
			BPath		path;
			entry_ref	ref;
			
			find_directory(B_BEOS_APPS_DIRECTORY, &path);
			path.Append(kSoftKeyboardAppName);
			get_ref_for_path(path.Path(), &ref);
			
			be_roster->Launch(&ref);
		}
	}
#endif

}

#endif	//_SUPPORTS_SOFT_KEYBOARD

// === Typedefs ===

struct flat_run {
	int32		offset;
	font_family	family;
	font_style	style;
	float		size;
	float		shear;
	uint16		face;
	rgb_color	color;
	uint16		_filler1;
};

struct flat_run_array {
	int32		magic;
	int32		version;
	int32		count;
	flat_run	runs[1];
};

class _BTextChangeResult_ {
public:
	int32		fInsertLength;
	int32		fInsertOffset;
	int32		fDeleteStart;
	int32		fDeleteEnd;
};

// === Constants ===

enum {
	kMagicNumber			= 'Ali!',
	kDR9FlatRunArray		= 0
};

enum {
	kUnclassifiedChar		= 0,
	kSpaceChar				= 1,
	kPunctuationChar		= 2,
	kEnclosedCJKChar		= 3,
	kFullOrHalfWidthChar	= 4,
	kHiraganaChar			= 5,
	kKatakanaChar			= 6,
	kIdeographChar			= 7
};

const uint32 kBeginTaboos[] = {
//	3001		3002		FF0C		FF0E		FF1A		FF1B	
0xE3808100, 0xE3808200, 0xEFBC8C00, 0xEFBC8E00, 0xEFBC9A00, 0xEFBC9B00,
//	FF1F		FF01		2019		201D		FF09		3015
0xEFBC9F00, 0xEFBC8100, 0xE2809900, 0xE2809D00, 0xEFBC8900, 0xE3809500,
//	FF3D		FF5D		3009		300B		300D		300F
0xEFBCBD00, 0xEFBD9D00, 0xE3808900, 0xE3808B00, 0xE3808D00, 0xE3808F00,
//	3011		3005		30FC		3041		3043		3045
0xE3809100, 0xE3808500, 0xE383BC00, 0xE3818100, 0xE3818300, 0xE3818500,
//	3047		3049		3063		3083		3085		3087
0xE3818700, 0xE3818900, 0xE381A300, 0xE3828300, 0xE3828500, 0xE3828700,
//	308E		30A1		30A3		30A5		30A7		30A9
0xE3828E00, 0xE382A100, 0xE382A300, 0xE382A500, 0xE382A700, 0xE382A900,
//	30C3		30E3		30E5		30E7		30EE		30F5
0xE3838300, 0xE383A300, 0xE383A500, 0xE383A700, 0xE383AE00, 0xE383B500,
//	30F6		309B		309C		30FD		30FE		309D
0xE383B600, 0xE3829B00, 0xE3829C00, 0xE383BD00, 0xE383BE00, 0xE3829D00,
//	309E		2015		2010		00B0		2032		2033
0xE3829E00, 0xE2809500, 0xE2809000, 0xC2B00000, 0xE280B200, 0xE280B300,
//	2103		FF05		FF61		FF63		FF64		FF70
0xE2848300, 0xEFBC8500, 0xEFBDA100, 0xEFBDA300, 0xEFBDA400, 0xEFBDB000,
//	FF9E		FF9F
0xEFBE9E00, 0xEFBE9F00
};
const uint32 kNumBeginTaboos = sizeof(kBeginTaboos) / sizeof(kBeginTaboos[0]);

const uint32 kTerminateTaboos[] = {
//	2018		201C		FF08		3014		FF3B		FF5B
0xE2809800, 0xE2809C00, 0xEFBC8800, 0xE3809400, 0xEFBCBB00, 0xEFBD9B00,
//	3008		300A		300C		300E		3010		FFE5
0xE3808800, 0xE3808A00, 0xE3808C00, 0xE3808E00, 0xE3809000, 0xEFBFA500,
//	FF04		00A2		00A3		FFE0		00A7		3012
0xEFBC8400, 0xC2A20000, 0xC2A30000, 0xEFBCA000, 0xC2A70000, 0xE3809200,
//	FF03		FF62
0xEFBC8300, 0xEFBDA200
};
const uint32 kNumTerminateTaboos = sizeof(kTerminateTaboos) / sizeof(kTerminateTaboos[0]);

#if _SUPPORTS_FEATURE_SCRIPTING
const property_info kPropertyList[] = {
	// actual text data
	{ "Text", 
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_RANGE_SPECIFIER, B_REVERSE_RANGE_SPECIFIER},
		"",
		0,
		{},
		{},
		{}
	},
	// text length
	{ "Text",
		{B_COUNT_PROPERTIES},
		{B_DIRECT_SPECIFIER},
		"",
		0,
		{},
		{},
		{}
	},
	// style info 
	{ "text_run_array", 
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_RANGE_SPECIFIER, B_REVERSE_RANGE_SPECIFIER},
		"",
		0,
		{},
		{},
		{}
	},
	// selection
	{ "selection",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		"",
		0,
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

const rgb_color kClauseLowColor			= {152, 203, 255, 255};
const rgb_color kSelectedClauseLowColor	= {255, 152, 152, 255};


// === Static Member Variables ===

_BWidthBuffer_*	BTextView::sWidths = NULL;
sem_id			BTextView::sWidthSem = -1;
int32			BTextView::sWidthAtom = 0;


// ------------------------------------------------------------
// 	BTextView
// ------------------------------------------------------------
// Default Constructor
//
// textRect is in local coordinates

BTextView::BTextView(
	BRect		frame,
	const char	*name,
	BRect		textRect,
	uint32		resizeMask, 
	uint32		flags)
		: BView(frame, name, resizeMask, 
				(flags | B_FRAME_EVENTS | B_INPUT_METHOD_AWARE) & ~B_PULSE_NEEDED)
{
	InitObject(textRect, NULL, NULL);
}


// ------------------------------------------------------------
// 	BTextView
// ------------------------------------------------------------
//
// inititalStyle is the default style, all fields must be valid

BTextView::BTextView(
	BRect			frame,
	const char		*name,
	BRect			textRect,
	const BFont		*initialStyle,
	const rgb_color	*initialColor,
	uint32			resizeMask,
	uint32			flags)
		: BView(frame, name, resizeMask, 
				(flags | B_FRAME_EVENTS | B_INPUT_METHOD_AWARE) & ~B_PULSE_NEEDED)
{
	InitObject(textRect, initialStyle, initialColor);
}


// ------------------------------------------------------------
// 	BTextView
// ------------------------------------------------------------
// Rehydrating Constructor

BTextView::BTextView(
	BMessage	*data)
		: BView(data)
{
	BRect	r(0.0, 0.0, 0.0, 0.0);
	int32	l = 0;
	bool	b = false;
	float	f = 0.0;
	const char *text = NULL;

	// text rect
	if (data->FindRect(S_TEXT_RECT, &r)!=B_OK) {
		r=BRect(0,0,0,0);
	}
	InitObject(r, NULL, NULL);

	// text	
	if ((data->FindString(S_TEXT, &text)==B_OK)&&(text!=NULL)) {
		SetText(text);
	}

	// alignment
	if (data->FindInt32(S_ALIGN, &l)==B_OK) {
		SetAlignment((alignment)l);
	}

	// tab width
	if (data->FindFloat(S_TAB_WIDTH, &f)==B_OK) {
		SetTabWidth(f);
	}
	
	// max bytes
	if (data->FindInt32(S_MAX, &l)==B_OK) {
//		l=0x7fffffff;
		SetMaxBytes(l);
	}
	
	// wrap
	if (data->FindBool(S_WORD_WRAP, &b)==B_OK) {
		SetWordWrap(b);
	}

	// autoindent
	if (data->FindBool(S_AUTO_INDENT, &b)==B_OK) {
		SetAutoindent(b);
	}
		
	// selectable
	if (data->FindBool(S_NOT_SELECTABLE, &b)==B_OK) {
		MakeSelectable(!b);
	}

	// editable
	if (data->FindBool(S_NOT_EDITABLE, &b)==B_OK) {
		MakeEditable(!b);
	}

	// selection	
	int32 start = 0;
	int32 end = 0;
	if ((data->FindInt32(S_SELECTION, &start)==B_OK)&&(data->FindInt32(S_SELECTION, 1, &end)==B_OK)) {
		Select(start, end);
	}

	// stylable
	if (data->FindBool(S_STYLABLE, &b)==B_OK) {
		SetStylable(b);
	}

	// colorspace
	int32 colSpace;
	if (data->FindInt32(S_COLORSPACE, &colSpace)==B_OK) {
		SetColorSpace((color_space)colSpace);
	}

	// disallowed chars
	int32		size = 0;
	const void	*dChars = NULL;
	if (data->FindData(S_DISALLOWED_CHARS, B_RAW_TYPE, &dChars, &size)==B_OK) {
		int32 numDChars = size / sizeof(void *);
		if (numDChars > 0) {
			if (fDisallowedChars == NULL)
				fDisallowedChars = new BList(1);
			for (int32 i = 0; i < numDChars; i++) 
				fDisallowedChars->AddItem(((void *)((int32 *)dChars)[i]));
		}
	}

	// run array
	int32		raSize = 0;
	const void	*fra = NULL;
	if (data->FindData(S_RUNS, B_RAW_TYPE, &fra, &raSize)==B_OK) {
		text_run_array *tra = UnflattenRunArray(fra, &raSize);
		SetRunArray(0, TextLength(), tra);
		FreeRunArray(tra);
	}
}


// ------------------------------------------------------------
// 	~BTextView
// ------------------------------------------------------------
// Destructor

BTextView::~BTextView()
{
	//LockWidthBuffer();
	//sWidths->CCheck();	// collision stats
	//UnlockWidthBuffer();
	
	StopMouseTracking();
	CancelInputMethod();

	delete (fText);
	delete (fLines);
	delete (fStyles);
	delete (fDisallowedChars);
	delete (fUndo);
	delete (fInline);
	delete (fPulseRunner);
	delete (fClickRunner);
	delete (fDragRunner);
	DeleteOffscreen();
}


// ------------------------------------------------------------
// 	Instantiate
// ------------------------------------------------------------
// Static funtion that instantiates an object of this class

BArchivable*
BTextView::Instantiate(
	BMessage	*data)
{
	if (!validate_instantiation(data, "BTextView"))
		return (NULL);
		
	return (new BTextView(data));
}


// ------------------------------------------------------------
// 	Archive
// ------------------------------------------------------------
// Freeze this object with all its data intact

status_t
BTextView::Archive(
	BMessage	*data, 
	bool		deep) const
{
	BView::Archive(data, deep);
	
//+	data->AddString(B_CLASS_NAME_ENTRY, "BTextView");

	// text	
	data->AddString(S_TEXT, fText->Text());

	// alignment
	if (fAlignment != B_ALIGN_LEFT)
		data->AddInt32(S_ALIGN, fAlignment);

	// tab width
	data->AddFloat(S_TAB_WIDTH, fTabWidth);

	// max bytes
	data->AddInt32(S_MAX, fMaxBytes);

	// atuoindent
	if (fAutoindent)
		data->AddBool(S_AUTO_INDENT, true);

	// wrap
	if (fWrap)
		data->AddBool(S_WORD_WRAP, true);

	// text rect
	data->AddRect(S_TEXT_RECT, fTextRect);

	// selectable
	if (!fSelectable)
		data->AddBool(S_NOT_SELECTABLE, true);

	// editable
	if (!fEditable)
		data->AddBool(S_NOT_EDITABLE, true);

	// selection
	data->AddInt32(S_SELECTION, fSelStart);
	data->AddInt32(S_SELECTION, fSelEnd);

	// disallowed chars
	if (fDisallowedChars != NULL) {
		int32 		count = fDisallowedChars->CountItems();
		const void	*items = fDisallowedChars->Items();
		if (count > 0)
			data->AddData(S_DISALLOWED_CHARS, B_RAW_TYPE, 
						  items, sizeof(void *) * count);
	}

	// stylable
	if (fStylable)
		data->AddBool(S_STYLABLE, true);

	// colorspace
	if (fColorSpaceSet)
		data->AddInt32(S_COLORSPACE, fColorSpace);

	// run array
	text_run_array	*tra = RunArray(0, TextLength());
	int32			size = 0;
	void			*fra = FlattenRunArray(tra, &size);
	data->AddData(S_RUNS, B_RAW_TYPE, fra, size);
	free(fra);
	FreeRunArray(tra);

	return (B_NO_ERROR);
}


// ------------------------------------------------------------
// 	SetText
// ------------------------------------------------------------
// Replace the current text with inText
//
// Pass NULL for inStyles if there is no style data

void
BTextView::SetText(
	const char*				inText, 
	const text_run_array	*inStyles)
{
	// keep on going even if inText == NULL, the existing text 
	// needs to be deleted
	SetText(inText, (inText == NULL) ? 0 : strlen(inText), inStyles);
}


// ------------------------------------------------------------
// 	SetText
// ------------------------------------------------------------
// Replace the current text with inText that is inLength long
//
// Pass NULL for inStyles if there is no style data

void
BTextView::SetText(
	const char				*inText,
	int32					inLength,
	const text_run_array	*inStyles)
{	
	CancelInputMethod();

	bool safeToDraw = (fActive) && (Window() != NULL);
	
	// hide the caret/unhighlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	
	// remove data from buffer
	int32 start = 0;
	if (fText->Length() > 0) {
		_BTextChangeResult_ change;
		DoDeleteText(start, fText->Length(), &change);
		start = change.fDeleteStart;
	}
	
	_BTextChangeResult_ change;
	DoInsertText(inText, inLength, start, inStyles, &change);

	fClickOffset = fSelStart = fSelEnd = change.fInsertOffset;	

	// recalc line breaks and draw the text
	Refresh(change.fInsertOffset, change.fInsertLength, true, true);
	
	// draw the caret
	if ((safeToDraw) && (fEditable)) {
		if (!fCaretVisible)
			InvertCaret();
	}
}


// ------------------------------------------------------------
// 	SetText
// ------------------------------------------------------------
// Replace the current text with the text in inFile
// starting at startOffset and inLength long
//
// Pass NULL for inStyles if there is no style data

void
BTextView::SetText(
	BFile					*inFile,
	int32					startOffset,
	int32					inLength,
	const text_run_array	*inRuns)
{
	CancelInputMethod();

	bool safeToDraw = (fActive) && (Window() != NULL);
	
	// hide the caret/unhighlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	
	// remove data from buffer
	if (fText->Length() > 0) {
		_BTextChangeResult_ change;
		DoDeleteText(0, fText->Length(), &change);
		startOffset += change.fDeleteStart;
	}
		
	if ((inFile != NULL) && (inLength > 0)) {
		// add the text to the buffer
		fText->InsertText(inFile, startOffset, inLength, 0);
	
		// update the start offsets of each line below inOffset
		fLines->BumpOffset(inLength, fLines->OffsetToLine(0) + 1);
	
		// update the style runs
		fStyles->BumpOffset(inLength, fStyles->OffsetToRun(0) + 1);

		if ((fStylable) && (inRuns != NULL)) {
			// SetRunArray(inOffset, inOffset + inLength, inStyles);
			int32 numRuns = inRuns->count;
			if (numRuns > 0) {
				if (!fStylable)
					numRuns = 1;
		
				// pin offsets at reasonable values
				int32 textLength = fText->Length();
				int32 startOffset = 0;
				int32 endOffset = inLength;
				endOffset = (endOffset < 0) ? 0 : endOffset;
				endOffset = (endOffset > textLength) ? textLength : endOffset;
				startOffset = (startOffset < 0) ? 0 : startOffset;
				startOffset = (startOffset > endOffset) ? endOffset : startOffset;
		
				// loop through the style runs
				const text_run *theRun = &inRuns->runs[0];
				for (int32 index = 0; index < numRuns; index++) {
					int32 fromOffset = theRun->offset + startOffset;
					int32 toOffset = endOffset;
					if ((index + 1) < numRuns) {
						toOffset = (theRun + 1)->offset + startOffset;
						toOffset = (toOffset > endOffset) ? endOffset : toOffset;
					}

					BFont theFont(theRun->font);
					NormalizeFont(&theFont);
		
					fStyles->SetStyleRange(fromOffset, toOffset, textLength,
										   B_FONT_ALL, &theFont, &theRun->color);
						
					theRun++;
				}
	
				fStyles->InvalidateNullStyle();
			}
		}
		else {		
			// apply nullStyle to inserted text
			const BFont		*font = NULL;
			const rgb_color	*color = NULL;
			fStyles->SyncNullStyle(0);
			fStyles->GetNullStyle(&font, &color);
			fStyles->SetStyleRange(0, inLength, fText->Length(), 
								   B_FONT_ALL, font, color);
		}
	}

	fClickOffset = fSelStart = fSelEnd = 0;	

	// recalc line breaks and draw the text
	Refresh(0, inLength, true, true);
	
	// draw the caret
	if ((safeToDraw) && (fEditable)) {
		if (!fCaretVisible)
			InvertCaret();
	}
}
	

// ------------------------------------------------------------
// 	Insert
// ------------------------------------------------------------
// Copy inText and insert it at the caret position, or at 
// the beginning of the selection range
// 
// Pass NULL for inStyles if there is no style data
//
// The caret/selection will move with the insertion

void
BTextView::Insert(
	const char				*inText, 
	const text_run_array	*inStyles)
{
	if (inText == NULL)
		return;

	Insert(inText, strlen(inText), inStyles);
}


// ------------------------------------------------------------
// 	Insert
// ------------------------------------------------------------
// Copy inLength bytes from inText and insert it at the caret
// position, or at the beginning of the selection range
// 
// Pass NULL for inStyles if there is no style data
//
// The caret/selection will move with the insertion

void
BTextView::Insert(
	const char				*inText,
	int32					inLength,
	const text_run_array	*inStyles)
{
	// do we really need to do anything?
	if ((inText == NULL) || (inLength < 1))
		return;
	
	// will the new text fit?
	if ((fText->Length() + inLength) > fMaxBytes)
		return;
	
	// are all characters allowed?
	if (fDisallowedChars != NULL) {
		for (int32 i = 0; i < inLength; i++) {
			if (fDisallowedChars->HasItem((void *)inText[i])) {
				beep();
				return;	 
			}	
		}
	}	

	CancelInputMethod();

	bool safeToDraw = (fActive) && (Window() != NULL);

	// hide the caret/unhighlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable)) 
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}

	// copy data into buffer
	_BTextChangeResult_ change;
	DoInsertText(inText, inLength, fSelStart, inStyles, &change);

	// offset the caret/selection
	int32 saveStart = fSelStart;
	fSelEnd = change.fInsertOffset+change.fInsertLength+(fSelEnd-fSelStart);
	fSelStart = change.fInsertOffset+change.fInsertLength;
	fClickOffset = fSelEnd;

	// recalc line breaks and draw the text
	Refresh(saveStart, fSelStart, true, false);

	// draw the caret/highlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if ((!fCaretVisible) && (fEditable))
				InvertCaret();
		}
	}
}


// ------------------------------------------------------------
// 	Insert
// ------------------------------------------------------------
// Insert text that is length long at startOffset

void
BTextView::Insert(
	int32					startOffset,
	const char				*inText,
	int32					inLength,
	const text_run_array	*inStyles)
{
	// do we really need to do anything?
	if ((inText == NULL) || (inLength < 1))
		return;
	
	// will the new text fit?
	if ((fText->Length() + inLength) > fMaxBytes)
		return;
	
	// are all characters allowed?
	if (fDisallowedChars != NULL) {
		for (int32 i = 0; i < inLength; i++) {
			if (fDisallowedChars->HasItem((void *)inText[i])) {
				beep();
				return;	 
			}	
		}
	}

	CancelInputMethod();

	bool safeToDraw = (fActive) && (Window() != NULL);

	// hide the caret/unhighlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable)) 
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}

	// copy data into buffer
	_BTextChangeResult_ change;
	DoInsertText(inText, inLength, startOffset, inStyles, &change);

	// offset the caret/selection
	if (fSelStart >= change.fInsertOffset)	
		fSelStart += change.fInsertLength;
	if (fSelEnd >= change.fInsertOffset)
		fSelEnd += change.fInsertLength;
	fClickOffset = fSelEnd;

	// recalc line breaks and draw the text
	Refresh(change.fInsertOffset,
			change.fInsertOffset + change.fInsertLength,
			true, false);

	// draw the caret/highlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if ((!fCaretVisible) && (fEditable))
				InvertCaret();
		}
	}
}


// ------------------------------------------------------------
// 	Delete
// ------------------------------------------------------------
// Delete the current selection

void
BTextView::Delete()
{
	// anything to delete?
	if (fSelStart == fSelEnd)
		return;
	
	CancelInputMethod();

	bool safeToDraw = (fActive) && (Window() != NULL);

	// hide the caret/unhighlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	
	// remove data from buffer
	_BTextChangeResult_ change;
	DoDeleteText(fSelStart, fSelEnd, &change);
	
	// collapse the selection
	fClickOffset = fSelEnd = fSelStart = change.fDeleteStart;
	
	// recalc line breaks and draw what's left
	Refresh(fSelStart, fSelEnd, true, true);

	// draw the caret
	if (safeToDraw) {
		if ((!fCaretVisible) && (fEditable))
			InvertCaret();
	}
}


// ------------------------------------------------------------
// 	Delete
// ------------------------------------------------------------
// The text starting at startOffset, up to endOffset

void
BTextView::Delete(
	int32	startOffset,
	int32	endOffset)
{
	int32 textLen = fText->Length();

	// sanity checking
	startOffset = (startOffset < 0) ? 0 : startOffset;
	endOffset = (endOffset > textLen) ? textLen : endOffset;

	// anything to delete?
	if (startOffset == endOffset)
		return;

	CancelInputMethod();
		
	bool safeToDraw = (fActive) && (Window() != NULL);

	// hide the caret/unhighlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	
	// remove data from buffer
	_BTextChangeResult_ change;
	DoDeleteText(startOffset, endOffset, &change);

	// make sure selection is still valid
	int32 selDelta = (fSelStart > change.fDeleteStart)
					? fSelStart - change.fDeleteStart : 0;
	fSelStart -= selDelta;
	if (fSelEnd > change.fDeleteEnd)
		fSelEnd -= fSelEnd - change.fDeleteEnd;
	fSelEnd -= selDelta;
	if ((fSelEnd == change.fDeleteEnd) && (fSelEnd == textLen))		// yuck
		fSelEnd--;
	fClickOffset = fSelEnd;

	// recalc line breaks and draw what's left
	Refresh(change.fDeleteStart, change.fDeleteStart, true, true);

	// draw the caret/highlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if ((!fCaretVisible) && (fEditable))
				InvertCaret();
		}
	}
}


// ------------------------------------------------------------
// 	Text
// ------------------------------------------------------------
// Return a pointer to the text, NULL if error
//
// Do not free the returned text or alter it in any way
//
// The pointer that is returned may become invalid after a
// subsequent call to any other BTextView function
// Copy it into your own buffer or use BTextView::GetText() if 
// you need the text for an extended period of time
//
// The text is null terminated

const char*
BTextView::Text() const
{
	return (fText->RealText());
}


// ------------------------------------------------------------
// 	TextLength
// ------------------------------------------------------------
// Return the length of the text buffer
//
// The length does not include the null terminator

int32
BTextView::TextLength() const
{
	return (fText->Length());
}


// ------------------------------------------------------------
// 	GetText
// ------------------------------------------------------------
// Copy into buffer up to length characters of the text 
// starting at offset
//
// The text is null terminated

void
BTextView::GetText(
	int32 	offset,
	int32 	length,
	char	*buffer) const
{
	int32 textLen = fText->Length();
	
	if ((offset < 0) || (offset > (textLen - 1))) {
		buffer[0] = '\0';
		return;
	}
		
	length = ((offset + length) > textLen) ? textLen - offset : length;
	for (int32 i = 0; i < length; i++)
		buffer[i] = (*fText)[offset + i];
	buffer[length] = '\0';
}


// ------------------------------------------------------------
// 	ByteAt
// ------------------------------------------------------------
// Return the character at offset

uchar
BTextView::ByteAt(
	int32	offset) const
{
	if ((offset < 0) || (offset > (fText->Length() - 1)))
		return ('\0');
		
	return ((*fText)[offset]);
}


// ------------------------------------------------------------
// 	CountLines
// ------------------------------------------------------------
// Return the number of lines

int32
BTextView::CountLines() const
{
	int32 numLines = fLines->NumLines();
	
	int32 textLength = fText->Length();
	if (textLength > 0) {
		// add a line if the last character is a newline
		if ((*fText)[textLength - 1] == '\n')
			numLines++;
	}
	
	return (numLines);
}


// ------------------------------------------------------------
// 	CurrentLine
// ------------------------------------------------------------
// Return the line number of the caret or the beginning of
// the selection
//
// Line numbers start at 0

int32
BTextView::CurrentLine() const
{
	// call LineAt() instead of _BLineBuffer_::OffsetToLine()
	return (LineAt(fSelStart));
}


// ------------------------------------------------------------
// 	GoToLine
// ------------------------------------------------------------
// Move the caret to the beginning of lineNum
//
// This function does not automatically scroll the view, 
// use ScrollToSelection() if you want to ensure that lineNum 
// is visible
//
// Line numbers start at 0

void
BTextView::GoToLine(
	int32	lineNum)
{
	if (!fEditable)
		return;

	CancelInputMethod();

	bool safeToDraw = (fActive) && (Window() != NULL);

	// hide the caret/unhighlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	
	int32 saveLineNum = lineNum;
	int32 maxLine = fLines->NumLines() - 1;
	lineNum = (lineNum > maxLine) ? maxLine : lineNum;
	lineNum = (lineNum < 0) ? 0 : lineNum;
	
	fClickOffset = fSelStart = fSelEnd = (*fLines)[lineNum]->offset;
	
	int32 textLength = fText->Length();
	if ((textLength > 0) && (saveLineNum > maxLine)) {
		// add a line if the last character is a newline
		if ((*fText)[textLength - 1] == '\n')
			fClickOffset = fSelStart = fSelEnd = textLength;
	}

	if (safeToDraw)	
		InvertCaret();
}


// ------------------------------------------------------------
// 	Cut
// ------------------------------------------------------------
// Copy the current selection into the clipboard and delete it
// from the buffer

void
BTextView::Cut(
	BClipboard	*clipboard)
{
	if (!fEditable)
		return;

	CancelInputMethod();

	if (fUndo != NULL) {
		delete (fUndo);
		fUndo = new _BCutUndoBuffer_(this);	
	}

	int32	start = fSelStart;
	int32	end = fSelEnd;
	if(fText->PasswordMode())
	{
		// convert offset in real string to offset in password string
		start = fText->Chars(start) * kPasswordGlyphLen;
		end = fText->Chars(end) * kPasswordGlyphLen;
	}

	if (start >= end) {
		// nothing selected, don't kill what is currently in the
		// clipboard and just bail
		return;
	}
	
	clipboard->Lock();
	clipboard->Clear();
	
	// add text
	BMessage *clip_msg = clipboard->Data();
	// TODO selstart/selend
	clip_msg->AddData("text/plain", B_MIME_TYPE, fText->Text() + start,
					  end - start);
	
	if (fStylable) {
		// add corresponding styles
		int32 			length = 0;
		text_run_array	*styles = RunArray(fSelStart, fSelEnd, &length);

		// XXX Gross Hack.  Remove overlay information, so we don't
		// stuff fonts with fExtra pointers into the message.
		text_run* run = styles->runs;
		for (int32 i=0; i<styles->count; i++, run++)
			run->font.RemoveAllOverlays();
			
		clip_msg->AddData("application/x-vnd.Be-text_run_array", 
						  B_MIME_TYPE, styles, length);
	
		FreeRunArray(styles);
	}

	clipboard->Commit();
	clipboard->Unlock();
	
	Delete();
}


// ------------------------------------------------------------
// 	Copy
// ------------------------------------------------------------
// Copy the current selection into the clipboard

void
BTextView::Copy(
	BClipboard	*clipboard)
{
	CancelInputMethod();

	int32	start = fSelStart;
	int32	end = fSelEnd;
	if(fText->PasswordMode())
	{
		// convert offset in real string to offset in password string
		start = fText->Chars(start) * kPasswordGlyphLen;
		end = fText->Chars(end) * kPasswordGlyphLen;
	}
	
	if (start >= end) {
		// nothing selected, don't kill what is currently in the
		// clipboard and just bail
		return;
	}
	
	clipboard->Lock();
	clipboard->Clear();
	
	// add text
	BMessage *clip_msg = clipboard->Data();
	clip_msg->AddData("text/plain", B_MIME_TYPE, fText->Text() + start,
					  end - start);
	
	if (fStylable) {
		// add corresponding styles
		int32 			length = 0;
		text_run_array	*styles = RunArray(fSelStart, fSelEnd, &length);

		// XXX Gross Hack.  Remove overlay information, so we don't
		// stuff fonts with fExtra pointers into the message.
		text_run* run = styles->runs;
		for (int32 i=0; i<styles->count; i++, run++)
			run->font.RemoveAllOverlays();
			
		clip_msg->AddData("application/x-vnd.Be-text_run_array", 
						  B_MIME_TYPE, styles, length);
	
		FreeRunArray(styles);
	}
	
	clipboard->Commit();
	clipboard->Unlock();
}


// ------------------------------------------------------------
// 	Paste
// ------------------------------------------------------------
// Copy the contents of the clipboard into the text buffer

void
BTextView::Paste(
	BClipboard	*clipboard)
{
	clipboard->Lock();

	// do we really want what's in the clipboard?
	if (!AcceptsPaste(clipboard)) {
		clipboard->Unlock();
		return;
	}

	CancelInputMethod();
	
	// get data from clipboard
	const char		*text = NULL;
	int32			textLen = 0;
	text_run_array	*styles = NULL;
	int32			styleLen = 0;
	BMessage		*clip_msg = clipboard->Data();

	clip_msg->FindData("text/plain", B_MIME_TYPE, (const void **)&text, &textLen);
	
	if( text == NULL && fSelStart == fSelEnd ) {
		// if there is no text in message and nothing is selected in the
		// control, there is nothing to do.
		return;
	}
	
	if (fStylable) {
		clip_msg->FindData("application/x-vnd.Be-text_run_array", 
						   B_MIME_TYPE, (const void **)&styles, &styleLen);
	}

	if (fUndo != NULL) {
		delete (fUndo);
		fUndo = new _BPasteUndoBuffer_(this, text, textLen, styles, styleLen);
	}

	// unhighlight and delete the selection
	if (fSelStart != fSelEnd) {
		if ((fActive) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
	
		_BTextChangeResult_ change;
		DoDeleteText(fSelStart, fSelEnd, &change);
		fClickOffset = fSelEnd = fSelStart = change.fDeleteStart;
	}
		
	// copy text and styles into the buffers
	Insert(text, textLen, styles);
	ScrollToOffset(fSelEnd);

	clipboard->Unlock();
}


// ------------------------------------------------------------
// 	Clear
// ------------------------------------------------------------
// Undo-able delete of the current selection 

void
BTextView::Clear()
{
	if (fUndo != NULL) {
		delete (fUndo);
		fUndo = new _BClearUndoBuffer_(this);
	}	

	Delete();
}


// ------------------------------------------------------------
// 	AcceptsPaste
// ------------------------------------------------------------
// Return true if the clipboard contains data that can inserted 

bool
BTextView::AcceptsPaste(
	BClipboard	*clipboard)
{
	if (!fEditable)
		return (false);
		
	bool result = false;
	
	clipboard->Lock();
	
	BMessage *clip_msg = clipboard->Data();
	result = clip_msg->HasData("text/plain", B_MIME_TYPE);
	
	clipboard->Unlock();
	
	return (result);
}


// ------------------------------------------------------------
// 	AcceptsDrop
// ------------------------------------------------------------
// Return true if the message contains data that can be dropped

bool
BTextView::AcceptsDrop(
	const BMessage	*inMessage)
{
	// HACK!!!
	if (inMessage->what == B_MESSAGE_NOT_UNDERSTOOD)
		return (false);

	if (!fEditable)
		return (false);

	if (inMessage->HasData("text/plain", B_MIME_TYPE))
		return (true);
		
	return (false);
}


// ------------------------------------------------------------
// 	Select
// ------------------------------------------------------------
// Select (and highlight) a range of text

void
BTextView::Select(
	int32	startOffset,
	int32	endOffset)
{
	if (Window() == NULL)
		return;
		
	// a negative selection?
	if (startOffset > endOffset)
		return;
		
	// pin offsets at reasonable values
	int32 textLen = fText->Length();
	startOffset = (startOffset < 0) ? 0 : startOffset;
	startOffset = (startOffset > textLen) ? textLen : startOffset;
	endOffset = (endOffset < 0) ? 0 : endOffset;
	endOffset = (endOffset > textLen) ? textLen : endOffset;
	startOffset = (startOffset > endOffset) ? endOffset : startOffset;

	// is the new selection any different from the current selection?
	if ((startOffset == fSelStart) && (endOffset == fSelEnd))
		return;
	
	CancelInputMethod();

	fStyles->InvalidateNullStyle();
	
	// hide the caret
	if (fCaretVisible)
		InvertCaret();
	
	if (startOffset == endOffset) {
		if (fSelStart != fSelEnd) {
			// unhighlight the selection
			if ((fActive) && (fSelectable))
				Highlight(fSelStart, fSelEnd); 
		}
		fClickOffset = fSelStart = fSelEnd = startOffset;
		if ((fActive) && (fEditable))
			InvertCaret();
	}
	else {	
		if ((fActive) && (fSelectable)) {
			// does the new selection overlap with the current one?
			if ( ((startOffset < fSelStart) && (endOffset < fSelStart)) ||
				 ((endOffset > fSelEnd) && (startOffset > fSelEnd)) ) {
				// they don't overlap, don't bother with stretching
				// thanks to Brian Stern for the code snippet
				Highlight(fSelStart, fSelEnd);
				Highlight(startOffset, endOffset);	
			}
			else {
				// stretch the selection, draw only what's different
				int32 start, end;
				if (startOffset != fSelStart) {
					// start of selection has changed
					if (startOffset > fSelStart) {
						start = fSelStart;
						end = startOffset;
					}
					else {
						start = startOffset;
						end = fSelStart;
					}
					Highlight(start, end);
				}
				
				if (endOffset != fSelEnd) {
					// end of selection has changed
					if (endOffset > fSelEnd) {
						start = fSelEnd;
						end = endOffset;
					}
					else {
						start = endOffset;
						end = fSelEnd;
					}
					Highlight(start, end);
				}
			}
		}
		fSelStart = startOffset;
		fSelEnd = endOffset;
		fClickOffset = fSelEnd;
	}
}


// ------------------------------------------------------------
// 	SelectAll
// ------------------------------------------------------------
// Select everything

void
BTextView::SelectAll()
{
	if (fSelectable)
		Select(0, fText->Length());
}


// ------------------------------------------------------------
// 	GetSelection
// ------------------------------------------------------------
// Pass back the current selection offsets

void
BTextView::GetSelection(
	int32	*outStart,
	int32 	*outEnd) const
{
	*outStart = fSelStart;
	*outEnd = fSelEnd;
}


// ------------------------------------------------------------
// 	SetFontAndColor
// ------------------------------------------------------------
// Set the font and color for the current selection
//
// inMode specifies the pertinent fields of inStyle

void
BTextView::SetFontAndColor(
	const BFont		*inStyle,
	uint32			inMode,
	const rgb_color	*inColor)
{
	SetFontAndColor(fSelStart, fSelEnd, inStyle, inMode, inColor);
}


// ------------------------------------------------------------
// 	SetFontAndColor
// ------------------------------------------------------------
// Set the style for the range startOffset - endOffset
//
// inMode specifies the pertinent fields of inStyle
	
void
BTextView::SetFontAndColor(
	int32			startOffset,
	int32			endOffset,
	const BFont		*inStyle,
	uint32			inMode,
	const rgb_color	*inColor)
{
	CancelInputMethod();

	int32 textLen = fText->Length();

	// set everything if we're not stylable
	if (!fStylable) {
		startOffset = 0;
		endOffset = textLen;
	}

	// sanity checking
	startOffset = (startOffset < 0) ? 0 : startOffset;
	startOffset = (startOffset > textLen) ? textLen : startOffset;
	endOffset = (endOffset < 0) ? 0 : endOffset;
	endOffset = (endOffset > textLen) ? textLen : endOffset; 
	startOffset = (startOffset > endOffset) ? endOffset : startOffset;

	bool safeToDraw = (fActive) && (Window() != NULL);

	// hide the caret/unhighlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	

	BFont theFont;
	if (inStyle != NULL) { 
		theFont = *inStyle;
		NormalizeFont(&theFont);
		inStyle = &theFont;
	}

	// add the style to the style buffer
	fStyles->SetStyleRange(startOffset, endOffset, fText->Length(), 
						   inMode, inStyle, inColor);
						
	if ((inMode & B_FONT_FAMILY_AND_STYLE) || (inMode & B_FONT_SIZE))
		// recalc the line breaks and redraw with new style
		Refresh(startOffset, endOffset, startOffset != endOffset, false);
	else
		if (safeToDraw) {
			// the line breaks wont change, simply redraw
			PushState();
			DrawLines(fLines->OffsetToLine(startOffset), 
					  fLines->OffsetToLine(endOffset), 
					  startOffset, true);
			PopState();
		}
	
	// draw the caret/highlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if ((!fCaretVisible) && (fEditable))
				InvertCaret();
		}
	}
}


// ------------------------------------------------------------
// 	SetRunArray
// ------------------------------------------------------------
// Set the styles of a range of text

void
BTextView::SetRunArray(
	int32					startOffset,
	int32					endOffset,
	const text_run_array	*inRuns)
{
	int32 numRuns = inRuns->count;
	if (numRuns < 1)
		return;

	CancelInputMethod();

	if (!fStylable)
		numRuns = 1;
	
	bool safeToDraw = (fActive) && (Window() != NULL);
		
	// hide the caret/unhighlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	
	// pin offsets at reasonable values
	int32 textLength = fText->Length();
	endOffset = (endOffset < 0) ? 0 : endOffset;
	endOffset = (endOffset > textLength) ? textLength : endOffset;
	startOffset = (startOffset < 0) ? 0 : startOffset;
	startOffset = (startOffset > endOffset) ? endOffset : startOffset;
	
	// loop through the style runs
	const text_run *theRun = &inRuns->runs[0];
	for (int32 index = 0; index < numRuns; index++) {
		int32 fromOffset = theRun->offset + startOffset;
		int32 toOffset = endOffset;
		if ((index + 1) < numRuns) {
			toOffset = (theRun + 1)->offset + startOffset;
			toOffset = (toOffset > endOffset) ? endOffset : toOffset;
		}

		BFont theFont(theRun->font);
		NormalizeFont(&theFont);
		
		fStyles->SetStyleRange(fromOffset, toOffset, textLength,
							   B_FONT_ALL, &theFont, &theRun->color);
						
		theRun++;
	}
	
	fStyles->InvalidateNullStyle();
	
	Refresh(startOffset, endOffset, true, false);
		
	// draw the caret/highlight the selection
	if (safeToDraw) {
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if ((!fCaretVisible) && (fEditable))
				InvertCaret();
		}
	}
}


// ------------------------------------------------------------
// 	GetFontAndColor
// ------------------------------------------------------------
// Get the font and color at inOffset

void
BTextView::GetFontAndColor(
	int32		inOffset,
	BFont		*outFont,
	rgb_color	*outColor) const
{
	fStyles->GetStyle(inOffset, outFont, outColor);
}


// ------------------------------------------------------------
// 	RunArray
// ------------------------------------------------------------
// Return the styles of a range of text
//
// You are responsible for freeing the buffer that is returned
//
// Pass NULL for outLength if you are not interested in the
// length of the buffer

text_run_array*
BTextView::RunArray(
	int32	startOffset,
	int32	endOffset,
	int32	*outLength) const
{
	text_run_array *result = fStyles->GetStyleRange(startOffset, 
													endOffset - 1);
	if (outLength != NULL)
		*outLength = sizeof(int32) + (sizeof(text_run) * result->count);
	
	return (result);
}


// ------------------------------------------------------------
// 	GetFontAndColor
// ------------------------------------------------------------
// Return the font and color settings of the selection range 
//
// outStyle will be set with the continuous attributes only

void
BTextView::GetFontAndColor(
	BFont		*outFont,
	uint32		*outMode,
	rgb_color	*outColor,
	bool		*outEqColor) const
{
	if ((fSelStart == fSelEnd) && (fStyles->IsValidNullStyle())) {
		const BFont		*font = NULL;
		const rgb_color	*color = NULL;
		fStyles->GetNullStyle(&font, &color);
		if (outFont != NULL)
			*outFont = *font;
		if (outColor != NULL)
			*outColor = *color;
		if (outMode != NULL)
			*outMode = B_FONT_ALL;
		if (outEqColor != NULL)
			*outEqColor = true;
	}
	else
		fStyles->ContinuousGetStyle(outFont, outMode, outColor, outEqColor,
									fSelStart, fSelEnd);
}


// ------------------------------------------------------------
// 	LineAt
// ------------------------------------------------------------
// Return the number of the line in the line array that 
// contains offset
// 
// Line numbers start at 0

int32
BTextView::LineAt(
	int32	offset) const
{
	int32 lineNum = fLines->OffsetToLine(offset);
	
	int32 textLength = fText->Length();
	if ((textLength > 0) && (offset == textLength)) {
		// add a line if the last character is a newline
		if ((*fText)[textLength - 1] == '\n')
			lineNum++;
	}
	
	return (lineNum);
}


// ------------------------------------------------------------
// 	LineAt
// ------------------------------------------------------------
// Return the number of the line at the vertical location
// point should be in local coordinates
// 
// Line numbers start at 0

int32
BTextView::LineAt(
	BPoint	point) const
{
	int32 lineNum = fLines->PixelToLine(point.y - fTextRect.top);

	int32 offset = (*fLines)[lineNum + 1]->offset;
	int32 textLength = fText->Length();
	if ((textLength > 0) && (offset == textLength)) {
		// add a line if the last character is a newline
		if ((*fText)[textLength - 1] == '\n')
			lineNum++;
	}

	return (lineNum);
}


// ------------------------------------------------------------
// 	PointAt
// ------------------------------------------------------------
// Return the local coordinates of the character at inOffset
// Pass back the height of the line that contains inOffset in outHeight
//
// The vertical coordinate will be at the origin (top) of the line
// The horizontal coordinate will be to the left of the character
// at offset
//
// Pass NULL for outHeight if you do not need to know the height

BPoint
BTextView::PointAt(
	int32	inOffset,
	float	*outHeight) const
{
	BPoint 			result;
	int32			textLength = fText->Length();
	int32			lineNum = fLines->OffsetToLine(inOffset);	
	const STELine	*line = (*fLines)[lineNum];
	float 			height = (line + 1)->origin - line->origin;

	result.x = 0.0;
	result.y = line->origin + fTextRect.top;
	
	if (textLength > 0) {
		// special case: go down one line if inOffset is a newline
		if ((inOffset == textLength) && ((*fText)[textLength - 1] == '\n')) {
			float ascent = 0.0;
			float descent = 0.0;

			StyledWidth(inOffset - 1, 1, &ascent, &descent);

			result.y += height;
			height = (ascent + descent);
			
			line++;
		}
		else {
			int32	offset = line->offset;
			int32	length = inOffset - line->offset;
			int32	numChars = length;
			bool	foundTab = false;		
			do {
				foundTab = fText->FindChar('\t', offset, &numChars);
			
				result.x += StyledWidth(offset, numChars);
		
				if (foundTab) {
					int32 numTabs = 0;
					for (numTabs = 0; (numChars + numTabs) < length; numTabs++) {
						if ((*fText)[offset + numChars + numTabs] != '\t')
							break;
					}
											
					float tabWidth = ActualTabWidth(result.x);
					if (numTabs > 1)
						tabWidth += ((numTabs - 1) * fTabWidth);
		
					result.x += tabWidth;
					numChars += numTabs;
				}
				
				offset += numChars;
				length -= numChars;
				numChars = length;
			} while ((foundTab) && (length > 0));
		} 
	}

	// take alignment into account
	switch (fAlignment) {
		case B_ALIGN_RIGHT:
			result.x += fTextRect.Width() - line->width;
			break;
				
		case B_ALIGN_CENTER:
			result.x += (fTextRect.Width() - line->width) / 2;
			break;
		
		case B_ALIGN_LEFT:
		default:	
			break;
	}

	// convert from text rect coordinates
	result.x += fTextRect.left;

	if (outHeight != NULL)
		*outHeight = height;
		
	return (result);
}


// ------------------------------------------------------------
// 	OffsetAt
// ------------------------------------------------------------
// Return the offset of the character that lies at point
// 
// point should be in local coordinates

int32
BTextView::OffsetAt(
	BPoint	point) const
{
	// should we even bother?
	if (point.y >= fTextRect.bottom)
		return (fText->Length());
	else {
		if (point.y < fTextRect.top)
			return (0);
	}

	int32			lineNum = fLines->PixelToLine(point.y - fTextRect.top);
	const STELine	*line = (*fLines)[lineNum];

	// take alignment into account
	switch (fAlignment) {
		case B_ALIGN_RIGHT:
			point.x -= fTextRect.Width() - line->width;
			break;
				
		case B_ALIGN_CENTER:
			point.x -= (fTextRect.Width() - line->width) / 2;
			break;
		
		case B_ALIGN_LEFT:
		default:
			break;
	}
	
	// special case: if point is within the text rect and PixelToLine()
	// tells us that it's on the last line, but if point is actually  
	// lower than the bottom of the last line, return the last offset 
	// (can happen for newlines)
	if (lineNum == (fLines->NumLines() - 1)) {
		if (point.y >= ((line + 1)->origin + fTextRect.top))
			return (fText->Length());
	}
	
	// convert to text rect coordinates
	point.x -= (fTextRect.left + 1.0);
	point.x = (point.x < 0.0) ? 0.0 : point.x;

	int32	offset = line->offset;
	int32	limit = (line + 1)->offset;
	float	sigmaWidth = 0.0;
	bool	done = false;

	do {
		int32	numBytes = limit - offset;
		bool	foundTab = fText->FindChar('\t', offset, &numBytes);

		while (numBytes > 0) {
			int32	charLen = UTF8CharLen(fText->RealCharAt(offset));
			float	charWidth = StyledWidth(offset, charLen);
	
			if (point.x < (sigmaWidth + charWidth)) {
				if (point.x > (sigmaWidth + (charWidth / 2))) 
					offset += charLen;
				done = true;
				break;
			}

			sigmaWidth += charWidth;
			numBytes -= charLen;
			offset += charLen;
		}

		if ((!done) && (foundTab)) {
			float tabWidth = ActualTabWidth(sigmaWidth);
			
			// is the point in the left-half of the tab?
			if (point.x < (sigmaWidth + (tabWidth / 2))) {
				done = true;
				break;
			}
			else {
				// is the point in the right-half of the tab?
				if (point.x < (sigmaWidth + tabWidth)) {
					offset++;
					done = true;
					break;
				}
			}
				
			sigmaWidth += tabWidth;
			offset++;
			
			// maybe we have more tabs?
			while ((offset < limit) && ((*fText)[offset] == '\t')) {
				if (point.x < (sigmaWidth + (fTabWidth / 2))) {
					done = true;
					break;
				}
				else {
					if (point.x < (sigmaWidth + fTabWidth)) {
						offset++;
						done = true;
						break;
					}
				}
				
				sigmaWidth += fTabWidth;
				offset++;
			}
		}
	} while ((offset < limit) && (!done));
	
	if (offset == (line + 1)->offset && offset > 0) {
		// special case: newlines aren't visible (duh)
		// return the offset of the character preceding the newline
		if ((*fText)[offset - 1] == '\n')
			return (offset - 1);

		// special case: return the offset preceding any spaces that 
		// aren't at the end of the buffer
		if ((offset != fText->Length()) && ((*fText)[offset - 1] == ' '))
			return (offset - 1);
	}

	return (offset);
}


// ------------------------------------------------------------
// 	OffsetAt
// ------------------------------------------------------------
// Return the offset of the character that starts the line line

int32
BTextView::OffsetAt(
	int32	line) const
{
	int32 lineCount = fLines->NumLines();
	line = (line < 0) ? 0 : line;
	line = (line > lineCount) ? lineCount : line;

	return ((*fLines)[line]->offset);
}

// ------------------------------------------------------------
// 	FindWord
// ------------------------------------------------------------
// Return a pair of offsets that describe a word at inOffset

void
BTextView::FindWord(
	int32	inOffset, 
	int32	*outFromOffset,
	int32	*outToOffset)
{
	int32 textLen = fText->Length();

	// sanity checking
	if ((inOffset < 0) || (inOffset > textLen - 1))
		return;

	*outFromOffset = inOffset;
	*outToOffset = inOffset;

	uint32 saveClass = CharClassification(inOffset);

	// check to the left
	for (int32 offset = inOffset; offset >= 0; offset--) {
		uchar theChar = fText->RealCharAt(offset);
		if (IsInitialUTF8Byte(theChar)) {
			if (CharClassification(offset) != saveClass)
				break;
			*outFromOffset = offset;
		}
	}

	// check to the right
	for (int32 offset = inOffset; offset < textLen; offset++) {
		uchar theChar = fText->RealCharAt(offset);
		if (IsInitialUTF8Byte(theChar)) {
			*outToOffset = offset;
			if (CharClassification(offset) != saveClass)
				break;

			// do this in case this is the last char
			*outToOffset += UTF8CharLen(theChar);
		}
	}

	// sanity checking...
	*outToOffset = (*outToOffset > textLen) ? textLen : *outToOffset;
	*outFromOffset = (*outFromOffset > *outToOffset) ? *outToOffset : 
													   *outFromOffset;
	*outFromOffset = (*outFromOffset < 0) ? 0 : *outFromOffset;
}


// ------------------------------------------------------------
// 	CanEndLine
// ------------------------------------------------------------
// Return true if offset can be the last char on a line

bool
BTextView::CanEndLine(
	int32	offset)
{
	if (offset < 0)
		return (true);
	
	uchar	theByte = (*fText)[offset];
	int32	textLen = fText->Length();
	int32	charLen = UTF8CharLen(fText->RealCharAt(offset));

	// sanity checking
	if ((offset + charLen) > textLen)
		return (true);	// for the hell of it

	if (charLen == 1) {
		// 7-bit ASCII
		switch (theByte) {
			case '\0':
			case '\t':
			case '\n':
			case ' ':
			case '&':
			case '*':
			case '+':
			case '-':
			case '/':
			case '<':
			case '=':
			case '>':
			case '\\':
			case '^':
			case '|':
				return (true);
			
			default:
				return (false);
		}
	}

	uint32 theChar = fText->UTF8CharToUint32(offset, charLen);

	// can this character end a line?
	for (uint32 i = 0; i < kNumTerminateTaboos; i++) {
		if (theChar == kTerminateTaboos[i])
			return (false);
	}
	
	// can the next character start a line?
	int32 nextCharOffset = offset + charLen;
	if (nextCharOffset < textLen) {
		int32 nextCharLen = UTF8CharLen(fText->RealCharAt(nextCharOffset));

		// sanity checking
		if ((nextCharOffset + nextCharLen) > textLen) 
			return (true);	// for the hell of it
		else {
			uint32 theNextChar = fText->UTF8CharToUint32(nextCharOffset, nextCharLen);

			for (uint32 i = 0; i < kNumBeginTaboos; i++) {
				if (theNextChar == kBeginTaboos[i])
					return (false);
			}
		}
	}

	// CJK symbols/puctuation, full/half-widths, hiragana, katakana, ideographs
	if ( ((theChar >= 0xE3808000) && (theChar <= 0xE380BF00)) ||
		 ((theChar >= 0xEFBC8000) && (theChar <= 0xEFBFAF00)) ||
		 ((theChar >= 0xE3818000) && (theChar <= 0xE3829F00)) ||
		 ((theChar >= 0xE382A000) && (theChar <= 0xE383BF00)) ||
		 ((theChar >= 0xE4B88000) && (theChar <= 0xE9BFBF00)) ||
		 ((theChar >= 0xEFA48000) && (theChar <= 0xEFABBF00)) ) 
		return (true);

	return (false);
}


// ------------------------------------------------------------
// 	LineWidth
// ------------------------------------------------------------
// Return the width of the line with an index of lineNum

float
BTextView::LineWidth(
	int32	lineNum) const
{
	if ((lineNum < 0) || (lineNum >= fLines->NumLines()))
		return (0.0);
		
	return ((*fLines)[lineNum]->width);
}


// ------------------------------------------------------------
// 	LineHeight
// ------------------------------------------------------------
// Return the height of the line with an index of lineNum

float
BTextView::LineHeight(
	int32	lineNum) const
{
	return (TextHeight(lineNum, lineNum));
}


// ------------------------------------------------------------
// 	TextHeight
// ------------------------------------------------------------
// Return the height from startLine to endLine
						
float
BTextView::TextHeight(
	int32	startLine,
	int32	endLine) const
{
	int32 lastChar = fText->Length() - 1;
	int32 lastLine = fLines->NumLines() - 1;
	startLine = (startLine < 0) ? 0 : startLine;
	endLine = (endLine > lastLine) ? lastLine : endLine;

	float height = (*fLines)[endLine + 1]->origin - 
				   (*fLines)[startLine]->origin;

	if ((endLine == lastLine) && (lastChar >= 0) && ((*fText)[lastChar] == '\n')) {
		float ascent = 0.0;
		float descent = 0.0;

		StyledWidth(lastChar, 1, &ascent, &descent);
		
		height += (ascent + descent);
	}
	
	return (height);
}


// ------------------------------------------------------------
// 	GetTextRegion
// ------------------------------------------------------------
// Return the region that encompasses the characters in the
// range of startOffset to endOffset

void
BTextView::GetTextRegion(
	int32	startOffset,
	int32	endOffset,
	BRegion	*outRegion) const
{
	outRegion->MakeEmpty();

	// return an empty region if the range is invalid
	if (startOffset >= endOffset)
		return;

	float	startLineHeight = 0.0;
	float	endLineHeight = 0.0;
	BPoint	startPt = PointAt(startOffset, &startLineHeight);
	startPt.x = ceil(startPt.x);
	startPt.y = ceil(startPt.y);
	BPoint	endPt = PointAt(endOffset, &endLineHeight);
	endPt.x = ceil(endPt.x);
	endPt.y = ceil(endPt.y);
	BRect	selRect;

	if (startPt.y == endPt.y) {
		// this is a one-line region
		selRect.left = (startPt.x < fTextRect.left) ? fTextRect.left : startPt.x;
		selRect.top = startPt.y;
		selRect.right = endPt.x - 1.0;
		selRect.bottom = endPt.y + endLineHeight - 1.0;
		outRegion->Include(selRect);
	}
	else {
		// more than one line in the specified offset range
		selRect.left = (startPt.x < fTextRect.left) ? fTextRect.left : startPt.x;
		selRect.top = startPt.y;
		selRect.right = fTextRect.right;
		selRect.bottom = startPt.y + startLineHeight - 1.0;
		outRegion->Include(selRect);
		
		if ((startPt.y + startLineHeight) < endPt.y) {
			// more than two lines in the range
			selRect.left = fTextRect.left;
			selRect.top = startPt.y + startLineHeight;
			selRect.right = fTextRect.right;
			selRect.bottom = endPt.y - 1.0;
			outRegion->Include(selRect);
		}
		
		selRect.left = fTextRect.left;
		selRect.top = endPt.y;
		selRect.right = endPt.x - 1.0;
		selRect.bottom = endPt.y + endLineHeight - 1.0;
		outRegion->Include(selRect);
	}
}


// ------------------------------------------------------------
// 	ScrollToOffset
// ------------------------------------------------------------
// Scroll the view so that inOffset is fully visible

void
BTextView::ScrollToOffset(
	int32	inOffset)
{
	BRect	bounds = Bounds();
	BPoint  scrollTo = bounds.LeftTop();
	float	lineHeight = 0.0;
	BPoint	point = PointAt(inOffset, &lineHeight);
	bool	scroll = false;
	
	if ( (fResizable) && (fContainerView == NULL) && 
		 (fAlignment != B_ALIGN_LEFT) ) {
		// Special case for a resizable control with non-left alignment.
		// This is the amount of space that we allow the text rectangle to move
		// into the view boundaries.
		const float zone = 0; //bounds.Width()/4;
		if( fTextRect.Width() < bounds.Width() ) {
			// If the text fits wholly within the view, always keep scroll point
			// at origin.
			scrollTo.x = 0;
		} else if ((point.x < bounds.left) || (point.x >= bounds.right)) {
			// If point goes outside of view bounds, scroll so that it is now
			// in the middle of the control.  Don't allow the scroll to make the
			// edges of the text rect be too far into the visible bounds.
			scrollTo.x = point.x - (bounds.Width() / 2);
			scrollTo.x = (scrollTo.x < (fTextRect.left-zone)) ? (fTextRect.left-zone) : scrollTo.x;
			scrollTo.x = (scrollTo.x > (fTextRect.right-bounds.Width()+zone)) ? (fTextRect.right-bounds.Width()+zone) : scrollTo.x; 
		} else {
			// Otherwise, just be sure the text rectangle is positioned reasonably
			// within the view boundaries.
			if( fTextRect.left > bounds.left+zone ) {
				scrollTo.x = fTextRect.left;
			} else if( fTextRect.right < bounds.right-zone ) {
				scrollTo.x = fTextRect.right-bounds.Width();
			}
		}
	} else  if ((point.x < bounds.left) || (point.x >= bounds.right)) {
		float maxRange = (fTextRect.Width() + fTextRect.left) - bounds.Width();
		maxRange = (maxRange < 0.0) ? 0.0 : maxRange;
		scrollTo.x = point.x - (bounds.Width() / 2.0);
		scrollTo.x = (scrollTo.x < 0.0) ? 0.0 : scrollTo.x;
		scrollTo.x = (scrollTo.x > maxRange) ? maxRange : scrollTo.x;
		scroll = true;
	}
	
	if ((point.y < bounds.top) || ((point.y + lineHeight) >= bounds.bottom)) {
		float maxRange = (fTextRect.Height() + fTextRect.top) - bounds.Height();
		maxRange = (maxRange < 0.0) ? 0.0 : maxRange;
		scrollTo.y = point.y - (bounds.Height() / 2.0);
		scrollTo.y = (scrollTo.y < 0.0) ? 0.0 : scrollTo.y;
		scrollTo.y = (scrollTo.y > maxRange) ? maxRange : scrollTo.y;
		scroll = true;
	}
	
	if (scroll) {
		ScrollTo(scrollTo);
		fWhere += scrollTo - bounds.LeftTop();
	};
}


// ------------------------------------------------------------
// 	ScrollToSelection
// ------------------------------------------------------------
// Scroll the view so that the beginning of the selection is
// within visible range

void
BTextView::ScrollToSelection()
{
	ScrollToOffset(fSelStart);
}


// ------------------------------------------------------------
// 	SetTextRect
// ------------------------------------------------------------
// Set the text rect
//
// This function will also resize the offscreen bitmap 
// (if there is one)

void
BTextView::SetTextRect(
	BRect	rect)
{
	if (rect.right < rect.left)
		rect.right = rect.left;

	if ( (rect.top == fTextRect.top) && 
		 (rect.left == fTextRect.left) &&
		 (rect.right == fTextRect.right) )
		return;
	
	fTextRect = rect;
	fTextRect.bottom = fTextRect.top;
	
	if (fOffscreen != NULL) {
		// resize the offscreen bitmap
		DeleteOffscreen();
		NewOffscreen();
	}

	bool attached = Window() != NULL;
	
	if ((fActive) && (attached)) {
		// hide the caret, unhighlight the selection
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
		
	Refresh(0, fText->Length(), true, false);
		
	if (attached) {
		// invalidate and immediately redraw in case the 
		// text rect has gotten smaller 
		Invalidate();
		Window()->UpdateIfNeeded();
	}
}


// ------------------------------------------------------------
// 	TextRect
// ------------------------------------------------------------
// Get the text rect

BRect
BTextView::TextRect() const
{
	return (fTextRect);
}


// ------------------------------------------------------------
// 	SetStylable
// ------------------------------------------------------------
// Set whether the view is multi-stylable

void
BTextView::SetStylable(
	bool    stylable)
{
	if (stylable == fStylable)
		return;

	fStylable = stylable;

	if (!fStylable) {
		int32		textLen = fText->Length();
		BFont		theFont;
		rgb_color	theColor;
		GetFontAndColor(0, &theFont, &theColor);

		fStyles->SetStyleRange(0, textLen, textLen, B_FONT_ALL, 
							   &theFont, &theColor);

		bool safeToDraw = (fActive) && (Window() != NULL);

		if (safeToDraw) {
			// hide the caret, unhighlight the selection
			if ((fSelStart != fSelEnd) && (fSelectable))
				Highlight(fSelStart, fSelEnd);
			else {
				if (fCaretVisible)
					InvertCaret();
			}
		}

		Refresh(0, textLen, true, false);
      
		if (safeToDraw) {
			// show the caret, highlight the selection
			if ((fSelStart != fSelEnd) && (fSelectable))
				Highlight(fSelStart, fSelEnd);
			else {
				if ((!fCaretVisible) && (fEditable))
					InvertCaret();
			}
		}
	}
}


// ------------------------------------------------------------
// 	IsStylable
// ------------------------------------------------------------
// Return whether the view is multi-stylable

bool
BTextView::IsStylable() const
{
	return (fStylable);
}
	

// ------------------------------------------------------------
// 	SetPrintWhiteBackground
// ------------------------------------------------------------
// Set whether background is white when printing
void
BTextView::SetPrintWhiteBackground(bool printWhite)
{
	fPrintWhiteBackground = printWhite;
}

// ------------------------------------------------------------
// 	PrintWhiteBackground
// ------------------------------------------------------------
// Return whether background is white when printing
bool
BTextView::PrintWhiteBackground() const
{
	return fPrintWhiteBackground;
}

// ------------------------------------------------------------
// 	SetTabWidth
// ------------------------------------------------------------
// Set the width of the tab character

void
BTextView::SetTabWidth(
	float	width)
{
	if ((width == fTabWidth) || (fTabWidth < 0.0))
		return;
		
	fTabWidth = width;
	
	bool safeToDraw = (fActive) && (Window() != NULL);

	if (safeToDraw) {
		// hide the caret, unhighlight the selection
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
	
	Refresh(0, fText->Length(), true, false);
		
	if (safeToDraw) {
		// show the caret, highlight the selection
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if ((!fCaretVisible) && (fEditable))
				InvertCaret();
		}
	}
}


// ------------------------------------------------------------
// 	TabWidth
// ------------------------------------------------------------
// Return the width of the tab character

float
BTextView::TabWidth() const
{
	return (fTabWidth);
}


// ------------------------------------------------------------
// 	MakeSelectable
// ------------------------------------------------------------
// Set whether the caret/selection range will be displayed

void
BTextView::MakeSelectable(
	bool	selectable)
{
	if (selectable == fSelectable)
		return;
		
	fSelectable = selectable;
	
	if (Window() != NULL) {
		if (fActive) {
			// highlight/unhighlight the selection
			if (fSelStart != fSelEnd)
				Highlight(fSelStart, fSelEnd);
		}
	}

	// no point having a selection when the user can't see it
	if (!fSelectable)
		fClickOffset = fSelStart = fSelEnd;
}


// ------------------------------------------------------------
// 	IsSelectable
// ------------------------------------------------------------
// Return whether the caret/selection range will be displayed

bool
BTextView::IsSelectable() const
{
	return (fSelectable);
}


// ------------------------------------------------------------
// 	MakeEditable
// ------------------------------------------------------------
// Set whether the text can be edited

void
BTextView::MakeEditable(
	bool	editable)
{
	if (editable == fEditable)
		return;
		
	fEditable = editable;
	
	if (Window() != NULL) {
		if (fActive) {
			if ((!fEditable) && (fCaretVisible))
				InvertCaret();
		}
	}

	if (!fEditable)
		CancelInputMethod();
}


// ------------------------------------------------------------
// 	IsEditable
// ------------------------------------------------------------
// Return whether the text can be edited

bool
BTextView::IsEditable() const
{
	return (fEditable);
}


// ------------------------------------------------------------
// 	SetWordWrap
// ------------------------------------------------------------
// Set whether the text should be wrapped 

void
BTextView::SetWordWrap(
	bool	wrap)
{
	if (wrap == fWrap)
		return;
		
	fWrap = wrap;
	
	bool safeToDraw = (fActive) && (Window() != NULL);

	if (safeToDraw) {
		// hide the caret, unhighlight the selection
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if (fCaretVisible)
				InvertCaret();
		}
	}
		
	Refresh(0, fText->Length(), true, false);
		
	if (safeToDraw) {
		// show the caret, highlight the selection
		if ((fSelStart != fSelEnd) && (fSelectable))
			Highlight(fSelStart, fSelEnd);
		else {
			if ((!fCaretVisible) && (fEditable))
				InvertCaret();
		}
	}
}


// ------------------------------------------------------------
// 	DoesWordWrap
// ------------------------------------------------------------
// Return whether the text is wrapped 

bool
BTextView::DoesWordWrap() const
{
	return (fWrap);
}


// ------------------------------------------------------------
// 	SetMaxBytes
// ------------------------------------------------------------
// Set the maximum number of bytes allowed

void
BTextView::SetMaxBytes(
	int32	max)
{
	if (max == fMaxBytes)
		return;
		
	fMaxBytes = max;

	int32 textLen = fText->Length();

	if (textLen > fMaxBytes) {
		CancelInputMethod();

		bool safeToDraw = (fActive) && (Window() != NULL);

		if (safeToDraw) {
			// hide the caret, unhighlight the selection
			if ((fSelStart != fSelEnd) && (fSelectable))
				Highlight(fSelStart, fSelEnd);
			else {
				if (fCaretVisible)
					InvertCaret();
			}
		}
		
		int32 deleteStart = PreviousInitialByte(fMaxBytes + 1);
		_BTextChangeResult_ change;
		DoDeleteText(deleteStart, textLen, &change);
		if (fSelStart > change.fDeleteStart)
			fClickOffset = fSelEnd = fSelStart = change.fDeleteStart;

		Refresh(change.fDeleteStart, change.fDeleteStart, true, false);

		if (safeToDraw) {
			// show the caret, highlight the selection
			if ((fSelStart != fSelEnd) && (fSelectable))
				Highlight(fSelStart, fSelEnd);
			else {
				if ((!fCaretVisible) && (fEditable))
					InvertCaret();
			}
		}
	}
}


// ------------------------------------------------------------
// 	MaxBytes
// ------------------------------------------------------------
// Return the maximum number of bytes allowed

int32
BTextView::MaxBytes() const
{
	return (fMaxBytes);
}


// ------------------------------------------------------------
// 	DisallowChar
// ------------------------------------------------------------
// No longer accept aChar

void
BTextView::DisallowChar(
	uint32	aChar)
{
	if (fDisallowedChars == NULL)
		fDisallowedChars = new BList(1);

	if (!fDisallowedChars->HasItem((void *)aChar))
		fDisallowedChars->AddItem((void *)aChar);
}


// ------------------------------------------------------------
// 	AllowChar
// ------------------------------------------------------------
// Accept aChar

void
BTextView::AllowChar(
	uint32	aChar)
{
	if (fDisallowedChars != NULL)
		fDisallowedChars->RemoveItem((void *)aChar);
}

	
// ------------------------------------------------------------
//  SetAlignment
// ------------------------------------------------------------
// Set the alignment mode for the entire document
// 
// Modes are: B_ALIGN_LEFT, B_ALIGN_CENTER, and B_ALIGN_RIGHT

void
BTextView::SetAlignment(
	alignment	flag)
{
	if (flag == fAlignment)
		return;
		
	switch (flag) {
		case B_ALIGN_LEFT:
		case B_ALIGN_CENTER:
		case B_ALIGN_RIGHT:
			// valid alignment
			break;
			
		default:
			// invalid alignment, return
			return;
	}
	
	fAlignment = flag;
	
	if (Window() != NULL) {
		Invalidate();
		Window()->UpdateIfNeeded();
	}
}


// ------------------------------------------------------------
// 	Alignment
// ------------------------------------------------------------
// Return the alignment of the text

alignment
BTextView::Alignment() const
{
	return (fAlignment);
}


// ------------------------------------------------------------
// 	SetAutoindent
// ------------------------------------------------------------
// Copy tabs and spaces from the previous line

void
BTextView::SetAutoindent(
	bool	state)
{
	fAutoindent = state;
}


// ------------------------------------------------------------
// 	DoesAutoindent
// ------------------------------------------------------------
// Return whether autoindent is on

bool
BTextView::DoesAutoindent() const
{
	return (fAutoindent);
}


// ------------------------------------------------------------
// 	SetColorSpace
// ------------------------------------------------------------
// Use an offscreen bitmap that is colors deep
//
// The bitmap is used only for drawing the current line

void
BTextView::SetColorSpace(
	color_space	colors)
{
	if (colors == B_NO_COLOR_SPACE) {
		if (Window()) colors = BScreen(Window()).ColorSpace();
		else colors = BScreen().ColorSpace();
		fColorSpaceSet = false;
	} else {
		fColorSpaceSet = true;
	}
	
	if (fColorSpace == colors)
		return;

	fColorSpace = colors;
	
	if (fOffscreen != NULL) {
		DeleteOffscreen();	
		NewOffscreen();
	}
}


// ------------------------------------------------------------
// 	ColorSpace
// ------------------------------------------------------------
// Return the color_space of the offscreen bitmap 

color_space
BTextView::ColorSpace() const
{
	return (fColorSpace);
}


// ------------------------------------------------------------
// 	MakeResizable
// ------------------------------------------------------------
// Resize this view and containerView so that its size 
// matches that of the text

void
BTextView::MakeResizable(
	bool	resize,
	BView	*resizeView)
{
	if (!resize) {
		fResizable = false;
		fContainerView = NULL;
		if (fOffscreen != NULL) {
			// get rid of the offscreen's padding
			DeleteOffscreen();
			NewOffscreen();
		}
		return;
	}

	fResizable = true;
	fContainerView = resizeView;
	SetWordWrap(false);
}


// ------------------------------------------------------------
// 	IsResizable
// ------------------------------------------------------------
// Return whether this view resizes itself after the text

bool
BTextView::IsResizable() const
{
	return (fResizable);
}


// ------------------------------------------------------------
// 	SetDoesUndo
// ------------------------------------------------------------
// Set whether undo should be enabled

void
BTextView::SetDoesUndo(
	bool	undo)
{
	if (((undo) && (fUndo != NULL)) || ((!undo) && (fUndo == NULL)))
		return;

	if (undo)
		fUndo = new _BUndoBuffer_(this);
	else {
		delete (fUndo);
		fUndo = NULL;
	}
}


// ------------------------------------------------------------
// 	DoesUndo
// ------------------------------------------------------------
// Return whether undo is enabled

bool
BTextView::DoesUndo() const
{
	return (fUndo != NULL);
}

void 
BTextView::HideTyping(bool enabled)
{
	SetText("", 0, 0);					// kill existing text/styles when switching mode
	fText->SetPasswordMode(enabled);	// switch mode
	Invalidate();						// repaint
}

bool 
BTextView::IsTypingHidden() const
{
	return fText->PasswordMode();
}

// ------------------------------------------------------------
// 	AllocRunArray
// ------------------------------------------------------------
// Allocate and initialize a run array structure.

text_run_array*
BTextView::AllocRunArray(
	int32		entryCount,
	int32		*outSize)
{
	int32			size = sizeof(int32) + (sizeof(text_run) * entryCount);
	text_run_array	*result = (text_run_array *)malloc(size);
	
	if (result) {
		static const rgb_color black = { 0, 0, 0, 255 };
		text_run *run = result->runs;
		
		result->count = entryCount;
		
		for (int32 i=0; i<entryCount; i++, run++) {
			run->offset = 0;
			run->color = black;
			new(&run->font) BFont();
		}
	}
	
	if (outSize)
		*outSize = size;
	
	return result;
}

// ------------------------------------------------------------
// 	CopyRunArray
// ------------------------------------------------------------
// Duplicate an existing run array

text_run_array*
BTextView::CopyRunArray(
	const text_run_array	*orig,
	int32					countDelta)
{
	if (!orig)
		return NULL;
	
	int32 count = orig->count + countDelta;
	if (count <= 0)
		return NULL;
	
	int32			size = sizeof(int32) + (sizeof(text_run) * count);
	text_run_array	*result = (text_run_array *)malloc(size);
	
	if (result) {
		result->count = count;
		
		static const rgb_color black = { 0, 0, 0, 255 };
		const int32 avail = count <= orig->count ? count : orig->count;
			
		text_run *resultRun = result->runs;
		const text_run *origRun = orig->runs;
		
		int32 i;
		for (i=0; i<avail; i++, resultRun++, origRun++) {
			resultRun->offset = origRun->offset;
			resultRun->color = origRun->color;
			new(&resultRun->font) BFont(origRun->font);
		}
		for (; i<count; i++, resultRun++) {
			resultRun->offset = 0;
			resultRun->color = black;
			new(&resultRun->font) BFont();
		}
	}
	
	return result;
}

// ------------------------------------------------------------
// 	FreeRunArray
// ------------------------------------------------------------
// Destroy and free memory of a run array

void
BTextView::FreeRunArray(
	text_run_array	*array)
{
	if (array) {
		const int32 entryCount = array->count;
		text_run *run = array->runs;
		for (int32 i=0; i<entryCount; i++, run++) {
			run->font.~BFont();
		}
		
		free(array);
	}
}

// ------------------------------------------------------------
// 	FlattenRunArray
// ------------------------------------------------------------
// Flatten a text_run_array so that it is persistent

void*
BTextView::FlattenRunArray(
	const text_run_array	*array,
	int32					*outSize)
{	
	int32	size = (sizeof(int32) * 3) + (sizeof(flat_run) * array->count);
	char	*result = (char *)malloc(size);
	if (result) {
		memset(result, 0, size);	/* don't save junk */
	}
	else {	/* out of memory! */
		if (outSize != NULL) *outSize = 0;
		return NULL;
	}

	char	*curField = result;

	// magic value
	*((int32 *)curField) = B_HOST_TO_BENDIAN_INT32(kMagicNumber);
	curField += sizeof(int32);

	// version number
	*((int32 *)curField) = B_HOST_TO_BENDIAN_INT32(kDR9FlatRunArray);
	curField += sizeof(int32);

	// number of runs
	*((int32 *)curField) = B_HOST_TO_BENDIAN_INT32(array->count);
	curField += sizeof(int32);

	const text_run *tRun = array->runs;
	for (int32 i = 0; i < array->count; i++) {
		// offset
		*((int32 *)curField) = B_HOST_TO_BENDIAN_INT32(tRun->offset);
		curField += sizeof(int32);

		// font_family and font_style
		font_family	family;
		font_style	style;
		memset(family, 0, sizeof(family));
		memset(style, 0, sizeof(style));
		tRun->font.GetFamilyAndStyle(&family, &style);
		strncpy(curField, family, sizeof(font_family));
		curField += sizeof(font_family);
		strncpy(curField, style, sizeof(font_style));
		curField += sizeof(font_style);

		// size
		*((float *)curField) = B_HOST_TO_BENDIAN_FLOAT(tRun->font.Size());
		curField += sizeof(float);

		// shear
		*((float *)curField) = B_HOST_TO_BENDIAN_FLOAT(tRun->font.Shear());
		curField += sizeof(float);

		// face
		// fhl 10/5/98: face actually means something now, but we're going to ignore it
		//*((uint16 *)curField) = B_HOST_TO_BENDIAN_INT16(tRun->font.Face());
		*((uint16 *)curField) = 0;
		curField += sizeof(uint16);

		// color
		*((uint8 *)curField) = tRun->color.red;
		curField += sizeof(uint8);
		*((uint8 *)curField) = tRun->color.green;
		curField += sizeof(uint8);
		*((uint8 *)curField) = tRun->color.blue;
		curField += sizeof(uint8);
		*((uint8 *)curField) = tRun->color.alpha;
		curField += sizeof(uint8);

		// filler
		*((uint16 *)curField) = B_HOST_TO_BENDIAN_INT16(0);
		curField += sizeof(uint16);

		tRun++;
	}

	if (outSize != NULL)
		*outSize = size;

	return (result);
}


// ------------------------------------------------------------
// 	UnflattenRunArray
// ------------------------------------------------------------
// Unflatten a text_run_array

text_run_array*
BTextView::UnflattenRunArray(
	const void	*data,
	int32		*outSize)
{
	const char	*array = (char *)data;
	const char	*curField = array;

	// check the magic value and the version number
	if ( (B_BENDIAN_TO_HOST_INT32(*((int32 *)curField)) != kMagicNumber) || 
		 (B_BENDIAN_TO_HOST_INT32(*((int32 *)(curField + sizeof(int32)))) != kDR9FlatRunArray) )
		return (NULL);
	curField += sizeof(int32) * 2;

	// number of runs
	int32 aCount = B_BENDIAN_TO_HOST_INT32(*((int32 *)curField));
	curField += sizeof(int32);

	text_run_array	*result = AllocRunArray(aCount, outSize);

	text_run *tRun = result->runs;
	for (int32 i = 0; i < result->count; i++) {
		// offset
		tRun->offset = B_BENDIAN_TO_HOST_INT32(*((int32 *)curField));
		curField += sizeof(int32);

		// font_family and font_style 
		BFont font;
		font.SetFamilyAndStyle(curField, curField + sizeof(font_family));
		curField += sizeof(font_family) + sizeof(font_style);

		// size
		font.SetSize(B_BENDIAN_TO_HOST_FLOAT(*((float *)curField)));
		curField += sizeof(float);

		// shear
		font.SetShear(B_BENDIAN_TO_HOST_FLOAT(*((float *)curField)));
		curField += sizeof(float);

		// face
		// fhl 10/5/98: face actually returns something now, but we don't care
		//font.SetFace(B_BENDIAN_TO_HOST_INT16(*((uint16 *)curField)));
		curField += sizeof(uint16);

		// set the font
		tRun->font = font;

		// color
		tRun->color.red = *((uint8 *)curField);
		curField += sizeof(uint8);
		tRun->color.green = *((uint8 *)curField);
		curField += sizeof(uint8);
		tRun->color.blue = *((uint8 *)curField);
		curField += sizeof(uint8);
		tRun->color.alpha = *((uint8 *)curField);
		curField += sizeof(uint8);

		// filler
		curField += sizeof(uint16);		

		tRun++;
	}

	return (result);	
}


// ------------------------------------------------------------
// 	AttachedToWindow
// ------------------------------------------------------------
// Reset some member variables, recalculate the line breaks, 
// redraw the text

void
BTextView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	SetDrawingMode(B_OP_COPY);

	fCaretVisible = false;
	fCaretTime = 0;
	fClickCount = 0;
	fClickTime = 0;
	fDragOffset = -1;
	fActive = false;
	BMessage pulseMsg(B_PULSE);
	fPulseRunner = new BMessageRunner(BMessenger(this),&pulseMsg,500000);
	
	// we now have access to our parents/scrollers
	if (fResizable)
		AutoResize();
	UpdateScrollbars();
	SetViewCursor((fCursor) ? B_CURSOR_I_BEAM : B_CURSOR_SYSTEM_DEFAULT);
}


// ------------------------------------------------------------
// 	DetachedFromWindow
// ------------------------------------------------------------
// Reset the cursor if it is above this view

void
BTextView::DetachedFromWindow()
{
	delete fPulseRunner;
	fPulseRunner = NULL;
	BView::DetachedFromWindow();
}


// ------------------------------------------------------------
// 	Draw
// ------------------------------------------------------------
// Draw any lines that need to be updated and display the 
// caret or the current selection

void 
BTextView::SetViewColor(rgb_color c)
{
	fViewColorSet = true;
	fViewColor = c;
	BView::SetViewColor(B_TRANSPARENT_COLOR);
}

void
BTextView::Draw(
	BRect	inRect)
{
	if (!fColorSpaceSet && Window()) {
		SetColorSpace(BScreen(Window()).ColorSpace());
		fColorSpaceSet = false;
	}
	
	if (!fViewColorSet) fViewColor = ViewColor();
	else {
		SetDrawingMode(B_OP_COPY);
		SetLowColor((fPrintWhiteBackground && IsPrinting())
						? make_color(255, 255, 255) : fViewColor);
		FillRect(Bounds(),B_SOLID_LOW);
	}

	// what lines need to be drawn?
	int32 startLine = fLines->PixelToLine(inRect.top - fTextRect.top);
	int32 endLine = fLines->PixelToLine(inRect.bottom - fTextRect.top);

	DrawLines(startLine, endLine);

	if (!IsPrinting()) {	
		// draw the caret/highlight the selection
		if (fActive) {
			if ((fSelStart != fSelEnd) && (fSelectable))
			  Highlight(fSelStart, fSelEnd);
			else {
				if (fCaretVisible)
					DrawCaret(fSelStart);
			}
		}
	}
}


// ------------------------------------------------------------
// 	MouseDown
// ------------------------------------------------------------
// Move the caret and track the mouse while it's down

void
BTextView::MouseDown(
	BPoint	where)
{
	// should we even bother?
	if ((!fEditable) && (!fSelectable))
		return;
	
	CancelInputMethod();

	bool gotFocus = false;
	
	// if this view isn't the focus, make it the focus
	if (!IsFocus()) {
		// A resizable (single-line) text view handles focus
		// differently -- clicking on the text view to give it
		// focus shouldn't change the current selection state.
		if( IsResizable() ) {
			// For backwards compatibility, if someone is implementing
			// their own BTextControl then we still want the first click
			// to select everything.
			SelectAll();
			gotFocus = true;
		}
		MakeFocus();
	}
	
#if _SUPPORTS_SOFT_KEYBOARD
	// Clicking on a text field should auto-show the keyboard,
	// this is because sometimes fields get focus in programattic
	// ways.  We don't want the keyboard to pop up then, but
	// sometimes, it's a pain to get it to come up without reverting
	// to using the hardware button.
	OpenSoftKeyboard();
#endif	

	SetExplicitFocus();
	
	// hide the caret if it's visible	
	if (fCaretVisible)
		InvertCaret();
	
	BWindow		*window = Window();
	BMessage	*message = window->CurrentMessage();
	
	// Create the mouse tracking context.  This will include a
	// BMessageRunner for regular pokes.  In an asynchronous
	// controls window, these pokes are used only for window
	// scrolling.  Otherwise, they are also used to simulate
	// MouseDown() and MouseUp() events.
	StopMouseTracking();
	fTrackingMouse = new _BTextTrackState_(BMessenger(this));
	
	uint32		mods = 0;
	if (message)
		message->FindInt32("modifiers", (int32 *)&mods);
	fTrackingMouse->fShiftDown = mods & B_SHIFT_KEY;
	uint32		clickButtons = 0;
	if (message)
		message->FindInt32("buttons", (int32 *)&clickButtons);
	fTrackingMouse->fMouseOffset = OffsetAt(where);	
	bigtime_t	clickTime = system_time();

	// get the system-wide click speed
	bigtime_t clickSpeed = 0;
	get_click_speed(&clickSpeed);

	bool fastClick = (clickSpeed > (clickTime - fClickTime)) &&
					 (fTrackingMouse->fMouseOffset == fClickOffset);
	PRINT(("fastClick=%d, thisTime=%Ld, lastTime=%Ld, thisOffset=%ld, lastOffset=%ld\n",
			fastClick, clickTime, fClickTime,
			fTrackingMouse->fMouseOffset, fClickOffset));
			
	fTrackingMouse->fLastPoint = where;
	fWhere = where;
	SetMouseEventMask(B_POINTER_EVENTS | B_KEYBOARD_EVENTS,
					  B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
	
	// Don't do normal click handling if this one was used to give
	// the text view focus.
	
	if( !gotFocus ) {
	
		// should we initiate a drag?
		if ((fSelStart != fSelEnd)
				&& (!fTrackingMouse->fShiftDown) && (!fastClick)) {
			// was the click within the selection range?
			BRegion textRegion;
			GetTextRegion(fSelStart, fSelEnd, &textRegion);
			if (textRegion.Contains(where)) {
				fTrackingMouse->fDragRect =
					BRect(where.x - 3.0, where.y - 3.0, 
						  where.x + 3.0, where.y + 3.0);
				return;
			}
		}
		
		// is this a double/triple click, or maybe it's a new click?
		if (fastClick && fClickCount > 0) {
			if (fClickCount > 1) {
				// triple click
				fClickCount = 0;
				fClickTime = 0;
			}
			else {
				// double click
				fClickCount = 2;
				fClickTime = clickTime;
			}
		}
		else {
			// new click
			fClickOffset = fTrackingMouse->fMouseOffset;
			fClickCount = 1;
			fClickTime = clickTime;
			
			if (!fTrackingMouse->fShiftDown)
				Select(fTrackingMouse->fMouseOffset, fTrackingMouse->fMouseOffset);
		}
		
	} else {
		// First click in a single-line text view.
		fClickOffset = fTrackingMouse->fMouseOffset;
		fClickCount = 0;
		fClickTime = clickTime;
	}
	
	if (fClickTime == clickTime) {
		BMessage checkClick(_PING_);
		checkClick.AddInt64("clickTime",clickTime);
		if (fClickRunner) delete fClickRunner;
		fClickRunner = new BMessageRunner(BMessenger(this),&checkClick,clickSpeed,1);
	};
	
	// no need to track the mouse if we can't select
	if (!fSelectable || gotFocus) {
		StopMouseTracking();
		
	} else {
		fTrackingMouse->fAnchor = (fTrackingMouse->fMouseOffset > fSelStart)
								? fSelStart : fSelEnd;
	
		// Perform initial selection.
		MouseMoved(where, B_INSIDE_VIEW, 0);
	}
}


// ------------------------------------------------------------
// 	MouseUp
// ------------------------------------------------------------
// Just in case...

void
BTextView::MouseUp(
	BPoint	where)
{
	BView::MouseUp(where);
	
	PerformMouseUp(where);
	
	if (fDragRunner) {
		delete fDragRunner;
		fDragRunner = NULL;
	}
}


// ------------------------------------------------------------
// 	MouseUp
// ------------------------------------------------------------
// Just in case...

bool
BTextView::PerformMouseUp(
	BPoint	where)
{
	if( fTrackingMouse ) {
		_BTextTrackState_& tm = *fTrackingMouse;
		if( tm.fDragRect.IsValid() ) {
			// didn't start dragging -- clear current selection.
			Select(tm.fMouseOffset, tm.fMouseOffset);
		} else {
			// finish selection operation.
			// check if the cursor needs to be reset if this is a new click
			if ((fClickCount == 1) && (!tm.fShiftDown)) {
				TrackMouse(where, NULL);
			}
		}
	
		StopMouseTracking();
		return true;
	}
	
	return false;
}


// ------------------------------------------------------------
// 	MouseMoved
// ------------------------------------------------------------
// Set the cursor to the I-Beam when it's above this view and
// track any drags that are over this view

void
BTextView::MouseMoved(
	BPoint			where,
	uint32			code, 
	const BMessage	*message)
{
	// Try to track the mouse from a mouse down.  If not tracking,
	// look for drags and set cursor.
	if( !PerformMouseMoved(where, code) ) {
	
		switch (code) {
			case B_ENTERED_VIEW:
				TrackMouse(where, message, true);
				break;
				
			case B_INSIDE_VIEW:
				TrackMouse(where, message);
				break;
				
			case B_EXITED_VIEW:
				DragCaret(-1);
				if ( (Window()->IsActive()) && (message == NULL) &&
					 ((fEditable) || (fSelectable)) )
					SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
				break;
				
			default:
				BView::MouseMoved(where, code, message);
				break;
		}
		
	}
}


// ------------------------------------------------------------
// 	PerformMouseMoved
// ------------------------------------------------------------
// Perform selection and drag initiation when button is down.

bool
BTextView::PerformMouseMoved(
	BPoint			where,
	uint32			/*code*/)
{
	fWhere = where;
	
	if( fTrackingMouse ) {
		_BTextTrackState_& tm = *fTrackingMouse;
		
		if( tm.fDragRect.IsValid() ) {
			// looking for start of a drag.
			if (!tm.fDragRect.Contains(where)) {
				StopMouseTracking();
				InitiateDrag();
			}
		} else {
			tm.fMouseOffset = OffsetAt(where);
			
			if (tm.fMouseOffset > tm.fAnchor) {
				tm.fStart = tm.fAnchor;
				tm.fEnd = tm.fMouseOffset;
			}
			else {
				tm.fStart = tm.fMouseOffset;
				tm.fEnd = tm.fAnchor;
			}
			
			switch (fClickCount) {
				case 0:
					// triple click, select paragraph by paragraph
					while (tm.fStart > 0) {
						if ((*fText)[tm.fStart] == '\n') {
							// found it! but we don't want to include the newline
							tm.fStart++;
							break;
						}
	
						tm.fStart--;
					}
			
					tm.fTextLen = fText->Length();
					while (tm.fEnd < tm.fTextLen) {
						if ((*fText)[tm.fEnd] == '\n') {
							// found it, and include the newline
							tm.fEnd++;
							break;
						}
	
						tm.fEnd++;
					}			
					break;
													
				case 2:
				{
					// double click, select word by word
					int32 anOffset = 0;
					FindWord(tm.fStart, &tm.fStart, &anOffset);
					FindWord(tm.fEnd, &anOffset, &tm.fEnd);
					break;
				}
					
				default:
					// new click, select char by char
					break;			
			}
			if (tm.fShiftDown) {
				if (tm.fMouseOffset > tm.fAnchor)
					tm.fStart = tm.fAnchor;
				else
					tm.fEnd = tm.fAnchor;
			}
			
			// Select new area and update cursor icon.
			Select(tm.fStart, tm.fEnd);
			fClickOffset = tm.fMouseOffset;
			TrackMouse(where, 0);
		}
		
		return true;
	}
	
	return false;
}


// ------------------------------------------------------------
// 	WindowActivated
// ------------------------------------------------------------
// Activate this view if and only if its window is active and
// the view is the focus of that window

void
BTextView::WindowActivated(
	bool	state)
{
	BView::WindowActivated(state);
	
	if (state && IsFocus()) {
		if (!fActive)
			Activate();
	}
	else {
		if (fActive)
			Deactivate();
	}

	if (state) {
		BPoint 	where;
		uint32	buttons;
		GetMouse(&where, &buttons);
		if (Bounds().Contains(where))
			TrackMouse(where, NULL);
	}
}


// ------------------------------------------------------------
// 	KeyDown
// ------------------------------------------------------------
// Respond to key presses

void
BTextView::KeyDown(
	const char	*bytes,
	int32		numBytes)
{
	uchar theChar = bytes[0];

	if (!fEditable) {
		// if text isn't editable, only movement keys work.
		switch (theChar) {
			case B_LEFT_ARROW:
			case B_RIGHT_ARROW:
			case B_UP_ARROW:
			case B_DOWN_ARROW:
				HandleArrowKey(theChar);
				break;
			
			case B_HOME:
			case B_END:
			case B_PAGE_UP:
			case B_PAGE_DOWN:
				HandlePageKey(theChar);
				break;
				
			default:
			BView::KeyDown(bytes, numBytes);
		}
		return;
	}

	// hide the cursor and caret
	be_app->ObscureCursor();
	if (fCaretVisible)
		InvertCaret();
	
	// user is editing, so make sure focus is locked in
	SetExplicitFocus(true);
	
	switch (theChar) {
		case B_BACKSPACE:
			HandleBackspace();
			break;
			
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
		case B_UP_ARROW:
		case B_DOWN_ARROW:
			HandleArrowKey(theChar);
			break;
		
		case B_DELETE:
			HandleDelete();
			break;
			
		case B_HOME:
		case B_END:
		case B_PAGE_UP:
		case B_PAGE_DOWN:
			HandlePageKey(theChar);
			break;

		case B_INSERT:
		case B_ESCAPE:
		case B_FUNCTION_KEY:
			beep();
			break;
		
		default:
			if ((fText->Length() - (fSelEnd - fSelStart)) < fMaxBytes) {
				// Filter out all other control characters.
				if (theChar < ' ' && theChar != B_RETURN && theChar != B_TAB) {
					beep();
					break;
				}
				
				if (fDisallowedChars != NULL) {
					if (fDisallowedChars->HasItem((void *)theChar)) {
						beep();
						break;
					}
				}

				HandleAlphaKey(bytes, numBytes);				
			}
			break;
	}	

	// draw the caret
	if (fSelStart == fSelEnd) {
		if (!fCaretVisible)
			InvertCaret();
	}
}


// ------------------------------------------------------------
// 	Pulse
// ------------------------------------------------------------
// Flash the caret at 1/2 second intervals and track drags

void
BTextView::Pulse()
{
	if (fActive) {
		bigtime_t sysTime = system_time();

		if ((fEditable) && (fSelStart == fSelEnd)) {
			if (sysTime > (fCaretTime + 480000))	// 480000 == allowing for some error
				InvertCaret();
		}
	}
}


// ------------------------------------------------------------
// 	FrameResized
// ------------------------------------------------------------
// Update the scroll bars to mirror the visible area

void
BTextView::FrameResized(
	float	width,
	float 	height)
{
	BView::FrameResized(width, height);

	UpdateScrollbars();
}


// ------------------------------------------------------------
// 	MakeFocus
// ------------------------------------------------------------
// Activate this view if and only if its window is active and
// the view is the focus of that window

void
BTextView::MakeFocus(
	bool	focusState)
{
#if _SUPPORTS_SOFT_KEYBOARD
	//
	//	hard coded now to never use auto-hide
	if (!Parent())
		return;
	
	// Note that we only auto-show the soft keyboard if the focus
	// was made as a result of a user action (tabbing into the field,
	// clicking in it).  If it was programmatic (like the user navigated
	// to a page with a text field on it), don't pop up the keyboard.
	BMessage *m;
	if (Window() && (m = Window()->CurrentMessage()) != 0 &&
		(m->what == B_KEY_DOWN || m->what == B_MOUSE_DOWN)
		&& focusState)
			OpenSoftKeyboard();
#endif	
	
	BView::MakeFocus(focusState);
	
	if (focusState && Window()->IsActive()) {
		if (!fActive)
			Activate();
	}
	else {
		if (fActive)
			Deactivate();
	} 
}


// ------------------------------------------------------------
// 	MessageReceived
// ------------------------------------------------------------
// Check for dropped messages and respond to the standard 
// Cut, Copy, and Paste messages

void
BTextView::MessageReceived(
	BMessage	*message)
{
	bool handled = false;
	
	// was this message dropped?
	if (message->WasDropped()) {
		BPoint dropLoc;
		BPoint offset;
		
		dropLoc = message->DropPoint(&offset);
		ConvertFromScreen(&dropLoc);
		ConvertFromScreen(&offset);

		handled = MessageDropped(message, dropLoc, offset);
	}
	else {
		switch (message->what) {
#if _SUPPORTS_FEATURE_SCRIPTING
			case B_GET_PROPERTY:
			case B_SET_PROPERTY:
			case B_COUNT_PROPERTIES:
			{
				BMessage	reply(B_REPLY);
				int32		index = 0;
				BMessage	specifier;
				int32		form = 0;
				const char	*property = NULL;
				message->GetCurrentSpecifier(&index, &specifier, &form, &property);
				
				if (property) {
					if (message->what == B_GET_PROPERTY) 
						handled = GetProperty(&specifier, form, property, &reply);
					else if (message->what == B_SET_PROPERTY)
						handled = SetProperty(&specifier, form, property, &reply);
					else if (message->what == B_COUNT_PROPERTIES)
						handled = CountProperties(&specifier, form, property, &reply);
				}
				
				if (handled)
					message->SendReply(&reply);
				break;
			}
#endif
	
			case B_PULSE:
				Pulse();
				handled = true;
				break;
				
			case _DISPOSE_DRAG_:
				if (fEditable) TrackDrag(fWhere);
				break;

			case _PING_: {
				if( message->HasInt64("clickTime") ) {
					// This is an "end of click timeout" poke.
					int64 time;
					message->FindInt64("clickTime",&time);
					if (time == fClickTime) {
						if ((fSelectable) && (fSelStart != fSelEnd)) {
							BRegion textRegion;
							GetTextRegion(fSelStart, fSelEnd, &textRegion);
							if (textRegion.Contains(fWhere)) TrackMouse(fWhere,NULL);
						};
						if (fClickRunner) delete fClickRunner;
						fClickRunner = NULL;
					};
				} else if( fTrackingMouse ) {
					// Poke to scroll window and maybe also track the mouse.
					_BTextTrackState_& tm = *fTrackingMouse;
					
					tm.SimulateMouseEvent(this);
					
					// Should we scroll the view?
					BRect bounds = Bounds();
					if (!bounds.Contains(fWhere)) {
						PRINT(("Scrolling...\n"));
						float  hDelta = 0.0;
						float  vDelta = 0.0;
						BPoint scrollTo = bounds.LeftTop();
			
						if (fWhere.x < bounds.left)
							hDelta = fWhere.x - bounds.left;
						else {
							if (fWhere.x > bounds.right)
								hDelta = fWhere.x - bounds.right;
						}
							
						if (hDelta != 0.0) {
							if ( (fResizable) && (fContainerView == NULL) && 
								 (fAlignment != B_ALIGN_LEFT) ) {
								// Special case of non-left aligned text:
								if( fTextRect.Width() < bounds.Width() ) {
									// If text fits in view, don't scroll.
									scrollTo.x = 0;
								} else {
									// Otherwise, scroll by delta, making sure the
									// text rectangle edges don't move into view.
									scrollTo.x += hDelta;
									scrollTo.x = (scrollTo.x < fTextRect.left) ? fTextRect.left : scrollTo.x;
									scrollTo.x = (scrollTo.x > (fTextRect.right-Bounds().Width())) ? (fTextRect.right-bounds.Width()) : scrollTo.x; 
								}
							}
							else {
								float maxRange = (fTextRect.Width() + fTextRect.left) - bounds.Width();
								maxRange = (maxRange < 0.0) ? 0.0 : maxRange;
								scrollTo.x += hDelta;
								scrollTo.x = (scrollTo.x < 0.0) ? 0.0 : scrollTo.x;
								scrollTo.x = (scrollTo.x > maxRange) ? maxRange : scrollTo.x;
							}
						}
			
						if (fWhere.y < bounds.top)
							vDelta = fWhere.y - bounds.top;
						else {
							if (fWhere.y > bounds.bottom)
								vDelta = fWhere.y - bounds.bottom;
						}
							
						if (vDelta != 0.0) {	
							float maxRange = (fTextRect.Height() + fTextRect.top) - bounds.Height();
							maxRange = (maxRange < 0.0) ? 0.0 : maxRange;
							scrollTo.y += vDelta;
							scrollTo.y = (scrollTo.y < 0.0) ? 0.0 : scrollTo.y;
							scrollTo.y = (scrollTo.y > maxRange) ? maxRange : scrollTo.y;
						}
			
						if (scrollTo != bounds.LeftTop()) {
							ScrollTo(scrollTo);
							fWhere += scrollTo - bounds.LeftTop();
							Window()->UpdateIfNeeded();
							// And update selection.
							PerformMouseMoved(fWhere, B_OUTSIDE_VIEW);
						}
					}
				} 
				break;
			}

			case B_UNDO:
				Undo(be_clipboard);
				handled = true;
				break;

			case B_CUT:
				Cut(be_clipboard);
				handled = true;
				break;
				
			case B_COPY:
				Copy(be_clipboard);
				handled = true;
				break;
				
			case B_PASTE:
				Paste(be_clipboard);
				handled = true;
				break;
		
			case B_SELECT_ALL:
				SelectAll();
				handled = true;
				break;		

			case B_TRASH_TARGET:
				if (fEditable) {
					Delete();
				}
				handled = true;
				break;	

			case B_INPUT_METHOD_EVENT:
			{
				uint32 opcode = 0;
				message->FindInt32("be:opcode", (int32 *)&opcode);

				switch (opcode) {
					case B_INPUT_METHOD_STARTED:
					{
						BMessenger replyTo;
						if (message->FindMessenger("be:reply_to", &replyTo) == B_NO_ERROR) {
							if (!fEditable) {
								// we're not editable, stop the input method right away
								BMessage stop(B_INPUT_METHOD_EVENT);
								stop.AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);

								replyTo.SendMessage(&stop);
							}
							else {
								// hide the caret/unhighlight the selection
								if ((fActive) && (Window() != NULL)) {
									if ((fSelStart != fSelEnd) && (fSelectable))
										Highlight(fSelStart, fSelEnd);
									else {
										if (fCaretVisible)
											InvertCaret();
									}
								}
	
								fInline = new _BInlineInput_(replyTo);
							}
						}
						handled = true;
						break;
					}

					case B_INPUT_METHOD_STOPPED:
						if (fInline != NULL) {
							delete (fInline);
							fInline = NULL;

							// show the caret/unhighlight the selection
							if ((fActive) && (Window() != NULL)) {
								if ((fSelStart != fSelEnd) && (fSelectable))
									Highlight(fSelStart, fSelEnd);
								else {
									if ((!fCaretVisible) && (fEditable))
										InvertCaret();
								}
							}
						}
						handled = true;
						break;

					case B_INPUT_METHOD_CHANGED:
						HandleInputMethodChanged(message);
						handled = true;
						break;
		
					case B_INPUT_METHOD_LOCATION_REQUEST:
						HandleInputMethodLocationRequest();
						handled = true;
						break;

					default:
						break;
				}
				break;
			}

			default:
				handled = false;
				break;
		}
	}

	if (!handled)
		BView::MessageReceived(message);
}


// ------------------------------------------------------------
// 	ResolveSpecifier
// ------------------------------------------------------------

BHandler*
BTextView::ResolveSpecifier(
	BMessage	*_SCRIPTING_ONLY(message),
	int32		_SCRIPTING_ONLY(index),
	BMessage	*_SCRIPTING_ONLY(specifier),
	int32		_SCRIPTING_ONLY(form),
	const char	*_SCRIPTING_ONLY(property))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	BHandler		*target = NULL;
	BPropertyInfo	pi((property_info *)kPropertyList);
	
	if (pi.FindMatch(message, index, specifier, form, property) >= 0)
		target = this;
	else
		target = BView::ResolveSpecifier(message, index, specifier, form, property);

	return (target);
#else
	return NULL;
#endif
}


// ------------------------------------------------------------
// 	GetSupportedSuites
// ------------------------------------------------------------

status_t
BTextView::GetSupportedSuites(
	BMessage	*_SCRIPTING_ONLY(data))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	data->AddString("suites", "suite/vnd.Be-text-view");
	BPropertyInfo	pi((property_info *)kPropertyList);
	data->AddFlat("messages", &pi);
	return BView::GetSupportedSuites(data);
#else
	return B_UNSUPPORTED;
#endif
}


// ------------------------------------------------------------
// 	Perform
// ------------------------------------------------------------

status_t
BTextView::Perform(
	perform_code	d, 
	void			*arg)
{
	return (BView::Perform(d, arg));
}


// ------------------------------------------------------------
// 	InitObject
// ------------------------------------------------------------
// Initialize object

void
BTextView::InitObject(
	BRect			textRect,
	const BFont		*initialFont,
	const rgb_color	*initialColor)
{
	// static data initialized in _init_interface_kit_()

	if (textRect.right < textRect.left)
		textRect.right = textRect.left;

	BFont defaultFont(be_plain_font);
	if (initialFont != NULL)
		defaultFont = *initialFont;
	NormalizeFont(&defaultFont);

	rgb_color defaultColor = ui_color(B_DOCUMENT_TEXT_COLOR);
	if (initialColor != NULL)
		defaultColor = *initialColor;

	fText = new _BTextGapBuffer_();
	fLines = new _BLineBuffer_();
	fStyles = new _BStyleBuffer_(&defaultFont, &defaultColor);
	fTextRect = textRect;
	fSelStart = 0;
	fSelEnd = 0;
	fCaretVisible = false;
	fCaretTime = 0;
	fClickOffset = 0;
	fClickCount = 0;
	fClickTime = 0;
	fDragOffset = -1;
	fActive = false;
	fStylable = false;
	fTabWidth = 28.0;
	fSelectable = true;
	fEditable = true;
	fPrintWhiteBackground = true;
	fWrap = true;
	fMaxBytes = LONG_MAX;
	fDisallowedChars = NULL;
	fAlignment = B_ALIGN_LEFT;
	fAutoindent = false;
	fOffscreen = NULL;
	fColorSpaceSet = false;
	fColorSpace = B_COLOR_8_BIT;
	fResizable = false;
	fContainerView = NULL;
	fUndo = NULL;
	fInline = NULL;
	fPulseRunner = NULL;
	fDragRunner = NULL;
	fClickRunner = NULL;
	fCursor = false;
	fTrackingMouse = NULL;
	fTextChange = NULL;
	
	SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	fViewColor = B_TRANSPARENT_COLOR;
	fViewColorSet = false;
	SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);
	
	// initialize the various buffers
	Refresh(0, 0, false, false);
}
			 
							 
// ------------------------------------------------------------
// 	HandleBackspace
// ------------------------------------------------------------
// The Backspace key has been pressed

void
BTextView::HandleBackspace()
{
	if (fUndo != NULL) {
		_BTypingUndoBuffer_ *undo = dynamic_cast<_BTypingUndoBuffer_ *>(fUndo);
		if (undo == NULL) {
			delete (fUndo);	
			fUndo = undo = new _BTypingUndoBuffer_(this); 
		}
		undo->BackwardErase();	
	}

	if (fSelStart == fSelEnd) {
		if (fSelStart == 0)
			return;
		else
			fSelStart = PreviousInitialByte(fSelStart);
	}
	else {
		if (fSelectable)
			Highlight(fSelStart, fSelEnd);
	}

	_BTextChangeResult_ change;
	DoDeleteText(fSelStart, fSelEnd, &change);
	fClickOffset = fSelEnd = fSelStart = change.fDeleteStart;
	
	Refresh(fSelStart, fSelEnd, true, true);
}


// ------------------------------------------------------------
// 	HandleArrowKey
// ------------------------------------------------------------
// One of the four arrow keys has been pressed

void
BTextView::HandleArrowKey(
	uint32	inArrowKey)
{
	// return if there's nowhere to go
	if (fText->Length() == 0)
		return;
	
	if( !fEditable ) {
		// if not editable, just scroll in document.
		BRect	bounds = Bounds();
		BPoint  scrollTo = bounds.LeftTop();
		BScrollBar* vbar = ScrollBar(B_VERTICAL);
		BScrollBar* hbar = ScrollBar(B_HORIZONTAL);
	
		switch( inArrowKey) {
			case B_UP_ARROW: {
				int32 line = LineAt(BPoint(0, bounds.top+1));
				while( line >= 0
						&& (*fLines)[line]->origin >= scrollTo.y ) {
					line--;
				}
				if( line >= 0 ) {
					scrollTo.y = (*fLines)[line]->origin;
				} else {
					scrollTo.y = 0;
				}
			} break;
			case B_DOWN_ARROW: {
				int32 line = LineAt(BPoint(0, bounds.bottom-1));
				while( line < CountLines()
						&& (*fLines)[line]->origin <= (scrollTo.y+bounds.Height()-1) ) {
					line++;
				}
				if( line < CountLines() ) {
					scrollTo.y = (*fLines)[line]->origin - bounds.Height() + 1;
				} else {
					scrollTo.y = INT_MAX;
				}
			} break;
			case B_RIGHT_ARROW:
			case B_LEFT_ARROW: {
				if( hbar ) {
					float sstep=0, lstep=0;
					hbar->GetSteps(&sstep, &lstep);
					scrollTo.x += (inArrowKey==B_LEFT_ARROW ? -1:1)*sstep;
				}
			} break;
		}
		
		if( hbar ) {
			float minval=0, maxval=0;
			hbar->GetRange(&minval, &maxval);
			if( scrollTo.x < minval ) scrollTo.x = minval;
			if( scrollTo.x > maxval ) scrollTo.x = maxval;
		}
		if( vbar ) {
			float minval=0, maxval=0;
			vbar->GetRange(&minval, &maxval);
			if( scrollTo.y < minval ) scrollTo.y = minval;
			if( scrollTo.y > maxval ) scrollTo.y = maxval;
		}
		if (scrollTo != bounds.LeftTop()) {
			ScrollTo(scrollTo);
			fWhere += scrollTo - bounds.LeftTop();
		};
		
		return;
	}
	
	BMessage	*message = Window()->CurrentMessage();
	uint32		mods = 0;
	if (message)
		message->FindInt32("modifiers", (int32 *)&mods);
	bool		shiftDown = mods & B_SHIFT_KEY;
	int32		selStart = fSelStart;
	int32		selEnd = fSelEnd;
	int32		scrollToOffset = 0;

	switch (inArrowKey) {
		case B_UP_ARROW:
			if (shiftDown) {
				int32	saveClickOffset = fClickOffset;
				BPoint	point = PointAt(fClickOffset);
				point.y--;
				fClickOffset = OffsetAt(point);
				if (fClickOffset <= selStart) {
					if (selEnd <= saveClickOffset)
						selEnd = selStart;
					selStart = fClickOffset;
					scrollToOffset = selStart;
				}
				else {
					selEnd = fClickOffset;
					scrollToOffset = selEnd;
				}
				break;
			}
			else {
				if ((selStart == selEnd) || (shiftDown)) {
					BPoint point = PointAt(selStart);
					point.y--;
					selStart = OffsetAt(point);
					if (!shiftDown)
						selEnd = selStart;
					scrollToOffset = selStart;
					break;
				}
				// else fall thru
			}
			
		case B_LEFT_ARROW:
			if (shiftDown) {
				if (fClickOffset > 0) {
					fClickOffset = PreviousInitialByte(fClickOffset);
					if (fClickOffset < selStart) {
						selStart = fClickOffset;
						scrollToOffset = selStart;
					}
					else {
						selEnd = fClickOffset;
						scrollToOffset = selEnd;
					}
				}
			}
			else {
				if (selStart == selEnd) {
					if (selStart > 0)
						selEnd = selStart = PreviousInitialByte(selStart);
				}
				else
					selEnd = selStart;
				scrollToOffset = selStart;
			}
			break;
			
		case B_DOWN_ARROW:
			if (shiftDown) {
				int32	saveClickOffset = fClickOffset;
				float	height;
				BPoint	point = PointAt(fClickOffset, &height);
				point.y += height;
				fClickOffset = OffsetAt(point);
				if (fClickOffset >= selEnd) {
					if (selStart >= saveClickOffset) 
						selStart = selEnd;
					selEnd = fClickOffset;
					scrollToOffset = selEnd;
				}
				else {
					selStart = fClickOffset;
					scrollToOffset = selStart;
				}
				break;
			}
			else {
				if ((selStart == selEnd) || (shiftDown)) {
					float	height;
					BPoint	point = PointAt(selEnd, &height);
					point.y += height;
					selEnd = OffsetAt(point);
					if (!shiftDown)
						selStart = selEnd;
					scrollToOffset = selEnd;
					break;
				}
				// else fall thru
			}
			
		case B_RIGHT_ARROW:
			if (shiftDown) {
				if (fClickOffset < fText->Length()) {
					fClickOffset = NextInitialByte(fClickOffset);
					if (fClickOffset > selEnd) {
						selEnd = fClickOffset;
						scrollToOffset = selEnd;
					}
					else {
						selStart = fClickOffset;
						scrollToOffset = selStart;
					}
				}
			}
			else {
				if (selStart == selEnd) {
					if (selStart < fText->Length())
						selStart = selEnd = NextInitialByte(selEnd);
				}
				else
					selStart = selEnd;
				scrollToOffset = selEnd;
			}
			break;
	}
	
	// invalidate the null style
	fStyles->InvalidateNullStyle();
	
	int32 saveClickOffset = fClickOffset;
	Select(selStart, selEnd);
	if (selStart == selEnd)
		fClickOffset = selStart;
	else
		fClickOffset = saveClickOffset;

	// scroll if needed
	ScrollToOffset(scrollToOffset);
}


// ------------------------------------------------------------
// 	HandleDelete
// ------------------------------------------------------------
// The Delete key has been pressed

void
BTextView::HandleDelete()
{
	if (fUndo != NULL) {
		_BTypingUndoBuffer_ *undo = dynamic_cast<_BTypingUndoBuffer_ *>(fUndo);
		if (undo == NULL) {
			delete (fUndo);	
			fUndo = undo = new _BTypingUndoBuffer_(this); 
		}
		undo->ForwardErase();	
	}

	if (fSelStart == fSelEnd) {
		if (fSelEnd == fText->Length())
			return;
		else 
			fSelEnd = NextInitialByte(fSelEnd);
	}
	else {
		if (fSelectable)
			Highlight(fSelStart, fSelEnd);
	}

	_BTextChangeResult_ change;
	DoDeleteText(fSelStart, fSelEnd, &change);
	fClickOffset = fSelEnd = fSelStart = change.fDeleteStart;
	
	Refresh(fSelStart, fSelEnd, true, true);
}


// ------------------------------------------------------------
// 	HandlePageKey
// ------------------------------------------------------------
// Home, End, Page Up, or Page Down has been pressed

void
BTextView::HandlePageKey(
	uint32	inPageKey)
{
	BWindow		*window = Window();
	BMessage	*message = window->CurrentMessage();
	uint32		mods = 0;
	if (message)
		message->FindInt32("modifiers", (int32 *)&mods);
	bool		shiftDown = mods & B_SHIFT_KEY;
	bool		controlDown = mods & B_CONTROL_KEY;
	int32		selStart = fSelStart;
	int32		selEnd = fSelEnd;
	int32		scrollToOffset = selStart;

	if( !fEditable ) {
		// if not editable, just scroll in document.
		BRect	bounds = Bounds();
		BPoint  scrollTo = bounds.LeftTop();
		BScrollBar* vbar = ScrollBar(B_VERTICAL);
		BScrollBar* hbar = ScrollBar(B_HORIZONTAL);
	
		switch( inPageKey) {
			case B_PAGE_UP: {
				int32 line = LineAt(BPoint(0, bounds.top+1));
				if( line < CountLines()-2 ) {
					scrollTo.y = (*fLines)[line+2]->origin - bounds.Height() - 1;
				} else {
					scrollTo.y = 0;
				}
			} break;
			case B_PAGE_DOWN: {
				int32 line = LineAt(BPoint(0, bounds.bottom-1));
				if( line > 0 ) {
					scrollTo.y = (*fLines)[line-1]->origin;
				} else {
					scrollTo.y = INT_MAX;
				}
			} break;
			case B_END: {
				scrollTo.x = INT_MAX;
			} break;
			case B_HOME: {
				scrollTo.x = 0;
			} break;
		}
		
		if( hbar ) {
			float minval=0, maxval=0;
			hbar->GetRange(&minval, &maxval);
			if( scrollTo.x < minval ) scrollTo.x = minval;
			if( scrollTo.x > maxval ) scrollTo.x = maxval;
		}
		if( vbar ) {
			float minval=0, maxval=0;
			vbar->GetRange(&minval, &maxval);
			if( scrollTo.y < minval ) scrollTo.y = minval;
			if( scrollTo.y > maxval ) scrollTo.y = maxval;
		}
		if (scrollTo != bounds.LeftTop()) {
			ScrollTo(scrollTo);
			fWhere += scrollTo - bounds.LeftTop();
		};
		
		return;
	}
	
	switch (inPageKey) {
		case B_HOME:
		case B_END:
		{
			if (inPageKey == B_HOME) {
				if (controlDown) {
					scrollToOffset = selStart = 0;
					if (!shiftDown)
						selEnd = selStart;
				}
				else {
					scrollToOffset = selStart = (*fLines)[fLines->OffsetToLine(fSelStart)]->offset;
					if (!shiftDown)
						selEnd = selStart;
				}
			}
			else {
				if (controlDown) {
					scrollToOffset = selEnd = fText->Length();
					if (!shiftDown)
						selStart = selEnd;
				}
				else {
					scrollToOffset = selEnd = (*fLines)[fLines->OffsetToLine(fSelEnd) + 1]->offset - 1;
					if ((selEnd + 1) == fText->Length())
						scrollToOffset = selEnd = fText->Length();
					if (!shiftDown)
						selStart = selEnd;
				}
			}

			ScrollToOffset(scrollToOffset);
			Select(selStart, selEnd);
			break;
		}

		case B_PAGE_UP: 
		case B_PAGE_DOWN:
		{
			BRect bounds = Bounds();

			if (controlDown) {
				//int32 offset = 0;
			
				if (inPageKey == B_PAGE_UP) {
					const STELine *line = (*fLines)[fLines->PixelToLine(
															bounds.top - 
															fTextRect.top)];
					line = (line->origin < bounds.top) ? line + 1 : line;
					//offset = line->offset;
					selStart = line->offset;
					if (!shiftDown)
						selEnd = selStart;
				}
				else {
					const STELine *line  = (*fLines)[fLines->PixelToLine( 
															 bounds.bottom - 
															 fTextRect.top)];
					line = ((line + 1)->origin > bounds.bottom) ? line - 1 : 
																  line;
					//offset = (line + 1)->offset - 1;
					selEnd = (line + 1)->offset - 1;
					if (!shiftDown)
						selStart = selEnd;
				}

				Select(selStart, selEnd);
			}
			else {
				BPoint scrollTo = bounds.LeftTop();
				scrollTo.y += (inPageKey == B_PAGE_UP) ? -bounds.Height() : 
														  bounds.Height();

				// call LineAt() instead of _BLineBuffer_::PixelToLine()
				const STELine *line = (*fLines)[LineAt(scrollTo)];
				scrollTo.y = line->origin;
		  
				float maxRange = (fTextRect.Height() + fTextRect.top) - 
								 bounds.Height();

				maxRange = (maxRange < 0.0) ? 0.0 : maxRange;
				scrollTo.y = (scrollTo.y < 0.0) ? 0.0 : scrollTo.y;
				scrollTo.y = (scrollTo.y > maxRange) ? maxRange : scrollTo.y;
				ScrollTo(scrollTo);
				fWhere += scrollTo - bounds.LeftTop();
				Window()->UpdateIfNeeded();
		  
				if (inPageKey == B_PAGE_UP) {
					selStart = line->offset;
					if (!shiftDown)
						selEnd = selStart;
				}
				else {
					selEnd = line->offset;
					if (!shiftDown)
						selStart = selEnd;
				}				

				Select(selStart, selEnd);
			}
			break;
		}
	}
}


// ------------------------------------------------------------
// 	HandleAlphaKey
// ------------------------------------------------------------
// A printing key has been pressed

void
BTextView::HandleAlphaKey(
	const char	*bytes,
	int32		numBytes)
{
	if (fUndo != NULL) {
		_BTypingUndoBuffer_ *undo = dynamic_cast<_BTypingUndoBuffer_ *>(fUndo);
		if (undo == NULL) {
			delete (fUndo);	
			fUndo = undo = new _BTypingUndoBuffer_(this); 
		}
		undo->InputACharacter(numBytes);	
	}

	int32	textLen = fText->Length();
	int32	refreshStart = fSelStart;
	int32	refreshEnd = fSelEnd;
	bool	refresh = ((fSelStart != textLen) || (fAlignment != B_ALIGN_LEFT));
	
	// is there a selection to delete?
	if (fSelStart != fSelEnd) {
		if (fSelectable)
			Highlight(fSelStart, fSelEnd);
		_BTextChangeResult_ change;
		DoDeleteText(fSelStart, fSelEnd, &change);
		fSelStart = fSelEnd = fSelStart = change.fDeleteStart;
		refresh = true;
	}
	
	// add the alpha key
	_BTextChangeResult_ change;
	DoInsertText(bytes, numBytes, fSelStart, NULL, &change);
	fSelStart = change.fInsertOffset + change.fInsertLength;
	fClickOffset = fSelEnd = fSelStart;
	
	// should we autoindent?
	if ((fAutoindent) && (numBytes == 1) && (bytes[0] == '\n')) {
		const STELine	*line = (*fLines)[fLines->OffsetToLine(fSelStart)];
		int32			numChars = 0;

		for (int32 i = line->offset; i < textLen; i++) {
			uchar theChar = (*fText)[i];
			if ((theChar != '\t') && (theChar != ' '))
				break;	
			numChars++;
		}
				
		if (numChars > 0) {
			char *indent = (char *)malloc(numChars);
			const char *str = fText->GetString(line->offset, &numChars);
			memcpy(indent, str, numChars);
			_BTextChangeResult_ change;
			DoInsertText(indent, numChars, fSelStart, NULL, &change);
			free(indent);		

			fSelStart = change.fInsertOffset + change.fInsertLength;
			fClickOffset = fSelEnd = fSelStart;
			if (fSelEnd > refreshEnd)
				refreshEnd = fSelEnd;
		}
	}
	
	Refresh(refreshStart, refreshEnd, refresh, true);		
}


// ------------------------------------------------------------
// 	InsertText
// ------------------------------------------------------------
// Copy inLength bytes of inText to the buffer, starting at offset
//
// Optionally apply inStyles to the newly inserted text

void
BTextView::InsertText(
	const char				*inText,
	int32					inLength,
	int32					inOffset,
	const text_run_array	*inStyles)
{
	if( fTextChange ) {
		fTextChange->fInsertLength += inLength;
		if( fTextChange->fInsertOffset < 0 ) {
			fTextChange->fInsertOffset = inOffset;
		}
	}
	
	// why add nothing?
	if ((inText == NULL) || (inLength < 1))
		return;

	// add the text to the buffer
	fText->InsertText(inText, inLength, inOffset);

	// update the start offsets of each line below inOffset
	fLines->BumpOffset(inLength, fLines->OffsetToLine(inOffset) + 1);
	
	// update the style runs
	fStyles->BumpOffset(inLength, fStyles->OffsetToRun(inOffset - 1) + 1);

	if ((fStylable) && (inStyles != NULL)) {
		// SetRunArray(inOffset, inOffset + inLength, inStyles);
		int32 numRuns = inStyles->count;
		if (numRuns > 0) {
			if (!fStylable)
				numRuns = 1;
	
			// pin offsets at reasonable values
			int32 textLength = fText->Length();
			int32 startOffset = inOffset;
			int32 endOffset = inOffset + inLength;
			endOffset = (endOffset < 0) ? 0 : endOffset;
			endOffset = (endOffset > textLength) ? textLength : endOffset;
			startOffset = (startOffset < 0) ? 0 : startOffset;
			startOffset = (startOffset > endOffset) ? endOffset : startOffset;
	
			// loop through the style runs
			const text_run *theRun = &inStyles->runs[0];
			for (int32 index = 0; index < numRuns; index++) {
				int32 fromOffset = theRun->offset + startOffset;
				int32 toOffset = endOffset;
				if ((index + 1) < numRuns) {
					toOffset = (theRun + 1)->offset + startOffset;
					toOffset = (toOffset > endOffset) ? endOffset : toOffset;
				}

				BFont theFont(theRun->font);
				NormalizeFont(&theFont);
		
				fStyles->SetStyleRange(fromOffset, toOffset, textLength,
									   B_FONT_ALL, &theFont, &theRun->color);
						
				theRun++;
			}
	
			fStyles->InvalidateNullStyle();
		}
	}
	else {		
		// apply nullStyle to inserted text
		const BFont		*font = NULL;
		const rgb_color	*color = NULL;
		fStyles->SyncNullStyle(inOffset);
		fStyles->GetNullStyle(&font, &color);
		fStyles->SetStyleRange(inOffset, inOffset + inLength, 
							   fText->Length(), B_FONT_ALL, font, color);
	}
}


// ------------------------------------------------------------
// 	DeleteText
// ------------------------------------------------------------
// Remove data that lies between fromOffset and toOffset

void
BTextView::DeleteText(
	int32	fromOffset,
	int32 	toOffset)
{
	// sanity checking
	if ((fromOffset >= toOffset) || (fromOffset < 0) || (toOffset < 0))
		return;
		
	if( fTextChange ) {
		if( fTextChange->fDeleteStart < 0
			|| fTextChange->fDeleteStart > fromOffset ) {
			fTextChange->fDeleteStart = fromOffset;
		}
		if( fTextChange->fDeleteEnd < 0
			|| fTextChange->fDeleteEnd < toOffset ) {
			fTextChange->fDeleteEnd = toOffset;
		}
	}
	
	// set nullStyle to style at beginning of range
	fStyles->InvalidateNullStyle();
	fStyles->SyncNullStyle(fromOffset);	
	
	// remove from the text buffer
	fText->RemoveRange(fromOffset, toOffset);
	
	// remove any lines that have been obliterated
	fLines->RemoveLineRange(fromOffset, toOffset);
	
	// remove any style runs that have been obliterated
	fStyles->RemoveStyleRange(fromOffset, toOffset);
}


// ------------------------------------------------------------
// 	Refresh
// ------------------------------------------------------------
// Recalculate the line breaks from fromOffset to toOffset
// and redraw the text with the new line breaks
//
// If erase is true, the affected text area will be erased  
// before the text is drawn
//
// If scroll is true, the view will be scrolled so that
// the end of the selection is visible

void
BTextView::Refresh(
	int32	fromOffset,
	int32	toOffset,
	bool	erase,
	bool	scroll)
{
	float	saveHeight = fTextRect.Height();
	int32 	fromLine = fLines->OffsetToLine(fromOffset);
	int32 	toLine = fLines->OffsetToLine(toOffset);
	int32	saveFromLine = fromLine;
	int32	saveToLine = toLine;
	float	saveLineHeight = TextHeight(fromLine, fromLine);

	RecalLineBreaks(&fromLine, &toLine);

	if (!fViewColorSet) fViewColor = ViewColor();

	if (Window() != NULL) {
		PushState();
		
		float newHeight = fTextRect.Height();

		// if the line breaks have changed, force an erase
		if ( (fromLine != saveFromLine) || (toLine != saveToLine) || 
			 (newHeight != saveHeight) )
			erase = true;
	
		if (newHeight != saveHeight) {
			// the text area has changed
			if (newHeight < saveHeight)
				toLine = fLines->PixelToLine(saveHeight);
			else
				toLine = fLines->PixelToLine(newHeight);
		}
	
		int32 drawOffset = fromOffset;
		if ( (TextHeight(fromLine, fromLine) != saveLineHeight) || 
			 (newHeight < saveHeight) || (fromLine < saveFromLine) )
			drawOffset = (*fLines)[fromLine]->offset;

		// resize the view as per MakeResizable()
		if (fResizable)
			AutoResize(false);

		DrawLines(fromLine, toLine, drawOffset, erase);

		// erase the area below the text
		BRect bounds = Bounds();
		BRect eraseRect = bounds;
		eraseRect.top = fTextRect.top + (*fLines)[fLines->NumLines()]->origin;
		eraseRect.bottom = fTextRect.top + saveHeight;
		if ( (eraseRect.bottom > eraseRect.top) && 
			 (eraseRect.Intersects(bounds)) ) {
			SetLowColor(fViewColor);
			FillRect(eraseRect, B_SOLID_LOW);
		}

		// update the scroll bars if the text area has changed
		if (newHeight != saveHeight)
			UpdateScrollbars();

		// scroll to the end of the selection?
		if (scroll)
			ScrollToOffset(fSelEnd);
		
		PopState();
	}
}


// ------------------------------------------------------------
// 	RecalLineBreaks
// ------------------------------------------------------------
// Recalculate the line breaks starting at startLine
// Recalculate at least up to endLine
//
// Pass back the range of affected lines in startLine and endLine

void
BTextView::RecalLineBreaks(
	int32	*startLine,
	int32	*endLine)
{
	// are we insane?
	*startLine = (*startLine < 0) ? 0 : *startLine;
	*endLine = (*endLine > fLines->NumLines() - 1) ? fLines->NumLines() - 1 : *endLine;
	
	int32	textLength = fText->Length();
	int32	lineIndex = (*startLine > 0) ? *startLine - 1 : 0;
	int32	recalThreshold = (*fLines)[*endLine + 1]->offset;
	float	width = fTextRect.Width();
	
	// cast away the const-ness
	STELine *curLine = (STELine *)(*fLines)[lineIndex];
	STELine *nextLine = (STELine *)curLine + 1;

	do {
		int32 	fromOffset = curLine->offset;
		float	ascent = 0.0;
		float	descent = 0.0;
		float	strWidth = width;
		int32 	toOffset = FindLineBreak(fromOffset, &ascent, 
										 &descent, &strWidth);

		// we want to advance at least by one character
		if ((toOffset == fromOffset) && (fromOffset < textLength))
			toOffset++;
		
		// set the ascent of this line
		curLine->ascent = ascent;
		curLine->width = strWidth;

		lineIndex++;
		STELine saveLine = *nextLine;		
		if ( (lineIndex > fLines->NumLines()) || 
			 (toOffset < nextLine->offset) ) {
			// the new line comes before the old line start, add a line
			STELine newLine;
			newLine.offset = toOffset;
			newLine.origin = curLine->origin + ascent + descent;
			newLine.ascent = 0.0;
			newLine.width = 0.0;
			fLines->InsertLine(&newLine, lineIndex);
		}
		else {
			// update the exising line
			nextLine->offset = toOffset;
			nextLine->origin = curLine->origin + ascent + descent;
			
			// remove any lines that start before the current line
			while ( (lineIndex < fLines->NumLines()) &&
					(toOffset >= ((*fLines)[lineIndex] + 1)->offset) )
				fLines->RemoveLines(lineIndex + 1);
			
			nextLine = (STELine *)(*fLines)[lineIndex];
			if (nextLine->offset == saveLine.offset) {
				if (nextLine->offset >= recalThreshold) {
					if (nextLine->origin != saveLine.origin)
						fLines->BumpOrigin(nextLine->origin - saveLine.origin, 
										   lineIndex + 1);
					break;
				}
			}
			else {
				if ((lineIndex > 0) && (lineIndex == *startLine))
					*startLine = lineIndex - 1;
			}
		}

		curLine = (STELine *)(*fLines)[lineIndex];
		nextLine = (STELine *)curLine + 1;
	} while (curLine->offset < textLength);

	// update the text rect
	float newHeight = TextHeight(0, fLines->NumLines() - 1);
	fTextRect.bottom = fTextRect.top + newHeight;

	*endLine = lineIndex - 1;
	*startLine = (*startLine > *endLine) ? *endLine : *startLine;
}


// ------------------------------------------------------------
// 	FindLineBreak
// ------------------------------------------------------------
// Determine where to break a line that is ioWidth wide, 
// starting at fromOffset
//
// Pass back the maximum ascent and descent for the line in
// outAscent and outDescent
// Set ioWidth to the width of the string in the line

int32
BTextView::FindLineBreak(
	int32	fromOffset,
	float	*outAscent,
	float	*outDescent,
	float	*ioWidth)
{
	const float	width = *ioWidth;
	const int32	limit = fText->Length();
	
	*outAscent = 0.0;
	*outDescent = 0.0;
	*ioWidth = 0.0;

	// is fromOffset at the end?
	if (fromOffset >= limit) {
		// try to return valid height info anyway			
		if (fStyles->NumRuns() > 0)
			fStyles->Iterate(fromOffset, 1, NULL, NULL, NULL, outAscent, outDescent);
		else {
			if (fStyles->IsValidNullStyle()) {
				const BFont		*theFont = NULL;
				const rgb_color	*theColor = NULL;
				fStyles->GetNullStyle(&theFont, &theColor);
				font_height			fontHeight;
				theFont->GetHeight(&fontHeight);

				*outAscent = ceil(fontHeight.ascent);
				*outDescent = ceil(fontHeight.descent) + 
							  ceil(fontHeight.leading);
			}
		}

		return (limit);
	}
	
	bool	done = false;
	float	ascent = 0.0;
	float	descent = 0.0;
	int32	offset = fromOffset;
	int32	delta = 0;
	float	deltaWidth = 0.0;
	float	tabWidth = 0.0;
	float	strWidth = 0.0;
	
	// maybe we don't need to wrap the text?
	if (!DoesWordWrap() || IsResizable()) {
		fStyles->Iterate(fromOffset, 1, NULL, NULL, NULL, outAscent, outDescent);
		int32 length = limit - fromOffset;
		fText->FindChar('\n', fromOffset, &length);
		offset = fromOffset + length + 1;
		offset = (offset > limit) ? limit : offset;
		
		int32	numChars = length;
		bool	foundTab = false;		
		do {
			foundTab = fText->FindChar('\t', fromOffset, &numChars);

			*ioWidth += StyledWidth(fromOffset, numChars, &ascent, &descent);
			*outAscent = (ascent > *outAscent) ? ascent : *outAscent;
			*outDescent = (descent > *outDescent) ? descent : *outDescent;
		
			if (foundTab) {
				int32 numTabs = 0;
				for (numTabs = 0; (numChars + numTabs) < length; numTabs++) {
					if ((*fText)[fromOffset + numChars + numTabs] != '\t')
						break;
				}
										
				float tabWidth = ActualTabWidth(*ioWidth);
				if (numTabs > 1)
					tabWidth += ((numTabs - 1) * fTabWidth);
	
				*ioWidth += tabWidth;
				numChars += numTabs;
			}
			
			fromOffset += numChars;
			length -= numChars;
			numChars = length;
		} while ((foundTab) && (length > 0));

		return (offset);
	}
	
	// wrap the text
	do {
		int32 tabCount = 0;

		// find the next line break candidate
		for ( ; (offset + delta) < limit ; delta++) {
			uchar theChar = fText->RealCharAt(offset + delta);
			if (IsInitialUTF8Byte(theChar)) {
				if (CanEndLine(offset + delta)) {
					delta += UTF8CharLen(theChar) - 1;
					break;
				}
			}
		}

		// add trailing spaces and tabs to delta
		for ( ; (offset + delta) < limit; delta++) {
			uchar theChar = fText->RealCharAt(offset + delta);
			if (!IsInitialUTF8Byte(theChar)) 
				continue;

			if (!CanEndLine(offset + delta))
				break;
			
			if (theChar == '\n') {
				// found a newline, we're done!
				done = true;
				delta++;
				break;
			}
			else {
				// include all trailing spaces and tabs,
				// but not spaces after tabs
				if ((theChar != ' ') && (theChar != '\t'))
					break;
				else {
					if ((theChar == ' ') && (tabCount > 0))
						break;
					else {
						if (theChar == '\t')
							tabCount++;
					}
				}
			}
		}
		delta = (delta < 1) ? 1 : delta;

		deltaWidth = StyledWidth(offset, delta, &ascent, &descent);
		strWidth += deltaWidth;

		if (tabCount < 1)
			tabWidth = 0.0;
		else {
			tabWidth = ActualTabWidth(strWidth);
			if (tabCount > 1)
				tabWidth += ((tabCount - 1) * fTabWidth);

			strWidth += tabWidth;
		}
		
		if (strWidth >= width) {
			// we've found where the line will wrap
			int32	pos = delta - 1;
			uchar	theChar = '\0';
			bool	foundNewline = done;
			done = true;
					
			strWidth -= (deltaWidth + tabWidth);
	
			theChar = (*fText)[offset + pos];
			if ( (theChar != ' ') && 
				 (theChar != '\t') && 
				 (theChar != '\n') )
				break;
			
			for ( ; (offset + pos) > offset; pos--) {
				theChar = (*fText)[offset + pos];
				if ( (theChar != ' ') &&
					 (theChar != '\t') &&
					 (theChar != '\n') )
					break;
			}

			deltaWidth = StyledWidth(offset, pos + 1, &ascent, &descent);
			if ((strWidth + deltaWidth) >= width)
				break;

			if (!foundNewline) {
				strWidth += deltaWidth;
			
				for ( ; (offset + delta) < limit; delta++) {
					theChar = (*fText)[offset + delta];
					if ((theChar != ' ') && (theChar != '\t'))
						break;
				}
				
				if ( ((offset + delta) < limit) && 
					 ((*fText)[offset + delta] == '\n') )
					delta++;
			}
			
			// get the ascent and descent of the spaces/tabs
			StyledWidth(offset, delta, &ascent, &descent);
		}
		
		*outAscent = (ascent > *outAscent) ? ascent : *outAscent;
		*outDescent = (descent > *outDescent) ? descent : *outDescent;
		
		offset += delta;
		delta = 0;
	} while ((offset < limit) && (!done));
	
	if ((offset - fromOffset) < 1) {
		// there wasn't a single break in the line, force a break
		*outAscent = 0.0;
		*outDescent = 0.0;
		strWidth = 0.0;
		
		offset = fromOffset;
		while (offset < limit) {
			int32 charLen = UTF8CharLen(fText->RealCharAt(offset));

			deltaWidth = StyledWidth(offset, charLen, &ascent, &descent);
			strWidth += deltaWidth;
			
			if (strWidth >= width) {
				strWidth -= deltaWidth;
				break;
			}
				
			*outAscent = (ascent > *outAscent) ? ascent : *outAscent;
			*outDescent = (descent > *outDescent) ? descent : *outDescent;
			offset += charLen;
		}
	}
	
	offset = (offset < limit) ? offset : limit;
	*ioWidth = strWidth;

	return (offset);
}


// ------------------------------------------------------------
// 	StyledWidth
// ------------------------------------------------------------
// Return the width of length bytes of styled text beginning at
// fromOffset
//
// Pass back the maximum ascent and maximum descent of the text
// in outAscent and outDescent
//
// Pass NULL for outAscent and/or outDescent if you are not
// interested in that data
//
// Tab-widths are not calculated, use PointAt() if you need
// tab-inclusive widths

float
BTextView::StyledWidth(
	int32	fromOffset,
	int32 	length,
	float	*outAscent,
	float	*outDescent) const
{
	float result = 0.0;
	float ascent = 0.0;
	float descent = 0.0;
	float maxAscent = 0.0;
	float maxDescent = 0.0;
	
	// iterate through the style runs
	const BFont *font = NULL;
	while ( int32 numChars = fStyles->Iterate(fromOffset, length, NULL, &font, NULL, 
											  &ascent, &descent) ) {		
		maxAscent = (ascent > maxAscent) ? ascent : maxAscent;
		maxDescent = (descent > maxDescent) ? descent : maxDescent;

		LockWidthBuffer();
		result += sWidths->StringWidth(*fText, fromOffset, numChars, font);
		UnlockWidthBuffer();

		fromOffset += numChars;
		length -= numChars;
	}

	if (outAscent != NULL)
		*outAscent = maxAscent;
	if (outDescent != NULL)
		*outDescent = maxDescent;

	return (result);
}


// ------------------------------------------------------------
// 	ActualTabWidth
// ------------------------------------------------------------
// Return the actual tab width at location 
// 
// location should be in text rect coordinates

float
BTextView::ActualTabWidth(
	float	location) const
{
	return ( fTabWidth - 
			 (location - ((int32)(location / fTabWidth)) * fTabWidth) );
}

// ------------------------------------------------------------
// 	DoInsertText
// ------------------------------------------------------------
// Call InsertText(), setting up state to notice changes by
// child classes.

void
BTextView::DoInsertText(
	const char				*inText, 
	int32					inLength, 
	int32					inOffset,
	const text_run_array	*inRuns,
	_BTextChangeResult_		*outResult)
{
	fTextChange = outResult;
	if( outResult ) {
		outResult->fInsertLength = 0;
		outResult->fInsertOffset = -1;
	}
	if (inText) InsertText(inText, inLength, inOffset, inRuns);
	if( outResult && outResult->fInsertOffset < 0 ) {
		outResult->fInsertOffset = 0;
	}
	fTextChange = NULL;
}

// ------------------------------------------------------------
// 	DoDeleteText
// ------------------------------------------------------------
// Call DeleteText(), setting up state to notice changes by
// child classes.

void BTextView::DoDeleteText(
	int32 				fromOffset,
	int32 				toOffset,
	_BTextChangeResult_	*outResult)
{
	fTextChange = outResult;
	if( outResult ) {
		outResult->fDeleteStart = -1;
		outResult->fDeleteEnd = -1;
	}
	DeleteText(fromOffset, toOffset);
	if( outResult ) {
		if( outResult->fDeleteStart < 0 ) outResult->fDeleteStart = 0;
		if( outResult->fDeleteEnd < 0 ) outResult->fDeleteEnd = 0;
	}
	fTextChange = NULL;
}

// ------------------------------------------------------------
// 	DrawLines
// ------------------------------------------------------------
// Draw the lines from startLine to endLine
// Erase the affected area before drawing if erase is true
//
// startOffset gives the offset of the first character in startLine
// that needs to be erased (avoids flickering of the entire line)
//
// Pass a value of -1 in startOffset if you want the entire 
// line of startLine to be erased 
 
void
BTextView::DrawLines(
	int32	startLine,
	int32	endLine,
	int32	startOffset,
	bool	erase)
{	
	BRect bounds = Bounds();
	
	// clip the text	
	BRect clipRect = bounds & fTextRect;
	clipRect.InsetBy(-1.0, -1.0);
	BRegion newClip;
	newClip.Set(clipRect);
	ConstrainClippingRegion(&newClip);

	// set the low color to the view color so that 
	// drawing to a non-white background will work	
	rgb_color viewColor = (fPrintWhiteBackground && IsPrinting())
						? make_color(255, 255, 255) : fViewColor;
	SetLowColor(viewColor);

	// draw only those lines that are visible
	int32 startVisible = fLines->PixelToLine(bounds.top - fTextRect.top);
	int32 endVisible = fLines->PixelToLine(bounds.bottom - fTextRect.top);
	startLine = (startLine < startVisible) ? startVisible : startLine;
	endLine = (endLine > endVisible) ? endVisible : endLine;

	BRect 			eraseRect = clipRect;
	int32			startEraseLine = startLine;
	const STELine	*line = (*fLines)[startLine];

	if ((fOffscreen == NULL || IsPrinting()) && (erase) && (startOffset != -1)) {
		startEraseLine++;
		
		if ((!DoesWordWrap()) || (fAlignment == B_ALIGN_LEFT) || (IsResizable())) { 
			// erase only portion of first line to reduce flickering
			int32 startErase = startOffset;
			if (startErase > line->offset) {
				while (startErase > line->offset) {
					uchar theChar = (*fText)[startErase];
					if ((theChar == ' ') || (theChar == '\t'))
						break;
					startErase--;
				}
				if (startErase > line->offset)
					startErase--;
			}
			eraseRect.left = PointAt(startErase).x;
		}
		
		eraseRect.top = line->origin + fTextRect.top;
		eraseRect.bottom = (line + 1)->origin + fTextRect.top;
		FillRect(eraseRect, B_SOLID_LOW);
		eraseRect = clipRect;
	}
	
	for (int32 i = startLine; i <= endLine; i++) {
		const int32 nextOffset = (line + 1)->offset;
		int32		length = nextOffset - line->offset;
		BPoint		penLoc(0.0, 0.0);
		float		penDelta = 0.0;		
		BView		*drawView = this;
		
		// take alignment into accout
		switch (fAlignment) {
			case B_ALIGN_RIGHT:
				penDelta = fTextRect.Width() - line->width;
				break;
					
			case B_ALIGN_CENTER:
				penDelta = (fTextRect.Width() - line->width) / 2;
				break;
					
			case B_ALIGN_LEFT:
			default:
				break;
		}

		// DrawString() chokes if you draw a newline
		if (nextOffset > 0 && (*fText)[nextOffset - 1] == '\n')
			length--;
		
		if ((fOffscreen == NULL || IsPrinting()) || (i > startLine) || (startOffset == -1)) {
			if ((erase) && (i >= startEraseLine)) {
				eraseRect.top = line->origin + fTextRect.top;
				eraseRect.bottom = (line + 1)->origin + fTextRect.top;
				
				FillRect(eraseRect, B_SOLID_LOW);
			}
			
			penDelta += fTextRect.left;
			penLoc.Set(penDelta, line->origin + line->ascent + fTextRect.top);
			MovePenTo(penLoc);			
		}
		else {
			startEraseLine++;
			
			fOffscreen->Lock();
			drawView = fOffscreen->ChildAt(0);
			
			BRect	offBounds = fOffscreen->Bounds();
			float	lineHeight = (line + 1)->origin - line->origin;
			if (offBounds.Height() < lineHeight) {
				// bitmap isn't tall enough for the current line, resize
				delete (fOffscreen);
				fOffscreen = NULL;

				offBounds.bottom = lineHeight;
				fOffscreen = new BBitmap(offBounds, fColorSpace, true);
				drawView = new BView(offBounds, B_EMPTY_STRING, B_FOLLOW_NONE, 0);
				fOffscreen->Lock();				
				fOffscreen->AddChild(drawView);
			}
			
			drawView->SetLowColor(viewColor);
			drawView->FillRect(offBounds, B_SOLID_LOW);
			
			penLoc.Set(penDelta, line->ascent);
			drawView->MovePenTo(penLoc);
		}
		
		// do we have any text to draw?
		if (length > 0) {
			// iterate through each style on this line
			BPoint			startPenLoc = penLoc;
			bool			foundTab = false;
			int32			tabChars = 0;
			int32			offset = line->offset;
			const BFont		*font = NULL;
			const rgb_color	*color = NULL;
			float			ascent = 0.0;
			float			descent = 0.0;
			uint32			inlineFlags = kInlineNoClause;
			bool			inClause = false;

			while (int32 numChars = fStyles->Iterate(offset, length, fInline, &font, &color, &ascent, &descent, &inlineFlags)) {
				drawView->SetFont(font);
				drawView->SetHighColor(*color);
				tabChars = numChars;
				inClause = inlineFlags & kInlineInClause;

				do {
					startPenLoc = penLoc;

					foundTab = fText->FindChar('\t', offset, &tabChars);					

					LockWidthBuffer();
					penLoc.x += sWidths->StringWidth(*fText, offset, tabChars, font);
					UnlockWidthBuffer();					

					if (inClause) {
						BRect clauseRect;
						clauseRect.left = startPenLoc.x;
						clauseRect.top = startPenLoc.y - ascent + 1.0;
						clauseRect.right = penLoc.x - 3.0;
						clauseRect.bottom = startPenLoc.y + descent - 1.0;

						drawView->SetLowColor((inlineFlags & kInlineSelectedClause) ? kSelectedClauseLowColor : kClauseLowColor);
						drawView->FillRect(clauseRect, B_SOLID_LOW);
					}

					int32 ch = tabChars;
					const char *str = fText->GetString(offset, &ch);
					drawView->DrawString(str, ch);

					if (inClause)
						drawView->SetLowColor(viewColor);

					if (foundTab) {
						int32 numTabs = 0;
						for (numTabs = 0; (tabChars + numTabs) < numChars; numTabs++) {
							if ((*fText)[offset + tabChars + numTabs] != '\t')
								break;
						}
												
						float tabWidth = ActualTabWidth(penLoc.x - penDelta);
						if (numTabs > 1)
							tabWidth += ((numTabs - 1) * fTabWidth);

						penLoc.x += tabWidth;
						drawView->MovePenTo(penLoc);
						
						tabChars += numTabs;
					}

					offset += tabChars;
					length -= tabChars;
					numChars -= tabChars;
					tabChars = numChars;
				} while ((foundTab) && (tabChars > 0));
			}
		}
		
		if (drawView != this) {
			float lineWidth = fTextRect.Width();
			float lineHeight = (line + 1)->origin - line->origin;
			
			BRect srcRect = fOffscreen->Bounds();
			srcRect.bottom = srcRect.top + lineHeight;
			srcRect.right = srcRect.left + lineWidth;
			
			BRect dstRect;
			dstRect.left = fTextRect.left;
			dstRect.top = line->origin + fTextRect.top;	
			dstRect.right = dstRect.left + lineWidth;
			dstRect.bottom = dstRect.top + lineHeight;

			drawView->Sync();
			SetLowColor(fViewColor);
			DrawBitmap(fOffscreen, srcRect, dstRect);
			
			fOffscreen->Unlock();
		}
		
		line++;
	}

	ConstrainClippingRegion(NULL);
}


// ------------------------------------------------------------
// 	Highlight
// ------------------------------------------------------------
// Highlight the characters between startOffset and endOffset

void
BTextView::Highlight(
	int32	startOffset,
	int32 	endOffset)
{		
	// get real
	if (startOffset >= endOffset)
		return;

	// don't highlight when an active input area exists!
	if (fInline != NULL)
		return;
		
	BRegion selRegion;
	GetTextRegion(startOffset, endOffset, &selRegion);

	SetDrawingMode(B_OP_INVERT);
	FillRegion(&selRegion);
	SetDrawingMode(B_OP_COPY);
}


// ------------------------------------------------------------
// 	DrawCaret
// ------------------------------------------------------------
// Draw the caret at offset

void
BTextView::DrawCaret(
	int32	offset)
{
	float	lineHeight = 0.0;
	BPoint	caretPoint = PointAt(offset, &lineHeight);
	caretPoint.x = (caretPoint.x > fTextRect.right) ? fTextRect.right : caretPoint.x;

	BRect caretRect;
	caretRect.left = caretRect.right = caretPoint.x;
	caretRect.top = caretPoint.y;
	caretRect.bottom = caretPoint.y + lineHeight - 1.0;

	InvertRect(caretRect);
}


// ------------------------------------------------------------
// 	InvertCaret
// ------------------------------------------------------------
// Invert the caret at fSelStart

void
BTextView::InvertCaret()
{
	DrawCaret(fSelStart);
	fCaretVisible = !fCaretVisible;
	fCaretTime = system_time();
}


// ------------------------------------------------------------
// 	DragCaret
// ------------------------------------------------------------
// Draw a temporary caret at offset 

void
BTextView::DragCaret(
	int32	offset)
{
	// does the caret need to move?
	if (offset == fDragOffset)
		return;
	
	// hide the previous drag caret
	if (fDragOffset != -1)
		DrawCaret(fDragOffset);
		
	// do we have a new location?
	if (offset != -1) {
		if (fActive) {
			// ignore if offset is within active selection
			if ((offset >= fSelStart) && (offset <= fSelEnd)) {
				fDragOffset = -1;
				return;
			}
		}
		
		DrawCaret(offset);
	}
	
	fDragOffset = offset;
}


// ------------------------------------------------------------
// 	TrackMouse
// ------------------------------------------------------------
// Track the mouse while it is over this view

void
BTextView::TrackMouse(
	BPoint			where,
	const BMessage	*message,
	bool force)
{
	if ((fEditable) && (message != NULL)) {
		// something is being dragged
		if ((fDragRunner) || (AcceptsDrop(message)))
			TrackDrag(where);
			
	} else {
		bool ibeam = fEditable || fSelectable;

		if( !Window() || !Window()->IsActive() ) {
			// Cursor is always the default when window isn't active.
			ibeam = false;
			
		} else if (fTrackingMouse) {
			// Always show i-beam while selecting.
			ibeam = true;
			
		} else if (fDragRunner) {
			// HACK! a drag has been initiated but the drag message 
			// isn't coming through with the B_MOUSE_MOVED messages yet
			ibeam = false;
			
		} else {
			if ((fActive) && (fSelectable) && (fSelStart != fSelEnd)) {
				// should the cursor be a hand to indicate dragability?
				BRegion textRegion;
				GetTextRegion(fSelStart, fSelEnd, &textRegion);
				if (textRegion.Contains(where)) ibeam = false;
			}
		}

		if (force || fCursor != ibeam) {
			SetViewCursor((ibeam) ? B_CURSOR_I_BEAM : B_CURSOR_SYSTEM_DEFAULT);
			fCursor = ibeam;
		};
	}
}


// ------------------------------------------------------------
// 	StopMouseTracking
// ------------------------------------------------------------
// Stop watching the mouse with buttons down.
//
// Don't confuse this with TrackMouse(), which is for mouse
// movements when the button is up.

void
BTextView::StopMouseTracking()
{
	if( fTrackingMouse ) {
		delete fTrackingMouse;
		fTrackingMouse = 0;
	}
}

// ------------------------------------------------------------
// 	TrackDrag
// ------------------------------------------------------------
// Track and give feedback for a drag
//
// This function gets called repeatedly while there is a drag
// that needs to be tracked
//
// When mDragOwner is true, this function will get called
// even when the drag is not above this view
//
// When this view is the drag owner (mDragOwner), it is assumed
// that it can handle its own drag (AcceptsDrop() is not consulted)

void
BTextView::TrackDrag(
	BPoint	where)
{
	BRect bounds = Bounds();

	// are we the drag owner?
	if (!fDragRunner) {
		// drag isn't ours, don't worry about auto-scrolling
		if (bounds.Contains(where))
			DragCaret(OffsetAt(where));
		return;
	}
	
	BRect expandedBounds = bounds;
	// expand the bounds
	expandedBounds.InsetBy(-B_V_SCROLL_BAR_WIDTH, -B_H_SCROLL_BAR_HEIGHT);
	
	// is the mouse within the expanded bounds?
	if (expandedBounds.Contains(where)) {
		float  hDelta = 0.0;
		float  vDelta = 0.0;
		BPoint scrollTo = bounds.LeftTop();

		// left edge
		if (where.x < bounds.left)
			hDelta = where.x - bounds.left;
		else {
		  // right edge
			if (where.x > bounds.right)
				hDelta = where.x - bounds.right;
		}
			
		if (hDelta != 0.0) {
			if ( (fResizable) && (fContainerView == NULL) && 
				 (fAlignment != B_ALIGN_LEFT) ) {
				if( fTextRect.Width() < bounds.Width() ) {
					scrollTo.x = 0;
				} else {
					scrollTo.x += hDelta;
					scrollTo.x = (scrollTo.x < fTextRect.left) ? fTextRect.left : scrollTo.x;
					scrollTo.x = (scrollTo.x > (fTextRect.right-Bounds().Width())) ? (fTextRect.right-bounds.Width()) : scrollTo.x; 
				}
			}
			else {
				float maxRange = (fTextRect.Width() + fTextRect.left) - bounds.Width();
				maxRange = (maxRange < 0.0) ? 0.0 : maxRange;
				scrollTo.x += (hDelta * 5.0);
				scrollTo.x = (scrollTo.x < 0.0) ? 0.0 : scrollTo.x;
				scrollTo.x = (scrollTo.x > maxRange) ? maxRange : scrollTo.x;
			}
		}
		
		// top edge
		if (where.y < bounds.top) 
			vDelta = where.y - bounds.top;
		else {
			// bottom edge
			if (where.y > bounds.bottom)
				vDelta = where.y - bounds.bottom;
		}
		
		if (vDelta != 0.0) {
			float maxRange = (fTextRect.Height() + fTextRect.top) - bounds.Height();
			maxRange = (maxRange < 0.0) ? 0.0 : maxRange;
			scrollTo.y += (vDelta * 5.0);
			scrollTo.y = (scrollTo.y < 0.0) ? 0.0 : scrollTo.y;
			scrollTo.y = (scrollTo.y > maxRange) ? maxRange : scrollTo.y;
		}
		
		if ((hDelta != 0.0) || (vDelta != 0.0)) {
			DragCaret(-1);
			if (scrollTo != bounds.LeftTop()) {
				ScrollTo(scrollTo);
				fWhere += scrollTo - bounds.LeftTop();
				Window()->UpdateIfNeeded();
			}
		}
		else
			DragCaret(OffsetAt(where));
	}
}


// ------------------------------------------------------------
// 	InitiateDrag
// ------------------------------------------------------------
// Create a new drag message and pass it on to the app_server

void
BTextView::InitiateDrag()
{	
	BMessage	drag(B_MIME_DATA);
	BBitmap		*bitmap = NULL;
	BPoint		point(0.0, 0.0);
	BHandler	*handler = NULL;

	// find out everything we need to know about this drag
	GetDragParameters(&drag, &bitmap, &point, &handler);

	SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);

	if (bitmap != NULL)
		DragMessage(&drag, bitmap, point, handler);
	else {
		BRegion highlightRgn;
		GetTextRegion(fSelStart, fSelEnd, &highlightRgn);
		BRect dragRect = highlightRgn.Frame();
		BRect bounds = Bounds();	
		if (!bounds.Contains(dragRect))
			dragRect = bounds & dragRect;
			
		DragMessage(&drag, dragRect, handler);
	}

	BMessage msg(_DISPOSE_DRAG_); // We'll just use this reserved constant
	fDragRunner = new BMessageRunner(BMessenger(this),&msg,100000);
	SetMouseEventMask(B_POINTER_EVENTS,B_NO_POINTER_HISTORY);
}


// ------------------------------------------------------------
// 	MessageDropped
// ------------------------------------------------------------
// Respond to dropped messages

bool
BTextView::MessageDropped(
	BMessage	*inMessage,
	BPoint 		where,
	BPoint		/*offset*/)
{
	// figure out if this is our own drag
	void* orig=0;
	inMessage->FindPointer("be:originator", &orig);
	bool dragOwner = (orig == this) ? true : false;

	// make sure the drag caret is erased
	DragCaret(-1);

	// HACK! the following if-block should be at the beginning of the function...
	// Temporarily moved here until the B_MESSAGE_NOT_UNDERSTOOD->WasDropped()
	// bug gets fixed
	if (fDragRunner) {
		// our drag has come to an end
		delete fDragRunner;
		fDragRunner = NULL;
	}

	// see if we need to reset the cursor
	if (fActive)
		TrackMouse(where, NULL);
	
	// are we sure we like this message?
	if (!AcceptsDrop(inMessage))
		return (false);

	int32 		dropOffset = OffsetAt(where);
	const char	*text = NULL;
	int32		dataLen = 0;
	inMessage->FindData("text/plain", B_MIME_TYPE, (const void **)&text, &dataLen);

	if (text == NULL)
		return (false);

	text_run_array *styles = NULL;
	int32		styleLen;
	if (fStylable) {
		inMessage->FindData("application/x-vnd.Be-text_run_array", B_MIME_TYPE, 
							(const void **)&styles, &styleLen);
	}

	// if this was our drag, move instead of copy
	if (dragOwner && (fSelStart != fSelEnd)) {
		// dropping onto itself?
		if ((dropOffset >= fSelStart) && (dropOffset <= fSelEnd))
			return (true);
			
		// adjust the offset if the drop is after the selection
		if (dropOffset > fSelEnd)
			dropOffset -= (fSelEnd - fSelStart);
			
		// delete the selection
		if (fSelectable)
			Highlight(fSelStart, fSelEnd);
		
		// store undo state for drag portion and delete selection
		if (fUndo != NULL) {
			delete (fUndo);
			fUndo = new _BDropUndoBuffer_(this, text, dataLen,
										  styles, styleLen,
										  dropOffset, dragOwner);
		}
		_BTextChangeResult_ change;
		DoDeleteText(fSelStart, fSelEnd, &change);
		
		fClickOffset = fSelEnd = fSelStart = change.fDeleteStart;
		Refresh(fSelStart, fSelEnd, true, false);
		
	} else {
		dragOwner = false;
	}
		
	Select(dropOffset, dropOffset);

	if( !dragOwner ) {
		// create undo state for drop portion.
		if (fUndo != NULL) {
			delete (fUndo);
			fUndo = new _BDropUndoBuffer_(this, text, dataLen,
										  styles, styleLen,
										  dropOffset, dragOwner);
		}
	}
	
	Insert(text, dataLen, styles);
	ScrollToOffset(fSelEnd);
/*
	else {
		int32	v;
		uchar	theChar;
		if (inMessage->FindInt32("char", &v) == B_NO_ERROR) {
			theChar = v;
			Insert((char *)&theChar, 1);
			ScrollToOffset(fSelEnd);
		}
	}
*/		
	return (true);
}


// ------------------------------------------------------------
// 	UpdateScrollbars
// ------------------------------------------------------------
// Adjust the scroll bars so that they reflect the current 
// visible size and position of the text area

void
BTextView::UpdateScrollbars()
{
	BRect bounds = Bounds();
	
	// do we have a horizontal scroll bar?
	BScrollBar *hScroll = ScrollBar(B_HORIZONTAL);
	if (hScroll != NULL) {
		float viewWidth = bounds.Width();
		float dataWidth = fTextRect.Width() + fTextRect.left;
		
		float maxRange = dataWidth - viewWidth;
		maxRange = (maxRange < 0.0) ? 0.0 : maxRange;
		
		hScroll->SetProportion(bounds.Width() / fTextRect.Width());
		hScroll->SetRange(0.0, maxRange);
		hScroll->SetSteps(10.0, dataWidth / 10.0);
	}
	
	// how about a vertical scroll bar?
	BScrollBar *vScroll = ScrollBar(B_VERTICAL);
	if (vScroll != NULL) {
		float viewHeight = bounds.Height();
		float dataHeight = fTextRect.Height() + fTextRect.top;
		
		float maxRange = dataHeight - viewHeight;
		maxRange = (maxRange < 0.0) ? 0.0 : maxRange;
	
		vScroll->SetProportion(bounds.Height() / fTextRect.Height());
		vScroll->SetRange(0.0, maxRange);
		vScroll->SetSteps(12.0, viewHeight);
	}
}


// ------------------------------------------------------------
// 	AutoResize
// ------------------------------------------------------------
// Resize this view and its container to match the size
// of the text

void
BTextView::AutoResize(bool doredraw)
{
	float lineWidth = ceil(LineWidth());
	float delta = lineWidth - fTextRect.Width();

	const BRect bounds(Bounds());
	const BRect oldTextRect(fTextRect & bounds);
	
	switch (fAlignment) {
		case B_ALIGN_LEFT:
			fTextRect.right += delta;
			break;

		case B_ALIGN_CENTER:
			if (fContainerView == NULL)
				fTextRect.InsetBy(-(delta / 2), 0.0);
			else {
				fContainerView->MoveBy(-(delta / 2), 0.0);
				fTextRect.right += delta;
			}
			break;
 
		case B_ALIGN_RIGHT:
			if (fContainerView == NULL) 
				fTextRect.left -= delta;
			else {
				fContainerView->MoveBy(-delta, 0.0);
				fTextRect.right += delta;
			}
			break;
	}

	if (fContainerView != NULL) {
		fContainerView->ResizeBy(delta, 0.0);
		fContainerView->Invalidate();
	}

	if (fOffscreen != NULL) {
		if (fOffscreen->Bounds().Width() <= fTextRect.Width()) {
			// resize the offscreen so that its 30.0 pixels 
			// wider than needed, this prevents it from being
			// resized after every key input
			DeleteOffscreen();
			NewOffscreen(30.0);
		}	
	}

	if (Window()) PushState();
	
	if (delta > 0.0 && doredraw) {
		DrawLines(0, 0);
	}
	
	const BRect newTextRect(fTextRect & bounds);
	
	// set the low color to the view color so that 
	// drawing to a non-white background will work	
	if (!fViewColorSet) fViewColor = ViewColor();
	SetLowColor(fViewColor);
	
	// Erase any portions that are no longer covered by text rectangle
	if( oldTextRect.left < newTextRect.left ) {
		FillRect(BRect(oldTextRect.left, oldTextRect.top,
					   newTextRect.left, oldTextRect.bottom),
				 B_SOLID_LOW);
	}
	if( oldTextRect.right > newTextRect.right ) {
		FillRect(BRect(newTextRect.right, oldTextRect.top,
					   oldTextRect.right, oldTextRect.bottom),
				 B_SOLID_LOW);
	}
	
	if (Window()) PopState();
}


// ------------------------------------------------------------
// 	NewOffscreen
// ------------------------------------------------------------
// Create a new offscreen bitmap

void
BTextView::NewOffscreen(
	float	padding)
{
	if (fOffscreen != NULL)
		DeleteOffscreen();

	BRect offBounds;
	offBounds.left = 0.0;
	offBounds.top = 0.0;
	offBounds.right = fTextRect.Width() + padding;
	offBounds.bottom = (*fLines)[1]->origin - (*fLines)[0]->origin;
	fOffscreen = new BBitmap(offBounds, fColorSpace, true);
	fOffscreen->Lock();
	fOffscreen->AddChild(new BView(offBounds, B_EMPTY_STRING, B_FOLLOW_NONE, 0));
	fOffscreen->Unlock();
}


// ------------------------------------------------------------
// 	DeleteOffscreen
// ------------------------------------------------------------
//  Delete the offscreen bitmap

void
BTextView::DeleteOffscreen()
{
	if (fOffscreen == NULL)
		return;

	fOffscreen->Lock();
	delete (fOffscreen);
	fOffscreen = NULL;
}

// ------------------------------------------------------------
// 	Activate
// ------------------------------------------------------------
// Activate the text area
//
// Draw the caret/highlight the selection, set the cursor

void
BTextView::Activate()
{
	fActive = true;

	NewOffscreen();

	if (fSelStart != fSelEnd) {
		if (fSelectable)
			Highlight(fSelStart, fSelEnd);
	}
	else {
		if (fEditable)
			InvertCaret();
	}
}


// ------------------------------------------------------------
// 	Deactivate
// ------------------------------------------------------------
// Deactivate the text area
//
// Hide the caret/unhighlight the selection, set the cursor

void
BTextView::Deactivate()
{
	fActive = false;

	CancelInputMethod();
	
	DeleteOffscreen();

	if (fSelStart != fSelEnd) {
		if (fSelectable)
			Highlight(fSelStart, fSelEnd);
	}
	else {
		if (fCaretVisible)
			InvertCaret();
	}
}


// ------------------------------------------------------------
//	NormalizeFont
// ------------------------------------------------------------
// Make sure the fonts are to BTextView's liking

void
BTextView::NormalizeFont(
	BFont	*font)
{
	font->SetRotation(0.0);
	font->SetSpacing(B_BITMAP_SPACING);
	font->SetEncoding(B_UNICODE_UTF8);
	font->SetFlags(0);
}


// ------------------------------------------------------------
//	CharClassification
// ------------------------------------------------------------
// Try to classify the character at offset into something
// interesting

uint32
BTextView::CharClassification(
	int32	offset) const
{
	uchar	firstByte = (*fText)[offset];	
	int32	charLen = UTF8CharLen(fText->RealCharAt(offset));
	int32	textLen = fText->Length();

	// sanity checking
	if ((offset + charLen) > textLen)
		return (kUnclassifiedChar);

	uint32 theChar = fText->UTF8CharToUint32(offset, charLen);
	
	if (isspace(firstByte))
		// POSIX space
		return (kSpaceChar);
	else if ( (ispunct(firstByte)) || 
			  ((theChar >= 0xE2808000) && (theChar <= 0xE281AF00)) ||
			  ((theChar >= 0xE3808000) && (theChar <= 0xE380BF00)) )
		// POSIX punctuation, general punctuation, CJK symbols/punctuation
		return (kPunctuationChar);
	else if ((theChar >= 0xE3888000) && (theChar <= 0xE38BBF00))
		// enclosed CJK letters and months
		return (kEnclosedCJKChar);
	else if ((theChar >= 0xEFBC8000) && (theChar <= 0xEFBFAF00))
		// halfwidth and fullwidth forms
		return (kFullOrHalfWidthChar);
	else if ((theChar >= 0xE3818000) && (theChar <= 0xE3829F00))
		// hiragana
		return (kHiraganaChar);
	else if ((theChar >= 0xE382A000) && (theChar <= 0xE383BF00))
		// katakana
		return (kKatakanaChar);	
	else if ( ((theChar >= 0xE4B88000) && (theChar <= 0xE9BFBF00)) ||
			  ((theChar >= 0xEFA48000) && (theChar <= 0xEFABBF00)) )
		// CJK unified ideographs, CJK compatibility ideographs
		return (kIdeographChar);	

	return (kUnclassifiedChar);
}


// ------------------------------------------------------------
// 	NextInitialByte
// ------------------------------------------------------------
// Return the offset of the next occurance of a initial byte 
// of a UTF-8 character 

int32
BTextView::NextInitialByte(
	int32	offset) const
{
	int32 limit = fText->Length();

	while (++offset < limit) {
		if (IsInitialUTF8Byte(fText->RealCharAt(offset)))
			return (offset);
	}

	return (limit);
}


// ------------------------------------------------------------
// 	PreviousInitialByte
// ------------------------------------------------------------
// Return the offset of the previous occurance of a initial 
// byte of a UTF-8 character

int32
BTextView::PreviousInitialByte(
	int32	offset) const
{
	while (--offset > 0) {
		if (IsInitialUTF8Byte(fText->RealCharAt(offset)))
			return (offset);
	}

	return (0);
}


// ------------------------------------------------------------
// 	GetProperty
// ------------------------------------------------------------
// Respond to B_GET_PROPERTY messages

bool
BTextView::GetProperty(
	BMessage	*specifier, 
	int32		form, 
	const char	*property,
	BMessage	*reply)
{
	bool handled = false;

	if (strcmp(property, "Text") == 0) {
		int32 index = 0;
		int32 range = 0;
		if ( (specifier->FindInt32("index", &index) != B_NO_ERROR) ||
			 (specifier->FindInt32("range", &range) != B_NO_ERROR) )
			return (false);

		switch (form) {
			case B_RANGE_SPECIFIER:
			case B_REVERSE_RANGE_SPECIFIER:
			{
				char	*text = (char *)malloc(range + 1);
				int32	offset = (form == B_RANGE_SPECIFIER) ? index :
								 fText->Length() - index;

				GetText(offset, range, text);	
				reply->AddString("result", text);

				free(text); 
				handled = true;
				break;
			}

			default:
				break;
		}		
	}
	else if (strcmp(property, "text_run_array") == 0) {
		int32 index = 0;
		int32 range = 0;
		if ( (specifier->FindInt32("index", &index) != B_NO_ERROR) ||
			 (specifier->FindInt32("range", &range) != B_NO_ERROR) )
			return (false);

		switch (form) {
			case B_RANGE_SPECIFIER:
			case B_REVERSE_RANGE_SPECIFIER:
			{
				int32			len = 0;
				int32			offset = (form == B_RANGE_SPECIFIER) ? index :
										 fText->Length() - index;
			
				text_run_array *array = RunArray(offset, offset + range, &len);
				reply->AddData("result", B_RAW_TYPE, array, len);

				FreeRunArray(array); 
				handled = true;
				break;
			}

			default:
				break;
		}		
	}
	else if ( (strcmp(property, "selection") == 0) && 
			  (form == B_DIRECT_SPECIFIER) ) {
		reply->AddInt32("result", fSelStart);
		reply->AddInt32("result", fSelEnd);
		handled = true;
	}

	return (handled);
}


// ------------------------------------------------------------
// 	SetProperty
// ------------------------------------------------------------
// Respond to B_SET_PROPERTY messages

bool
BTextView::SetProperty(
	BMessage	*specifier,
	int32		form, 
	const char	*property,
	BMessage	*)
{
	bool		handled = false;
	BMessage	*message = Window()->CurrentMessage();	// ack!!

	if (strcmp(property, "Text") == 0) {
		int32 index = 0;
		int32 range = 0;
		if ( (specifier->FindInt32("index", &index) != B_NO_ERROR) ||
			 (specifier->FindInt32("range", &range) != B_NO_ERROR) )
			return (false);

		const char *text = NULL;
		if (message)
			message->FindString("data", &text);

		switch (form) {
			case B_RANGE_SPECIFIER:
			case B_REVERSE_RANGE_SPECIFIER:
			{
				int32 offset = (form == B_RANGE_SPECIFIER) ? index :
							   fText->Length() - index;
				
				if (text != NULL) {
					int32 textLen = strlen(text);
					range = (textLen < range) ? textLen : range;

					Insert(offset, text, range);
					ScrollToOffset(fSelEnd);
				}
				else
					Delete(offset, offset + range);
				handled = true;
				break;
			}

			default:
				break;
		}		
	}
	else if (strcmp(property, "text_run_array") == 0) {
		int32					index = 0;
		int32					range = 0;
		const text_run_array	*array = NULL;
		int32					len = 0;
		if ( (specifier->FindInt32("index", &index) != B_NO_ERROR) ||
			 (specifier->FindInt32("range", &range) != B_NO_ERROR) ||
			 (message->FindData("data", B_RAW_TYPE, (const void **)&array, &len) != 
			  B_NO_ERROR) )
			return (false);

		switch (form) {
			case B_RANGE_SPECIFIER:
			case B_REVERSE_RANGE_SPECIFIER:
			{
				int32 offset = (form == B_RANGE_SPECIFIER) ? index :
							   fText->Length() - index;
				SetRunArray(offset, offset + range, array);
				handled = true;
				break;
			}

			default:
				break;
		}		
	}
	else if ( (strcmp(property, "selection") == 0) && 
			  (form == B_DIRECT_SPECIFIER) ) {
		int32 selStart = 0;
		int32 selEnd = 0;
		if ( (message->FindInt32("data", 0, &selStart) == B_NO_ERROR) &&
			 (message->FindInt32("data", 1, &selEnd) == B_NO_ERROR) ) {
			Select(selStart, selEnd);
			handled = true;
		}
	}

	return (handled);
}


// ------------------------------------------------------------
// 	CountProperties
// ------------------------------------------------------------
// Respond to B_COUNT_PROPERTIES message

bool
BTextView::CountProperties(
	BMessage	*,
	int32		form,
	const char	*property,
	BMessage	*reply)
{
	bool handled = false;

	if ((strcmp(property, "Text") == 0) && (form == B_DIRECT_SPECIFIER)) {
		reply->AddInt32("result", fText->Length());
		handled = true;		
	}	

	return (handled);
}


// ------------------------------------------------------------
//	HandleInputMethodChanged
// ------------------------------------------------------------
// Respond to B_INPUT_METHOD_CHANGED

void
BTextView::HandleInputMethodChanged(
	BMessage	*message)
{
	if (fInline == NULL)
		return;

	const char *inlineText = NULL;
	message->FindString("be:string", &inlineText);
	if (inlineText == NULL)
		// can't do anything if there's no string...
		return;

	bool safeToDraw = (fActive) && (Window() != NULL);

	// hide the caret/unhighlight the selection
	if (safeToDraw) {
		be_app->ObscureCursor();

		if (fCaretVisible)
			InvertCaret();
	}

	int32	inlineOffset = 0;
	int32	inlineLength = strlen(inlineText);
	bool	inlineConfirmed = false;
	message->FindBool("be:confirmed", &inlineConfirmed);

	if (fInline->IsActive()) {
		// we're updating an active input area
		inlineOffset = fInline->Offset();
		_BTextChangeResult_ change;
		DoDeleteText(inlineOffset, inlineOffset + fInline->Length(), &change);
		inlineOffset = change.fDeleteStart;

		if ((inlineConfirmed) || (inlineLength < 1))
			// close active input area
			fInline->SetActive(false);
	}
	else {
		// create a new active input area
		if (inlineLength < 1)
			// maybe not...
			return;
		
		if (fSelStart != fSelEnd) {
			_BTextChangeResult_ change;
			DoDeleteText(fSelStart, fSelEnd, &change);
			fSelStart = fSelEnd = change.fDeleteStart;
			fClickOffset = fSelEnd = fSelStart = change.fDeleteStart;
		}

		fInline->SetActive(!inlineConfirmed);
		inlineOffset = fSelStart;
	}

	int32 inlineSelStart = 0;
	int32 inlineSelEnd = 0;
	if ( (message->FindInt32("be:selection", 0, &inlineSelStart) != B_NO_ERROR) || 
		 (message->FindInt32("be:selection", 1, &inlineSelEnd) != B_NO_ERROR) ) 
		inlineSelStart = inlineSelEnd = inlineLength;
	inlineSelStart += inlineOffset;
	inlineSelEnd += inlineOffset;

	fSelStart = inlineSelStart;
	fSelEnd = inlineSelEnd;	
	fClickOffset = fSelEnd;

	if (fInline->IsActive()) {
		// set the current values
		fInline->SetOffset(inlineOffset);
		fInline->SetLength(inlineLength);
		fInline->ResetClauses();
		for (int32 i = 0; /* nothing */; i++) {
			int32 clauseStart = 0;
			int32 clauseEnd = 0;
			if ( (message->FindInt32("be:clause_start", i, &clauseStart) != B_NO_ERROR) || 
				 (message->FindInt32("be:clause_end", i, &clauseEnd) != B_NO_ERROR) )
				break;
	
			fInline->AddClause(clauseStart + inlineOffset, clauseEnd + inlineOffset);
		}
		fInline->CommitClauses(inlineSelStart, inlineSelEnd);
	}

	if (inlineLength > 0) {
		_BTextChangeResult_ change;
		DoInsertText(inlineText, inlineLength, inlineOffset, NULL, &change);
		inlineLength = change.fInsertLength;
		inlineOffset = change.fInsertOffset;
	}

	// recalc line breaks and draw the text
	Refresh(inlineOffset, inlineOffset + inlineLength, true, true);
	
	// draw the caret
	if ((safeToDraw) && (fSelStart == fSelEnd) && (!fCaretVisible) && (fEditable))
		InvertCaret();
}


// ------------------------------------------------------------
//	HandleInputMethodLocationRequest
// ------------------------------------------------------------
// Respond to B_INPUT_METHOD_LOCATION_REQUEST

void
BTextView::HandleInputMethodLocationRequest()
{
	if (fInline == NULL)
		return;

	BMessage reply(B_INPUT_METHOD_EVENT);
	reply.AddInt32("be:opcode", B_INPUT_METHOD_LOCATION_REQUEST);
	
	int32	inlineOffset = fInline->Offset();
	int32	inlineEnd = inlineOffset + fInline->Length();
	BPoint	screenDelta = ConvertToScreen(B_ORIGIN);
	screenDelta -= B_ORIGIN;

	for (int32 i = inlineOffset; i < inlineEnd; i = NextInitialByte(i)) {
		float height = 0.0;

		reply.AddPoint("be:location_reply", PointAt(i, &height) + screenDelta);
		reply.AddFloat("be:height_reply", height);
	}

	fInline->Method()->SendMessage(&reply);
}


// ------------------------------------------------------------
// 	CancelInputMethod
// ------------------------------------------------------------
// 

void
BTextView::CancelInputMethod()
{
	if (fInline == NULL)
		return;

	_BInlineInput_ *theInline = fInline;
	fInline = NULL;

	if (theInline->IsActive()) {
		int32 inlineOffset = theInline->Offset();
		int32 inlineLength = theInline->Length();

		fSelStart = fSelEnd = inlineOffset + inlineLength;

		if (Window() != NULL) {
			if (fCaretVisible)
				InvertCaret();

			PushState();
			DrawLines(LineAt(inlineOffset), LineAt(inlineOffset + inlineLength), inlineOffset, true);
			PopState();
			
			if ((fActive) && (!fCaretVisible) && (fEditable))
				InvertCaret();
		}
	}

	BMessage stop(B_INPUT_METHOD_EVENT);
	stop.AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);

	theInline->Method()->SendMessage(&stop);

	delete (theInline);
}


// ------------------------------------------------------------
// 	LockWidthBuffer
// ------------------------------------------------------------
// Lock the width buffer

void
BTextView::LockWidthBuffer()
{
	if (atomic_add(&sWidthAtom, -1) <= 0) {
		while (acquire_sem(sWidthSem) == B_INTERRUPTED)
			/* loop until we lock the semaphore */;
	}
}


// ------------------------------------------------------------
// 	UnlockWidthBuffer
// ------------------------------------------------------------
// Unlock the width buffer

void
BTextView::UnlockWidthBuffer()
{
	if (atomic_add(&sWidthAtom, 1) < 0)
		release_sem(sWidthSem);
}


// ------------------------------------------------------------
// 	Undo
// ------------------------------------------------------------
// Undo the user's last actions
// Was _ReservedTextView1()

void
BTextView::Undo(
	BClipboard	*clipboard)
{
	if ((!fEditable) || (fUndo == NULL))
		return;

	fUndo->Undo(clipboard);		
}


// ------------------------------------------------------------
// 	UndoState
// ------------------------------------------------------------
// Return the state of the under buffer

undo_state
BTextView::UndoState(
	bool	*isRedo) const
{
	if (fUndo == NULL) {
		*isRedo = false;
		return (B_UNDO_UNAVAILABLE);
	}

	return (fUndo->State(isRedo));
}


// ------------------------------------------------------------
// 	GetDragParameters
// ------------------------------------------------------------
// Get all the necessary info so that a drag can be started
// Was _ReservedTextView2()

void
BTextView::GetDragParameters(
	BMessage	*drag, 
	BBitmap		**bitmap,
	BPoint		*point,
	BHandler	**handler)
{	
	// make it trashable, delete text if dropped on Trash
	if (fEditable) {
		drag->AddInt32("be:actions", B_TRASH_TARGET);
	}

	int32	start = fSelStart;
	int32	end = fSelEnd;
	if(fText->PasswordMode())
	{
		// convert offset in real string to offset in password string
		start = fText->Chars(start) * kPasswordGlyphLen;
		end = fText->Chars(end) * kPasswordGlyphLen;
	}

	// add the text
	drag->AddData("text/plain", B_MIME_TYPE, fText->Text() + start, 
				  end - start);

	if (fStylable) {
		// get the corresponding styles
		int32			styleLen = 0;
		text_run_array	*styles = RunArray(fSelStart, fSelEnd, &styleLen);
	
		// XXX Gross Hack.  Remove overlay information, so we don't
		// stuff fonts with fExtra pointers into the message.
		text_run* run = styles->runs;
		for (int32 i=0; i<styles->count; i++, run++)
			run->font.RemoveAllOverlays();
		
		drag->AddData("application/x-vnd.Be-text_run_array", 
					  B_MIME_TYPE, styles, styleLen);
	  
		FreeRunArray(styles);
	}
 
	// add id to later determine if this is dropped on ourself
	drag->AddPointer("be:originator", this);
	
	*bitmap = NULL;
	point->Set(0.0, 0.0);
	*handler = NULL;
}


/*---------------------------------------------------------------*/

void BTextView::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}

/*---------------------------------------------------------------*/

void BTextView::GetPreferredSize(float *width, float *height)
{
	BView::GetPreferredSize(width, height);
}

/*---------------------------------------------------------------*/

void BTextView::AllAttached()
{
	BView::AllAttached();
}

/*---------------------------------------------------------------*/

void BTextView::AllDetached()
{
	BView::AllDetached();
}


// ------------------------------------------------------------
// 	_ReservedTextViewX
// ------------------------------------------------------------
// FBC...

#if _PR2_COMPATIBLE_

extern "C" void _ReservedTextView1__9BTextViewFv(BTextView *object, BClipboard *clipboard);
// prototype of ReservedTextView2 is in <TextView.h> (it's a protected function)


void
_ReservedTextView1__9BTextViewFv(
	BTextView	*object,
	BClipboard	*clipboard)
{
	// patch the vtable
	void (BTextView::*func)(BClipboard *);
	func = &BTextView::Undo;
	_patch_vtable_(((void **)object)[0], _ReservedTextView1__9BTextViewFv, &func);

	// explicitly call BTextView::Undo()
	object->BTextView::Undo(clipboard);
}


void
_ReservedTextView2__9BTextViewFv(
	BTextView	*object,
	BMessage	*drag,
	BBitmap		**bitmap,
	BPoint		*point,
	BHandler	**handler)
{
	// patch the vtable
	void (BTextView::*func)(BMessage *, BBitmap **, BPoint *, BHandler **);
	func = &BTextView::GetDragParameters;
	_patch_vtable_(((void **)object)[0], _ReservedTextView2__9BTextViewFv, &func);

	// explicitly call BTextView::GetDragParameters()
	object->BTextView::GetDragParameters(drag, bitmap, point, handler);
}

#endif


void
BTextView::_ReservedTextView3()
{
}


void
BTextView::_ReservedTextView4()
{
}


void
BTextView::_ReservedTextView5()
{
}


void
BTextView::_ReservedTextView6()
{
}


void
BTextView::_ReservedTextView7()
{
}


void
BTextView::_ReservedTextView8()
{
}

#if !_PR3_COMPATIBLE_

void 
BTextView::_ReservedTextView9()
{
}

void 
BTextView::_ReservedTextView10()
{
}

void 
BTextView::_ReservedTextView11()
{
}

void 
BTextView::_ReservedTextView12()
{
}

#endif
