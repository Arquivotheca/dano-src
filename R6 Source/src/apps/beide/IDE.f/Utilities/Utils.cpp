// ===========================================================================
//	Utils.cpp
// ===========================================================================
//	Copyright 1995 - 1996 Metrowerks Corporation. All rights reserved.

#include <string.h>
#include <ctype.h>
#include <time.h>

#include "Utils.h"
#include "MFindWindow.h"
#include "MMessageWindow.h"
#include "MTargetTypes.h"
#include "IDEMessages.h"
#include "ProjectCommands.h"
#include "MPrefsStruct.h"
#include "MAccessPathsView.h"
#include "MLocker.h"
#include <File.h>
#include <NodeInfo.h>
#include <Resources.h>
#include <Screen.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Bitmap.h>
#include <TextView.h>

// const arrays of suffixes for the various file types that we know about
// These aughta be resources and may need to be more flexible if a
// tool targetting system is set up
const char* sCSuffixes[] =
{
	"c",
	""
};

const char* sCPlusSuffixes[] =
{
	"cpp",
	"cp",
	"c++",
	"cc",
	"C",
	"cxx",
	""
};

extern const char* sHeaderSuffixes[] =
{
	"h",
	"hpp",
	"h++",
	"H",
	"HH",
	"hxx",
	"hh",
	""
};

const char* sPCHSuffixes[] =
{
	"pch",
	"pch++",
	""
};

const char* sDumpSuffixes[] =
{
	"dump",
	""
};

const char* sObjectSuffixes[] =
{
	"o",
	"a",
	"lib",
	""
};

const char* sXCOFFSuffixes[] =
{
	"xcoff",
	"xcof",
	""
};

const char* sProjectSuffixes[] =
{
	"µ",		// mac roman pi
	"¹",		// mac roman mu
	"proj",
	"prj",
	"Ï€",
	"Âµ",
	""
};

const char* sEXPSuffixes[] =
{
	"exp",
	""
};

const char* sJavaSuffixes[] =
{
	"java",
	""
};


// This array should include all of the C and C++ source suffixes
// used for andy feature
extern const char* sSourceSuffixes[] =
{
	"c",
	"cp",
	"cpp",
	"c++",
	"cc",
	"C",
	"cxx",
	""
};


// Private prototypes
static bool	ScanArray(
			const char *	inArray[],
			const char *	inSuffix);

void
PrintBytes(
	void*	inBytes,
	int32	inLength)
{
	int32	size = inLength;
	char*	bytes = (char*) inBytes;

	while (size > 0)
	{
		int32		j = 0;
		while (j < 4 && size > 0)
		{
			union dataT{
				int32	data;
				char	chars[4];
			};
			
			dataT	temp;
			temp.data = 0L;
			int32	i = 0;
			while (i < 4 && size-- > 0)
				temp.chars[i++] = *bytes++;
			fprintf(stderr, "\"%4s\"/%#.8x  ", temp.chars, temp.data);
		}
		
		fprintf(stderr, "\n");
	}
}

// ---------------------------------------------------------------------------
//		GetFileName
// ---------------------------------------------------------------------------
//	This is not thread-safe. So what.

char *
GetFileName(
	BEntry *file)
{
	static FileNameT 	name;

	name[0] = '\0';
	file->GetName(name);

	return name;
}

// ---------------------------------------------------------------------------
//		GetTextRect
// ---------------------------------------------------------------------------

BRect
GetTextRect(
	const BRect & area)
{
	BRect r = area;

	r.OffsetTo(B_ORIGIN);
	r.InsetBy(2.0, 2.0);

	return r;
}

// ---------------------------------------------------------------------------
//		CenterWindow
// ---------------------------------------------------------------------------

void
CenterWindow(
	BWindow* inWindow)
{
	BScreen			screen(inWindow);
	BRect			frame = screen.Frame();
	BRect			windowFrame = inWindow->Frame();
	const float		screenCenterWid = frame.Width() / 2.0f;
	const float		screenCenterHeight = frame.Height() / 2.0f;

	inWindow->MoveBy(screenCenterWid - (windowFrame.Width() / 2.0f) - windowFrame.left, 
						screenCenterHeight - (windowFrame.Height() / 2.0f) - windowFrame.top);
}

// ---------------------------------------------------------------------------
//		WindowIsHidden
// ---------------------------------------------------------------------------
//	Function to return whether a window is hidden or not.  BWindow::IsHidden()
//	requires that the window be locked when calling it.

