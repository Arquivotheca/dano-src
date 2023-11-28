//*****************************************************************************************************
// TranslatorSS.cpp
//
//	This document contains the implementation of classes for use when reading or writing spreadsheet
// parts from a translator document.
//*****************************************************************************************************

#include <string.h>
#include <stdio.h>
#include <Message.h>

#include "TranslatorStyle.h"
#include "TranslatorSS.h"
#include "TranslatorWP.h"

#define SHOW_ROW_PROGRESS_INC	250

// Used to store row and column styles.
class TRowColInfo
{
	public:
		TRowColInfo(int32 index, float width, TTranslatorStyle* stylePtr)
		{
			mIndex = index;
			mWidth = width;
			mStyle = stylePtr;
		}		
		~TRowColInfo()
		{
			if (mStyle && mStyle->Index() == 0xFFFF)
				delete mStyle;			
		}
		
		int32				mIndex;
		float				mWidth;
		TTranslatorStyle*	mStyle;
};

//*****************************************************************************************************
//									TRowColumnWidthsTable
//*****************************************************************************************************
TRowColumnInfoTable::TRowColumnInfoTable()
{
	mDefaultWidth = 72;
	mDefaultStylePtr = NULL;
}

TRowColumnInfoTable::~TRowColumnInfoTable()
{
	if (mDefaultStylePtr && mDefaultStylePtr->Index() == 0xFFFF)
		delete mDefaultStylePtr;
		
	while (CountItems())
		delete static_cast<TRowColInfo*>(RemoveItem(0L));
}

status_t TRowColumnInfoTable::Read(TBlockStreamReader* reader, TTranslatorDoc* docPtr)
{
	TTranslatorStylesTable* stylesTable = docPtr ? docPtr->StylesTable() : NULL;
	int32					index = -1;
	float					width = 72.0;
	TTranslatorStyle*		stylePtr = NULL;
	block_id 				id;
	status_t				err = reader->Error();
	
	while (reader->NextBlock(&id))
	{
		switch (id)
		{
			case kIndex_id:
				reader->ReadInt32(&index);
				break;
				
			case kValue_id:
				reader->ReadFloat(&width);
				break;

			case kStyleIndex_id:
			{
				int32 styleIndex;
				if (!reader->ReadInt32(&styleIndex) && stylesTable)
					stylePtr = (*stylesTable)[styleIndex];
				break;
			}

			case kStyle_id:
				stylePtr = new TTranslatorStyle();
				stylePtr->Read(reader);
				break;

			default:
				reader->SkipBlock();
				break;
		}
	}
	
	if (index < 0)
	{
		SetDefaultWidth(width);
		SetDefaultStyle(stylePtr);
	}
	else
		AddInfo( index, width, stylePtr );
	return reader->Error();
}

status_t TRowColumnInfoTable::Write(TBlockStreamWriter* writer, TTranslatorDoc* docPtr, int32 chunkID) const
{
	// Write default row or column info first.
	writer->BeginChunk( chunkID );
	writer->WriteInt32( kIndex_id, -1);
	writer->WriteFloat( kValue_id, mDefaultWidth );
	if (mDefaultStylePtr)
		mDefaultStylePtr->Write(writer);
	writer->EndChunk( chunkID );
	
	// Write one row/column info chunk for each entry in the table.
	int32 count = CountItems();
	for (int32 x = 0; x < count; x++)
	{
		int32 index = -1;
		float width = mDefaultWidth;
		const TTranslatorStyle* stylePtr = GetItem(x, &index, &width);

		if (x && !(x % 1000))
			docPtr->SendProgress(NULL, (float)(x+1)/count);
			
		if (index >= 0)
		{
			writer->BeginChunk( chunkID );
			writer->WriteInt32( kIndex_id, index );
			writer->WriteFloat( kValue_id, width );
			if (stylePtr)
				stylePtr->Write(writer);
			writer->EndChunk( chunkID );
		}
	}
	return writer->Error();
}

float TRowColumnInfoTable::DefaultWidth(void) const
{
	return mDefaultWidth;
}

