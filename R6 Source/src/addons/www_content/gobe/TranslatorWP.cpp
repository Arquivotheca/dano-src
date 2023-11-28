//*****************************************************************************************************
// TranslatorWP.cpp
//
//	This document contains the implementation of classes for use when reading or writing wordprocessing
// parts from a translator document.
//*****************************************************************************************************

#include <string.h>
#include <stdio.h>
#include <Bitmap.h>

#include "TranslatorStyle.h"
#include "TranslatorWP.h"
#include "TranslatorSS.h"
#include "TranslatorTable.h"
#include "FilterUtils.h"

//*****************************************************************************************************
//									TTranslatorPart_WP
//*****************************************************************************************************
TTranslatorPart_WP::TTranslatorPart_WP(TTranslatorDoc* docPtr)
	: TTranslatorPart(docPtr)
{
	mTotalLength = 0;
	mNumColumns = 1;
}

TTranslatorPart_WP::~TTranslatorPart_WP()
{
	while (mSpecialChars.CountItems())
		delete static_cast<TSpecialCharacter*>(mSpecialChars.RemoveItem(0L));
		
	while (mTextBlocks.CountItems())
		delete [] mTextBlocks.RemoveItem(0L);
	while (mParagraphStyles.CountItems())
	{
		TTranslatorStyle* stylePtr = static_cast<TTranslatorStyle*>(mParagraphStyles.RemoveItem(0L));
		if (!stylePtr->StyleSheet())
			delete stylePtr;
	}
	while (mCharacterStyles.CountItems())
	{
		TTranslatorStyle* stylePtr = static_cast<TTranslatorStyle*>(mCharacterStyles.RemoveItem(0L));
		if (!stylePtr->StyleSheet())
			delete stylePtr;
	}
}

status_t TTranslatorPart_WP::WriteData(TBlockStreamWriter* writer) const
{
	TTranslatorPart::WriteData(writer);

	int32 totalLength = TotalLength();
	int32 padLength = 0;
	int32 start;
	int32 end;
	
	if (totalLength % 4)
		padLength = 4 - (totalLength % 4);
		
	if (TextBlocks())
	{
		writer->WriteBlockHeader(kText_id, B_RAW_TYPE);
		writer->write_int32(totalLength);
		for (int32 x = 0; x < TextBlocks(); x++)
		{
			ssize_t length = 0;
			const char* textPtr = TextBlock(x, &length);
			writer->Write(textPtr, length);
		}
	}

	// Write out pad bytes if needed to 4 byte align the text.
	if (padLength)
	{
		int32 zero = 0;
		writer->Write(&zero, padLength);
	}
	
	writer->WriteInt32( kNumColumns_id, mNumColumns );

	int32 parStyles = ParagraphStyleRuns();
	if (parStyles)
	{
		writer->BeginChunk( kParStyleRuns_id );
		writer->WriteInt32( kArrayCount_id, parStyles );
		for (int32 x = 0; x < parStyles; x++)
		{
			const TTranslatorStyle*	stylePtr = ParagraphStylePtr(x, &start, &end);
			writer->BeginChunk( kStyleRun_id );
			writer->WriteInt32( kStyleRunStart_id, start );
			writer->WriteInt32( kStyleRunEnd_id, end );
			stylePtr->Write( writer ); 
			writer->EndChunk( kStyleRun_id );
		}
		writer->EndChunk( kParStyleRuns_id );
	}
	
	int32 charStyles = CharacterStyleRuns();
	if (charStyles)
	{
		writer->BeginChunk( kCharStyleRuns_id );
		writer->WriteInt32( kArrayCount_id, charStyles );
		for (int32 x = 0; x < charStyles; x++)
		{
			const TTranslatorStyle*	stylePtr = CharacterStylePtr(x, &start, &end);
			writer->BeginChunk( kStyleRun_id );
			writer->WriteInt32( kStyleRunStart_id, start );
			writer->WriteInt32( kStyleRunEnd_id, end );
			stylePtr->Write( writer ); 
			writer->EndChunk( kStyleRun_id );
		}
		writer->EndChunk( kCharStyleRuns_id );
	}
	
	int32 sCount = SpecialCharacters();
	for (int32 x = 0; x < sCount; x++)
		SpecialCharacter(x)->Write(writer, mDocPtr);
			
	return writer->Error();
}

