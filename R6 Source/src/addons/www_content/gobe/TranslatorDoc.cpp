//*****************************************************************************************************
// TranslatorDoc.cpp
//
//	This document contains the implementation of classes of the top level helper class for reading a
// block protocol structured stream. This class when used in conjunction with its subsidery classes
// completely hides the actual block stream protocol from the translator writer.
//*****************************************************************************************************

#include <string.h>
#include <stdio.h>
#include <Bitmap.h>
#include <Invoker.h>

#include "BlockStream.h"
#include "TranslatorDoc.h"
#include "TranslatorStyle.h"
#include "TranslationConsts.h"
#include "FilterUtils.h"

#include "TranslatorWP.h"
#include "TranslatorSS.h"
#include "TranslatorTable.h"

class TTaskInfo
{
	public:
		TTaskInfo(const char* taskName, float startPercent, float endPercent)
		{
			mTaskName = taskName;
			mStartPercent = startPercent;
			mEndPercent = endPercent;
		}
		~TTaskInfo()
		{
			delete mTaskName;
		}
		
		float				mStartPercent;
		float				mEndPercent;
		const char*			mTaskName;
};

//*****************************************************************************************************
//									TTranslatorDoc
//*****************************************************************************************************
TTranslatorDoc::TTranslatorDoc(BMessage* ioExt)
{
	mIOExt = ioExt;
	mProgressInvoker = NULL;
	if (ioExt)
		ioExt->FindPointer("progress_invoker", (void**) &mProgressInvoker);
	mStyleSheet = new TTranslatorStylesTable();
	mActiveSheetIndex = 0;
}

TTranslatorDoc::~TTranslatorDoc()
{
	SendProgress("Translation complete. (Cleaning up...)", -1);
	while (mTaskStack.CountItems())
		delete static_cast<TTaskInfo*>(mTaskStack.RemoveItem(0L));
	while (mSheets.CountItems())
		delete static_cast<TTranslatorSheet*>(mSheets.RemoveItem(0L));
	while (mNamedRefs.CountItems())
		delete static_cast<TNamedReference*>(mNamedRefs.RemoveItem(0L));
	delete mStyleSheet;
}

status_t TTranslatorDoc::Write(TBlockStreamWriter* writer)
{
	int32 sheetCount = mSheets.CountItems();
	float divPercent = 1.0/(sheetCount+1);
	BeginTask("Translating ", .33, .67);
	BeginTask("Styles ", .0, divPercent);
	status_t err = mStyleSheet->Write( writer );
	EndTask();

	// Write out the sheets...		
	if (!err)
	{
		BeginTask("Sheet: ", divPercent, 1.0);
		writer->WriteInt32( kActiveSheetIndex_id, mActiveSheetIndex );
		writer->WriteInt32( kArrayCount_id, sheetCount );
		for (long x = 0; !err && x < sheetCount; x++)
		{
			const char* sheetName = SheetPtr(x)->SheetName();
			char buffer[128];
			if (sheetName)
				sprintf(buffer, "%s ", SheetPtr(x)->SheetName());
			else
				sprintf(buffer, "Sheet%d", x+1);
			BeginTask(buffer, (x+1)*divPercent, (x+2)*divPercent);
			err = SheetPtr(x)->Write( writer, this );
			EndTask();
		}
		EndTask();
	}

	// Write out the named references...	
	for (long x = 0; !err && x < NamedReferences(); x++)
		err = NamedReferencePtr(x)->Write(writer);

	EndTask();
	return err ? err : writer->Error();
}

status_t TTranslatorDoc::Read(TBlockStreamReader* reader)
{
	status_t err = reader->Error();
	block_id id;
	while (!err && reader->NextBlock(&id))
	{
		switch (id)
		{
			case kStyleSheet_id:
				err = mStyleSheet->Read(reader);
				break;
			case kActiveSheetIndex_id:
				reader->ReadInt32( &mActiveSheetIndex );
				break;
			case kSheet_id:
			{
				TTranslatorSheet* sheetPtr = new TTranslatorSheet();
				err = sheetPtr->Read(reader, this);
				AddSheet( sheetPtr );
				break;
			}
			case kNameRef_id:
			{
				TNamedReference* refPtr = new TNamedReference(this);
				err = refPtr->Read(reader);
				AddNamedReference( refPtr );
				break;
			}
				
			default:
				err = reader->SkipBlock();
				break;
		}
	}		
	return err ? err : reader->Error();;
}

