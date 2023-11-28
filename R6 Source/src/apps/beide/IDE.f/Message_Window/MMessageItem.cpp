//========================================================================
//	MMessageItem.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#include <string.h>

#include "MMessageItem.h"
#include "MMessageView.h"
#include "MDynamicMenuHandler.h"
#include "MTextWindow.h"
#include "MWEditUtils.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "stDrawingMode.h"
#include "MAlert.h"

#include <String.h>
#include <Bitmap.h>

const float kBitMapWidth = 16.0;
const float kBitMapHeight = 16.0;
const float kIconMargin = 2.0;

BBitmap * MMessageItem::sErrorBitmap;
BBitmap * MMessageItem::sWarningBitmap;
BBitmap * MMessageItem::sInfoBitmap;

#define _ 0xff
#define B 0
#define r 42
#define f 217
#define y 248
#define b 178
#define w 63

const char sStopIcon[] = {
	_,_,_,_,B,B,B,B,B,B,B,B,_,_,_,_,
	_,_,_,B,r,r,r,B,r,r,r,r,B,_,_,_,
	_,_,B,r,r,B,B,f,B,B,r,r,r,B,_,_,
	_,B,r,r,B,f,B,f,B,f,B,r,r,r,B,_,
	B,r,r,r,B,f,B,f,B,f,B,r,r,r,r,B,
	B,r,r,B,B,f,B,f,B,f,B,r,r,r,r,B,
	B,r,r,B,B,f,B,f,B,f,B,r,B,r,r,B,
	B,r,r,B,B,f,f,f,f,f,B,B,f,B,r,B,
	B,r,r,B,f,f,f,f,f,f,B,f,f,B,r,B,
	B,r,r,B,f,f,f,f,f,f,f,f,B,r,r,B,
	B,r,r,B,f,f,f,f,f,f,f,f,B,r,r,B,
	B,r,r,r,B,f,f,f,f,f,f,B,r,r,r,B,
	_,B,r,r,B,f,f,B,f,f,f,B,r,r,B,_,
	_,_,B,r,r,B,B,B,B,B,B,r,r,B,_,_,
	_,_,_,B,r,r,r,r,r,r,r,r,B,_,_,_,
	_,_,_,_,B,B,B,B,B,B,B,B,_,_,_,_,
};


const char sWarningIcon[] = {
	_,_,_,_,_,_,_,B,B,_,_,_,_,_,_,_,
	_,_,_,_,_,_,B,y,y,B,_,_,_,_,_,_,
	_,_,_,_,_,_,B,y,y,B,_,_,_,_,_,_,
	_,_,_,_,_,B,y,y,y,y,B,_,_,_,_,_,
	_,_,_,_,_,B,y,B,B,y,B,_,_,_,_,_,
	_,_,_,_,B,y,y,B,B,y,y,B,_,_,_,_,
	_,_,_,_,B,y,y,B,B,y,y,B,_,_,_,_,
	_,_,_,B,y,y,y,B,B,y,y,y,B,_,_,_,
	_,_,_,B,y,y,y,B,B,y,y,y,B,_,_,_,
	_,_,B,y,y,y,y,B,B,y,y,y,y,B,_,_,
	_,_,B,y,y,y,y,B,B,y,y,y,y,B,_,_,
	_,B,y,y,y,y,y,y,y,y,y,y,y,y,B,_,
	_,B,y,y,y,y,y,B,B,y,y,y,y,y,B,_,
	B,y,y,y,y,y,y,B,B,y,y,y,y,y,y,B,
	B,y,y,y,y,y,y,y,y,y,y,y,y,y,y,B,
	_,B,B,B,B,B,B,B,B,B,B,B,B,B,B,_,
};

