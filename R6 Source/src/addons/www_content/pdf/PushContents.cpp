#include "ASCII85Decode.h"
#include "ASCIIHexDecode.h"
#include "BreakAwayPusher.h"
#include "CCITTFaxDecode.h"
#include "DCTDecode.h"
#include "FlateDecode.h"
#include "LZWDecode.h"
#include "PredictedDecode.h"
#include "RC4Decode.h"
#include "RunLengthDecode.h"
#include "Object2.h"
#include "pdf_doc.h"

Pusher *
MakePredictorPusher(Pusher *sink, PDFObject *parms)
{
	// no predictor needed if no parms
	if (!parms) return sink;
	// get predictor
	uint32 predictor = 1;
	PDFObject *num = parms->Find(PDFAtom.Predictor)->AsNumber();
	if (num)
	{
		predictor = num->GetInt32();
		num->Release();
	}
	// don't anything for a type 1 predictor
	if (predictor == 1) return sink;

	// get columns
	uint32 columns = 1;
	num = parms->Find(PDFAtom.Columns)->AsNumber();
	if (num)
	{
		columns = num->GetInt32();
		num->Release();
	}
	// get colors
	uint32 colors = 1;
	num = parms->Find(PDFAtom.Colors)->AsNumber();
	if (num)
	{
		colors = num->GetInt32();
		num->Release();
	}
	// get bitsPerComponent
	uint32 bitsPerComponent = 8;
	num = parms->Find(PDFAtom.BitsPerComponent)->AsNumber();
	if (num)
	{
		bitsPerComponent = num->GetInt32();
		num->Release();
	}
	return new PredictedDecode(sink, predictor, columns, colors, bitsPerComponent);
}

Pusher *
BuildPusher(Pusher *sink, const char *atom, PDFObject *parms)
{
	Pusher *f = 0;

	if (atom == PDFAtom.ASCIIHexDecode) {
		f = new ASCIIHexDecode(sink);
		//printf("creating ASCIIHexDecode\n");
	}
	else if (atom == PDFAtom.ASCII85Decode) {
		f = new ASCII85Decode(sink);
		//printf("creating ASCII85Decode\n");
	}
	else if (atom == PDFAtom.LZWDecode) {
		int32 earlyChange = 1;
		if (parms)
		{
			PDFObject *num = parms->Find(PDFAtom.EarlyChange)->AsNumber();
			if (num)
			{
				earlyChange = num->GetInt32();
				num->Release();
			}
		}
		f = new LZWDecode(MakePredictorPusher(sink, parms), earlyChange);
		//printf("creating LZWDecode\n");
	}
	else if (atom == PDFAtom.RunLengthDecode) {
		f = new RunLengthDecode(sink);
		//printf("creating RunLengthDecode\n");
	}
	else if (atom == PDFAtom.CCITTFaxDecode) {
		f = new CCITTFaxDecode(sink, parms);
		//printf("creating CCITTFaxDecode\n");
	}
	else if (atom == PDFAtom.DCTDecode) {
		f = new DCTDecode(sink);
		//printf("creating DCTDecode\n");
	}
	else if (atom == PDFAtom.FlateDecode) {
		f = new FlateDecode(MakePredictorPusher(sink, parms));
		//printf("creating FlateDecode\n");
	}
	return f;
}

Pusher *
BuildPusherChain(PDFObject *contents, Pusher *sink)
{
	PDFObject *filters = 0;
	PDFObject *parms = 0;
	Pusher *chain = sink;

	// find the filter list
	filters = contents->Find(PDFAtom.Filter);
	// find the decode parameters
	parms = contents->Find(PDFAtom.DecodeParms);
	
#if 0
	// We still need to handle the F (File) source some day.
#endif

	// make each of the specified filters
	if (!filters) {}
	else if (filters->IsName()) {
		PDFObject *parms_d = parms->AsDictionary();
		// only one filter - so it is base and chain
		Pusher *p = BuildPusher(chain, filters->GetCharPtr(), parms_d);
		parms_d->Release();
		if (!p) goto nuke_chain;
		chain = p;
	}
	else if (filters->IsArray())
	{
		object_array *filters_a = filters->Array();
		object_array *parms_a = parms ? (parms->IsArray() ? parms->Array() : 0) : 0;
		// build an empty parms array if we need to
		if (!parms_a)
		{
			parms_a = new object_array;
			for (uint ix = filters_a->size(); ix; ix--)
				parms_a->push_back(PDFObject::makeNULL());
		}
		chain = sink;
		for (int32 ix = filters_a->size() - 1; ix >= 0; ix--)
		{
			if (PDFObject *n = ((*filters_a)[ix])->AsName())
			{
				PDFObject *parms_d = ((*parms_a)[ix])->AsDictionary();
				Pusher *p = BuildPusher(chain, n->GetCharPtr(), parms_d);
				n->Release();
				parms_d->Release();
				if (!p) goto nuke_chain;
				chain = p;
			}
		}
		// free up the parms array if we created it
		if (!parms->IsArray()) delete parms_a;
	}
	//else printf("no filters needed!\n");
	// all done
	goto exit0;

nuke_chain:
	//printf("nuke_chain :-(\n");
	delete chain;
	chain = 0;
exit0:
	return chain;
}

