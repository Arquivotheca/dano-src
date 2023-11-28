//*****************************************************************************************************
// TranslatorDoc.h
//
//	This document contains the definition of classes of the top level helper class for reading a
// block protocol structured stream. This class when used in conjunction with its subsidery classes
// completely hides the actual block stream protocol from the translator writer.
//*****************************************************************************************************

#ifndef __TRANSLATORDOC_H__
#define __TRANSLATORDOC_H__

#include <Rect.h>
#include "TranslatorPart.h"

class TTranslatorStylesTable;
class TTranslatorStyle;
class TTranslatorSheet;
class TNamedReference;

#pragma mark TTranslatorDoc
//*****************************************************************************************************
// TTranslatorDoc
//*****************************************************************************************************
class TTranslatorDoc
{
	public:
		TTranslatorDoc(BMessage* ioExt = NULL);
		virtual ~TTranslatorDoc();
		
		status_t				Write(TBlockStreamWriter* writer);
		status_t				Read(TBlockStreamReader* reader);

		TTranslatorStylesTable*	StylesTable(void) const;
		
		int32					Sheets(void) const;
		const TTranslatorSheet*	SheetPtr(int32 sheetIndex) const;
		void					AddSheet( TTranslatorSheet* sheetPtr );

		int32					ActiveSheetIndex(void) const;
		void					SetActiveSheetIndex(int32 sheetIndex);

		// Used to create inter part references (e.g. Excel Named Ranges)
		int32					NamedReferences(void) const;
		const TNamedReference*	NamedReferencePtr(int32 refIndex) const;
		void					AddNamedReference( TNamedReference* refPtr );

		// Low level status methods.
		void					SendProgress(const char* label, float percent) const;
		void 					SendProgressRange(float startRange, float endRange) const;

		// Higher level status methods.
		void					BeginTask(const char* taskName, float startPercent, float endPercent);
		const char*				CurrentTaskName(float* startPercent = NULL, float* endPercent = NULL) const;
		void					SendTaskProgress(float taskPercent) const;
		void					EndTask(void);

		// Helpful methods.
		TTranslatorPart*		ReadPart(TBlockStreamReader* reader);
		void					DumpBytes(const uchar* bytes, int32 numBytes) const;

	private:
		BMessage*				mIOExt;
		BInvoker*				mProgressInvoker;
		BList					mSheets;
		int32					mActiveSheetIndex;
		TTranslatorStylesTable*	mStyleSheet;
		BList					mTaskStack;
		BList					mNamedRefs;
};

//*****************************************************************************************************
// TTranslatorSheet
//*****************************************************************************************************
class TTranslatorSheet
{
	public:
		TTranslatorSheet();
		virtual ~TTranslatorSheet();
		
		virtual status_t		Write(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const;
		virtual status_t		Read(TBlockStreamReader* reader, TTranslatorDoc* docPtr);

		TTranslatorPart*		ReadPart(TBlockStreamReader* reader, TTranslatorDoc* docPtr);
		
		int32					SheetKind(void) const;
		const char*				SheetName(void) const;

		void					SetSheetKind( int32 sheetKind );
		void					SetSheetName(const char* nameStr);

		bool					GetShowPaginated(bool& showPaginated) const;
		void					SetShowPaginated(bool showPaginated);
		bool					GetMargins(BRect& sheetMargins) const;
		void					SetMargins(BRect sheetMargins);
		
		const TTranslatorPart*	MainPartPtr(void) const;
		const TTranslatorPart*	HeaderPartPtr(void) const;
		const TTranslatorPart*	FooterPartPtr(void) const;
		
		void					SetMainPart(TTranslatorPart* part);
		void					SetHeaderPart(TTranslatorPart* part);
		void					SetFooterPart(TTranslatorPart* part);
	
	private:
		int32					mSheetKind;
		bool					mHasMargins;
		BRect					mSheetMargins;
		TTranslatorPart*		mMainPart;
		TTranslatorPart*		mHeaderPart;
		TTranslatorPart*		mFooterPart;
		char*					mSheetName;
		int32					mShowPaginated;		// -1 Don't care, 0 - not paginated, 1 - paginated
};

//*****************************************************************************************************
// TNamedReference
//*****************************************************************************************************
class TNamedReference
{
	public:
		TNamedReference(TTranslatorDoc* docPtr);
		virtual ~TNamedReference();
		
		status_t				Write(TBlockStreamWriter* writer) const;
		status_t				Read(TBlockStreamReader* reader);
		
		const char*				Name(void) const				{ return mRefName; }
		const char*				Description(void) const			{ return mRefDescription; }
		
		void					SetName(const char* newName);
		void					SetDescription(const char* newDescription);
		
	private:
		TTranslatorDoc*			mDocPtr;
		char*					mRefName;		
		char*					mRefDescription;
};

#endif // __TRANSLATORDOC_H__

// $Header: /usr/local/cvsroot/8ball/Datatypes/TranslatorDoc.h,v 1.32 1999/12/20 19:24:19 tom Exp $
