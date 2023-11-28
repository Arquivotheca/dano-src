//========================================================================
//	MMessageItem.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MMESSAGEITEM_H
#define _MMESSAGEITEM_H

#include "IDEConstants.h"
#include "BeIDEComm.h"
#include "CString.h"
#include "MList.h"
#include "DocumentationLookup.h"
#include "MMessageView.h"

#include <StorageDefs.h>
#include <Entry.h>


const float kMessageItemMinHeight = 16.0;
const float	kMessageTextLeftBorder = 20.0;


struct TokenIdentifier
{
	int32		eLineNumber;		// line number in file, 0-based
	int32		eOffset;			// Offset in file
	int32		eLength;			// Length of token
	int32		eSyncLength;		// Length of sync data
	int32		eSyncOffset;		// Offset of token in sync array
	char		eSync[32];			// 32 bytes of data that hold the token
	bool		eIsFunction;		// Is the token a function name?
};

const int32 kLineTextLength = 512;

struct InfoStruct
{
	entry_ref*		iRef;			// entry_ref of the file
	int32			iLineNumber;	// linenumber of the info
	FileNameT		iFileName;		// file name
	char			iLineText[kLineTextLength];	// info text
	TokenIdentifier	iToken;
	bool			iTextOnly;		// the only valid info in this struct is the iLineText field
};

typedef MList<InfoStruct*> InfoStructList;

class MMessageItem {
public:
	
								MMessageItem(
									const char* inText);
		virtual 				~MMessageItem();
        
		virtual	void			Draw(
									BRect 			inFrame, 
									MMessageView& 	inParentView);
		
		virtual void			Invoke();
		
		int32					GetLineCount()
								{
									return fLines;
								}

		static	int32			FindLineStart(const char * inText, int32 inTextLen, int32 inLineNumber);
		static	int32			FindLineStart(const char * inText, int32 inOffset);
		static	int32			CountLines(const char * inText, int32 inTextLen);

		void					GetFileRef(
									entry_ref&	outRef)
								{
									outRef = fFileRef;
								}
		int32					GetLineNumber()
								{
									return fLineNum;
								}
		int32					TextLength()
								{
									return fText.GetLength();
								}
		const char *			Text()
								{
									return fText;
								}

		static void				GetErrorBitmap(const BBitmap*& outErrorBitmap);
		static void				GetWarningBitmap(const BBitmap*& outWarningBitmap);
		static void				GetInfoBitmap(const BBitmap*& outInfoBitmap);

protected:

		int32					fLines;
		entry_ref				fFileRef;
		int32					fLineNum;
		String					fText;
		String					fFileName;
		TokenIdentifier			fToken;
		bool					fTokenIsGood;
		bool					fEntryRefIsGood;
		bool					fHasUnderLine;
		int32					fUnderLineNumber;
		int32					fUnderLineOffset;		// offset in the line

		static BBitmap			*sErrorBitmap;
		static BBitmap			*sWarningBitmap;
		static BBitmap			*sInfoBitmap;

		static void				InitBitmaps();
								
								MMessageItem();

		void					BuildErrorText(
									const ErrorNotificationMessage& inMessage);
		void					BuildTokenStruct(
									const ErrorNotificationMessage& inMessage);
		void					PruneNewlines();
};


class MErrorMessage : public MMessageItem 
{
public:
								MErrorMessage(
									const ErrorNotificationMessage& inMessage);
								MErrorMessage(
									const char* inText);

		virtual	void			Draw(
									BRect 			inFrame, 
									MMessageView& 	inParentView);
};


class MWarningMessage : public MMessageItem 
{
public:
								MWarningMessage(
									const ErrorNotificationMessage& inMessage);
								MWarningMessage(
									const char * inText);

		virtual	void			Draw(
									BRect 			inFrame, 
									MMessageView& 	inParentView);
};

class MInfoMessage : public MMessageItem 
{
public:
								MInfoMessage(
									const InfoStruct& 	inInfoStruct,
									uint32				inKind);
		virtual	void			Draw(
									BRect 			inFrame, 
									MMessageView& 	inParentView);

		void					BuildInfoText(
									const InfoStruct& 	inInfoStruct,
									uint32				inKind);
protected:
								MInfoMessage();
};

class MDocumentationMessage : public MInfoMessage
{
public:
					MDocumentationMessage(const DocumentationLookupEntry& inDocInfo);
	virtual	void	Draw(BRect inFrame, MMessageView& inParentView);
	virtual void	Invoke();
	
	void			BuildMessageText(const DocumentationLookupEntry& inDocInfo);
};

#endif
