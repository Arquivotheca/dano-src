//*****************************************************************************************************
// TranslatorTable.h
//
//	This document contains the definition of classes for use when reading or writing table parts from
// a translator document.
//*****************************************************************************************************

#ifndef __TRANSLATORTABLE_H__
#define __TRANSLATORTABLE_H__

#include "TranslatorDoc.h"

class TTranslatorPart_WP;
class TTranslatorStyle;

//*****************************************************************************************************
// TTableCell
//*****************************************************************************************************
class TTableCell
{
	public:
		TTableCell();
		~TTableCell();
		
		status_t				Read(TBlockStreamReader* reader, TTranslatorDoc* docPtr);
		status_t				Write(TBlockStreamWriter* writer, TTranslatorDoc* docPtr, int32 id) const;
		
		void					SetText(TTranslatorPart_WP* wpPtr);
		TTranslatorPart_WP*		Text(void) const;

		int32					RowSpan(void) const;
		int32					ColumnSpan(void) const;
		int32					Width(bool& percent) const;
		int32					Height(bool& percent) const;
		const TTranslatorStyle*	CellStylePtr(void) const;
		bool					GetCellPosition(BPoint* cellPos) const;
		
		void					SetRowSpan(int32 rows);
		void					SetColumnSpan(int32 cols);
		void					SetWidth(int32 width, bool isPercent = false);
		void					SetHeight(int32 height, bool isPercent = false);
		void					SetCellStylePtr( TTranslatorStyle* stylePtr );
		void					SetCellPosition(BPoint cellPos);
				
	private:
		TTranslatorPart_WP*		mWPPtr;
		int32					mRowSpan;
		int32					mColumnSpan;
		int32					mWidth;
		int32					mHeight;
		bool					mIsPercentWidth;
		bool					mIsPercentHeight;
		BPoint					mCellPos;
		bool					mHasCellPos;
		TTranslatorStyle*		mStylePtr;		
};

//*****************************************************************************************************
// TTableRow
//*****************************************************************************************************
class TTableRow : public BList
{
	public:
		TTableRow();
		~TTableRow();
		
		status_t			Read(TBlockStreamReader* reader, TTranslatorDoc* docPtr);
		status_t			Write(TBlockStreamWriter* writer, TTranslatorDoc* docPtr, int32 id) const;
		
		int32				Columns(void) const;
		int32				Cells(void) const;
		TTableCell*			CellPtr(int32 index) const;
		
		void				AddCell(TTableCell* cellPtr);		
};

//*****************************************************************************************************
// TTranslatorPart_Table
//*****************************************************************************************************
class TTranslatorPart_Table : public TTranslatorPart
{
	public:
		TTranslatorPart_Table(TTranslatorDoc* docPtr);
		virtual ~TTranslatorPart_Table();
		
		virtual status_t			WriteData(TBlockStreamWriter* writer) const;
		virtual bool				ReadBlock(TBlockStreamReader* reader, int32 id);
		virtual int32				PartKind(void) const;
		
		int32						Columns(void) const;

		int32						TableKind(void) const;
		int32						Width(bool& percent) const;
		int32						Height(void) const;
		int32						Border(void) const;
		int32						CellPadding(void) const;
		int32						CellSpacing(void) const;

		void						SetTableKind(int32 kind);
		void						SetWidth(int32 width, bool isPercent);
		void						SetHeight(int32 height);
		void						SetBorder(int32 borderSize);
		void						SetCellPadding(int32 cellPadding);
		void						SetCellSpacing(int32 cellSpacing);
		
		int32						Rows(void) const;
		TTableRow*					RowPtr(int32 index) const;
		void						AddRow(TTableRow* rowPtr);

		const TTranslatorStyle*		TableStylePtr(void) const;
		void						SetTableStylePtr( TTranslatorStyle* stylePtr );
						
	private:
		TBlockStreamReader*			mReader;
		
		BList						mRows;
		int32						mWidth;
		int32						mHeight;
		bool						mIsPercentWidth;
		TTranslatorStyle*			mStylePtr;
		int32						mBorderSize;
		int32						mCellPadding;
		int32						mCellSpacing;
		int32						mTableKind;		
};

#endif // __TRANSLATORTABLE_H__

// $Header: /usr/local/cvsroot/8ball/Datatypes/TranslatorTable.h,v 1.12 1999/12/20 19:24:20 tom Exp $