const TTranslatorStyle* TRowColumnInfoTable::DefaultStyle(void) const
{
	return mDefaultStylePtr;
}

void TRowColumnInfoTable::SetDefaultWidth(float width)
{
	mDefaultWidth = width;
}

void TRowColumnInfoTable::SetDefaultStyle(TTranslatorStyle* stylePtr)
{
	if (mDefaultStylePtr && mDefaultStylePtr->Index() == 0xFFFF)
		delete mDefaultStylePtr;
	mDefaultStylePtr = stylePtr;
}

void TRowColumnInfoTable::SetWidth(int32 index, float width)
{
	// Don't bother if same as default.
	if (width == mDefaultWidth)
		return;

	// See if we already have a width for this row/column if so change it.
	for (int32 x = 0; x < CountItems(); x++)
	{
		TRowColInfo* ptr = static_cast<TRowColInfo*>(ItemAt(x));
		if (ptr->mIndex == index)
		{
			ptr->mWidth = width;
			return;
		}
	}
	AddInfo( index, width, NULL );
}

void TRowColumnInfoTable::SetStyle(int32 index, TTranslatorStyle* stylePtr)
{
	// Don't bother if same as default.
	if (stylePtr == mDefaultStylePtr)
		return;

	// See if we already have a width for this row/column if so change it.
	for (int32 x = 0; x < CountItems(); x++)
	{
		TRowColInfo* itemPtr = static_cast<TRowColInfo*>(ItemAt(x));
		if (itemPtr->mIndex == index)
		{
			if (itemPtr->mStyle && itemPtr->mStyle->Index() == 0xFFFF)
				delete itemPtr->mStyle;
			itemPtr->mStyle = stylePtr;
			return;
		}
	}
	AddInfo( index, mDefaultWidth, stylePtr );
}

void TRowColumnInfoTable::SetInfo(int32 index, float width, TTranslatorStyle* stylePtr)
{
	// See if we already have a width for this row/column if so change it.
	for (int32 x = 0; x < CountItems(); x++)
	{
		TRowColInfo* ptr = static_cast<TRowColInfo*>(ItemAt(x));
		if (ptr->mIndex == index)
		{
			ptr->mWidth = width;
			if (ptr->mStyle && ptr->mStyle->Index() == 0xFFFF)
				delete ptr->mStyle;
			ptr->mStyle = stylePtr;
			return;
		}
	}
	AddInfo( index, width, stylePtr );
}

void TRowColumnInfoTable::AddInfo(int32 index, float width, TTranslatorStyle* stylePtr)
{
	AddItem( new TRowColInfo(index, width, stylePtr) );
}

float TRowColumnInfoTable::GetWidth(int32 index) const
{
	for (int32 x = 0; x < CountItems(); x++)
	{
		TRowColInfo* ptr = static_cast<TRowColInfo*>(ItemAt(x));
		if (ptr->mIndex == index)
			return ptr->mWidth;
	}
	return mDefaultWidth;
}

const TTranslatorStyle* TRowColumnInfoTable::GetStyle(int32 index) const
{
	for (int32 x = 0; x < CountItems(); x++)
	{
		TRowColInfo* ptr = static_cast<TRowColInfo*>(ItemAt(x));
		if (ptr->mIndex == index)
			return ptr->mStyle;
	}
//	return mDefaultStylePtr;
	return NULL;
}

const TTranslatorStyle* TRowColumnInfoTable::GetItem(int32 itemIndex, int32* index, float* width) const
{
	TRowColInfo* ptr = static_cast<TRowColInfo*>(ItemAt(itemIndex));
	if (index)
		*index = ptr->mIndex;
	if (width)
		*width = ptr->mWidth;
	return ptr->mStyle;
}

