//
//	HTMLExport.cpp
//

#include <Bitmap.h>
#include <stdio.h>
#include <string.h>
#include <UTF8.h>
#include <TranslationKit.h>
#include <Directory.h>

#include "HTMLExport.h"
#include "HTMLTables.h"

#define DEBUG 1

//-------------------------------------------------------------------
// TExportHTML
//-------------------------------------------------------------------

TExportHTML::TExportHTML(TBlockStreamReader* reader, BPositionIO *outStream, int32 charEncoding)
{
	mReader = reader;
	mOutStream = outStream;
	mCharEncoding = charEncoding;	

/*
	mOutFile = dynamic_cast<BFile*>(mOutStream);
	if (mOutFile)
	{
		BNodeInfo nodeInfo(mOutFile);	
		nodeInfo.SetType("text/html");
	}
*/
	
	mBufferMaxSize = INIT_BUFFER_SIZE;
	mBuffer = new char[mBufferMaxSize];
	mBufferSize = 0;
}


TExportHTML::~TExportHTML()
{
	delete [] mBuffer;	
}


int32 TExportHTML::DoTranslate(BMessage *ioExt)
{

	printf("DoTranslate begin\n");
		
	status_t err = mTransDoc.Read( mReader );
	if (err)
		return err;

	mIOExt = ioExt;		
	// look in the ioExt for the entry_ref of the file we're writing to, and it's name
	if (ioExt && ioExt->HasRef("ref") && ioExt->HasString("fileName"))
	{
		entry_ref	ref;
		status_t result = ioExt->FindRef("ref", &ref);
		ioExt->FindString("fileName", &mFileName);
		IFDEBUG(fprintf(stderr, "HTML: %s\n", mFileName));
//
//		char dirName[256];
//		strcpy(dirName, mFileName);
//		strcpy(dirName + strlen(mFileName), ".images");
//
//		// Try to use fileName.images folder for images if not use save dir as file.
//		BDirectory 	dir(&ref);
//		BEntry		dirEntry;
//		dir.GetEntry(&dirEntry);
//		if (B_NO_ERROR == dir.FindEntry(dirName, &dirEntry, true))
//			mDir.SetTo(&dirEntry);
//		else if (B_NO_ERROR != dir.CreateDirectory(dirName, &mDir))
//			mDir.SetTo(&ref);
	}
	else
	{
		IFDEBUG(fprintf(stderr, "HTML: No ioExt\n"));
		mFileName = NULL;
		mFileName = "test.html";
	}
	mFileCount = 0;

	printf("DoTranslate here\n");
	
	WriteString("<HTML>\n");
	writeHeader();
	writeBody();
	WriteString("</HTML>\n");
	
    return B_NO_ERROR;
}

void TExportHTML::CreateImagesDirectory(void)
{
	if (mIOExt && mIOExt->HasRef("ref") && mIOExt->HasString("fileName"))
	{
		entry_ref ref;
		status_t result = mIOExt->FindRef("ref", &ref);
		char dirName[256];
		strcpy(dirName, mFileName);
		strcpy(dirName + strlen(mFileName), ".images");

		// Try to use fileName.images folder for images if not use save dir as file.
		BDirectory 	dir(&ref);
		BEntry		dirEntry;
		dir.GetEntry(&dirEntry);
		if (B_NO_ERROR == dir.FindEntry(dirName, &dirEntry, true))
			mDir.SetTo(&dirEntry);
		else if (B_NO_ERROR != dir.CreateDirectory(dirName, &mDir))
			mDir.SetTo(&ref);
	}
}

void TExportHTML::WriteString(const char* str) const
{
	mOutStream->Write(str, strlen(str));
}

void TExportHTML::WriteColor(const char* attrName, rgb_color theColor) const
{
	char colorStr[64];
	sprintf(colorStr, " %s=\"#%02x%02x%02x\"", attrName, theColor.red, theColor.green, theColor.blue);
	WriteString(colorStr);
}

void TExportHTML::WriteIntAttribute(const char* attrName, int32 value) const
{
	char buffer[256];
	sprintf(buffer, " %s=%d", attrName, value);
	WriteString(buffer);
}

void TExportHTML::WritePercentAttribute(const char* attrName, int32 value) const
{
	char buffer[256];
	sprintf(buffer, " %s=\"%d%%\"", attrName, value);
	WriteString(buffer);
}

