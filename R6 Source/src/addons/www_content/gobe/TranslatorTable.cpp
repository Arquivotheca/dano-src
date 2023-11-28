//*****************************************************************************************************
// TranslatorTable.cpp
//
//	This document contains the implementation of classes for use when reading or writing table parts from
// a translator document.
//*****************************************************************************************************

#include <string.h>
#include <stdio.h>

#include "TranslatorStyle.h"
#include "TranslatorTable.h"
#include "TranslatorWP.h"

//*****************************************************************************************************
//									TTranslatorPart_Table
//*****************************************************************************************************
TTranslatorPart_Table::TTranslatorPart_Table(TTranslatorDoc* docPtr)
	: TTranslatorPart(docPtr)
{
	mReader = NULL;
	mIsPercentWidth = false;
	mHeight = mWidth = 0;
	mStylePtr = NULL;
	mBorderSize = 0;
	mCellSpacing = 2;
	mCellPadding = 1;
	mTableKind = kTableKind_HTML;
}

TTranslatorPart_Table::~TTranslatorPart_Table()
{
	if (mStylePtr && mStylePtr->Index() == 0xFFFF)
		delete mStylePtr;
	while (mRows.CountItems())
	{
		TTableRow* rowPtr = static_cast<TTableRow*>(mRows.RemoveItem(0L));
		delete rowPtr;
	}
}

status_t TTranslatorPart_Table::WriteData(TBlockStreamWriter* writer) const
{
	TTranslatorPart::WriteData(writer);
	
	int32 rowCount = Rows();
	writer->WriteInt32(kTableKind_id, mTableKind);
	writer->WriteInt32(kColumnSpan_id, Columns());
	writer->WriteInt32(kRowSpan_id, Rows());
	writer->WriteInt32(kTableBorderSize_id, mBorderSize);
	writer->WriteInt32(kTableCellSpacing_id, mCellSpacing);
	writer->WriteInt32(kTableCellPadding_id, mCellPadding);
	
	if (mStylePtr)
		mStylePtr->Write( writer );
	if (mWidth)
	{
		block_id id = mIsPercentWidth ? kTableRelWidth_id : kTableWidth_id;
		writer->WriteInt32(id, mWidth);
	}
	if (mHeight)
		writer->WriteInt32(kTableHeight_id, mHeight);

	for (int32 x = 0; x < rowCount; x++)
		RowPtr(x)->Write(writer, mDocPtr, kTableRow_id);

	return writer->Error();
}

bool TTranslatorPart_Table::ReadBlock(TBlockStreamReader* reader, int32 id)
{
	mReader = reader;
	switch( id )
	{
		case kStyleIndex_id:
		{
			TTranslatorStylesTable* stylesTable = mDocPtr->StylesTable();
			int32 styleIndex;
			reader->ReadInt32(&styleIndex);
			if (stylesTable)
				mStylePtr = (*stylesTable)[styleIndex];
			return true;
		}
		
		case kStyle_id:
			mStylePtr = new TTranslatorStyle();
			mStylePtr->Read(reader);
			return true;

		case kTableWidth_id:
		case kTableRelWidth_id:
			mIsPercentWidth = (id == kTableRelWidth_id);
			reader->ReadInt32(&mWidth);
			return true;
		case kTableHeight_id:
			reader->ReadInt32(&mHeight);
			return true;
		case kTableBorderSize_id:
			reader->ReadInt32(&mBorderSize);
			return true;
		case kTableCellSpacing_id:
			reader->ReadInt32(&mCellSpacing);
			return true;
		case kTableCellPadding_id:
			reader->ReadInt32(&mCellPadding);
			return true;
		case kTableKind_id:
			reader->ReadInt32(&mTableKind);
			return true;
			
		case kTableRow_id:
		{
			TTableRow* rowPtr = new TTableRow();
			rowPtr->Read(reader, mDocPtr);
			AddRow(rowPtr);
			return true;
		}
	}
	return TTranslatorPart::ReadBlock(reader, id);
}

int32 TTranslatorPart_Table::PartKind(void) const
{
	return TABLE_MINOR_KIND;
}

int32 TTranslatorPart_Table::Rows(void) const
{
	return mRows.CountItems();
}

TTableRow* TTranslatorPart_Table::RowPtr(int32 index) const
{
	ASSERTC(index >= 0 && index < mRows.CountItems());
	if (index < 0 || index > mRows.CountItems())
		return NULL;
	return static_cast<TTableRow*>(mRows.ItemAt(index));	
}

void TTranslatorPart_Table::AddRow(TTableRow* rowPtr)
{
	mRows.AddItem(rowPtr);
}