bool
WindowIsHidden(
	BWindow*	inWindow)
{
	MLocker<BWindow>	lock(inWindow);

	return (lock.IsLocked() && inWindow->IsHidden());
}

// ---------------------------------------------------------------------------
//		WindowIsHidden
// ---------------------------------------------------------------------------
//	Function to return whether a window is hidden or not.  BWindow::IsHidden()
//	requires that the window be locked when calling it.

void
ShowAndActivate(
	BWindow*	inWindow)
{
	MLocker<BWindow>	lock(inWindow);

	if (lock.IsLocked())
	{
		if (inWindow->IsHidden())
			inWindow->Show();

		inWindow->Activate();
	}
}

// ---------------------------------------------------------------------------
//		EnableItem
// ---------------------------------------------------------------------------
//	This should be in BMenu.

void
EnableItem(BMenu* inMenu, uint32 inCommand, bool inEnabled)
{
	BMenuItem*		theItem = inMenu->FindItem(inCommand);
//	ASSERT(theItem);
	if (theItem)
		theItem->SetEnabled(inEnabled);
}

// ---------------------------------------------------------------------------
//		SetCommand
// ---------------------------------------------------------------------------
//	Should be in BMenuItem.  Now it is.

void
SetCommand(BMenuItem& inMenuItem, uint32 inCommand)
{
	BMessage*		msg = new BMessage(inCommand);
	
	inMenuItem.SetMessage(msg);
}

// ---------------------------------------------------------------------------
//		SetItemMessageAndName
// ---------------------------------------------------------------------------

void
SetItemMessageAndName(
	const BMenu*	inMenu,
	int32			inIndex,
	uint32			inCommand,
	const char *	inName)
{
	BMenuItem*			item = inMenu->ItemAt(inIndex);

//	ASSERT(item);
	if (item)
	{
		BMessage*		msg = item->Message();

		if (msg != nil)
			msg->what = inCommand;
		if (inName)
			item->SetLabel(inName);	
	}
}

// ---------------------------------------------------------------------------
//		GetBFontFamilyAndStyle
// ---------------------------------------------------------------------------
//	Get the font_family and font_style for a BFont without the garbage that
//	BFont leaves in the unused bytes after the null.

void 
GetBFontFamilyAndStyle(
	const BFont&	inFont,
	font_family		outFamily,
	font_style		outStyle)
{
	font_family		family;	// GetFamilyAndStyle fills the family and style with
	font_style		style;	// garbage after the strings :-/

	inFont.GetFamilyAndStyle(&family, &style);

	strcpy(outFamily, family);
	strcpy(outStyle, style);
}


// -----------------------------------------------------------------
// Font and Size handling utilities
// -----------------------------------------------------------------

void
FillFontSizeMenu(BMenu* sizeMenu)
{
	// Fill a menu with a bunch of sizes

	for (int32 i = kFirstFontSize; i <= kLastFontSize; i++) {
		char	name[5];
		sprintf(name, "%ld", i);
		sizeMenu->AddItem(new BMenuItem(name, new BMessage(msgSizeChosen)));
	}
}

// ---------------------------------------------------------------------------

void
FillFontFamilyMenu(BMenu* fontMenu)
{
	// Fill a menu with all the font families and their styles
	
	int32 numFamilies = count_font_families();
	for (int32 i = 0; i < numFamilies; i++) {
		font_family family;
		get_font_family(i, &family);
		
		BMenu* styleMenu = new BMenu(family);
		int32 numStyles = count_font_styles(family);
		for (int32 j = 0; j < numStyles; j++) {
			font_style style;
			get_font_style(family, j, &style);
			styleMenu->AddItem(new BMenuItem(style, 
											 new BMessage(msgStyleChosen)));
		}

		fontMenu->AddItem(new BMenuItem(styleMenu, new BMessage(msgFontChosen)));
	}
}

// ---------------------------------------------------------------------------

void
MarkCurrentFontSize(BMenu* sizeMenu, float size)
{
	// clear the currently marked item
	// mark the current font size
		
	BMenuItem* currentMarked = sizeMenu->FindMarked();
	if (currentMarked) {
		currentMarked->SetMarked(false);
	}
	
	char name[5];
	sprintf(name, "%ld", (int32) size);
	BMenuItem* currentSizeItem = sizeMenu->FindItem(name);
	if (currentSizeItem) {
		currentSizeItem->SetMarked(true);
	}
}

// ---------------------------------------------------------------------------

