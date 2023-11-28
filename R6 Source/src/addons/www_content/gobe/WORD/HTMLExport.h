//
//	HTMLExport.h
//
#ifndef __HTMLEXPORT_H__
#define __HTMLEXPORT_H__

#include <StorageKit.h>
#include "TranslatorLib.h"
#include "HTMLconsts.h"

class TTableRow;
class TTableCell;

class TWPExportStateInfo
{
	public:
		TWPExportStateInfo(const TTranslatorPart_WP* partPtr);
		
		const TTranslatorPart_WP*	mPartPtr;
		int32						mTextBlockIndex;
		int32						mTextBlockOffset;
		int32						mTextOffset;
		int32						mParStyleIndex;
		int32						mCharStyleIndex;
		int32						mSpecialCharIndex;
		
		// char props
		bool						mFont;
		bool						mBold;
		bool						mItalic;
		bool						mUnderline;
		bool						mStrikeThru;
		bool						mSuperScript;
		bool						mSubScript;	
};

//-------------------------------------------------------------------
// TExportHTML
//-------------------------------------------------------------------

class TExportHTML
{
	public:	
		TExportHTML(TBlockStreamReader* reader, BPositionIO *outStream, int32 charEncoding);
		virtual	~TExportHTML();
	
		int32	DoTranslate(BMessage *ioExt);

	private:
		void	PreCheckCharStyles(void);
		void	PostCheckCharStyles(void);
		void	writeBitmap(const BBitmap *bitmap);
		int32	convertToHTMLBuffer(const uchar *dataPtr, int32 dataSize);
		
		void	WriteColor(const char* attrName, rgb_color theColor) const;
		void	WriteWidthAttribute(const char* attrName, int32 value, bool isPercent) const;
		void	WriteIntAttribute(const char* attrName, int32 value) const;
		void	WritePercentAttribute(const char* attrName, int32 value) const;
		void	WriteString(const char* str) const;
		void	writeHeader(void);
		void	writeBody(void);

		void 	WriteWord(const TTranslatorPart* partPtr, bool beginNewPar = true);
		void	WriteTable(const TTranslatorPart* partPtr);
		
		void	WriteTableRow(const TTableRow* rowPtr);
		void	WriteTableCell(const TTableCell* cellPtr);
		
		void	OutputHTMLText(const uchar* buffer, int32 bufferLength);
		int32 	NextRun(uchar* buffer, int32 maxSize);
		void	BeginParagraph(void);
		void	EndParagraph(void);
		void	CreateImagesDirectory(void);
		
		void	RememberWPExportState(TWPExportStateInfo& info);
		void	RestoreWPExportState(TWPExportStateInfo& info);
			
		BPositionIO*				mOutStream;
		TBlockStreamReader* 		mReader;
		int32						mCharEncoding;
		BFile*						mOutFile;
		BMessage*					mIOExt;		
	
		BDirectory					mDir;
		const char*					mFileName;
		int32						mFileCount;

		TTranslatorDoc				mTransDoc;
		
		char*						mBuffer;
		int32						mBufferMaxSize;
		int32						mBufferSize;
		
		const TTranslatorPart_WP*	mPartPtr;
		int32						mTextBlockIndex;
		int32						mTextBlockOffset;
		int32						mTextOffset;
		int32						mParStyleIndex;
		int32						mCharStyleIndex;
		int32						mSpecialCharIndex;
		
		// char props
		bool						mFont;
		bool						mBold;
		bool						mItalic;
		bool						mUnderline;
		bool						mStrikeThru;
		bool						mSuperScript;
		bool						mSubScript;
		
};

#endif //__HTMLEXPORT_H__