void TExportHTML::WriteWidthAttribute(const char* attrName, int32 value, bool isPercent) const
{
	if (value && isPercent)
		WritePercentAttribute(attrName, value);
	else if (value)
		WriteIntAttribute(attrName, value);
}

void TExportHTML::writeHeader(void)
{
	WriteString("<HEAD>\n");
	WriteString("<TITLE>");
	if (mFileName)
		WriteString(mFileName);
	WriteString("</TITLE>\n");
	WriteString("</HEAD>\n");
}

void TExportHTML::writeBody(void)
{
	// Determine if the Text Background style is defined for this document.
	TTranslatorStylesTable* stylesTab = mTransDoc.StylesTable();
	const TTranslatorStyle* backStylePtr = NULL;
	for (int32 x = 0; x < stylesTab->CountItems(); x++)
	{
		const TTranslatorStyle* stylePtr = (*stylesTab)[x];
		const char* styleName;
		if (stylePtr->GetStyleName(&styleName) && !strcmp(styleName, "Text Background"))
		{
			backStylePtr = stylePtr;
			break;
		}
	}
	
	WriteString("<BODY");
	rgb_color 	high, low;
	pattern		pat;
	if (backStylePtr && backStylePtr->GetFillColor(high, low, pat))
		WriteColor("BGCOLOR", high);
	WriteString(">\n");

	// Write out each of the sheets that is defined.
	int32 sheets = mTransDoc.Sheets();
	
	printf("TExportHTML::writeBody()  %ld sheets\n", sheets);
	
	for (int32 x = 0; x < sheets; x++)
	{
		// Call appropriate code to export the sheet type.
		const TTranslatorSheet* sheetPtr = mTransDoc.SheetPtr(x);
		switch (sheetPtr->SheetKind())
		{
			case WORDPROCESSING_MINOR_KIND:
				WriteWord(sheetPtr->MainPartPtr());
				break;
			case SPREADSHEET_MINOR_KIND:
				break;
			case TABLE_MINOR_KIND:
				WriteTable(sheetPtr->MainPartPtr());
				break;
		}
	}
	WriteString("</BODY>\n");
}

#pragma mark -
void TExportHTML::WriteTable(const TTranslatorPart* partPtr)
{
	const TTranslatorPart_Table* tablePtr = dynamic_cast<const TTranslatorPart_Table*>(partPtr);
	if (!tablePtr)
		return;
	
	char		buffer[1024];
	rgb_color 	high, low;
	pattern		pat;
	int32		align;	
	bool 		isPercentWidth;
	int32 		tableWidth = tablePtr->Width(isPercentWidth);
	int32		tableHeight = tablePtr->Height();
	
	// Table tag with attributes.
	// <TABLE WIDTH=xxx HEIGHT=xxx BORDER=xxx CELLPADDING=xxx CELLSPACING=xxx BGCOLOR=xxx>
	WriteString("<TABLE");
	WriteWidthAttribute("WIDTH", tableWidth, isPercentWidth);
	WriteIntAttribute("HEIGHT", tableHeight);
	WriteIntAttribute("BORDER", tablePtr->Border());
	WriteIntAttribute("CELLPADDING", tablePtr->CellPadding());
	WriteIntAttribute("CELLSPACING", tablePtr->CellSpacing());
	const TTranslatorStyle* tableStylePtr = tablePtr->TableStylePtr();
	if (tableStylePtr && tableStylePtr->GetFillColor(high, low, pat))
		WriteColor("BGCOLOR", high);
	WriteString(">");
		
	// Write out the table rows.
	int32 rows = tablePtr->Rows();
	for (int32 x = 0; x < rows; x++)
	{
		TTableRow* rowPtr = tablePtr->RowPtr(x);
		WriteTableRow(rowPtr);
	}

	// Close table
	WriteString("</TABLE>");
}

void TExportHTML::WriteTableRow(const TTableRow* rowPtr)
{
	if (rowPtr)
	{	
		WriteString("<TR>");
		for (int32 i = 0; i < rowPtr->Cells(); i++)
		{
			TTableCell* cellPtr = rowPtr->CellPtr(i);
			WriteTableCell(cellPtr);
		}
		WriteString("</TR>");
	}
}