void
MarkCurrentFont(BMenu* fontMenu, const font_family family, 
				const font_style style, int32 firstFontItem)
{
	// clear the currently marked font
	// mark the current font + style

	// We can't do "FindMarked" because that will find the size item
	// iterate through and clear the mark off both font and style items
	BMenuItem* fontItem = nil;
	for (int32 i = firstFontItem; fontItem = fontMenu->ItemAt(i); i++) {
		if (fontItem->IsMarked()) {
			fontItem->SetMarked(false);
			BMenuItem* styleItem = fontItem->Submenu()->FindMarked();
			if (styleItem) {
				styleItem->SetMarked(false);
			}
			break;
		}
	}

	// Now mark the new font + style
	BMenuItem* currentFamilyItem = fontMenu->FindItem(family);
	if (currentFamilyItem) {
		currentFamilyItem->SetMarked(true);
		BMenuItem* styleItem = currentFamilyItem->Submenu()->FindItem(style);
		if (styleItem) {
			styleItem->SetMarked(true);
		}
		else {
			currentFamilyItem->Submenu()->ItemAt(0)->SetMarked(true);
		}
	}
}

// ---------------------------------------------------------------------------
//		SetGrey
// ---------------------------------------------------------------------------
//	Make this view draw in grey.

void
SetGrey(
	BView* 		inView,
	rgb_color	inColor)
{
	inView->SetViewColor(inColor);
	inView->SetLowColor(inColor);
}

// ---------------------------------------------------------------------------
//		ValidateWindowFrame
// ---------------------------------------------------------------------------

const float	kHScreenMargin = 20;
const float	kVScreenMargin = 10;

void
ValidateWindowFrame(
	BWindow& 	inWindow)
{
	ValidateWindowFrame(inWindow.Frame(), inWindow);
}

// ---------------------------------------------------------------------------
//		ValidateWindowFrame
// ---------------------------------------------------------------------------

void
ValidateWindowFrame(
	BRect 		inFrame,
	BWindow& 	inWindow)
{
	BScreen			screen(&inWindow);
	BRect			frame = screen.Frame();
	BRect			r = frame & inFrame;;

	if (r.Width() < kHScreenMargin || r.Height() < kVScreenMargin)
	{
		// Move the edge of the window on-screen
		frame.InsetBy(kHScreenMargin, kVScreenMargin);

		float	deltaH = 0;
		float	deltaV = 0;

		if (inFrame.left > frame.right)
			deltaH = inFrame.left - frame.right + kHScreenMargin;
		else
		if (inFrame.right < frame.left)
			deltaH = inFrame.right - frame.left + kHScreenMargin;

		if (inFrame.top > frame.bottom)
			deltaV = inFrame.top - frame.bottom + kVScreenMargin;
		else
		if (inFrame.bottom < frame.top)
			deltaV = inFrame.bottom - frame.top + kVScreenMargin;

		inWindow.MoveBy(-deltaH, -deltaV);
	}
}

// ---------------------------------------------------------------------------
//		LoadBitmap
// ---------------------------------------------------------------------------
//	Take an array holding the bitmap data and return a bitmap with that data.

BBitmap*
LoadBitmap(
	const char * 	inBitMapData,
	int32			inWidth,
	int32			inHeight)
{
	BRect			bounds(0.0, 0.0, inWidth - 1, inHeight - 1);
	BBitmap*		bitmap = new BBitmap(bounds, B_COLOR_8_BIT);
	
	const char * 		ptr = inBitMapData;
	int					rowBytes = bitmap->BytesPerRow();
	
	for (int i = 0; i < inHeight; i++)
	{
		bitmap->SetBits(ptr, inWidth, rowBytes * i, B_COLOR_8_BIT);
		ptr += inWidth;
	}
	
	return bitmap;
}

// ---------------------------------------------------------------------------
//		LoadBitmap
// ---------------------------------------------------------------------------
//	Take an array holding the bitmap data and return a bitmap with that data.

BBitmap*
LoadBitmap(
	BResources&		inFile,
	type_code		inResType,
	int32			inResID,
	int32			inWidth,
	int32			inHeight)
{
	size_t			length;
	BBitmap*		bitmap = nil;
	void*			resData = inFile.FindResource(inResType, inResID, &length);

	if (resData != nil)
	{
		BRect			bounds(0.0, 0.0, inWidth - 1, inHeight - 1);
		bitmap = new BBitmap(bounds, B_COLOR_8_BIT);
		
		char * 				ptr = static_cast<char *>(resData);
		int					rowBytes = bitmap->BytesPerRow();
		
		for (int i = 0; i < inHeight; i++)
		{
			bitmap->SetBits(ptr, inWidth, rowBytes * i, B_COLOR_8_BIT);
			ptr += inWidth;
		}

		free(resData);
	}
	
	return bitmap;
}