const char sInfoIcon[] = {
	B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
	B,f,f,f,B,B,B,B,B,B,B,B,B,B,B,B,
	B,f,f,f,B,B,B,B,B,B,B,B,B,B,B,B,
	B,f,B,f,B,B,B,B,b,b,b,b,B,B,B,B,
	B,f,B,f,B,B,B,b,w,w,w,w,b,B,B,B,
	B,f,f,f,B,B,B,b,w,w,w,w,b,B,B,B,
	B,f,f,f,B,B,b,w,B,w,B,B,w,b,B,B,
	B,f,f,f,B,B,b,w,w,w,w,w,w,b,B,B,
	B,f,f,f,B,B,b,w,B,B,w,B,B,b,B,B,
	B,f,f,f,B,B,b,w,w,w,w,w,w,b,B,B,
	B,f,B,B,B,B,b,w,B,w,B,B,B,b,B,B,
	B,f,f,B,B,B,b,w,w,w,w,w,b,B,B,B,
	B,f,f,B,B,B,b,w,w,w,w,b,B,B,B,B,
	B,f,B,B,b,b,b,b,b,b,b,B,B,B,B,B,
	B,f,f,B,B,B,B,B,B,B,B,B,B,B,B,B,
	B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
};

#undef B
#undef r
#undef f
#undef y
#undef b
#undef w

// Columns in the view
const int32 kLineWidth = 80;

// ---------------------------------------------------------------------------
//		MMessageItem::MMessageItem
// ---------------------------------------------------------------------------
//	Constructor

MMessageItem::MMessageItem()
{
	fText = "";
	fLines = 0;
	fLineNum = 0;
	fTokenIsGood = false;
	fHasUnderLine = false;
	fEntryRefIsGood = false;

	InitBitmaps();
}

// ---------------------------------------------------------------------------
//		MMessageItem::MMessageItem
// ---------------------------------------------------------------------------
//	Constructor

MMessageItem::MMessageItem(const char* inText)
{
	fText = inText;
	fText.Replace('\t', ' ');	// Translate tabs to spaces
	fLines = 0;
	fTokenIsGood = false;
	fHasUnderLine = false;
	
	InitBitmaps();
}

// ---------------------------------------------------------------------------
//		MMessageItem::~MMessageItem
// ---------------------------------------------------------------------------
//	Destructor

MMessageItem::~MMessageItem()
{
}

// ---------------------------------------------------------------------------
//		MMessageItem::Draw
// ---------------------------------------------------------------------------

void
MMessageItem::Draw(
	BRect 			inFrame, 
	MMessageView& 	inParentView)
{
	font_height		info = inParentView.GetCachedFontHeight();
	const float		kLineHeight = info.ascent + info.descent + info.leading;
	float			drawPoint = inFrame.top + info.ascent + info.leading;
	const char*		text = fText;
	int32			offset = 0;
	int32			textLen = fText.GetLength();
	int32			lineLen = FindEnd(text, textLen) - text;

	// Draw each line
	for (int32 i = 0; i < fLines; i++)
	{
		inParentView.MovePenTo(kMessageTextLeftBorder, drawPoint);
		inParentView.DrawString(&text[offset], lineLen);

		// Draw the underline if there is one
		if (fHasUnderLine && i == fUnderLineNumber)
		{
			const float		left = kMessageTextLeftBorder + 
				inParentView.StringWidth(&text[offset], fUnderLineOffset);
			const float		right = left + 
				inParentView.StringWidth(&text[offset + fUnderLineOffset], fToken.eLength) - 1.0;

			inParentView.StrokeLine(BPoint(left, drawPoint + 1.0), BPoint(right, drawPoint + 1.0));
		}

		textLen -= lineLen + 1;
		offset += lineLen + 1;
		lineLen = FindEnd(&text[offset], textLen) - &text[offset];
		drawPoint += kLineHeight;
	}
}

// ---------------------------------------------------------------------------
//		MMessageItem::BuildTokenStruct
// ---------------------------------------------------------------------------
//	Transfer the info from the ErrorNotificationStruct to our Token Struct.