#pragma mark -
//*****************************************************************************************************
//									TTranslatorPart_SS
//*****************************************************************************************************
TTranslatorPart_SS::TTranslatorPart_SS(TTranslatorDoc* docPtr)
	: TTranslatorPart(docPtr)
{
	mStartRow = mStartColumn = mEndRow = mEndColumn = 0;

	mShowGrid = true;
	mShowLockedCells = true;
	mShowFormulas = false;
	mShowColumnHeader = true;
	mShowRowHeader = true;
	mShowCircularRefs = true;
	mShowZeros = true;
	
	mReader = NULL;
	mDefaultStylePtr = NULL;
	
	mColumnInfo.SetDefaultWidth(72.0);
	mRowInfo.SetDefaultWidth(16.0);
}

TTranslatorPart_SS::~TTranslatorPart_SS()
{
	while (mCells.CountItems())
		delete static_cast<TTranslatorCell_SS*>(mCells.RemoveItem(0L));
	if (mDefaultStylePtr && mDefaultStylePtr->Index() == 0xFFFF)
		delete mDefaultStylePtr;
}

status_t TTranslatorPart_SS::WriteData(TBlockStreamWriter* writer) const
{
	status_t err = TTranslatorPart::WriteData(writer);

	// Write out default style if we have one.
	if (mDefaultStylePtr)
		mDefaultStylePtr->Write( writer );

	// Write column info
	mDocPtr->BeginTask(" (column formatting)", 0.0, 0.1);
	mColumnInfo.Write( writer, mDocPtr, kColumnInfo_id );
	mDocPtr->EndTask();

	// Write row info
	mDocPtr->BeginTask(" (row formatting)", 0.1, 0.3);
	mRowInfo.Write( writer, mDocPtr, kRowInfo_id );
	mDocPtr->EndTask();

	// Write out display options chunks
	writer->BeginChunk( kDisplayOptions_id );
	writer->WriteBool( kShowGrid_id, mShowGrid );
	writer->WriteBool( kShowLockedCells_id, mShowLockedCells );
	writer->WriteBool( kShowFormulas_id, mShowFormulas );
	writer->WriteBool( kShowColumnHeaders_id, mShowColumnHeader );
	writer->WriteBool( kShowRowHeaders_id, mShowRowHeader );
	writer->WriteBool( kShowCircularRefs_id, mShowCircularRefs );
	writer->WriteBool( kShowZeros_id, mShowZeros );
	writer->EndChunk( kDisplayOptions_id );

	// Write out data chunk
	writer->BeginChunk( kSpreadsheetData_id );
	writer->WriteInt32( kStartRow_id, mStartRow );
	writer->WriteInt32( kStartColumn_id, mStartColumn );
	writer->WriteInt32( kEndRow_id, mEndRow );
	writer->WriteInt32( kEndColumn_id, mEndColumn);

	mDocPtr->BeginTask("", 0.3, 1.0);
	uint32 lastRow = 0xFFFFFFFF;
	int32 cellCount = mCells.CountItems();
	for (long x = 0; !err && x < cellCount; x++)
	{
		const TTranslatorCell_SS* cellPtr = CellPtr(x);
		uint32 row = (uint32) cellPtr->Row();
		if (lastRow != row)
		{
			if (lastRow != 0xFFFFFFFF)
				writer->EndChunk( kRowData_id );

			if (row && !(row % SHOW_ROW_PROGRESS_INC))
			{
				char buffer[128];
				const char* taskName = mDocPtr->CurrentTaskName();
				sprintf(buffer, "%s (row: %u) ", taskName, (ushort) row);
				mDocPtr->SendProgress(buffer, (float)(x+1)/cellCount);
			}

			writer->BeginChunk( kRowData_id );
			writer->WriteInt32( kCellRow_id, row );
			lastRow = row;
		}
		err = cellPtr->Write( writer );
	}
	mDocPtr->EndTask();
	
	if (lastRow != 0xFFFFFFFF)
		writer->EndChunk( kRowData_id );
	writer->EndChunk( kSpreadsheetData_id );
	return err ? err : writer->Error();
}