void TExportHTML::WriteTableCell(const TTableCell* cellPtr)
{
	if (!cellPtr)
		return;
		
	rgb_color 	high, low;
	pattern		pat;
	int32		align;	
	bool 		isPercentWidth, isPercentHeight;
	bool		forceAlignment = false;
	int32 		cellWidth = cellPtr->Width(isPercentWidth);
	int32 		cellHeight = cellPtr->Height(isPercentHeight);
	const 		TTranslatorStyle* stylePtr = cellPtr->CellStylePtr();

	// <TD WIDTH=xxx HEIGHT=xxx COLSPAN=xxx ROWSPAN=xxx BGCOLOR=xxx ALIGN=xxx VALIGN=xxx >
	WriteString("<TD");
	WriteWidthAttribute("WIDTH", cellWidth, isPercentWidth);
	WriteWidthAttribute("HEIGHT", cellHeight, isPercentHeight);
	WriteIntAttribute("COLSPAN", cellPtr->ColumnSpan());
	WriteIntAttribute("ROWSPAN", cellPtr->RowSpan());

	TTranslatorPart_WP* cellWPPtr = cellPtr->Text();
	if (cellWPPtr && cellWPPtr->ParagraphStyleRuns())
	{
		const TTranslatorStyle* firstParStyle = cellWPPtr->ParagraphStylePtr(0, NULL, NULL);
		if (firstParStyle->GetParagraphAlignment(align))
		{
			forceAlignment = true;
			switch (align)
			{
				case TTranslatorStyle::kParAlignCenter:
					align = TTranslatorStyle::kAlignCenter;
					break;
				case TTranslatorStyle::kParAlignRight:
					align = TTranslatorStyle::kAlignEnd;
					break;
				default:
					align = TTranslatorStyle::kAlignStart;
					break;
			}
		}
	}

	if (stylePtr)
	{
		if (stylePtr->GetFillColor(high, low, pat))
			WriteColor("BGCOLOR", high);
		if (forceAlignment || stylePtr->GetTextAlignment(align, true))
		{
			WriteString(" ALIGN=");
			switch (align)
			{
				case TTranslatorStyle::kAlignCenter:
					WriteString("CENTER");
					break;
				case TTranslatorStyle::kAlignEnd:
					WriteString("RIGHT");
					break;
				default:
					WriteString("LEFT");
					break;
			}
		}
		if (stylePtr->GetTextAlignment(align, false))
		{
			WriteString(" VALIGN=");
			switch (align)
			{
				case TTranslatorStyle::kAlignCenter:
					WriteString("MIDDLE");
					break;
				case TTranslatorStyle::kAlignEnd:
					WriteString("BOTTOM");
					break;
				default:
					WriteString("TOP");
					break;
			}
		}
	}
	WriteString(">");
	
	// Write cell contents here
	if (cellWPPtr)
		WriteWord(cellWPPtr, true);
	
	WriteString("</TD>");
}