void TTranslatorDoc::DumpBytes(const uchar* bytes, int32 numBytes) const
{
	for (long x = 0; x < numBytes; x++)
	{
		if (!(x%16))
		{
			if (x)
				fprintf(stderr,"\n");
			fprintf(stderr,"   ");
		}
		fprintf(stderr,"%2.2hx  ", bytes[x]);
	}
	fprintf(stderr, "\n");
}

#pragma mark -
TTranslatorStylesTable*	TTranslatorDoc::StylesTable(void) const
{
	return mStyleSheet;
}
		
int32 TTranslatorDoc::Sheets(void) const
{
	return mSheets.CountItems();
}

const TTranslatorSheet* TTranslatorDoc::SheetPtr(int32 sheetIndex) const
{
	if (sheetIndex < 0 || sheetIndex >= Sheets())
		return NULL;
	return static_cast<TTranslatorSheet*>(mSheets.ItemAt(sheetIndex));
}

void TTranslatorDoc::AddSheet( TTranslatorSheet* sheetPtr )
{
	mSheets.AddItem( sheetPtr );
}

int32 TTranslatorDoc::ActiveSheetIndex(void) const
{
	return mActiveSheetIndex;
}

void TTranslatorDoc::SetActiveSheetIndex(int32 sheetIndex)
{
	mActiveSheetIndex = sheetIndex;
}

int32 TTranslatorDoc::NamedReferences(void) const
{
	return mNamedRefs.CountItems();
}

const TNamedReference* TTranslatorDoc::NamedReferencePtr(int32 refIndex) const
{
	if (refIndex < 0 || refIndex >= NamedReferences())
		return NULL;
	return static_cast<TNamedReference*>(mNamedRefs.ItemAt(refIndex));
}

void TTranslatorDoc::AddNamedReference( TNamedReference* refPtr )
{
	mNamedRefs.AddItem( refPtr );
}

TTranslatorPart* TTranslatorDoc::ReadPart(TBlockStreamReader* reader)
{
	TTranslatorPart* partPtr = NULL;
	block_id id;
	while (reader->NextBlock(&id))
	{
		switch (id)
		{
			case kKind_id:
				int32 partKind;
				reader->ReadInt32(&partKind);
				switch (partKind)
				{
					case WORDPROCESSING_MINOR_KIND:
						partPtr = new TTranslatorPart_WP(this);
						break;
					case SPREADSHEET_MINOR_KIND:
						partPtr = new TTranslatorPart_SS(this);
						break;
					case TABLE_MINOR_KIND:
						partPtr = new TTranslatorPart_Table(this);
						break;
				}
				if (partPtr)
				{
					// Let part handle reading the rest of this chunk.
					partPtr->ReadData(reader);
					return partPtr;
				}
				break;
				
			default:
				reader->SkipBlock();
				break;
		}
	}
	return partPtr;
}

#pragma mark -
void TTranslatorDoc::SendProgress(const char* label, float percent) const
{
	if (mProgressInvoker)
	{
		BMessage msg(*mProgressInvoker->Message());
		if (label)
			msg.AddString("label", label);
		if (percent >= 0)
			msg.AddFloat("percentage", percent);
		mProgressInvoker->Invoke(&msg);
	}
}

void TTranslatorDoc::SendProgressRange(float startRange, float endRange) const
{
	if (mProgressInvoker)
	{
		BMessage msg('RANG');
		msg.AddFloat("start", startRange);
		msg.AddFloat("end", endRange);
		mProgressInvoker->Invoke(&msg);
	}	
}