bool TTranslatorPart_SS::ReadBlock(TBlockStreamReader* reader, int32 id)
{
	mReader = reader;
	switch (id)
	{
		case kStartRow_id:
			reader->ReadInt32(&mStartRow);
			return true;
		case kStartColumn_id:
			reader->ReadInt32(&mStartColumn);
			return true;
		case kEndRow_id:
			reader->ReadInt32(&mEndRow);
			return true;
		case kEndColumn_id:
			reader->ReadInt32(&mEndColumn);
			return true;
		case kColumnInfo_id:
			mColumnInfo.Read(reader, mDocPtr);
			return true;
		case kRowInfo_id:
			mRowInfo.Read(reader, mDocPtr);
			return true;
	
		
		case kStyleIndex_id:
		{
			TTranslatorStylesTable* stylesTable = mDocPtr->StylesTable();
			int32 styleIndex;
			status_t err = reader->ReadInt32(&styleIndex);
			if (!err && stylesTable)
				mDefaultStylePtr = (*stylesTable)[styleIndex];
			return true;
		}
	
		case kStyle_id:
			mDefaultStylePtr = new TTranslatorStyle();
			mDefaultStylePtr->Read(reader);
			return true;
			
		case kSpreadsheetData_id:
			ReadSpreadsheetData(reader);
			return true;
			
		case kDisplayOptions_id:
			ReadDisplayOptions(reader);
			return true;
	}
	return TTranslatorPart::ReadBlock(reader, id);
}

#pragma mark -
status_t TTranslatorPart_SS::ReadDisplayOptions(TBlockStreamReader* reader)
{
	status_t err = reader->Error();
	block_id id;
	while (mReader->NextBlock(&id))
	{
		switch (id)
		{
			case kShowGrid_id:			reader->ReadBool(&mShowGrid);			break;
			case kShowLockedCells_id:	reader->ReadBool(&mShowLockedCells);	break;
			case kShowFormulas_id:		reader->ReadBool(&mShowFormulas);		break;
			case kShowColumnHeaders_id:	reader->ReadBool(&mShowColumnHeader);	break;
			case kShowRowHeaders_id:	reader->ReadBool(&mShowRowHeader);		break;
			case kShowCircularRefs_id:	reader->ReadBool(&mShowCircularRefs);	break;
			case kShowZeros_id:			reader->ReadBool(&mShowZeros);			break;
				break;
			default:
				reader->SkipBlock();
				break;
		}
	}
	return err ? err : reader->Error();
}

status_t TTranslatorPart_SS::ReadSpreadsheetData(TBlockStreamReader* reader)
{
	mReader = reader;
	status_t err = mReader->Error();
	block_id id;
	while (mReader->NextBlock(&id))
	{
		switch (id)
		{
			case kRowData_id:
				err = ReadRowData();
				break;

			default:
				mReader->SkipBlock();
				break;
		}
	}
	return err ? err : mReader->Error();
}

status_t TTranslatorPart_SS::ReadRowData()
{
	status_t 	err = mReader->Error();
	int32 		row = -1;
	block_id 	id;
	
	while (mReader->NextBlock(&id))
	{
		switch (id)
		{
			case kCellRow_id:
				mReader->ReadInt32( &row );
				break;
				
			case kCellData_id:
				if (row == -1)
					err = B_ERROR;
				else
					err = ReadCellData(row);
				break;

			default:
				mReader->SkipBlock();
				break;
		}
	}
	return err ? err : mReader->Error();
}

status_t TTranslatorPart_SS::ReadCellData(int32 row)
{
	TTranslatorCell_SS* cellPtr = new TTranslatorCell_SS();
	status_t err = cellPtr->Read(mReader, mDocPtr, row);
	AddCell( cellPtr );	
	return err ? err : mReader->Error();
}

int32 TTranslatorPart_SS::PartKind(void) const
{
	return SPREADSHEET_MINOR_KIND;
}

int32 TTranslatorPart_SS::Cells(void) const
{
	return mCells.CountItems();
}

const TTranslatorCell_SS* TTranslatorPart_SS::CellPtr(int32 cellIndex) const
{
	if (cellIndex < 0 || cellIndex >= mCells.CountItems())
		return NULL;
	return static_cast<TTranslatorCell_SS*>(mCells.ItemAt( cellIndex ));
}