int32 TTranslatorPart_Table::Columns(void) const
{
	int32 columns = 0;
	for (int32 x = 0; x < Rows(); x++)
	{
		int32 col = RowPtr(x)->Columns();
		if (col > columns)
			columns = col;
	}
	return columns;
}

int32 TTranslatorPart_Table::Width(bool& percent) const
{
	percent = mIsPercentWidth;
	return mWidth;
}

void TTranslatorPart_Table::SetWidth(int32 width, bool percent)
{
	mIsPercentWidth = percent;
	mWidth = width;
}

void TTranslatorPart_Table::SetHeight(int32 height)
{
	mHeight = height;
}

int32 TTranslatorPart_Table::Height(void) const
{
	return mHeight;
}

int32 TTranslatorPart_Table::Border(void) const
{
	return mBorderSize;
}

void TTranslatorPart_Table::SetBorder(int32 borderSize)
{
	mBorderSize = borderSize;
}

int32 TTranslatorPart_Table::CellPadding(void) const
{
	return mCellPadding;
}

void TTranslatorPart_Table::SetCellPadding(int32 cellPadding)
{
	mCellPadding = cellPadding;
}

int32 TTranslatorPart_Table::CellSpacing(void) const
{
	return mCellSpacing;
}

void TTranslatorPart_Table::SetCellSpacing(int32 cellSpacing)
{
	mCellSpacing = cellSpacing;
}

int32 TTranslatorPart_Table::TableKind(void) const
{
	return mTableKind;
}

void TTranslatorPart_Table::SetTableKind(int32 tableKind)
{
	mTableKind = tableKind;
}

void TTranslatorPart_Table::SetTableStylePtr( TTranslatorStyle* stylePtr )
{
	mStylePtr = stylePtr;
}

const TTranslatorStyle* TTranslatorPart_Table::TableStylePtr(void) const
{
	return mStylePtr;
}

#pragma mark -
//*****************************************************************************************************
//									TTableRow
//*****************************************************************************************************
TTableRow::TTableRow()
{
}

TTableRow::~TTableRow()
{
	while (CountItems())
		delete static_cast<TTableCell*>(RemoveItem(0L));
}

status_t TTableRow::Write(TBlockStreamWriter* writer, TTranslatorDoc* docPtr, int32 id) const
{
	int32 cellCount = Cells();
	writer->BeginChunk(id);
	writer->WriteInt32(kArrayCount_id, cellCount);
	for (int32 x = 0; x < cellCount; x++)
		CellPtr(x)->Write(writer, docPtr, kTableCell_id);
	writer->EndChunk(id);
	return writer->Error();
}

status_t TTableRow::Read(TBlockStreamReader* reader, TTranslatorDoc* docPtr)
{
	status_t err = reader->Error();
	block_id id;
	while (!err && reader->NextBlock(&id))
	{
		switch( id )
		{
			case kTableCell_id:
			{
				TTableCell* cellPtr = new TTableCell();
				cellPtr->Read(reader, docPtr);
				AddCell(cellPtr);
				break;
			}
				
			default:
				err = reader->SkipBlock();
				break;
		}
	}
	return err ? err : reader->Error();
}

int32 TTableRow::Cells(void) const
{
	return CountItems();
}

TTableCell* TTableRow::CellPtr(int32 index) const
{
	ASSERTC(index >= 0 && index < CountItems());
	if (index < 0 || index > CountItems())
		return NULL;
	return static_cast<TTableCell*>(ItemAt(index));	
}

void TTableRow::AddCell(TTableCell* cellPtr)
{
	AddItem(cellPtr);
}

int32 TTableRow::Columns(void) const
{
	int32 columns = 0;
	for (int32 x = 0; x < CountItems(); x++)
		columns += CellPtr(x)->ColumnSpan();
	return columns;
}

#pragma mark -
//*****************************************************************************************************
//									TTableCell
//*****************************************************************************************************
TTableCell::TTableCell()
{
	mWPPtr = NULL;
	mRowSpan = mColumnSpan = 1;
	mHeight = mWidth = 0;
	mIsPercentHeight = mIsPercentWidth = false;
	mStylePtr = NULL;
	mHasCellPos = false;
	mCellPos.Set(0.0, 0.0);
}

TTableCell::~TTableCell()
{
	if (mStylePtr && mStylePtr->Index() == 0xFFFF)
		delete mStylePtr;
	delete mWPPtr;
}