void TTranslatorDoc::BeginTask(const char* taskName, float startPercent, float endPercent)
{
	int32 len = strlen(taskName);
	float curStart, curEnd;
	const char* curName = CurrentTaskName(&curStart, &curEnd);
	char* newName = NULL;
	if (curName)
	{
		startPercent = (curEnd - curStart) * startPercent + curStart;
		endPercent = (curEnd - curStart) * endPercent + curStart;
		int32 curNameLen = strlen(curName);
		newName = new char[len + curNameLen + 1];
		strcpy(newName, curName);
		strcpy(newName + curNameLen, taskName);
	}
	else
	{
		newName = new char[len+1];
		strcpy(newName, taskName);
	}
	
	TTaskInfo* infoPtr = new TTaskInfo(newName, startPercent, endPercent);
	mTaskStack.AddItem(infoPtr);
	SendProgressRange(startPercent, endPercent);
	SendProgress(newName, 0);
}

const char* TTranslatorDoc::CurrentTaskName(float* startPercent, float* endPercent) const
{
	int32 count = mTaskStack.CountItems();
	if (count)
	{
		const TTaskInfo* infoPtr = static_cast<TTaskInfo*>(mTaskStack.ItemAt(count-1));
		if (startPercent)
			*startPercent = infoPtr->mStartPercent;
		if (endPercent)
			*endPercent = infoPtr->mEndPercent;
		return infoPtr->mTaskName;
	}
	return NULL;
}

void TTranslatorDoc::SendTaskProgress(float taskPercent) const
{
	int32 count = mTaskStack.CountItems();
	if (count)
		SendProgress(CurrentTaskName(), taskPercent);
}

void TTranslatorDoc::EndTask(void)
{
	// We're done make sure we send 100%.
	SendProgress(NULL, 1.0);

	int32 count = mTaskStack.CountItems();
	if (count)
		delete static_cast<TTaskInfo*>(mTaskStack.RemoveItem(count-1));
	count = mTaskStack.CountItems();
	
	// Restore previous task range if there was one.
	float startPercent, endPercent;
	if (CurrentTaskName(&startPercent, &endPercent))
		SendProgressRange(startPercent, endPercent);
}

#pragma mark -
//*****************************************************************************************************
//									TTranslatorSheet
//*****************************************************************************************************
TTranslatorSheet::TTranslatorSheet()
{
	mMainPart = NULL;
	mHeaderPart = mFooterPart = NULL;
	mSheetKind = 0;
	mSheetName = NULL;
	mShowPaginated = -1;
	mHasMargins = false;
	mSheetMargins.Set(0, 0, 0, 0);
}

TTranslatorSheet::~TTranslatorSheet()
{
	delete mMainPart;
	delete mHeaderPart;
	delete mFooterPart;
	delete [] mSheetName;
}