void
MMessageItem::BuildTokenStruct(
	const ErrorNotificationMessage& inMessage)
{
	if (inMessage.hasErrorRef)
	{
		fTokenIsGood = true;
		fToken.eLineNumber = inMessage.errorRef.linenumber - 1;
		fToken.eOffset = inMessage.errorRef.offset;
		fToken.eLength = inMessage.errorRef.length;
		fToken.eSyncLength = inMessage.errorRef.synclen;
		fToken.eSyncOffset = inMessage.errorRef.syncoffset;
		strcpy(fToken.eSync, inMessage.errorRef.sync);
		fToken.eIsFunction = false;
		// Translate cr to nl
		// Apparently mwcc translates nl to cr internally and the
		// sync text has cr in it.  It is unlikely that a token
		// will have any eol chars in it but it is better to translate
		// them here and be done with it.
		for (int32 i = 0; i < sizeof(fToken.eSync); i++)
			if (fToken.eSync[i] == '\r')
				fToken.eSync[i] = '\n';
	}
}

// ---------------------------------------------------------------------------
//		MMessageItem::Invoke
// ---------------------------------------------------------------------------
//	Open the window that this message item refers to if possible.

void
MMessageItem::Invoke()
{
	// If a window hasn't been saved yet it has no entry_ref
	// In that case we use the window name to open the file
	if (! fEntryRefIsGood)
	{
		MTextWindow*	wind = MDynamicMenuHandler::FindWindowByName(fFileName);
		if (wind)
		{
			BMessage		msg(msgGoToLine);
			msg.AddData(kTokenIdentifier, kTokenIDType, &fToken, sizeof(fToken));
			wind->PostMessage(&msg);
		}
	}
	else
	if (fTokenIsGood || fLineNum >= 0)
	{
		// Ask the app to open the window based on the entry_ref
		BMessage 		msg(msgOpenSourceFile);

		msg.AddRef("refs", &fFileRef);

		if (fTokenIsGood)
			msg.AddData(kTokenIdentifier, kTokenIDType, &fToken, sizeof(fToken));
		else
		if (fLineNum >= 0)
			msg.AddInt32(kLineNumber, fLineNum);
		
		be_app_messenger.SendMessage(&msg);
	}
}

// ---------------------------------------------------------------------------
//		MMessageItem::BuildErrorText
// ---------------------------------------------------------------------------
//	Concatenate the error text to the end of fText.