#pragma mark -
void TExportHTML::WriteWord(const TTranslatorPart* partPtr, bool beginNewPar)
{
	const TTranslatorPart_WP* wpPtr = dynamic_cast<const TTranslatorPart_WP*>(partPtr);
	if (!wpPtr)
		return;

	TWPExportStateInfo info(mPartPtr);
	RememberWPExportState(info);

	mPartPtr = wpPtr;
	mTextBlockIndex = 0;
	mTextBlockOffset = 0;
	mTextOffset = 0;
	mParStyleIndex = 0;
	mCharStyleIndex = 0;
	mSpecialCharIndex = 0;

	// Loop getting runs of text and outputing them as appropriate.
	bool inParagraph = !beginNewPar;
	while (	true )
	{
		// Get the next run of text.
		int32 startOffset = mTextOffset;
		uchar runBuffer[1024];
		int32 runLength = NextRun(runBuffer, 1024);
		if (!runLength)
			break;
		
		// Check for non paragraph elements here (like HR)
		
		// Do not output CR.
		bool hasCR = (runBuffer[runLength-1] == 0x0A);
		if (hasCR)
			runLength--;
		
		// Check for blank lines (if so use single non-breaking space to hold the line).
		if (!inParagraph && !runLength && hasCR)
		{
			BeginParagraph();
			PreCheckCharStyles();
			WriteString("&nbsp;");
			PostCheckCharStyles();		
			EndParagraph();
			continue;
		}
		
		// Check for special character handling.
		const TSpecialCharacter* sPtr = mPartPtr->SpecialCharacter(mSpecialCharIndex);
		if (sPtr && startOffset >= sPtr->Offset())
		{
			mSpecialCharIndex++;
			if (sPtr->Kind() == kSCFrame)
			{
				const TSpecialCharacter_Frame* ptr = dynamic_cast<const TSpecialCharacter_Frame*>(sPtr);
				if (ptr)
				{
					const TTranslatorPart* framePartPtr = ptr->FramePart();
					if (framePartPtr && framePartPtr->PartKind() == TABLE_MINOR_KIND)
						WriteTable(framePartPtr);
					else
					{
						if (!inParagraph)
						{
							BeginParagraph();
							inParagraph = true;
						} 
						writeBitmap(ptr->FrameImage());
					}
					runLength--;
				}
			}
			else if (sPtr->Kind() == kSCHorizontalRule)
			{
				WriteString("\n<HR>\n");
				runLength--;
			}
			else
			{
				// Default case for special character (if it has text output that).
				const char* scTextPtr = sPtr->Text();
				if (scTextPtr)
				{
					if (!inParagraph)
					{
						BeginParagraph();
						inParagraph = true;
					}
					WriteString(scTextPtr);
					runLength--;
				}
			}
		}

		// Must be text to put inside of paragraph tags.
		else if (!inParagraph)
		{
			BeginParagraph();
			inParagraph = true;
		}
		
		PreCheckCharStyles();
		OutputHTMLText(runBuffer, runLength);		
		PostCheckCharStyles();		

		if (hasCR)
		{
			EndParagraph();
			inParagraph = false;
		}
	}
	RestoreWPExportState(info);
}

void TExportHTML::BeginParagraph(void)
{
	// Get current paragraph style for alignment.
	const TTranslatorStyle* stylePtr = mPartPtr->ParagraphStylePtr(mParStyleIndex, NULL, NULL);
	int32 align;
	if (!stylePtr || !stylePtr->GetParagraphAlignment(align))
		align = -1;
		
	switch (align)
	{
		case TTranslatorStyle::kParAlignLeft:
			WriteString("<P ALIGN=\"left\">");
			break;
		case TTranslatorStyle::kParAlignCenter:
			WriteString("<P ALIGN=\"center\">");
			break;
		case TTranslatorStyle::kParAlignRight:
			WriteString("<P ALIGN=\"right\">");
			break;
		case TTranslatorStyle::kParAlignJust:
			WriteString("<P ALIGN=\"justify\">");
			break;
		default:
			WriteString("<P>");
			break;
	}	
}

void TExportHTML::EndParagraph(void)
{
	WriteString("</P>\n");
	int32 nextParStyleStart, nextParStyleEnd;
	const TTranslatorStyle* nextParStylePtr = mPartPtr->ParagraphStylePtr(mParStyleIndex+1, &nextParStyleStart, &nextParStyleEnd);
	if (nextParStylePtr && nextParStyleStart <= mTextOffset)
		mParStyleIndex++;
}

// Put characters into buffer until end of paragraph, maxSize reached,
// or style run start met. 
int32 TExportHTML::NextRun(uchar* buffer, int32 maxSize)
{
	ssize_t textLength;
	const char* textPtr = mPartPtr->TextBlock(mTextBlockIndex, &textLength);
	int32 count = 0;
	int32 startOffset = mTextOffset;

	// Determine the next character style run break.
	int32 nextCharStyleStart, nextCharStyleEnd;
	const TTranslatorStyle* nextCharStylePtr = mPartPtr->CharacterStylePtr(mCharStyleIndex+1, &nextCharStyleStart, &nextCharStyleEnd);	
	const TSpecialCharacter* sPtr = mPartPtr->SpecialCharacter(mSpecialCharIndex);
	
	while (textPtr && count < maxSize)
	{
		// Get current character
		char 	theChar = *(textPtr + mTextBlockOffset);
		bool 	isCR = (theChar == 0x0A);
		bool 	isSC = (sPtr && sPtr->Offset() == mTextOffset);

		// Add character to the buffer.
		buffer[count++] = theChar;
		
		// Position to read next character.
		if (++mTextBlockOffset >= textLength)
		{
			mTextBlockOffset = 0;
			mTextBlockIndex++;
			textPtr = mPartPtr->TextBlock(mTextBlockIndex, &textLength);
		}
		mTextOffset++;
		
		// Break if carriage return marking end of paragraph or we found special character.
		if (isCR || isSC)
			break;
		// If we've reached end of character style break and handle style change.
		if (nextCharStylePtr && mTextOffset == nextCharStyleStart)
			break;
		// If next character is a special character then handle buffered text first.
		if (sPtr && sPtr->Offset() == mTextOffset)
			break;
	}	
	return count;
}

