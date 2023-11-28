#include "IndirectBuilder.h"
#include "PDFKeywords.h"
#include "pdf_doc.h"
#include "crypto.h"

IndirectBuilder::IndirectBuilder(PDFDocument *doc)
	: ArrayBuilder(), fDoc(doc), fDecoder(0), fDone(false)
{
}


IndirectBuilder::~IndirectBuilder()
{
	delete fDecoder;
}

ssize_t 
IndirectBuilder::Write(const uint8 *, ssize_t, bool)
{
	// we won't ever handle raw data
	return fDone ? Pusher::SINK_FULL : Pusher::ERROR;
}

ssize_t 
IndirectBuilder::Write(BPrivate:: PDFObject *obj)
{
	if (fDone) return Pusher::SINK_FULL;
	status_t result = ObjectSink::OK;
	if (obj->IsKeyword()) result = DispatchKeyword(obj);
	else
	{
		// decrypt now
		if (obj->IsString() && fDecoder)
		{
			fDecoder->Encode(const_cast<uint8 *>(obj->Contents()), obj->Length());
			fDecoder->Reset();
		}
		// push item on stack
		fStack.push_back(obj);
	}
	return result;
}

status_t 
IndirectBuilder::DispatchKeyword(PDFObject *o)
{
	status_t result = ObjectSink::OK;
	pdf_keys keyword = (pdf_keys)o->GetInt32();

	switch (keyword)
	{
	// R - reference
		// pop the last two entries off the stack, create a reference, and push
		// the ref on the stack
		case PDF_R:
		{
			PDFObject *gen = fStack.back(); fStack.pop_back();
			PDFObject *obj = fStack.back(); fStack.pop_back();
			fStack.push_back(PDFObject::makeReference(obj->GetInt32(), gen->GetInt32(), fDoc));
			obj->Release();
			gen->Release();
		} break;
	// obj - begin object
		// if someone encrypted the document, create a new decoder (key made from
		// the top two stack numbers which stay on the stack)
		case PDF_obj:
		{
			// throw away the old decoder
			delete fDecoder;
			// document is encrypted?
			if (fDoc->IsEncrypted())
			{
				// make a new one for this object
				// we'll need the obj/gen nubmers
				PDFObject *gen = fStack.back();
				PDFObject *obj = *(fStack.end()-2);
				uchar key_buff[10];
				fDoc->MakeObjectKey(obj->GetInt32(), gen->GetInt32(), key_buff);
				fDecoder = new RC4codec(key_buff, sizeof(key_buff));
			}
			else
			{
				fDecoder = 0;
			}
		} break;
	// endobj - end object
	// stream - start stream data
		// tell fDoc about the object and report completion
		case PDF_endobj:
		case PDF_stream:
		{
			PDFObject *payload = fStack.back(); fStack.pop_back();
			PDFObject *gen = fStack.back(); fStack.pop_back();
			PDFObject *obj = fStack.back(); fStack.pop_back();
			fDoc->SetObject(obj->GetInt32(), gen->GetInt32(), payload);
			fDone = true;
			if (keyword == PDF_stream)
				payload->Assign(PDFObject::makeName(PDFAtom.__stream_off_t__), PDFObject::makeNumber(0.0));
			obj->Release();
			gen->Release();
			// payload gets taken by the document
			result = Pusher::SINK_FULL;
		} break;
		default:
			return ArrayBuilder::DispatchKeyword(o);
			break;
	}
	// don't need the keyword any longer
	o->Release();
	return result;
}