status_t
PushBuffer(PDFObject *dict, const uint8 *buffer, ssize_t length, Pusher *sink)
{
	Pusher *p = BuildPusherChain(dict, sink);
	if (!p) return B_ERROR;
	status_t result = p->Write(buffer, length, true);
	delete p;
	return result;
}

status_t
PushStream(PDFObject *stream, Pusher *sink)
{
	off_t position;
	ssize_t length;
	BPositionIO *src = 0;
	const ssize_t kMaxBufferSize = (64 * 1024);
	ssize_t bufferSize;
	uint8 *buffer;
	ssize_t bytesInBuffer;	// bytes remaining in used buffer
	ssize_t bufferedBytes;	// number of bytes read from source

	//printf("PushStream(\n"); stream->PrintToStream(3); printf("\n");
	status_t result = B_ERROR;
	// build the pusher chain
	Pusher *chain = BuildPusherChain(stream, sink);
	bool needBuffer = true;
	if (!chain) goto error0;

	// get the document and stream offset from the stream
	if (PDFObject *num = stream->Find(PDFAtom.__stream_off_t__)->AsNumber())
	{
		//printf("found off_t\n");
		position = num->GetInt64();
		//printf("offset is: %Ld\n", position);
		//if (position == 119383LL) debugger("resolving offending stream");
		num->Release();
	}
	else
		goto error1;

	// get the stream length
	if (PDFObject *num = stream->Find(PDFAtom.Length)->AsNumber())
	{
		//printf("found length\n");
		length = num->GetInt32();
		//printf("length is: %lu\n", length);
		num->Release();
	}
	else
		goto error1;

	// get the document
	if (PDFObject *doc_obj = stream->Find(PDFAtom.__document__))
	{
		PDFDocument *doc = (PDFDocument *)(((PDFOpaquePointer *)doc_obj->GetOpaquePtr())->fPointer);
		src = doc->Source();
		src->Seek(position, SEEK_SET);
	}
	else
		goto error1;


	// make a decryptor, if required
	if (PDFObject *s = stream->Find(PDFAtom.__stream_key__)->AsString())
	{
		//printf("found stream_key\n");
		RC4Decode *decoder = new RC4Decode(chain, s->Contents(), s->Length());
		if (!decoder) goto error1;
		chain = decoder;
		s->Release();
		needBuffer = false;
	}

	// grab a big chunk of RAM to buffer in
	bufferSize = length < kMaxBufferSize ? length : kMaxBufferSize;
	buffer = new uint8[bufferSize];
	bytesInBuffer = 0;	// bytes remaining in used buffer
	bufferedBytes = 0;	// number of bytes read from source
	// while bytes left to write
	while (length)
	{
		// move the last bytesInBuffer to the front of the buffer
		if (bytesInBuffer)
		{
			memcpy(buffer, buffer + (bufferedBytes - bytesInBuffer), bytesInBuffer);
			bufferedBytes = bytesInBuffer;
		}
		else bufferedBytes = 0;
		// fill the buffer
		bytesInBuffer = bufferSize - bufferedBytes;
		bytesInBuffer = src->Read(buffer + bufferedBytes, bytesInBuffer > length ? length : bytesInBuffer);
		if (bytesInBuffer < 0) break;
		length -= bytesInBuffer;
		bufferedBytes += bytesInBuffer;
		// push down the chain
		bytesInBuffer = chain->Write(buffer, bufferedBytes, length == 0);
		if (bytesInBuffer < 0) break;
		bytesInBuffer = bufferedBytes - bytesInBuffer;
	}
	// some kind of return code
	result = (length == 0) ? B_OK : bytesInBuffer;

	delete [] buffer;
error1:
	delete src;
	delete chain;
error0:
	return result;
}

status_t 
PushContent(PDFObject *obj, Pusher *sink)
{
	//printf("PushContent(\n"); obj->PrintToStream(3); printf("\n");
	PDFObject *array = 0;
	// get an extra ref to the array or stream
	obj->Acquire();
	
	if (!obj->IsArray())
	{
		// new array to traverse
		array = PDFObject::makeArray();
		// add object to array
		array->push_back(obj);
	}
	else
	{
		// remember incomming object as array
		array = obj;
	}
	// array now holds a list of content streams to run
	for (object_array::iterator i = array->begin(); i != array->end(); i++)
	{
		// build a new break away pusher so our common sink won't get nuked each time
		BreakAwayPusher *bap = new BreakAwayPusher(sink);
		PDFObject *stream = (*i)->AsDictionary();
		// push the contents to the sink
		if (stream)
		{
			PushStream(stream, bap);
			stream->Release();
		}
		else break;
	}
	// don't need the array any longer
	array->Release();
	return B_OK;
}