void TExportHTML::OutputHTMLText(const uchar* buffer, int32 bufferLength)
{
	if (bufferLength)
	{
		convertToHTMLBuffer(buffer, bufferLength);					
		mOutStream->Write(mBuffer, mBufferSize);
	}
}

void TExportHTML::PreCheckCharStyles(void)
{					
	// Get current paragraph style for alignment.
	const TTranslatorStyle* stylePtr = mPartPtr->CharacterStylePtr(mCharStyleIndex, NULL, NULL);
	if (!stylePtr)
		return;
	
	bool		hasFace, hasColor, hasSize;	
	int32 		textFace, textMask;
	float		textSize;		
	rgb_color 	high = OUR_BLACK;
	rgb_color	low = OUR_BLACK;
	pattern		pat = B_SOLID_HIGH;
	
	hasFace = stylePtr->GetTextFace(textFace, textMask);
	hasSize = stylePtr->GetTextSize(textSize);
	hasColor = stylePtr->GetTextColor(high, low, pat);

	// If pattern is not B_SOLID_HIGH and low color differs from high color - use black text.
	bool isSolidHigh = !memcmp(&pat, &B_SOLID_HIGH, sizeof(pattern));
	if (!isSolidHigh && (high.red != low.red || high.green != low.green || high.blue != low.blue))
		high = OUR_BLACK;

	mFont = mBold = mItalic = mUnderline = false; 
	mStrikeThru = mSuperScript = mSubScript = false;
	
	// Text Size and Color (not defaults of black and 12)
	if (hasColor || hasSize)
	{
		mFont = true;
		WriteString("<FONT");
		
		if (hasColor)
			WriteColor("COLOR", high);
		
		if (hasSize)
		{
			if (textSize <= 9)
				WriteString(" SIZE=1");
			else if (textSize <= 10)			
				WriteString(" SIZE=2");
			else if (textSize <= 12)			
				WriteString(" SIZE=3");
			else if (textSize <= 14)			
				WriteString(" SIZE=4");
			else if (textSize <= 18)			
				WriteString(" SIZE=5");
			else if (textSize <= 24)			
				WriteString(" SIZE=6");
			else			
				WriteString(" SIZE=7");
		}
		WriteString(">");
	}
		
	// Text Face
	if (hasFace)
	{
		mBold = (textFace & TTranslatorStyle::kBoldFace);
		if (mBold)
			WriteString("<B>");
		mItalic = (textFace & TTranslatorStyle::kItalicFace);
		if (mItalic)
			WriteString("<I>");
		mUnderline = (textFace & (TTranslatorStyle::kUnderlineFace | TTranslatorStyle::kDoubleUnderlineFace));
		if (mUnderline)
			WriteString("<U>");
		mStrikeThru = (textFace & TTranslatorStyle::kStrikeThruFace);
		if (mStrikeThru)
			WriteString("<STRIKE>");
		mSuperScript = (textFace & TTranslatorStyle::kSuperScriptFace);
		if (mSuperScript)
			WriteString("<SUP>");
		mSubScript = (textFace & TTranslatorStyle::kSubScriptFace);
		if (mSubScript)
			WriteString("<SUB>");
	}

	// Bump char style index if we are now past the current style.	
	int32 nextCharStyleStart, nextCharStyleEnd;
	const TTranslatorStyle* nextCharStylePtr = mPartPtr->CharacterStylePtr(mCharStyleIndex+1, &nextCharStyleStart, &nextCharStyleEnd);
	if (nextCharStylePtr && nextCharStyleStart <= mTextOffset)
		mCharStyleIndex++;
}


void TExportHTML::PostCheckCharStyles(void)
{
	if (mSubScript)
		WriteString("</SUB>");
	if (mSuperScript)
		WriteString("</SUP>");
	if (mStrikeThru)
		WriteString("</STRIKE>");
	if (mUnderline)
		WriteString("</U>");
	if (mItalic)
		WriteString("</I>");
	if (mBold)
		WriteString("</B>");
	if (mFont)
		WriteString("</FONT>");
}					