// ---------------------------------------------------------------------------
//		MimeType
// ---------------------------------------------------------------------------

uint32 
MimeType(
	const char *	inType)
{
//	uint32	result = kUnknownType;
	uint32	result = kNULLType;

	if (0 == strncmp(inType, "text/", 5))
		result = kTextType;
	else 
	if (0 == strcmp(inType, kProjectMimeType))
		result = kProjectType;

	return result;		
}

// ---------------------------------------------------------------------------
//		MimeType
// ---------------------------------------------------------------------------

uint32 
MimeType(
	const BEntry&	inEntry)
{
	mime_t		type;
	BFile		file(&inEntry, B_READ_ONLY);
	BNodeInfo	mime(&file);
	
	if (B_NO_ERROR == mime.GetType(type))
		return MimeType(type);
	else
		return kNULLType;
}

// ---------------------------------------------------------------------------
//		FixFileType
// ---------------------------------------------------------------------------
//	There are a lot of stinking files on Be that have no file types or
//	creators.  This is bad, but we try to fix this transparently.

static bool
FixFileType(
	const BEntry* 	inEntry,
	const char *	inName)
{
	bool			fixed = false;
	SuffixType		suffix = GetSuffixType(inName);
	const char*		newType = nil;

	switch (suffix)
	{
		// If it's a source file make it text/sourcecode
		case kCSuffix:
		case kCPSuffix:
		case kHSuffix:
		case kPCHSuffix:
		case kEXPSuffix:
		case kJavaSuffix:
			newType = kIDETextMimeType;
			break;
		
		// If it's an object file make it one of ours
		case kObjectSuffix:
			newType = kCWLibMimeType;
			break;

		// If it's an xcoff file make it one of ours
		case kXCOFFSuffix:
			newType = kXCOFFMimeType;
			break;

		// If it's a project file make it one of ours
		case kProjectSuffix:
			newType = kProjectMimeType;
			break;
		// If it's a java class file
		case kClassFileSuffix:
			newType = kClassFileMimeType;
			break;

		// If it's a java zip file
		case kZipFileSuffix:
			newType = kZipFileMimeType;
	}
	
	if (newType != nil)
	{
		BFile		file(inEntry, B_WRITE_ONLY);
		BNodeInfo	mimefile(&file);

		if (B_NO_ERROR == mimefile.SetType(newType))
			fixed = true;
	}

	return fixed;
}

// ---------------------------------------------------------------------------
//		FixFileType
// ---------------------------------------------------------------------------
//	There are a lot of stinking files on Be that have no file types or
//	creators.  This is bad, but we try to fix this transparently.

bool
FixFileType(
	const BEntry* 	inEntry)
{
	bool			fixed = false;
	FileNameT		fileName;

	if (B_NO_ERROR == inEntry->GetName(fileName))
	{
		fixed = FixFileType(inEntry, fileName);
	}

	return fixed;
}

// ---------------------------------------------------------------------------
//		FixFileType
// ---------------------------------------------------------------------------
//	There are a lot of stinking files on Be that have no file types or
//	creators.  This is bad, but we try to fix this transparently.

bool
FixFileType(
	const BEntry* 	inEntry, 
	TargetRec*		inTargetArray,
	int32			inTargetCount)
{
	bool			fixed = false;
	FileNameT		fileName;

	if (B_NO_ERROR == inEntry->GetName(fileName))
	{
		// Try to fix the type based on the extension and the
		// mapping of extensions to types that is in the targetarray
		const char *		extension = strrchr(fileName, '.');
		if (extension != nil)
			extension++;
		else
			extension = "";

		for (int32 i = 0; i < inTargetCount; i++)
		{
			const char *	mime = inTargetArray[i].MimeType;
			if (0 == strcmp(inTargetArray[i].Extension, extension) &&
				mime[0] != '\0' && mime[strlen(mime) - 1] != '*')	// ignore wild card entries
			{
				BNode		node(inEntry);
				BNodeInfo	mimefile(&node);
				
				if (B_NO_ERROR == mimefile.SetType(mime))
				{
					fixed = true;
					break;
				}
			}
		}

		// If that didn't work use this builtin mapping of extension
		// to file type
		if (! fixed)
		{
			fixed = FixFileType(inEntry, fileName);
		}
		
		// Could also try magic number here
	}

	return fixed;
}