void TTranslatorPart_SS::AddCell( TTranslatorCell_SS* cellPtr )
{
	int32 row = cellPtr->Row();
	int32 col = cellPtr->Column();
	if (!mCells.CountItems())
	{
		if (!mEndRow && !mEndColumn)
		{
			mStartRow = mEndRow = row;
			mStartColumn = mEndColumn = col;
		}
	}
	else
	{
		if (mStartRow > row)
			mStartRow = row;
		if (mEndRow < row)
			mEndRow = row;
		if (mStartColumn > col)
			mStartColumn = col;
		if (mEndColumn < col)
			mEndColumn = col;
	}
	mCells.AddItem( cellPtr );
}

int32 TTranslatorPart_SS::Rows(void) const
{
	return mEndRow - mStartRow + 1;
}

int32 TTranslatorPart_SS::Columns(void) const
{
	return mEndColumn - mStartColumn + 1;
}

void TTranslatorPart_SS::GetDataRange(int32* startRow, int32* startCol, int32* endRow, int32* endCol) const
{
	*startRow = mStartRow;
	*startCol = mStartColumn;
	*endRow = mEndRow;
	*endCol = mEndColumn;
}

const TTranslatorStyle* TTranslatorPart_SS::DefaultStyle(void) const
{
	return mDefaultStylePtr;
}

void TTranslatorPart_SS::SetDefaultStyle(TTranslatorStyle* stylePtr)
{
	mDefaultStylePtr = stylePtr;
	if (stylePtr && stylePtr->Index() == 0xFFFF)
		mDocPtr->StylesTable()->AddStyle( stylePtr );
}

#pragma mark -
//*****************************************************************************************************
//									TTranslatorCell_SS
//*****************************************************************************************************
TTranslatorCell_SS::TTranslatorCell_SS()
{
	mValuePtr = NULL;
	mFormulaPtr = NULL;
	mStylePtr = NULL;
	mKind = 0;
	mValue = 0.0;
	mHasValue = false;
	mResultPtr = NULL;
	mTransData = NULL;
}

TTranslatorCell_SS::~TTranslatorCell_SS()
{
	delete [] mValuePtr;
	delete [] mFormulaPtr;
	if (mStylePtr && mStylePtr->Index() == 0xFFFF)
		delete mStylePtr;
	delete mResultPtr;
	delete mTransData;
}

status_t TTranslatorCell_SS::Write(TBlockStreamWriter* writer) const
{
	status_t err = writer->Error();
	writer->BeginChunk( kCellData_id );
	writer->WriteInt32( kCellColumn_id, mColumn );
	if (mValuePtr)
		writer->WriteString( kCellText_id, mValuePtr );
	if (mFormulaPtr)
		writer->WriteString( kCellFormula_id, mFormulaPtr );
	if (mHasValue)
		writer->WriteDouble( kValue_id, mValue );
	if (mStylePtr)
		err = mStylePtr->Write( writer );
	if (mResultPtr)
	{
		writer->BeginChunk( kCellResult_id );
		mResultPtr->Write(writer);
		writer->EndChunk( kCellResult_id );
	}
	writer->EndChunk( kCellData_id );
	return err ? err : writer->Error();
}