void
MMessageItem::BuildErrorText(
	const ErrorNotificationMessage& inMessage)
{
	BuildTokenStruct(inMessage);

	// Add the error message text
	fText += inMessage.errorMessage;
	fText.Replace('\r', EOL_CHAR);		// Translate cr to nl
	PruneNewlines();
	fLines = CountLines(fText, fText.GetLength());

	// Get a CString containing the file name that we can play with
	char				filePath[MAX_PATH_LENGTH];
	
	p2cstrncpy(filePath, (const unsigned char *) inMessage.errorRef.errorfile.name, MAX_PATH_LENGTH);

	char*				ptr = strrchr(filePath, '/'); 

	if (ptr)
	{
		ptr++;
		fText += "\n";
		int32		len1 = fText.GetLength();
		fText += ptr;
		fText += " line ";
		fText += inMessage.errorRef.linenumber;
		fLines++;
	
		entry_ref			ref;
	
		fLineNum = inMessage.errorRef.linenumber - 1;	
		// MTextView linenumbers count from 0 while this linenumber is one-based

		// Add the line containing the error to the error text
		if (B_NO_ERROR == get_ref_for_path(filePath, &ref))
		{
			BEntry		file(&ref);
			off_t		len = 0;
			status_t	err = file.GetSize(&len);
			
			fFileRef = ref;
			fEntryRefIsGood = true;

			if (fToken.eOffset <= len)	// Is offset valid
			{
				char*		text = new char[len + 1];
				BFile		openFile(&file, B_READ_ONLY);

				if (B_OK == file.InitCheck() && openFile.IsReadable() &&
					len == openFile.Read(text, len))
				{
					// gcc only gives line numbers - but we want the eOffset and
					// eLength because then the natigation tries to track edits
					// to the file (see MTextWindow::DoGoToLine)
					// look for sentinal to set up the eOffset/eLength dynamically
					// (eLength will cause window to select entire line)
					if (fToken.eOffset == -1) {
						fToken.eOffset = FindLineStart(text, len, inMessage.errorRef.linenumber);
						char* dynamicLineStart = text + fToken.eOffset;
						char* dynamicLineEnd = FindEnd(dynamicLineStart, len - fToken.eOffset);
						fToken.eLength = dynamicLineEnd - dynamicLineStart;
					}

					// Some error offsets are at the end of the line
					if ((text[fToken.eOffset] == EOL_CHAR || text[fToken.eOffset] == MAC_RETURN) &&
						fToken.eOffset > 0) {
						fToken.eOffset--;
					}
					
					TextFormatType		format = MFormatUtils::FindFileFormat(text);
					int32				lineoffset = FindLineStart(text, fToken.eOffset);
					
					if (lineoffset >= 0)
					{
						text[len] = '\0';		// make sure there's always a null at the end
						char*		lineStart = text + lineoffset;
						char*		end = FindEnd(lineStart, len - lineoffset);
						*end = '\0';
						fText += "   ";
						int32		len2 = fText.GetLength();
						fText += lineStart;
						// Save info for an underline
						fHasUnderLine = true;
						fUnderLineNumber = fLines - 1;
						fUnderLineOffset = fToken.eOffset - lineoffset + len2 - len1;
					}
					
					// If there is no sync text try to generate it
					if (fToken.eSync[0] == 0 && fToken.eLength > 0)
					{
						int32		slen = min(fToken.eLength, 32L);
						slen = min(slen, (int32) len - fToken.eOffset);
					
						if (slen > 0)
						{
							strncpy(fToken.eSync, &text[fToken.eOffset], slen);
							fToken.eSyncLength = slen;
							fToken.eSyncOffset = 0;
						}
					}
					// If it's a dos file fix up the offset since it will be
					// a unix file in memory
					if (fToken.eOffset > 0 && kCRLFFormat == format)
					{
						fToken.eOffset -= fToken.eLineNumber;
					}
				}

				delete[] text;
			}
		}
		else
			fFileName = ptr;	// Copy the file name if the entry_ref is no good
	}
	
	fText.Replace('\t', ' ');	// Translate tabs to spaces
}

// ---------------------------------------------------------------------------
//		MMessageItem::FindLineStart
// ---------------------------------------------------------------------------
//	Search for the beginning of the specified line in the string.  return the 
//	offset of this line from inText.  returns B_ERROR if it runs past 
//	inTextLen.  Need to look for Mac returns also
//	because this is reading a file that has been opened without 
//	converting to newline format.

int32
MMessageItem::FindLineStart(
	const char * 	inText, 
	int32 			inOffset)
{
	// unsigned chars to prevent sign extension
	const unsigned char*	cp = (const unsigned char*) inText + inOffset;

	while (cp >= (const unsigned char*) inText)
	{
		switch (*cp)
		{
			case MAC_RETURN:
			case EOL_CHAR:
				return cp - (const unsigned char*) inText + 1;	// early return
				break;
		}

		cp--;
	}
	
	return 0;
}

// ---------------------------------------------------------------------------
//		MMessageItem::FindLineStart
// ---------------------------------------------------------------------------
//	Search for the beginning of the specified line in the string.  return the 
//	offset of this line from inText.  returns B_ERROR if it runs past 
//	inTextLen.  Need to look for Mac returns also
//	because this is reading a file that has been opened without 
//	converting to newline format.