// ---------------------------------------------------------------------------
//		DisallowInvalidChars
// ---------------------------------------------------------------------------
//	Prevent any of these useless and illegal characters to appear in our
//	text window and various edit boxes.

void
DisallowInvalidChars(
	BTextView& 		inText)
{
	// None of these keys do anything or can be allowed in C++ text
	inText.DisallowChar(B_ESCAPE);
	inText.DisallowChar(B_INSERT);
	inText.DisallowChar(B_FUNCTION_KEY);
	inText.DisallowChar(B_PRINT_KEY);
	inText.DisallowChar(B_SCROLL_KEY);
}

// ---------------------------------------------------------------------------
//		DisallowNonDigitChars
// ---------------------------------------------------------------------------
//	Useful for Btextviews that are for numerical inputs only.

void
DisallowNonDigitChars(
	BTextView& 		inText)
{
	const unsigned char kLastAsciiChar = 0xD7;	// opt-shif-V ×

	for (uint32 ch = ' '; ch < '0'; ch++)
		inText.DisallowChar(ch);
	for (uint32 ch = '9' + 1; ch < kLastAsciiChar; ch++)
		inText.DisallowChar(ch);
}

// ------------------------------------------------------------
// 	GlyphWidth
// ------------------------------------------------------------
// Return the number of bytes in the multibyte UTF8 character at
// the specified offset.  It doesn't matter whether the byte
// at this offset is the first, second, or third byte of
// the glyph.
// This code assumes only valid utf8 chars are in the buffer.

int32
GlyphWidth(
	const char *	inBytes)
{
	int32	result;
	uchar	c = inBytes[0];

	if (c < 0x80)			// one byte char
		result = 1;
	else
	if (c >= 0xe0)			// first byte of three byte glyph
		result = 3;
	else
	if (c >= 0xc0)			// first byte of two byte glyph
		result = 2;
	else
	{
		// won't reach here if inBytes is the first byte of a glyph
		uchar	d = inBytes[-1];
		
		if (d >= 0xe0)		// was second byte of three byte glyph
			result = 3;
		else
		if (d >= 0xc0)		// was second byte of two byte glyph
			result = 2;
		else
		if (d >= 0x80)
			result = 3;		// was third byte of three byte glyph
		else
			result = 1;		// invalid utf8 glyph
	}

	return result;
}

// ---------------------------------------------------------------------------
//		GetValue
// ---------------------------------------------------------------------------
//	Return a value for the text in this textview.  Not guaranteed to work
//	if the text isn't all digits.

int32
GetValue(
	BTextView*	inView)
{
	int32		value = 0;

	sscanf(inView->Text(), "%ld", &value);

	return value;
}

// ---------------------------------------------------------------------------
//		GetSuffixType
// ---------------------------------------------------------------------------
//	return a constant indicating the suffix type of the filename that is 
//	passed in.

SuffixType
GetSuffixType(
	const char *	inFileName)
{
	SuffixType		result = kInvalidSuffix;
	char * 			suffix = strrchr(inFileName, '.');
	
	if (suffix)
	{
		suffix++;

		if (ScanArray(sCSuffixes, suffix))
			result = kCSuffix;
		else
		if (ScanArray(sCPlusSuffixes, suffix))
			result = kCPSuffix;
		else
		if (ScanArray(sHeaderSuffixes, suffix))
			result = kHSuffix;
		else
		if (ScanArray(sPCHSuffixes, suffix))
			result = kPCHSuffix;
		else
		if (ScanArray(sDumpSuffixes, suffix))
			result = kDumpSuffix;
		else
		if (ScanArray(sObjectSuffixes, suffix))
			result = kObjectSuffix;
		else
		if (ScanArray(sProjectSuffixes, suffix))
			result = kProjectSuffix;
		else
		if (ScanArray(sEXPSuffixes, suffix))
			result = kEXPSuffix;
		else
		if (ScanArray(sXCOFFSuffixes, suffix))
			result = kXCOFFSuffix;
		else
		if (0 == strcmp("java", suffix))
			result = kJavaSuffix;
		else
		if (0 == strcmp("class", suffix))
			result = kClassFileSuffix;
		else
		if (0 == strcmp("zip", suffix))
			result = kZipFileSuffix;
	}

	return result;
}

// ---------------------------------------------------------------------------
//		ScanArray
// ---------------------------------------------------------------------------
//	private utility for scanning the const arrays.  returns true if the 
//	suffix passed in is found in the array passed in.  The array is
//	an array of strings with a null string as the last entry.