status_t TTranslatorCell_SS::Read(TBlockStreamReader* reader, TTranslatorDoc* docPtr, int32 row)
{
	TTranslatorStylesTable* stylesTable = docPtr ? docPtr->StylesTable() : NULL;
	int32		size;
	status_t 	err = reader->Error();
	block_id 	id;	

	mRow = row;

	while (!err && reader->NextBlock(&id))
	{
		switch (id)
		{
			case kKind_id:
				reader->ReadInt32(&mKind);
				break;
			case kCellColumn_id:
				reader->ReadInt32(&mColumn);
				break;
			case kValue_id:
				mHasValue = true;
				switch (reader->BlockKind())
				{
					case B_DOUBLE_TYPE:
						reader->ReadDouble(&mValue);
						break;
					case B_FLOAT_TYPE:
					{
						float tmp;
						reader->ReadFloat(&tmp);
						mValue = tmp;
						break;
					}
					case B_INT32_TYPE:
					{
						int32 tmp;
						reader->ReadInt32(&tmp);
						mValue = tmp;
						break;
					}
					default:
						mHasValue = false;
						break;
				}
				break;

			case kStyleIndex_id:
				int32 styleIndex;
				reader->ReadInt32(&styleIndex);
				if (stylesTable)
					mStylePtr = (*stylesTable)[styleIndex];
				break;

			case kStyle_id:
				mStylePtr = new TTranslatorStyle();
				err = mStylePtr->Read(reader);
				break;

			case kCellText_id:
				size = reader->BlockDataSize();
				mValuePtr = new char[size+1];
				if (mValuePtr)
				{
					err = reader->ReadString(mValuePtr, size);
					mValuePtr[size] = 0;
				}
				break;
			case kCellFormula_id:
				size = reader->BlockDataSize();
				mFormulaPtr = new char[size+1];
				if (mFormulaPtr)
				{
					err = reader->ReadString(mFormulaPtr, size);
					mFormulaPtr[size] = 0;
				}
				break;
			case kCellResult_id:
				mResultPtr = new TTranslatorCell_SS();
				err = mResultPtr->Read(reader, docPtr, row);
				break;
							
			default:
				reader->SkipBlock();
				break;
		}
	}
	return err ? err : reader->Error();	
}

void TTranslatorCell_SS::SetCellID(int32 row, int32 column)
{
	mRow = row; mColumn = column;
}

int32 TTranslatorCell_SS::Row(void) const
{
	return mRow;
}

int32 TTranslatorCell_SS::Column(void) const
{
	return mColumn;
}

const TTranslatorStyle* TTranslatorCell_SS::StylePtr(void) const
{
	return mStylePtr;
}

int32 TTranslatorCell_SS::Kind(void) const
{
	return mKind;
}

const char*	TTranslatorCell_SS::ValueText(void) const
{
	return mValuePtr;
}

const char*	TTranslatorCell_SS::FormulaText(void) const
{
	return mFormulaPtr;
}

void TTranslatorCell_SS::SetKind(int32 kind)
{
	mKind = kind;
}

void TTranslatorCell_SS::SetStylePtr( TTranslatorStyle* stylePtr )
{
	mStylePtr = stylePtr;
}

void TTranslatorCell_SS::SetValueText(const char* textPtr)
{
	if (textPtr)
	{
		int32 len = strlen(textPtr);
		mValuePtr = new char[ len + 1];
		strcpy( mValuePtr, textPtr );
	}
}

void TTranslatorCell_SS::SetFormulaText(const char* formulaPtr)
{
	if (formulaPtr)
	{
		int32 len = strlen(formulaPtr);
		mFormulaPtr = new char[ len + 1];
		strcpy( mFormulaPtr, formulaPtr );
	}
}

void TTranslatorCell_SS::SetResult(TTranslatorCell_SS* resultPtr)
{
	mResultPtr = resultPtr;
}

bool TTranslatorCell_SS::GetValue(double* valuePtr) const
{
	// Get value for formula cells from result cell if present.
	if (mResultPtr)
		return mResultPtr->GetValue(valuePtr);
	if (!valuePtr || !mHasValue)
		return false;
	*valuePtr = mValue;
	return true;
}

void TTranslatorCell_SS::SetValue(double number)
{
	mHasValue = true;
	mValue = number;
}

const TTranslatorCell_SS* TTranslatorCell_SS::Result(void) const
{
	return mResultPtr;
}

BMessage* TTranslatorCell_SS::TranslatorData(bool create) const
{
	if (!mTransData && create)
		mTransData = new BMessage();
	return mTransData;
}

// $Header: /usr/local/cvsroot/8ball/Datatypes/TranslatorSS.cpp,v 1.3 2000/01/27 19:12:24 tom Exp $