bool TTranslatorPart_WP::ReadBlock(TBlockStreamReader* reader, int32 id)
{
	mReader = reader;

	switch( id )
	{
		case kText_id:
		{
			ssize_t length = reader->BlockDataSize();
			char* textPtr = new char[length+1];
			reader->ReadBytes(textPtr, length);
			textPtr[length] = 0;
			mTextBlocks.AddItem( textPtr );
			mTextBlockSizes.AddItem( (void*) length );
			return true;
		}
			
		case kParStyleRuns_id:
		case kCharStyleRuns_id:
		{
			bool isPar = (id == kParStyleRuns_id);
			while (reader->NextBlock(&id))
			{
				if (id == kStyleRun_id)
				{
					int32 start, end;
					TTranslatorStyle* stylePtr = NULL;
					status_t err = ReadStyleRun(&start, &end, &stylePtr);
					if (!err)
					{
						if (isPar)
							AddParagraphStyleRun(start, end, stylePtr);
						else
							AddCharacterStyleRun(start, end, stylePtr);
					}
				}
				else
					reader->SkipBlock();
			}
			return true;
		}
			
		case kSpecialChar_id:
			ReadSpecialCharacter();
			return true;
			
		case kNumColumns_id:
			reader->ReadInt32(&mNumColumns);
			return true;			
	}
	return TTranslatorPart::ReadBlock(reader, id);
}

TSpecialCharacter* TTranslatorPart_WP::CreateSpecialCharacter(int32 kind) const
{
	switch (kind)
	{
		case kSCHorizontalRule:
			return new TSpecialCharacter_Rule();
		case kSCFrame:
			return new TSpecialCharacter_Frame();
		case kSCFormula:
			return new TSpecialCharacter_Formula();
		case kSCBreak:
			return new TSpecialCharacter_Break();
		case kSCPageNumber:
			return new TSpecialCharacter_PageNumber();
		case kSCFootnote:
			return new TSpecialCharacter_Footnote();
	}
	return new TSpecialCharacter();
}

status_t TTranslatorPart_WP::ReadSpecialCharacter(void)
{
	block_id id;
	while (mReader->NextBlock(&id))
	{
		switch (id)
		{
			// Kind is defined as being first and required block in special character chunk
			case kKind_id:
			{
				int32 kind;
				if (B_NO_ERROR == mReader->ReadInt32(&kind))
				{
					TSpecialCharacter* scPtr = CreateSpecialCharacter(kind);
					if (scPtr)
					{
						// This will read the rest of the chunk so it returns.
						scPtr->ReadData(mReader, mDocPtr);
						mSpecialChars.AddItem( scPtr );
						return mReader->Error();
					}
				}
				break;
			}
						
			default:
				mReader->SkipBlock();
				break;		
		}		
	}
	return mReader->Error();
}

status_t TTranslatorPart_WP::ReadStyleRun(int32* start, int32* end, TTranslatorStyle** stylePtr)
{
	status_t err = mReader->Error();
	TTranslatorStylesTable* stylesTable = mDocPtr->StylesTable();
	
	*start = 0;
	*end = 0;
	*stylePtr = NULL;

	block_id	id;
	while (mReader->NextBlock(&id))
	{
		switch (id)
		{
			case kStyleRunStart_id:
				err = mReader->ReadInt32(start);
				break;
			case kStyleRunEnd_id:
				err = mReader->ReadInt32(end);
				break;
			case kStyleIndex_id:
			{
				int32 styleIndex;
				mReader->ReadInt32(&styleIndex);
				if (stylesTable)
					*stylePtr = (*stylesTable)[styleIndex];
				break;
			}
			case kStyle_id:
			{
				TTranslatorStyle* sPtr = new TTranslatorStyle();
				err = sPtr->Read(mReader);
				*stylePtr = sPtr;
				break;
			}
		}
	}
	return err ? err : mReader->Error();	
}