// Write the bitmap out to a file as gif or jpeg
void TExportHTML::writeBitmap(const BBitmap* bitmap)
{
	if (!bitmap)
		return;
		
	BTranslatorRoster *	roster = BTranslatorRoster::Default();
	int32				num_translators, numOutFormats; 
	translator_id *		translators;
	const translation_format *outFormats; 
	translator_id		jpegTrans = 0, gifTrans = 0;
	float				jpegQuality = 0.0, gifQuality = 0.0;
	uint32				jpegType = 0, gifType = 0;
	
	roster->GetAllTranslators(&translators, &num_translators); 
	for (long i = 0; i < num_translators; i++) 
	{ 
		roster->GetOutputFormats(translators[i], &outFormats, &numOutFormats);
		for (long j = 0; j < numOutFormats; j++)
		{			
			if (!strcasecmp(outFormats[j].MIME, "image/gif"))
			{
				if (outFormats[j].quality > gifQuality)
				{
					gifTrans = translators[i];
					gifQuality = outFormats[j].quality;
					gifType = outFormats[j].type;
				}
			}
			if (!strcasecmp(outFormats[j].MIME, "image/jpeg"))
			{
				if (outFormats[j].quality > jpegQuality)
				{
					jpegTrans = translators[i];
					jpegQuality = outFormats[j].quality;
					jpegType = outFormats[j].type;
				}
			}
		}
	} 
	delete [] translators;	
	
	if (jpegQuality <= 0 && gifQuality <= 0)
	{
		IFDEBUG(fprintf(stderr, "HTML: no jpeg or gif translator\n"));
		delete bitmap;
		return;
	}

	// Wait until we know we have some images to output before creating images directory.
	if (!mFileCount)
		CreateImagesDirectory();

	// Create a file to contain the jpeg
	bool useJPG = (jpegQuality > 0);
	char newFileName[512];
	char* extStr = useJPG ? "jpg" : "gif";
	sprintf(newFileName, "image.%i.%s", mFileCount, extStr);
	BFile file;
	if (mDir.CreateFile(newFileName, &file, false))
	{
		IFDEBUG(fprintf(stderr, "HTML: error creating file\n"));
		delete bitmap;
		return;
	}
	
	mFileCount++;
		
	// bitmapStream will delete the bitmap on exit, so don't have to do it ourselves from this point on
	BBitmapStream bitmapStream(new BBitmap(bitmap));

	// call the translator to translate from BBitmap to JPEG or BMP
	int32 			transType = useJPG ? jpegType : gifType;
	translator_id 	transID = useJPG ? jpegTrans : gifTrans;
	status_t 		err = roster->Translate(transID, &bitmapStream, NULL, &file, transType);

	if (err != B_NO_ERROR)
	{
		IFDEBUG(fprintf(stderr, "HTML: error translating bitmap.\n"));
		return;
	}
	
	// write out the HTML to give an IMG that points to this file
	WriteString("<IMG SRC=\"");
	char dirName[512];
	sprintf(dirName, "%s.images/", mFileName);
	mOutStream->Write(dirName, strlen(dirName));
	mOutStream->Write(newFileName, strlen(newFileName));
	sprintf(newFileName, "\" WIDTH=\"%i\" HEIGHT=\"%i\">", (int32)bitmap->Bounds().Width(), (int32)bitmap->Bounds().Height());
	mOutStream->Write(newFileName, strlen(newFileName));
}