status_t TTableCell::Read(TBlockStreamReader* reader, TTranslatorDoc* docPtr)
{
	int32 partKind = WORDPROCESSING_MINOR_KIND;
	TTranslatorStylesTable* stylesTable = docPtr ? docPtr->StylesTable() : NULL;
	status_t err = reader->Error();
	block_id id;
	while (!err && reader->NextBlock(&id))
	{
		switch( id )
		{
			case kTableCellWidth_id:
			case kTableCellRelWidth_id:
				mIsPercentWidth = (id == kTableCellRelWidth_id);
				err = reader->ReadInt32(&mWidth);
				break;
				
			case kTableCellHeight_id:
			case kTableCellRelHeight_id:
				mIsPercentWidth = (id == kTableCellRelHeight_id);
				err = reader->ReadInt32(&mHeight);
				break;							
				
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
				err = mStylePtr->Read(reader);
				break;

			case kRowSpan_id:
				err = reader->ReadInt32(&mRowSpan);
				break;
				
			case kColumnSpan_id:
				err = reader->ReadInt32(&mColumnSpan);
				break;
			
			case kKind_id:
				err = reader->ReadInt32(&partKind);
				break;
				
			case kCellPosition_id:
				err = reader->ReadPoint(&mCellPos);
				if (!err)
					mHasCellPos = true;
				break;
				
			case kPart_id:
				if (partKind == WORDPROCESSING_MINOR_KIND)
				{
					TTranslatorPart_WP* wpPartPtr = new TTranslatorPart_WP(docPtr);
					err = wpPartPtr->ReadData(reader);
					SetText(wpPartPtr);
				}
				else
					reader->SkipBlock();
				break;
					
			default:
				err = reader->SkipBlock();
				break;
		}
	}
	return err ? err : reader->Error();
}

status_t TTableCell::Write(TBlockStreamWriter* writer, TTranslatorDoc* docPtr, int32 id) const
{
	writer->BeginChunk(id);
	writer->WriteInt32(kRowSpan_id, mRowSpan);
	writer->WriteInt32(kColumnSpan_id, mColumnSpan);

	// If cell width is defined send appropriate block with the width.
	if (mWidth)
	{
		block_id id = mIsPercentWidth ? kTableCellRelWidth_id : kTableCellWidth_id;
		writer->WriteInt32(id, mWidth);
	}
	if (mHeight)
	{
		block_id id = mIsPercentHeight ? kTableCellRelHeight_id : kTableCellHeight_id;
		writer->WriteInt32(id, mHeight);
	}
		
	if (mStylePtr)
		mStylePtr->Write( writer );

	if (mHasCellPos)
		writer->WritePoint(kCellPosition_id, mCellPos);
		
	if (mWPPtr)
	{
		writer->WriteInt32(kKind_id, mWPPtr->PartKind());
		mWPPtr->Write(writer, kPart_id);
	}
	writer->EndChunk(id);
	return writer->Error();
}

void TTableCell::SetText(TTranslatorPart_WP* wpPtr)
{
	if (mWPPtr)
		delete mWPPtr;
	mWPPtr = wpPtr;
}

TTranslatorPart_WP*	TTableCell::Text(void) const
{
	return mWPPtr;
}

void TTableCell::SetRowSpan(int32 rows)
{
	mRowSpan = rows;
}

void TTableCell::SetColumnSpan(int32 cols)
{
	mColumnSpan = cols;
}

int32 TTableCell::ColumnSpan(void) const
{
	return mColumnSpan;
}

int32 TTableCell::RowSpan(void) const
{
	return mRowSpan;
}

void TTableCell::SetWidth(int32 width, bool percent)
{
	mIsPercentWidth = percent;
	mWidth = width;
}

int32 TTableCell::Width(bool& percent) const
{
	percent = mIsPercentWidth;
	return mWidth;
}

void TTableCell::SetHeight(int32 height, bool percent)
{
	mIsPercentHeight = percent;
	mHeight = height;
}

int32 TTableCell::Height(bool& percent) const
{
	percent = mIsPercentHeight;
	return mHeight;
}

void TTableCell::SetCellStylePtr( TTranslatorStyle* stylePtr )
{
	mStylePtr = stylePtr;
}

void TTableCell::SetCellPosition(BPoint cellPos)
{
	mCellPos = cellPos;
	mHasCellPos = true;
}

bool TTableCell::GetCellPosition(BPoint* cellPosPtr) const
{
	if (!mHasCellPos)
		return false;
	if (cellPosPtr)
		*cellPosPtr = mCellPos;
	return true;
}

const TTranslatorStyle* TTableCell::CellStylePtr(void) const
{
	return mStylePtr;
}


// $Header: /usr/local/cvsroot/8ball/Datatypes/TranslatorTable.cpp,v 1.17 1999/12/20 19:24:20 tom Exp $