status_t TTranslatorSheet::Write(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const
{
	status_t err = writer->Error();
	writer->BeginChunk( kSheet_id );

	int32 kind = mMainPart ? mMainPart->PartKind() : writer->MinorKind();
	writer->WriteInt32( kSheetKind_id, kind );

	if (mSheetName)
		writer->WriteString( kName_id, mSheetName );
	if (mMainPart)
		err = mMainPart->Write(writer, kMainPart_id);
	if (mHeaderPart)
		err = mHeaderPart->Write(writer, kHeaderPart_id);
	if (mFooterPart)
		err = mFooterPart->Write(writer, kFooterPart_id);
	if (mHasMargins)
		writer->WriteRect( kSheetMargins_id, mSheetMargins );
	if (mShowPaginated >= 0)
		writer->WriteBool( kShowPaginated_id, (mShowPaginated != 0) );
		
	writer->EndChunk( kSheet_id );
	return err ? err : writer->Error();
}

status_t TTranslatorSheet::Read(TBlockStreamReader* reader, TTranslatorDoc* docPtr)
{
	status_t	err = reader->Error();
	bool		kindSeen = false;
	block_id 	id;
	int32		size;
	
	mSheetKind = reader->MinorKind();
	while (!err && reader->NextBlock(&id))
	{
		switch (id)
		{
			case kName_id:
				size = reader->BlockDataSize();
				mSheetName = new char[size+1];
				if (mSheetName)
				{
					err = reader->ReadString(mSheetName, size);
					mSheetName[size] = 0;
				}
				break;
				
			case kSheetKind_id:
				err = reader->ReadInt32(&mSheetKind);
				kindSeen = true;
				break;
				
			case kMainPart_id:
			{
				TTranslatorPart* partPtr = docPtr->ReadPart(reader);
				if (partPtr)
					SetMainPart(partPtr);
				break;
			}
			
			case kHeaderPart_id:
			{
				TTranslatorPart* headerPtr = docPtr->ReadPart(reader);
				if (headerPtr)
					SetHeaderPart(headerPtr);
				break;
			}
				
			case kFooterPart_id:
			{
				TTranslatorPart* footerPtr = docPtr->ReadPart(reader);
				if (footerPtr)
					SetFooterPart(footerPtr);
				break;
			}	
			
			case kSheetMargins_id:
				mHasMargins = true;
				err = reader->ReadRect(&mSheetMargins);
				break;

			case kShowPaginated_id:
			{
				bool showPaginated;
				err = reader->ReadBool(&showPaginated);
				mShowPaginated = showPaginated ? 1 : 0;
				break;
			}
			
			default:
				err = reader->SkipBlock();
				break;
		}
	}
	return err ? err : reader->Error();
}

int32 TTranslatorSheet::SheetKind(void) const
{
	return mSheetKind;
}

void TTranslatorSheet::SetSheetKind( int32 sheetKind )
{
	mSheetKind = sheetKind;
}

const char* TTranslatorSheet::SheetName(void) const
{
	return mSheetName;
}

void TTranslatorSheet::SetSheetName(const char* sheetName)
{
	delete [] mSheetName;
	mSheetName = NULL;
	if (sheetName)
	{
		int32 len = strlen(sheetName);
		mSheetName = new char[len+1];
		strcpy(mSheetName, sheetName);
	}
}

bool TTranslatorSheet::GetShowPaginated(bool& showPaginated) const
{
	showPaginated = (mShowPaginated != 0);
	return (mShowPaginated > 0);
}

void TTranslatorSheet::SetShowPaginated(bool showPaginated)
{
	mShowPaginated = showPaginated ? 1 : 0;
}

bool TTranslatorSheet::GetMargins(BRect& margins) const
{
	if (mHasMargins)
	{
		margins = mSheetMargins;
		return true;
	}
	return false;
}

void TTranslatorSheet::SetMargins(BRect margins)
{
	mHasMargins = true;
	mSheetMargins = margins;
}

const TTranslatorPart* TTranslatorSheet::MainPartPtr(void) const
{
	return mMainPart;
}

const TTranslatorPart* TTranslatorSheet::HeaderPartPtr(void) const
{
	return mHeaderPart;
}

const TTranslatorPart* TTranslatorSheet::FooterPartPtr(void) const
{
	return mFooterPart;
}

void TTranslatorSheet::SetMainPart(TTranslatorPart* part)
{
	if (mMainPart)
		delete mMainPart;
	mMainPart = part;
	if (mMainPart)
		mSheetKind = mMainPart->PartKind();
}

void TTranslatorSheet::SetHeaderPart(TTranslatorPart* part)
{
	if (mHeaderPart)
		delete mHeaderPart;
	mHeaderPart = part;
}

void TTranslatorSheet::SetFooterPart(TTranslatorPart* part)
{
	if (mFooterPart)
		delete mFooterPart;
	mFooterPart = part;
}

#pragma mark -
TNamedReference::TNamedReference(TTranslatorDoc* docPtr)
{
	mDocPtr = docPtr;
	mRefName = NULL;
	mRefDescription = NULL;
}

TNamedReference::~TNamedReference()
{
	delete [] mRefName;
	delete [] mRefDescription;
}

status_t TNamedReference::Write(TBlockStreamWriter* writer) const
{
	writer->BeginChunk(kNameRef_id);
	writer->WriteString(kName_id, mRefName);
	writer->WriteString(kText_id, mRefDescription);
	writer->EndChunk(kNameRef_id);
	return writer->Error();
}

status_t TNamedReference::Read(TBlockStreamReader* reader)
{
	block_id id;
	while (reader->NextBlock(&id))
	{
		switch( id )
		{
			case kName_id:
				SetName(reader->ReadString());
				break;
				
			case kText_id:
				SetDescription(reader->ReadString());
				break;

			default:
				reader->SkipBlock();
				break;
		}
	}
	return reader->Error();
}

void TNamedReference::SetName(const char* newName)
{
	if (mRefName)
	{
		delete [] mRefName;
		mRefName = NULL;
	}
	
	if (newName)
	{
		int32 len = strlen(newName);
		mRefName = new char[len+1];
		strcpy(mRefName, newName);
	}
}

void TNamedReference::SetDescription(const char* newDescription)
{
	if (mRefDescription)
	{
		delete [] mRefDescription;
		mRefDescription = NULL;
	}
	
	if (newDescription)
	{
		int32 len = strlen(newDescription);
		mRefDescription = new char[len+1];
		strcpy(mRefDescription, newDescription);
	}
}

#pragma mark -
TAttachment::TAttachment(TTranslatorDoc* docPtr)
{
	mDocPtr = docPtr;
}

TAttachment::~TAttachment()
{
}

status_t TAttachment::Write(TBlockStreamWriter* writer) const
{
	writer->BeginChunk( kAttachment_id );
	writer->WriteInt32( kKind_id, Kind() );
	WriteData( writer );
	writer->EndChunk( kAttachment_id );
	return writer->Error();		
}

status_t TAttachment::WriteData(TBlockStreamWriter* writer) const
{
	return B_NO_ERROR;
}

status_t TAttachment::ReadData(TBlockStreamReader* reader)
{
	block_id id;
	while (reader->NextBlock(&id))
	{
		if (!ReadBlock(reader, id))
			reader->SkipBlock();
	}
	return reader->Error();	
}

bool TAttachment::ReadBlock(TBlockStreamReader* reader, block_id id)
{
	return false;
}

//#pragma mark -
//TAttachment_NamedReference::TAttachment_NamedReference(TTranslatorDoc* docPtr)
//	: TAttachment(docPtr)
//{
//	mRefName = NULL;
//	mRefDescription = NULL;
//}
//
//TAttachment_NamedReference::~TAttachment_NamedReference()
//{
//	delete [] mRefName;
//	delete [] mRefDescription;
//}
//
//status_t TAttachment_NamedReference::WriteData(TBlockStreamWriter* writer)
//{
//	if (mRefName)
//		writer->WriteString(kName_id, mRefName);
//	if (mRefDescription)
//		writer->WriteString(kText_id, mRefDescription);
//	return TAttachment::WriteData(writer);
//}
//
//bool TAttachment_NamedReference::ReadBlock(TBlockStreamReader* reader, int32 id)
//{
//	switch (id)
//	{
//		case kName_id:
//			mRefName = reader->ReadString();
//			return true;
//		case kText_id:
//			mRefDescription = reader->ReadString();
//			return true;
//		default:
//			return TAttachment::ReadBlock(reader, id);
//	}
//	return false;
//}
//
//void TAttachment_NamedReference::SetName(const char* newName)
//{
//	if (mRefName)
//	{
//		delete [] mRefName;
//		mRefName = NULL;
//	}
//	
//	if (newName)
//	{
//		int32 len = strlen(newName);
//		mRefName = new char[len+1];
//		strcpy(mRefName, newName);
//	}
//}
//
//void TAttachment_NamedReference::SetDescription(const char* newDescription)
//{
//	if (mRefDescription)
//	{
//		delete [] mRefDescription;
//		mRefDescription = NULL;
//	}
//	
//	if (newDescription)
//	{
//		int32 len = strlen(newDescription);
//		mRefDescription = new char[len+1];
//		strcpy(mRefDescription, newDescription);
//	}
//}


// $Header: /usr/local/cvsroot/8ball/Datatypes/TranslatorDoc.cpp,v 1.49 1999/12/20 19:24:19 tom Exp $