// copy the dataPtr into our buffer and make it ANSI
int32 TExportHTML::convertToHTMLBuffer(const uchar *dataPtr, int32 dataSize)
{
	// Make sure our buffer is big enough
	if ((dataSize * 4) > mBufferMaxSize)
	{
		// need to make the buffer bigger
		delete [] mBuffer;
		mBufferMaxSize = (dataSize * 4);
		mBuffer = new char[mBufferMaxSize];
		mBufferSize = 0;
		if (!mBuffer)
			return B_ERROR;
	}

	int y = 0;
	for (int x = 0; x < dataSize; x++)
	{
		uchar c = dataPtr[x];
		if (c <= 0x7F)
		{
			char* s;
			int l;

			switch(c)
			{
				case X_LINEBREAK:
				case '<':
				case '>':
				case '&':
				{
					switch(c)
					{
						case X_LINEBREAK:
							s = "<BR>";
							break;
						case '<':
							s = "&lt;";
							break;
						case '>':
							s = "&gt;";
							break;
						case '&':
							s = "&amp;";
							break;
					}
					l = strlen(s);
					memcpy(&mBuffer[y], s, l);
					y += l;
					break;
				}
				default:
					mBuffer[y++] = c;
					break;
			}
		}
		else
		{
			char buf[32];
			ushort uni;
			int32 inSize = dataSize - x, outSize = 2, state = 0, delta = CHAR_BYTES_UTF8(c);
			char* htmlString;

			convert_from_utf8(B_UNICODE_CONVERSION, (char *)&dataPtr[x], &inSize, buf, &outSize, &state, '?');
			uni = B_HOST_TO_BENDIAN_INT16(*(ushort*)buf);
			
			htmlString = GetISOName(uni);
			if (htmlString)
			{
				mBuffer[y++] = '&';
				inSize = strlen(htmlString);
				memcpy(&mBuffer[y], htmlString, inSize);
				y += inSize;
				mBuffer[y++] = ';';
			}
			else if (mCharEncoding == -1)
			{
				// Destination is utf8 and it's already utf8 so just copy over utf8 character.
				memcpy(&mBuffer[y], &dataPtr[x], delta);
				y += delta;
			}
			else if (mCharEncoding == B_ISO1_CONVERSION)
			{
				sprintf(buf, "&#%d;", uni);
				inSize = strlen(buf);
				memcpy(&mBuffer[y], buf, inSize);
				y += inSize;
			}
			else
			{
				// Use native encoding.
				inSize = delta;
				outSize = mBufferMaxSize - y;
				state = 0;
				convert_from_utf8(mCharEncoding, (char *)&dataPtr[x], &inSize, &mBuffer[y], &outSize, &state, '?');
				y += outSize;
			}
			
			x += delta - 1;
		}
	}

	mBufferSize =  y;
    return B_NO_ERROR;
}

void TExportHTML::RememberWPExportState(TWPExportStateInfo& info)
{
	info.mPartPtr = mPartPtr;
	info.mTextBlockIndex = mTextBlockIndex;
	info.mTextBlockOffset = mTextBlockOffset;
	info.mTextOffset = mTextOffset;
	info.mParStyleIndex = mParStyleIndex;
	info.mCharStyleIndex = mCharStyleIndex;
	info.mSpecialCharIndex = mSpecialCharIndex;

	info.mFont = mFont;
	info.mBold = mBold;
	info.mItalic = mItalic;
	info.mUnderline = mUnderline;
	info.mStrikeThru = mStrikeThru;
	info.mSuperScript = mSuperScript;
	info.mSubScript = mSubScript;
}

void TExportHTML::RestoreWPExportState(TWPExportStateInfo& info)
{
	mPartPtr = info.mPartPtr;
	mTextBlockIndex = info.mTextBlockIndex;
	mTextBlockOffset = info.mTextBlockOffset;
	mTextOffset = info.mTextOffset;
	mParStyleIndex = info.mParStyleIndex;
	mCharStyleIndex = info.mCharStyleIndex;
	mSpecialCharIndex = info.mSpecialCharIndex;

	mFont = info.mFont;
	mBold = info.mBold;
	mItalic = info.mItalic;
	mUnderline = info.mUnderline;
	mStrikeThru = info.mStrikeThru;
	mSuperScript = info.mSuperScript;
	mSubScript = info.mSubScript;
}

TWPExportStateInfo::TWPExportStateInfo(const TTranslatorPart_WP* partPtr)
{
	mPartPtr = partPtr;
	mTextBlockIndex = 0;
	mTextBlockOffset = 0;
	mTextOffset = 0;
	mParStyleIndex = 0;
	mCharStyleIndex = 0;
	mSpecialCharIndex = 0;

	mFont = mBold = mItalic = mUnderline = false; 
	mStrikeThru = mSuperScript = mSubScript = false;
}

// $Header: /usr/local/cvsroot/8ball/Datatypes/HTML/HTMLExport.cpp,v 1.23 2000/02/17 00:22:09 tom Exp $
