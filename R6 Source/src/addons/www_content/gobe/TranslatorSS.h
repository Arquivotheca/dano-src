//*****************************************************************************************************
// TranslatorSS.h
//
//	This document contains the definition of classes for use when reading or writing spreadsheet
// parts from a translator document.
//*****************************************************************************************************

#ifndef __TRANSLATORSS_H__
#define __TRANSLATORSS_H__

#include "TranslatorDoc.h"

typedef enum
{
	kEmptyCellType 		= 'NONE',
	kBooleanCellType	= 'Bool',
	kErrorCellType		= 'Err#',
	kTimeCellType		= 'Time',
	kLongCellType		= 'Lng#',
	kFloatCellType		= 'Flt#',
	kDoubleCellType		= 'Dbl#',
	kStringCellType		= 'Strn',
	kFormulaCellType	= 'Fmla',
	kBadFormulaCellType	= 'FmEr'
} CellTypes;

class TTranslatorCell_SS;
class TTranslatorStyle;

//*****************************************************************************************************
// TRowColumnInfoTable
//*****************************************************************************************************
class TRowColumnInfoTable : public BList
{
	public:
		TRowColumnInfoTable();
		~TRowColumnInfoTable();
		
		status_t				Read(TBlockStreamReader* reader, TTranslatorDoc* docPtr);
		status_t				Write(TBlockStreamWriter* writer, TTranslatorDoc* docPtr, int32 id) const;
		
		float					DefaultWidth(void) const;
		const TTranslatorStyle*	DefaultStyle(void) const;

		void					SetDefaultWidth(float width);
		void					SetDefaultStyle(TTranslatorStyle* stylePtr);

		void					SetWidth(int32 index, float width);
		void					SetStyle(int32 index, TTranslatorStyle* stylePtr);
		void					SetInfo(int32 index, float width, TTranslatorStyle* stylePtr);

		float					GetWidth(int32 index) const;
		const TTranslatorStyle*	GetStyle(int32 index) const;
		
		// This method returns the nth item in our table's index and width.
		const TTranslatorStyle*	GetItem(int32 itemIndex, int32* index, float* width) const;
		
		// call this method only if you are sure that index is not already in table.
		void 					AddInfo(int32 index, float width, TTranslatorStyle* stylePtr);
		
	private:
		float					mDefaultWidth;
		TTranslatorStyle*		mDefaultStylePtr;
};

//*****************************************************************************************************
// TTranslatorPart_SS
//*****************************************************************************************************
class TTranslatorPart_SS : public TTranslatorPart
{
	public:
		TTranslatorPart_SS(TTranslatorDoc* docPtr);
		virtual ~TTranslatorPart_SS();
		
		virtual status_t			WriteData(TBlockStreamWriter* writer) const;
		virtual bool				ReadBlock(TBlockStreamReader* reader, int32 id);
		virtual int32				PartKind(void) const;
		
		bool						ShowGrid(void) const					{ return mShowGrid; }
		bool						ShowLockedCells(void) const				{ return mShowLockedCells; }
		bool						ShowFormulas(void) const				{ return mShowFormulas; }
		bool						ShowColumnHeader(void) const			{ return mShowColumnHeader; }
		bool						ShowRowHeader(void) const				{ return mShowRowHeader; }
		bool						ShowCircularRefs(void) const			{ return mShowCircularRefs; }
		bool						ShowZeros(void) const					{ return mShowZeros; }

		void						SetShowGrid(bool showGrid)				{ mShowGrid = showGrid; }
		void						SetShowLockedCells(bool showLocked)		{ mShowLockedCells = showLocked; }
		void						SetShowFormulas(bool showFormulas)		{ mShowFormulas = showFormulas; }
		void						SetShowColumnHeader(bool showHeader)	{ mShowColumnHeader = showHeader; }
		void						SetShowRowHeader(bool showHeader)		{ mShowRowHeader = showHeader; }
		void						SetShowCircularRefs(bool showCirc)		{ mShowCircularRefs = showCirc; }
		void						SetShowZeros(bool showZeros)			{ mShowZeros = showZeros; }
		
		int32						Cells(void) const;
		int32						Rows(void) const;
		int32						Columns(void) const;
		void						GetDataRange(int32* startRow, int32* startCol, int32* endRow, int32* endCol) const;
		
		void						AddCell( TTranslatorCell_SS* cellPtr );
		const TTranslatorCell_SS* 	CellPtr(int32 cellIndex) const;
		
		const TTranslatorStyle*		DefaultStyle(void) const;
		void						SetDefaultStyle(TTranslatorStyle* stylePtr);

		const TRowColumnInfoTable&	ColumnInfo(void) const			{ return mColumnInfo; }
		const TRowColumnInfoTable&	RowInfo(void) const				{ return mRowInfo; }
		TRowColumnInfoTable&		ColumnInfo(void)				{ return mColumnInfo; }
		TRowColumnInfoTable&		RowInfo(void)					{ return mRowInfo; }
				
	private:
		status_t					ReadSpreadsheetData(TBlockStreamReader* reader);
		status_t					ReadDisplayOptions(TBlockStreamReader* reader);
		status_t					ReadRowData(void);
		status_t					ReadCellData(int32 row);
	
		int32						mStartRow;
		int32						mStartColumn;
		int32						mEndRow;
		int32						mEndColumn;
		BList						mCells;
		
		// Display options
		bool						mShowGrid;
		bool						mShowLockedCells;
		bool						mShowFormulas;
		bool						mShowColumnHeader;
		bool						mShowRowHeader;
		bool						mShowCircularRefs;
		bool						mShowZeros;

		TBlockStreamReader*			mReader;
		TTranslatorStyle*			mDefaultStylePtr;
		
		TRowColumnInfoTable			mColumnInfo;
		TRowColumnInfoTable			mRowInfo;
};

//*****************************************************************************************************
// TTranslatorCell_SS
//*****************************************************************************************************
class TTranslatorCell_SS
{
	public:
		TTranslatorCell_SS();
		virtual ~TTranslatorCell_SS();
		
		virtual status_t			Write(TBlockStreamWriter* writer) const;
		virtual status_t			Read(TBlockStreamReader* reader, TTranslatorDoc* docPtr, int32 row);		

		int32						Row(void) const;
		int32						Column(void) const;
		void						SetCellID(int32 row, int32 column);
		
		void						SetStylePtr( TTranslatorStyle* stylePtr );
		const TTranslatorStyle*		StylePtr(void) const;
		
		int32						Kind(void) const;
		const char*					ValueText(void) const;
		bool						GetValue(double* valuePtr) const;
		
		void						SetKind(int32 kind);
		void						SetValueText(const char* textPtr);
		void						SetValue(double number);
		
		const char*					FormulaText(void) const;
		void						SetFormulaText(const char* formulaPtr);
		const TTranslatorCell_SS* 	Result(void) const;
		void						SetResult(TTranslatorCell_SS* resultPtr);

		// Translators can use this message to store translator specific data while
		// processing the document. Returns NULL if create is false and message
		// hasn't been previously created.
		BMessage*					TranslatorData(bool create) const;
				
	private:
		int32						mKind;
		int32						mRow;
		int32						mColumn;
		char*						mValuePtr;
		char*						mFormulaPtr;
		TTranslatorStyle*			mStylePtr;
		double						mValue;
		bool						mHasValue;
		TTranslatorCell_SS*			mResultPtr;
		mutable BMessage*			mTransData;
};

#endif // __TRANSLATORSS_H__

// $Header: /usr/local/cvsroot/8ball/Datatypes/TranslatorSS.h,v 1.2 2000/01/24 19:58:13 tom Exp $

