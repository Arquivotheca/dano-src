//*****************************************************************************************************
// TranslatorPart.cpp
//
//	This document contains the implementation of the TTranslatorPart base class and classes used by it.
//*****************************************************************************************************

#include "TranslatorDoc.h"
#include "BlockStream.h"

//*****************************************************************************************************
//									TTranslatorPart
//*****************************************************************************************************
TTranslatorPart::TTranslatorPart(TTranslatorDoc* docPtr)
{
	mDocPtr = docPtr;
}

TTranslatorPart::~TTranslatorPart()
{
}

status_t TTranslatorPart::Write(TBlockStreamWriter* writer, block_id id) const
{
	writer->BeginChunk( id );
	writer->WriteInt32( kKind_id, PartKind() );
	WriteData( writer );
	writer->EndChunk( id );
	return writer->Error();		
}

status_t TTranslatorPart::WriteData(TBlockStreamWriter* writer) const
{
	// Write out the attachments for the part...
	status_t err = writer->Error();	
	for (int32 x = 0; !err && x < Attachments(); x++)
		err = AttachmentPtr(x)->Write(writer);
	return err;
}

status_t TTranslatorPart::ReadData(TBlockStreamReader* reader)
{
	block_id id;
	while (reader->NextBlock(&id))
	{
		if (!ReadBlock(reader, id))
			reader->SkipBlock();
	}
	return reader->Error();	
}

bool TTranslatorPart::ReadBlock(TBlockStreamReader* reader, block_id id)
{
	if (id == kAttachment_id)
	{
		ReadAttachment(reader);
		return true;
	}
	return false;
}

status_t TTranslatorPart::ReadAttachment(TBlockStreamReader* reader)
{
	TAttachment* aPtr = NULL;
	int32		kind = -1;

	block_id id;
	while (reader->NextBlock(&id))
	{
//		if (id == kKind_id)
//		{
//			reader->ReadInt32(&kind);
//			if (kind == kAttachNamedRef)
//				aPtr = new TAttachment_NamedReference(mDocPtr);
//			if (aPtr)
//			{
//				aPtr->ReadData(reader);
//				AddAttachment(aPtr);
//			}
//		}
//		else
			reader->SkipBlock();	
	}
	return reader->Error();
}

int32 TTranslatorPart::Attachments(void) const
{
	return mAttachments.CountItems();
}

const TAttachment* TTranslatorPart::AttachmentPtr(int32 index) const
{
	if (index < 0 || index >= Attachments())
		return NULL;
	return static_cast<TAttachment*>(mAttachments.ItemAt(index));
}

void TTranslatorPart::AddAttachment( TAttachment* attachPtr )
{
	mAttachments.AddItem( attachPtr );
}