int32 TTranslatorPart_WP::PartKind(void) const
{
	return WORDPROCESSING_MINOR_KIND;
}

int32 TTranslatorPart_WP::TextBlocks(void) const
{
	return mTextBlocks.CountItems();
}

const char*	TTranslatorPart_WP::TextBlock(int32 index, ssize_t* textLength) const
{
	if (index < 0 || index >= mTextBlocks.CountItems())
		return NULL;
	if (textLength)
		*textLength = (ssize_t) mTextBlockSizes.ItemAt(index);
	return reinterpret_cast<const char*>(mTextBlocks.ItemAt(index));
}

void TTranslatorPart_WP::AddText(const char* textPtr, ssize_t textLength)
{
	char* ptr = new char[textLength];
	if (ptr)
	{
//		IFDEBUG(fprintf(stderr, "AddText[%d,%d]: %s\n", mTotalLength, mTotalLength+textLength, textPtr));
		ASSERT(UTF8IsLegal((const uchar*)textPtr, textLength), "AddText: Invalid UTF8!!");

		for (int32 x = 0; x < textLength; x++)
		{
			uchar c = (uchar) textPtr[x];
			bool goodChar = (c >= ' ' || c == 0x09 || c == 0x0a || c == X_LINEBREAK || c == X_PAGEBREAK);
			ASSERT(goodChar, "AddText: Invalid Text!!");
			ptr[x] = goodChar ? c : '?';
		}

//		memcpy(ptr, textPtr, textLength);
		mTextBlocks.AddItem( ptr );
		mTextBlockSizes.AddItem( (void*) textLength );
		mTotalLength += textLength;		
	}
}

ssize_t TTranslatorPart_WP::TotalLength(void) const
{
	return mTotalLength;
}

int32 TTranslatorPart_WP::ParagraphStyleRuns(void) const
{
	return mParagraphStyles.CountItems();
}

int32 TTranslatorPart_WP::CharacterStyleRuns(void) const
{
	return mCharacterStyles.CountItems();
}

const TTranslatorStyle*	TTranslatorPart_WP::ParagraphStylePtr(int32 index, int32* start, int32* end) const
{
	if (index < 0 || index >= ParagraphStyleRuns())
		return NULL;
	if (start)
		*start = (int32) mParagraphStarts.ItemAt( index );
	if (end)
		*end = (int32) mParagraphEnds.ItemAt( index );
	return reinterpret_cast<const TTranslatorStyle*>( mParagraphStyles.ItemAt( index ) );
}

const TTranslatorStyle*	TTranslatorPart_WP::CharacterStylePtr(int32 index, int32* start, int32* end) const
{
	if (index < 0 || index >= CharacterStyleRuns())
		return NULL;
	if (start)
		*start = (int32) mCharacterStarts.ItemAt( index );
	if (end)
		*end = (int32) mCharacterEnds.ItemAt( index );
	return reinterpret_cast<const TTranslatorStyle*>( mCharacterStyles.ItemAt( index ) );
}

void TTranslatorPart_WP::AddParagraphStyleRun(int32 start, int32 end, TTranslatorStyle* stylePtr)
{
	IFDEBUG(fprintf(stderr, "AddParagraphStyleRun[%d,%d]\n", start, end));
	int32 styleIndex = mDocPtr->StylesTable()->AddStyle(stylePtr);
	if (styleIndex != -1)
		stylePtr = (*mDocPtr->StylesTable())[styleIndex];
	mParagraphStyles.AddItem( stylePtr );
	mParagraphStarts.AddItem( (void*) start );
	mParagraphEnds.AddItem( (void*) end );
}

void TTranslatorPart_WP::AddCharacterStyleRun(int32 start, int32 end, TTranslatorStyle* stylePtr)
{
	IFDEBUG(fprintf(stderr, "AddCharacterStyleRun[%d,%d]\n", start, end));
	int32 styleIndex = mDocPtr->StylesTable()->AddStyle(stylePtr);
	if (styleIndex != -1)
		stylePtr = (*mDocPtr->StylesTable())[styleIndex];
	mCharacterStyles.AddItem( stylePtr );
	mCharacterStarts.AddItem( (void*) start );
	mCharacterEnds.AddItem( (void*) end );
}

