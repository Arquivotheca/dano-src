//*****************************************************************************************************
// TranslatorPart.h
//
//	This document contains the definition of the TTranslatorPart base class and classes used by it.
//*****************************************************************************************************

#ifndef __TRANSLATORPART_H__
#define __TRANSLATORPART_H__

#include <List.h>
#include "TranslationConsts.h"

class TAttachment;
class TTranslatorDoc;
class TBlockStreamWriter;
class TBlockStreamReader;

//*****************************************************************************************************
// TTranslatorPart
//*****************************************************************************************************
class TTranslatorPart
{
	public:
		TTranslatorPart(TTranslatorDoc* docPtr);
		virtual ~TTranslatorPart();
		
		status_t				Write(TBlockStreamWriter* writer, int32 id) const;
		
		virtual status_t		WriteData(TBlockStreamWriter* writer) const;
		virtual status_t		ReadData(TBlockStreamReader* reader);
		virtual bool			ReadBlock(TBlockStreamReader* reader, int32 id);
		
		virtual int32			PartKind(void) const = 0;

		int32					Attachments(void) const;
		const TAttachment*		AttachmentPtr(int32 index) const;
		void					AddAttachment(TAttachment* attchPtr);
		status_t				ReadAttachment(TBlockStreamReader* reader);
		
	protected:
		TTranslatorDoc*			mDocPtr;
		BList					mAttachments;
};


//*****************************************************************************************************
// TAttachment
//*****************************************************************************************************
class TAttachment
{
	public:
		TAttachment(TTranslatorDoc* docPtr);
		virtual ~TAttachment();
		
		status_t				Write(TBlockStreamWriter* writer) const;

		virtual int32			Kind(void) const							{ return kAttachUnknown; }
		virtual status_t		WriteData(TBlockStreamWriter* writer) const;
		virtual status_t		ReadData(TBlockStreamReader* reader);
		virtual bool			ReadBlock(TBlockStreamReader* reader, int32 id);
				
	protected:
		TTranslatorDoc*			mDocPtr;
};

//
//class TAttachment_NamedReference : public TAttachment
//{
//	public:
//		TAttachment_NamedReference(TTranslatorDoc* docPtr);
//		virtual ~TAttachment_NamedReference();
//		
//		virtual int32			Kind(void) const							{ return kAttachNamedRef; }
//		virtual status_t		WriteData(TBlockStreamWriter* writer);
//		virtual bool			ReadBlock(TBlockStreamReader* reader, int32 id);
//
//		const char*				Name(void) const							{ return mRefName; }
//		const char*				Description(void) const						{ return mRefDescription; }
//		
//		void					SetName(const char* newName);
//		void					SetDescription(const char* newDescription);
//				
//	private:
//		char*					mRefName;		
//		char*					mRefDescription;
//};

#endif // __TRANSLATORPART_H__

// $Header: /usr/local/cvsroot/8ball/Datatypes/TranslatorPart.h,v 1.1 1999/12/20 19:24:19 tom Exp $