int32
MMessageItem::FindLineStart(
	const char * 	inText, 
	int32 			inTextLen, 
	int32 			inLineNumber)
{
	// inLineNumber is one based... ie: the first line in the inText is line 1

	// unsigned chars to prevent sign extension
	unsigned char*		cp = (unsigned char*) inText;
	unsigned char*		term = (unsigned char*) inText + inTextLen;
	unsigned char*		lastLineStart = cp;
	int32				lines = 1;
	int32				result = B_ERROR;

	// if we are looking for line one, we have it at the start
	if (inLineNumber == 1) {
		return 0;
	}
	
	while (cp < term)
	{
		switch (*cp)
		{
			case MAC_RETURN:
				if (cp[1] == B_RETURN)	// Is this a dos format file?
					break;
				// else fall through
			case EOL_CHAR:
	
				if (++lines == inLineNumber)
				{
					// bump cp past the newline
					cp += 1;
					return (cp - (unsigned char*) inText);
				}
				break;
		}

		cp++;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//		MMessageItem::CountLines
// ---------------------------------------------------------------------------
//	Count the newlines in the specified text

int32
MMessageItem::CountLines(
	const char * 	inText, 
	int32 			inTextLen)
{
	// unsigned chars to prevent sign extension
	unsigned char*		cp = (unsigned char*) inText;
	unsigned char*		term = (unsigned char*) inText + inTextLen;
	int32				lines = 0;

	while (cp < term)
	{
		if (*cp == EOL_CHAR || *cp == 0)
		{
			++lines;
			if (*cp == 0)
			{
				break;
			}
		}

		cp++;
	}
	
	if (cp == term && (*cp == EOL_CHAR || *cp == 0))
		lines++;

	return lines;
}

// ---------------------------------------------------------------------------
//		MMessageItem::InitBitmaps
// ---------------------------------------------------------------------------

void
MMessageItem::InitBitmaps()
{
	if (sErrorBitmap)
		return;

	BRect bounds(0, 0, kBitMapWidth - 1, kBitMapHeight - 1);

	sErrorBitmap = new BBitmap(bounds, B_COLOR_8_BIT);

	const char * 		ptr = sStopIcon;
	uint32				wid = (uint32) kBitMapWidth;

	int 		i;
	for (i = 0; i < kBitMapHeight; i++)
	{
		sErrorBitmap->SetBits(ptr, kBitMapWidth, sErrorBitmap->BytesPerRow() * i, B_COLOR_8_BIT);
		ptr += wid;
	}

	sWarningBitmap = new BBitmap(bounds, B_COLOR_8_BIT);
	ptr = sWarningIcon;

	for (i = 0; i < kBitMapHeight; i++)
	{
		sWarningBitmap->SetBits(ptr, kBitMapWidth, sWarningBitmap->BytesPerRow() * i, B_COLOR_8_BIT);
		ptr += wid;
	}

	sInfoBitmap = new BBitmap(bounds, B_COLOR_8_BIT);
	ptr = sInfoIcon;
	for (i = 0; i < kBitMapHeight; i++)
	{
		sInfoBitmap->SetBits(ptr, kBitMapWidth, sInfoBitmap->BytesPerRow() * i, B_COLOR_8_BIT);
		ptr += wid;
	}
}

// ---------------------------------------------------------------------------
//		MMessageItem::InitBitmaps
// ---------------------------------------------------------------------------

void
MMessageItem::GetErrorBitmap(const BBitmap*& outErrorBitmap)
{
	InitBitmaps();
	outErrorBitmap = sErrorBitmap;
}

// ---------------------------------------------------------------------------

void
MMessageItem::GetWarningBitmap(const BBitmap*& outWarningBitmap)
{
	InitBitmaps();
	outWarningBitmap = sWarningBitmap;
}

// ---------------------------------------------------------------------------

void
MMessageItem::GetInfoBitmap(const BBitmap*& outInfoBitmap)
{
	InitBitmaps();
	outInfoBitmap = sInfoBitmap;	
}

// ---------------------------------------------------------------------------
//		MMessageItem::PruneNewlines();
// ---------------------------------------------------------------------------
//	Prune any trailing newelines.

void
MMessageItem::PruneNewlines()
{
	int32	offset = fText.ROffsetOf(EOL_CHAR);
	
	while (offset == fText.GetLength() - 1)
	{
		fText.Replace("", offset, 1);
		offset = fText.ROffsetOf(EOL_CHAR);
	}
}

// ---------------------------------------------------------------------------
//		MErrorMessage::MErrorMessage
// ---------------------------------------------------------------------------
//	Constructor

MErrorMessage::MErrorMessage(const ErrorNotificationMessage& inMessage)
	: MMessageItem("Error   : ")
{
	BuildErrorText(inMessage);
}

// ---------------------------------------------------------------------------
//		MErrorMessage::MErrorMessage
// ---------------------------------------------------------------------------
//	Constructor

MErrorMessage::MErrorMessage(const char* inText)
{
	fText = "Error   : ";
	fText += inText;
	PruneNewlines();
	fText.Replace('\t', ' ');	// Translate tabs to spaces
	fLines = CountLines(fText, fText.GetLength());
}

// ---------------------------------------------------------------------------
//		MErrorMessage::Draw
// ---------------------------------------------------------------------------

void
MErrorMessage::Draw(BRect inFrame, MMessageView& inParentView)
{
	BPoint				pt(kIconMargin, inFrame.top + 2);
{
	stDrawingMode		mode(inParentView, B_OP_OVER);

	inParentView.DrawBitmapAsync(sErrorBitmap, pt);
}
	
	MMessageItem::Draw(inFrame, inParentView);
}

// ---------------------------------------------------------------------------
//		MWarningMessage::MWarningMessage
// ---------------------------------------------------------------------------
//	Constructor

MWarningMessage::MWarningMessage(const ErrorNotificationMessage& inMessage)
	: MMessageItem("Warning : ")
{
	BuildErrorText(inMessage);
}

// ---------------------------------------------------------------------------
//		MWarningMessage::MWarningMessage
// ---------------------------------------------------------------------------
//	Constructor

MWarningMessage::MWarningMessage(const char * inText)
{
	fText = "Warning   : ";
	fText += inText;
	PruneNewlines();
	fText.Replace('\t', ' ');	// Translate tabs to spaces
	fLines = CountLines(fText, fText.GetLength());
}

// ---------------------------------------------------------------------------
//		MWarningMessage::Draw
// ---------------------------------------------------------------------------

void
MWarningMessage::Draw(
	BRect 			inFrame, 
	MMessageView& 	inParentView)
{
	BPoint		pt(kIconMargin, inFrame.top + 2);
{
	stDrawingMode		mode(inParentView, B_OP_OVER);

	inParentView.DrawBitmapAsync(sWarningBitmap, pt);
}	
	MMessageItem::Draw(inFrame, inParentView);
}

// ---------------------------------------------------------------------------
//		MInfoMessage::MInfoMessage
// ---------------------------------------------------------------------------
//	Constructor

MInfoMessage::MInfoMessage(
	const InfoStruct& inInfoStruct,
	uint32				inKind)
{
	BuildInfoText(inInfoStruct, inKind);
}

// ---------------------------------------------------------------------------

MInfoMessage::MInfoMessage()
{
	// for derived classes only...
	// the derived class is in charge of building any message text
}

// ---------------------------------------------------------------------------
//		MInfoMessage::BuildInfoText
// ---------------------------------------------------------------------------

void
MInfoMessage::BuildInfoText(
	const InfoStruct& 	inInfoStruct,
	uint32				inKind)
{
	switch (inKind)
	{
		case msgAddInfoToMessageWindow:
			// From the find window
			if (inInfoStruct.iTextOnly)
			{
				fText = inInfoStruct.iLineText;
				fLines = CountLines(inInfoStruct.iLineText, kLineTextLength);
				
				fTokenIsGood = false;
				fLineNum = -1;
			}
			else
			{
				ASSERT(inInfoStruct.iRef != nil);
				fFileRef = *inInfoStruct.iRef;
				delete inInfoStruct.iRef;
				if (fFileRef.device != -1)
					fEntryRefIsGood = true;
				else
					fFileName = inInfoStruct.iFileName;

				fLineNum = inInfoStruct.iLineNumber;

				fText = inInfoStruct.iFileName;
				fText += " line ";
				fText += fLineNum + 1;		// internally count from zero
				fText += "\n   ";
				fText += inInfoStruct.iLineText;

				fLines = 1 + CountLines(inInfoStruct.iLineText, kLineTextLength);
				fTokenIsGood = true;
				fToken = inInfoStruct.iToken;
			}
			break;
			
		case msgAddDefinitionToMessageWindow:
			// From a find definition
			ASSERT(inInfoStruct.iRef != nil);
			fFileRef = *inInfoStruct.iRef;
			delete inInfoStruct.iRef;
			fEntryRefIsGood = true;
			
			fLineNum = inInfoStruct.iLineNumber;

			fText = "Multiple Definitions in ";
			fText += inInfoStruct.iLineText;
			fText += "\n   ";
			fText += inInfoStruct.iFileName;
			fText += " line ";
			fText += fLineNum + 1;		// internally count from zero
			fText += "\n   ";

			fLines = 2;
			fTokenIsGood = true;
			fToken = inInfoStruct.iToken;
			break;
	}

	fText.Replace('\t', ' ');	// Translate tabs to spaces
}

// ---------------------------------------------------------------------------
//		MInfoMessage::Draw
// ---------------------------------------------------------------------------

void
MInfoMessage::Draw(BRect inFrame, MMessageView& inParentView)
{
	BPoint		pt(kIconMargin, inFrame.top + 2);
{
	stDrawingMode		mode(inParentView, B_OP_OVER);

	inParentView.DrawBitmapAsync(sInfoBitmap, pt);
}	
	MMessageItem::Draw(inFrame, inParentView);
}

// ---------------------------------------------------------------------------
// MDocumentationMessage - static strings
// ---------------------------------------------------------------------------

#define B_UTF8_BULLET "\xE2\x80\xA2"
#define BULLET B_UTF8_BULLET

// ---------------------------------------------------------------------------
// MDocumentationMessage - Member functions
// ---------------------------------------------------------------------------

MDocumentationMessage::MDocumentationMessage(const DocumentationLookupEntry& inDocInfo)
					  : MInfoMessage()
{
	// all the fields are correctly initialized in MMessageItem
	// just fill in the text from the documentation information	
	MDocumentationMessage::BuildMessageText(inDocInfo);
}

// ---------------------------------------------------------------------------

void
MDocumentationMessage::Draw(BRect inFrame, MMessageView& inParentView)
{
	BPoint pt(kIconMargin, inFrame.top + 2);

	{
		stDrawingMode mode(inParentView, B_OP_OVER);
		inParentView.DrawBitmapAsync(sInfoBitmap, pt);
	}	

	MMessageItem::Draw(inFrame, inParentView);
}

// ---------------------------------------------------------------------------
	
void
MDocumentationMessage::BuildMessageText(const DocumentationLookupEntry& inDocInfo)
{
	// The results of the documentation lookup query include the
	// title, class, and short description of the result.
	// Since they user can look up "BMenu" and get anything from
	// Start with the class, then the title, on the next line
	// add the description and url
	// It is structured to look like this:
	//	Class :: Title
	//	Short Description in: URL

	// start with the class (if any)
	if (strlen(inDocInfo.fClass) > 0) {
		fText += inDocInfo.fClass;
		fText += " :: ";
	}

	// add the title
	fText += inDocInfo.fTitle;
	
	// we know we have at least one line of message
	fLines = 1;
		
	// add the description and url so they have the url if they
	// want to save the list to a file
	fText += "\n   ";
	fLines += 1;
	
	// add the short description if we have one
	if (strlen(inDocInfo.fDescription) > 0) {
		fText += inDocInfo.fDescription;
		fText += " - ";
	}

	// end with url	
	fText += inDocInfo.fURL;
	
	// save the URL in the fFileName that our base class keeps around
	// we need it for ::Invoke
	fFileName = inDocInfo.fURL;
}

// ---------------------------------------------------------------------------

void
MDocumentationMessage::Invoke()
{
	// Launch NetPositive with the URL of the documentation -- that's it!
	DocumentationLookup::Open((const char*)fFileName);
}
