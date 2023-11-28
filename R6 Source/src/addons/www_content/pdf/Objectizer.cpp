#include "PDFKeywords.h"
#include "Objectizer.h"
#include "ObjectParser.h"
#include "pdf_doc.h"
#include "lex_maps.h"

Objectizer::Objectizer(PDFDocument *doc)
	: ArrayBuilder(), fDoc(doc), fSrc(doc->Source()), bytesInBuffer(0)
{
	// create an ObjectParser, with _this_ as the sink
	ObjectParser *op = new ObjectParser(this);
	// now set it as _our_ sink
	SetSink(op);
}


Objectizer::~Objectizer()
{
	delete fSrc;
	// break the circular reference
	Pusher *p = GetSink();
	p->SetSink(0);
}

ssize_t 
Objectizer::Write(const uint8 *, ssize_t, bool)
{
	// we don't do raw writes
	return ObjectSink::WANT_OBJECTS;
}

ssize_t 
Objectizer::Write(BPrivate:: PDFObject *obj)
{
	status_t result = ObjectSink::OK;
	
	if (obj->IsKeyword()) result = DispatchKeyword(obj);
	else fStack.push_back(obj);

	return result;
}

status_t 
Objectizer::Read(uint8 *buffer, size_t size)
{
	bytesInBuffer = 0;
	return fSrc->Read(buffer, size);
}

off_t 
Objectizer::Seek(off_t position, uint32 seek_mode)
{
	// flush all the items in the stack
	FlushStacks();
	// forget any remaining data
	bytesInBuffer = 0;
	// seek our source document
	return fSrc->Seek(position, seek_mode);
}

off_t 
Objectizer::Position()
{
	return fSrc->Position() - bytesInBuffer;
}

PDFObject *
Objectizer::GetObject(void)
{
	ssize_t bytesRead;

	// if there are no items on the stack OR we have an unclosed array/dict
	while ((fStack.size() == 0) || (NestDepth() != 0))
	{
		// fill remainder of buffer
		bytesRead = fSrc->Read(fBuffer + bytesInBuffer, sizeof(fBuffer) - bytesInBuffer);
		if (bytesRead < 0) break;
		bytesInBuffer += bytesRead;
#if 0
		bytesRead = bytesInBuffer - 1;
		// find trailing white space
		while ((bytesRead >= 0) && InMap(whitemap, fBuffer[bytesRead]))
			bytesRead--;
		bytesRead++;
		if (bytesRead == 0) bytesRead = bytesInBuffer;
#else
		bytesRead = 0;
		// skip past leading white space
		while ((bytesRead < bytesInBuffer) && InMap(whitemap, fBuffer[bytesRead]))
			bytesRead++;
		// skip past non-white space
		while ((bytesRead < bytesInBuffer) && !InMap(whitemap, fBuffer[bytesRead]))
			bytesRead++;
		// skip past trailing white space
		while ((bytesRead < bytesInBuffer) && InMap(whitemap, fBuffer[bytesRead]))
			bytesRead++;
		// back up one byte if not at end of buffer
		//if (bytesRead != bytesInBuffer) bytesRead--;
		// this leaves the "position" at the head of the next token
#endif
		// feed these bytes to the routine
		bytesRead = Pusher::Write(fBuffer, bytesRead, false);
		if (bytesRead < 0) break;
		// shove the buffer up in memory
		bytesInBuffer -= bytesRead;
		memcpy(fBuffer, fBuffer + bytesRead, bytesInBuffer);
	}
	// take the first item
	PDFObject *o = fStack.front();
	fStack.erase(fStack.begin());
	return o;
}

status_t 
Objectizer::DispatchKeyword(PDFObject *o)
{
	status_t result = ObjectSink::OK;
	pdf_keys keyword = (pdf_keys)o->GetInt32();
	switch (keyword)
	{
		case PDF_R:
		{
			PDFObject *gen = fStack.back(); fStack.pop_back();
			PDFObject *obj = fStack.back(); fStack.pop_back();
			fStack.push_back(PDFObject::makeReference(obj->GetInt32(), gen->GetInt32(), fDoc));
			gen->Release();
			obj->Release();
			o->Release();
		} break;
		default:
			result = ArrayBuilder::DispatchKeyword(o);
			break;
	}
	return result;
}

status_t 
Objectizer::UnknownKeyword(PDFObject *o)
{
	// push the keyword on the stack
	fStack.push_back(o);
	return ObjectSink::OK;
}