int32 TTranslatorPart_WP::SpecialCharacters(void) const
{
	return mSpecialChars.CountItems();
}

const TSpecialCharacter* TTranslatorPart_WP::SpecialCharacter(int32 index) const
{
	if (index >= 0 || index < mSpecialChars.CountItems())
	{
		TSpecialCharacter* charPtr = static_cast<TSpecialCharacter*>(mSpecialChars.ItemAt(index));
		return charPtr;
	}
	return NULL;
}

void TTranslatorPart_WP::AddSpecialCharacter(TSpecialCharacter* charPtr)
{
	ssize_t length = TotalLength();
	AddText("?", 1);
	charPtr->SetOffset(length);
	mSpecialChars.AddItem( charPtr );
}

int32 TTranslatorPart_WP::NumberOfColumns(void) const
{
	return mNumColumns;
}

void TTranslatorPart_WP::SetNumberOfColumns(int32 numCols)
{
	mNumColumns = numCols;
}

#pragma mark -
//*****************************************************************************************************
//									TSpecialCharacter
//*****************************************************************************************************
TSpecialCharacter::TSpecialCharacter()
{
	mOffset = 0;
	mText = NULL;
}

TSpecialCharacter::~TSpecialCharacter()
{
	delete [] mText;
}

status_t TSpecialCharacter::Write(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const
{
	writer->BeginChunk( kSpecialChar_id );
	writer->WriteInt32( kKind_id, Kind() );
	writer->WriteInt32( kIndex_id, Offset() );
	if (mText)
		writer->WriteString( kText_id, mText );
	
	// type specific data goes in its own chunk after base class data.
	WriteData( writer, docPtr );
	writer->EndChunk( kSpecialChar_id );
	return writer->Error();		
}

status_t TSpecialCharacter::WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const
{
	return B_NO_ERROR;
}

status_t TSpecialCharacter::ReadData(TBlockStreamReader* reader, TTranslatorDoc* docPtr)
{
	status_t err = reader->Error();
	block_id id;
	while (reader->NextBlock(&id))
	{
		if (!ReadBlock(reader, id, docPtr))
			reader->SkipBlock();
	}
	return reader->Error();	
}

bool TSpecialCharacter::ReadBlock(TBlockStreamReader* reader, block_id id, TTranslatorDoc* docPtr)
{
	switch (id)
	{
		case kIndex_id:
			reader->ReadInt32(&mOffset);
			return true;
		case kText_id:
			mText = reader->ReadString();
			return true;
	}
	return false;	
}

const char*	TSpecialCharacter::Text(void) const
{
	return mText;
}

#pragma mark -
//*****************************************************************************************************
//									TSpecialCharacter_Frame
//*****************************************************************************************************
TSpecialCharacter_Frame::TSpecialCharacter_Frame()
{
	mPartPtr = NULL;
	mStylePtr = NULL;
	mBitmapPtr = NULL;
}

TSpecialCharacter_Frame::~TSpecialCharacter_Frame()
{
	delete mPartPtr;
	if (mStylePtr && mStylePtr->Index() == 0xFFFF)
		delete mStylePtr;
	delete mBitmapPtr;
}

status_t TSpecialCharacter_Frame::WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const
{
	status_t result = TSpecialCharacter::WriteData(writer, docPtr);
	if (!result)
	{
		writer->BeginChunk(kFrame_id);
		if (mPartPtr)
		{
			writer->WriteInt32(kKind_id, mPartPtr->PartKind());
			result = mPartPtr->Write(writer, kPart_id);
		}
		if (mStylePtr)
			mStylePtr->Write(writer);
		if (mBitmapPtr)
			writer->WriteBitmap(kBitmap_id, mBitmapPtr);
		writer->EndChunk(kFrame_id);
	}
	return result;
}

bool TSpecialCharacter_Frame::ReadBlock(TBlockStreamReader* reader, block_id id, TTranslatorDoc* docPtr)
{
	if (id == kFrame_id)
	{
		ReadFrame(reader, docPtr);
		return true;
	}
	return TSpecialCharacter::ReadBlock(reader, id, docPtr);
}

status_t TSpecialCharacter_Frame::ReadFrame(TBlockStreamReader* reader, TTranslatorDoc* docPtr)
{
	TTranslatorStylesTable* stylesTable = docPtr->StylesTable();
	int32 partKind = 0;
	block_id id;
	while (reader->NextBlock(&id))
	{
		switch (id)
		{
			case kBitmap_id:
				reader->ReadBitmap(&mBitmapPtr);
				break;

			case kKind_id:
				reader->ReadInt32(&partKind);
				break;
				
			case kPart_id:
			{
				switch (partKind)
				{
					case TABLE_MINOR_KIND:
						mPartPtr = new TTranslatorPart_Table(docPtr);
						break;
					case SPREADSHEET_MINOR_KIND:
						mPartPtr = new TTranslatorPart_SS(docPtr);
						break;
					case WORDPROCESSING_MINOR_KIND:
						mPartPtr = new TTranslatorPart_WP(docPtr);
						break;
					default:
						mPartPtr = NULL;
						break;
				}
				if (mPartPtr)
					mPartPtr->ReadData(reader);
				else
					reader->SkipBlock();
				break;
			}
				
			case kStyleIndex_id:
			{
				int32 styleIndex;
				reader->ReadInt32(&styleIndex);
				if (stylesTable)
					mStylePtr = (*stylesTable)[styleIndex];
				break;
			}

			case kStyle_id:
				mStylePtr = new TTranslatorStyle();
				mStylePtr->Read(reader);
				break;

			default:
				reader->SkipBlock();
				break;
		}
	}
	return reader->Error();
}

void TSpecialCharacter_Frame::SetFramePart(TTranslatorPart* partPtr)
{
	delete mPartPtr;
	mPartPtr = partPtr;
}

void TSpecialCharacter_Frame::SetFrameStyle(TTranslatorStyle* stylePtr)
{
	if (mStylePtr && mStylePtr->Index() == 0xFFFF)
		delete mStylePtr;
	mStylePtr = stylePtr;
}

void TSpecialCharacter_Frame::SetFrameImage(BBitmap* bitmapPtr)
{
	delete mBitmapPtr;
	mBitmapPtr = bitmapPtr;
}

#pragma mark -
//*****************************************************************************************************
//									TSpecialCharacter_Footnote
//*****************************************************************************************************
TSpecialCharacter_Footnote::TSpecialCharacter_Footnote()
{
	mAutoNumber = true;
	mIsEndnote = false;
	mCustomMark = NULL;
	mPartPtr = NULL;
}

TSpecialCharacter_Footnote::~TSpecialCharacter_Footnote()
{
	delete mPartPtr;
	delete [] mCustomMark;
}

status_t TSpecialCharacter_Footnote::WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const
{
	status_t result = TSpecialCharacter::WriteData(writer, docPtr);
	if (!result)
	{
		writer->WriteBool(kAutoNumber_id, mAutoNumber);
		writer->WriteBool(kIsEndnote_id, mIsEndnote);
		if (mCustomMark)
			writer->WriteString(kCustomMark_id, mCustomMark);
		if (mPartPtr)
			mPartPtr->Write(writer, kPart_id);
	}
	return result;
}

bool TSpecialCharacter_Footnote::ReadBlock(TBlockStreamReader* reader, block_id id, TTranslatorDoc* docPtr)
{
	switch (id)
	{
		case kAutoNumber_id:
			reader->ReadBool(&mAutoNumber);
			return true;
		case kIsEndnote_id:
			reader->ReadBool(&mIsEndnote);
			return true;
		case kCustomMark_id:
			mCustomMark = reader->ReadString();
			return true;
		case kPart_id:
			mPartPtr = docPtr->ReadPart(reader);
			break;
	}
	return TSpecialCharacter::ReadBlock(reader, id, docPtr);
}

void TSpecialCharacter_Footnote::SetCustomMark(const char* customMark)
{
	if (mCustomMark)
	{
		delete [] mCustomMark;
		mCustomMark = NULL;
	}
	if (customMark)
	{
		int32 len = strlen(customMark);
		mCustomMark = new char[len+1];
		strcpy(mCustomMark, customMark);
	}
}

void TSpecialCharacter_Footnote::SetContentPart(TTranslatorPart* partPtr)
{
	delete mPartPtr;
	mPartPtr = partPtr;
}

#pragma mark -
//*****************************************************************************************************
//									TSpecialCharacter_Rule
//*****************************************************************************************************
TSpecialCharacter_Rule::TSpecialCharacter_Rule()
{
}

TSpecialCharacter_Rule::~TSpecialCharacter_Rule()
{
}

status_t TSpecialCharacter_Rule::WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const
{
	return TSpecialCharacter::WriteData(writer, docPtr);
}

#pragma mark -
//*****************************************************************************************************
//									TSpecialCharacter_Break
//*****************************************************************************************************
TSpecialCharacter_Break::TSpecialCharacter_Break()
{
	mBreakKind = kPageBreak;
}

TSpecialCharacter_Break::~TSpecialCharacter_Break()
{
}

status_t TSpecialCharacter_Break::WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const
{
	writer->WriteInt32(kValue_id, mBreakKind);
	return TSpecialCharacter::WriteData(writer, docPtr);
}

bool TSpecialCharacter_Break::ReadBlock(TBlockStreamReader* reader, int32 id, TTranslatorDoc* docPtr)
{
	switch (id)
	{
		case kValue_id:
			reader->ReadInt32(&mBreakKind);
			return true;
			
		default:
			return TSpecialCharacter::ReadBlock(reader, id, docPtr);
	}
	return false;
}

void TSpecialCharacter_Break::SetBreakKind(int32 breakKind)
{
	mBreakKind = breakKind;
}

#pragma mark -
//*****************************************************************************************************
//									TSpecialCharacter_PageNumber
//*****************************************************************************************************
TSpecialCharacter_PageNumber::TSpecialCharacter_PageNumber()
{
}

TSpecialCharacter_PageNumber::~TSpecialCharacter_PageNumber()
{
}

status_t TSpecialCharacter_PageNumber::WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const
{
	return TSpecialCharacter::WriteData(writer, docPtr);
}

#pragma mark -
//*****************************************************************************************************
//									TSpecialCharacter_Formula
//*****************************************************************************************************
TSpecialCharacter_Formula::TSpecialCharacter_Formula()
{
	mFormula = NULL;
	mFormulaResult = NULL;
	mFormatString = NULL;
}

TSpecialCharacter_Formula::~TSpecialCharacter_Formula()
{
	delete [] mFormula;
	delete [] mFormulaResult;
	delete [] mFormatString;
}

status_t TSpecialCharacter_Formula::WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const
{
	if (mFormula)
		writer->WriteString(kFormula_id, mFormula);
	if (mFormulaResult)
		writer->WriteString(kFormulaResult_id, mFormulaResult);
	if (mFormatString)
		writer->WriteString(kFormatString_id, mFormatString);
	return TSpecialCharacter::WriteData(writer, docPtr);
}

bool TSpecialCharacter_Formula::ReadBlock(TBlockStreamReader* reader, int32 id, TTranslatorDoc* docPtr)
{
	switch (id)
	{
		case kFormula_id:
			mFormula = reader->ReadString();
			return true;
		case kFormulaResult_id:
			mFormulaResult = reader->ReadString();
			return true;
		case kFormatString_id:
			mFormatString = reader->ReadString();
			return true;			
		default:
			return TSpecialCharacter::ReadBlock(reader, id, docPtr);
	}
	return false;
}

#pragma mark -
//*****************************************************************************************************
//									TTextIterator_WP
//*****************************************************************************************************
TTextIterator_WP::TTextIterator_WP(const TTranslatorPart_WP* target)
{
	mPartPtr = target;
	mTextBlockIndex = 0;
	mTextBlockOffset = 0;
	mTextOffset = 0;
	mParStyleIndex = 0;
	mCharStyleIndex = 0;
	mSpecialCharIndex = 0;
	mLastTextOffset = 0;
	mIsParStart = true;
	mIsParEnd = true;
}

TTextIterator_WP::~TTextIterator_WP()
{
}

int32 TTextIterator_WP::NextTextRun(uchar* buffer, int32 maxSize)
{
	ssize_t textLength;
	int32 	count = 0;
	int32 	startOffset = mTextOffset;

	const char* textPtr = mPartPtr->TextBlock(mTextBlockIndex, &textLength);
	if (!textLength)
		textPtr = NULL;
		
	mLastTextOffset = mTextOffset;
	mIsParStart = mIsParEnd;

	// Check to see if we are now on a new paragraph style.
	int32 nextParStyleStart;
	const TTranslatorStyle* nextParStylePtr = mPartPtr->ParagraphStylePtr(mParStyleIndex+1, &nextParStyleStart, NULL);
	if (nextParStylePtr && nextParStyleStart <= mTextOffset)
		mParStyleIndex++;

	// Check to see if we are now on a new character style.
	int32 nextCharStyleStart, nextCharStyleEnd;
	const TTranslatorStyle* nextCharStylePtr = mPartPtr->CharacterStylePtr(mCharStyleIndex+1, &nextCharStyleStart, &nextCharStyleEnd);	
	while (nextCharStylePtr && nextCharStyleStart <= mTextOffset)
	{
		mCharStyleIndex++;
		nextCharStylePtr = mPartPtr->CharacterStylePtr(mCharStyleIndex+1, &nextCharStyleStart, &nextCharStyleEnd);
	}

	// Check to see if we are now on a new special character.
	const TSpecialCharacter* sPtr = mPartPtr->SpecialCharacter(mSpecialCharIndex);
	while (sPtr && sPtr->Offset() < mTextOffset)
	{
		mSpecialCharIndex++;
		sPtr = mPartPtr->SpecialCharacter(mSpecialCharIndex);
	}
		
	// Fill the buffer until CR, SpecialChar, style change, end of text, or buffer full.
	sPtr = mPartPtr->SpecialCharacter(mSpecialCharIndex);
	while (textPtr && count < maxSize)
	{
		// Get current character
		char 	theChar = *(textPtr + mTextBlockOffset);
		bool 	isCR = (theChar == 0x0A);
		bool 	isSC = (sPtr && sPtr->Offset() == mTextOffset);

		if (IS_CHAR_START_UTF8(theChar) && count + 4 >= maxSize)
			break;

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
		
		// End the run if carriage return marking end of paragraph or we found special character.
		mIsParEnd = isCR;
		mIsSpecialCharacter = isSC;
		
		if (isCR || isSC)
			break;
		// End the run if we've reached end of character style run.
		if (nextCharStylePtr && mTextOffset == nextCharStyleStart)
			break;
		// End the run if next character is a special character.
		if (sPtr && sPtr->Offset() == mTextOffset)
			break;
	}
	
	if (textPtr == 0)
		mIsParEnd = true;
	
	return count;	
}

const TTranslatorStyle*	TTextIterator_WP::ParagraphStylePtr(void) const
{
	return mPartPtr->ParagraphStylePtr(mParStyleIndex, NULL, NULL);
}

const TTranslatorStyle*	TTextIterator_WP::CharacterStylePtr(void) const
{
	return mPartPtr->CharacterStylePtr(mCharStyleIndex, NULL, NULL);
}

const TSpecialCharacter* TTextIterator_WP::SpecialCharacter(void) const
{
	const TSpecialCharacter* scPtr = mPartPtr->SpecialCharacter(mSpecialCharIndex);
	if (scPtr && RunStart()== scPtr->Offset())
		return scPtr;
	return NULL;
}

int32 TTextIterator_WP::RunStart(void) const
{
	return mLastTextOffset;
}

bool TTextIterator_WP::IsParStart(void) const
{
	return mIsParStart;
}

bool TTextIterator_WP::IsParEnd(void) const
{
	return mIsParEnd;
}

bool TTextIterator_WP::IsSpecialCharacter(void) const
{
	return mIsSpecialCharacter;
}

// $Header: /usr/local/cvsroot/8ball/Datatypes/TranslatorWP.cpp,v 1.4 2000/02/21 21:05:45 tom Exp $