bool
ScanArray(
	const char *	inArray[],
	const char *	inSuffix)
{
	int				i = 0;
	bool			result = false;

	while (inArray[i][0] != 0)
	{
		if (0 == strcmp(inArray[i], inSuffix))
		{
			result = true;
			break;
		}
		i++;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//		ListCompareStrings
// ---------------------------------------------------------------------------
//	To be used with the BList SortItems function call.   
//	Needs a typecast.  Use like:
//	list.SortItems((ListCompareFunc) ListCompareStrings);

int 
ListCompareStrings(
	char ** inOne,
	char ** inTwo)
{
	return strcmp(*inOne, *inTwo);
}

// ---------------------------------------------------------------------------
//		SwapRectToHost
// ---------------------------------------------------------------------------
//	These two functions could be inlines.

void
SwapRectToHost(
	BRect&	inoutRect)
{
	if (B_HOST_IS_LENDIAN)
	{
		inoutRect.left = B_BENDIAN_TO_HOST_FLOAT(inoutRect.left);
		inoutRect.top = B_BENDIAN_TO_HOST_FLOAT(inoutRect.top);
		inoutRect.right = B_BENDIAN_TO_HOST_FLOAT(inoutRect.right);
		inoutRect.bottom = B_BENDIAN_TO_HOST_FLOAT(inoutRect.bottom);
	}
}

// ---------------------------------------------------------------------------
//		SwapRectToBig
// ---------------------------------------------------------------------------

void
SwapRectToBig(
	BRect&	inoutRect)
{
	if (B_HOST_IS_LENDIAN)
	{
		inoutRect.left = B_HOST_TO_BENDIAN_FLOAT(inoutRect.left);
		inoutRect.top = B_HOST_TO_BENDIAN_FLOAT(inoutRect.top);
		inoutRect.right = B_HOST_TO_BENDIAN_FLOAT(inoutRect.right);
		inoutRect.bottom = B_HOST_TO_BENDIAN_FLOAT(inoutRect.bottom);
	}
}

// ---------------------------------------------------------------------------
//		SwapRectLittleToHost
// ---------------------------------------------------------------------------

void
SwapRectLittleToHost(
	BRect&	inoutRect)
{
	if (B_HOST_IS_BENDIAN)
	{
		inoutRect.left = B_LENDIAN_TO_HOST_FLOAT(inoutRect.left);
		inoutRect.top = B_LENDIAN_TO_HOST_FLOAT(inoutRect.top);
		inoutRect.right = B_LENDIAN_TO_HOST_FLOAT(inoutRect.right);
		inoutRect.bottom = B_LENDIAN_TO_HOST_FLOAT(inoutRect.bottom);
	}
}

// ---------------------------------------------------------------------------
//		DeleteArgv
// ---------------------------------------------------------------------------
//	argv must have been allocated by new char*[] and the arguments must have
//	been allocated by malloc (or strdup);

void
DeleteArgv(
	char**	inArgv)
{
	char ** ptr = inArgv;
	while (*ptr)
		free((void *)*(ptr++));
	delete[] inArgv;
}

// ---------------------------------------------------------------------------
//		DeleteArgv
// ---------------------------------------------------------------------------
//	argv must have been allocated by new char*[] and the arguments must have
//	been allocated by malloc (or strdup);

void
DeleteArgv(
	BList&	inArgv)
{
	for (int32 i = 0; i < inArgv.CountItems(); i++)
	{
		void*		item = inArgv.ItemAt(i);
		if (item)
			free (item);
	}

	inArgv.MakeEmpty();
}

void 
PrintColor(rgb_color color)
{
	int		red = color.red;
	int		green = color.green;
	int		blue = color.blue;
	int		alpha = color.alpha;

	printf("rgb_color: %ld, %ld, %ld, %ld\n", red, green, blue, alpha);
}

// for debugging
static BView*
DeepestChild(
	BView*	inView)
{
	BView*		resultView = nil;
	BView*		view;
	BView*		view1 = inView;

	while ((view = view1->ChildAt(0)) != nil)
	{
		view1 = view;
		resultView = view;
	}

	return resultView;
}

static void PrintTime(time_t	inTime)
{
	struct tm *		time = localtime(&inTime);
	char			asciiTime[256];
	size_t			len = strftime(asciiTime, sizeof(asciiTime), "%a, %b %d %Y, %X", time);
	
	printf("time: %s\n", asciiTime);
}


