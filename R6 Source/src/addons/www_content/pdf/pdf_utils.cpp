#include <stdlib.h>

#include "PDFKeywords.h"
#include "pdf_utils.h"

#include "Object2.h"
#include "lexer.h"
#include "pdf_doc.h"
#include "crypto.h"

using namespace BPrivate;

void dump_buffer(uint8 *buffer, size_t bytes)
{
	printf("-->>");
	for (size_t i = 0; i < bytes; i++)
	{
		switch (buffer[i])
		{
			case '\\':
				printf("\\\\");
				break;
			case 0x0a:
				printf("\\n");
				break;
			case 0x0d:
				printf("\\r");
				break;
			default:
				if ((buffer[i] < ' ') || (buffer[i] > 126))
				{
					printf("\\x%.2x", buffer[i]);
				}
				else printf("%c", buffer[i]);
				break;
		}
	}
	printf("<<--\n");
}

PDFObject *
BPrivate::pdf_get_next_object(Tokenizer *lex, PDFDocument *doc, RC4codec *decoder) {
//	printf("pdf_get_next_object()\n");
	lexer_type t = lex->NextSkipCommentsAndNewLines();
	
	size_t length;
	const uint8 *token = lex->String(&length);
	PDFObject *value = NULL;
	
	switch(t) {
		case LEX_NAME:
			// makr name into object_id
//			printf("found PDFObject(%s)\n", (const char *)token);
			value = PDFObject::makeName((const char *)token);
			break;
		
		case LEX_NUMBER:
//			printf("found PDFObject(%f)\n", lex->AsNumber());
			value = PDFObject::makeNumber(lex->AsNumber());
			break;
		
		case LEX_STRING:
//			printf("founnd PDFObject\n");
			value = PDFObject::makeString(length, token);
			if (decoder)
			{
				decoder->Reset();
				decoder->Encode((uchar *)(((PDFObject *)value)->Contents()), length);
				//printf("decoded "); dump_buffer((uchar *)(((PDFObject *)value)->Contents()), length);
			}
			break;
		
		case LEX_KEYWORD: {
		
			if ((length == 2) && (memcmp("<<", token, 2) == 0)) {
//				printf("found dictionary\n");
				value = pdf_make_dictionary(lex, doc, decoder);
			}
			else if ((length == 1) && (*token == LEFT_SQUARE)) {
//				printf("found array\n");
				value = pdf_make_array(lex, doc, decoder);
			}
			else if ((length == 4) && (memcmp("true", token, 4) == 0)) {
//				printf("found boolean: true\n");
				value = PDFObject::makeBoolean(true);
			}
			else if ((length == 5) && (memcmp("false", token, 5) == 0)) {
//				printf("found boolean: false\n");
				value = PDFObject::makeBoolean(false);
			}
			else if ((length == 4) && (memcmp("null", token, 4) == 0)) {
//				printf("found null object\n");
				value = PDFObject::makeNULL();
			}
			else {
//				printf("found keyword\n");
				// value = pdf_make_keyword(length, token);
				value = PDFObject::makeKeyword((const char *)token);
			}
			break;
		}
		
		case LEX_EOL_MARKER:
		case LEX_COMMENT:
			// ignore these
		case LEX_END_OF_DATA:
			// we need to do something intelligent in this situation
		default:
			value = NULL;
			break;
	}
	return value;
}

PDFObject *
BPrivate::pdf_make_array(Tokenizer *lex, PDFDocument *doc, RC4codec *decoder) {
//	printf("pdf_make_array()\n");
	//BAtomizer *atomizer = doc->Atomizer();
	PDFObject *a = PDFObject::makeArray();
	object_array *aa = a->Array();
	int32 count = 0;
	bool notDone = true;
	while(notDone) {
//		printf("make_array count: %ld\n", count);
		count++;
		PDFObject *value = BPrivate::pdf_get_next_object(lex, doc, decoder);
		if (value->IsKeyword()) {
			int32 key = value->GetInt32();
			// get rid of the keyword object
			value->Release(); value = NULL;
			
			if (key == PDF_endarray) {
				notDone = false; continue;
			}
			else if (key == PDF_R) {
				// we have a reference we need to build
				PDFObject *o, *g;
				g = aa->back();
				aa->pop_back();
				o = aa->back();
				aa->pop_back();
				
				// ensure they are numbers
				if (o->IsNumber() && g->IsNumber()) {
					// make a reference object
					value = PDFObject::makeReference(o->GetInt32(), g->GetInt32(), doc);
				}
				g->Release();
				o->Release();
			}
		}

		// push the object onto the array
		if (value) {
			//value->IncRef();
			aa->push_back(value);
		}
	}
//	printf("done making array\n");
	return a;
}

PDFObject *
BPrivate::pdf_make_dictionary(Tokenizer *lex, PDFDocument *doc, RC4codec *decoder) {
	PDFObject *d = PDFObject::makeDictionary();
	object_array *a = d->Array();
	bool notDone = true;

	while(notDone) {
		PDFObject *value = BPrivate::pdf_get_next_object(lex, doc, decoder);
		if (value->IsKeyword())
		{
			int32 key = value->GetInt32();
			// get rid of the keyword object
			value->Release(); value = NULL;
			
			if (key == PDF_enddict)
			{
				notDone = false;
				continue;
			}
			else if (key == PDF_R)
			{
				// we have a reference we need to build
				PDFObject *o, *g;
				g = a->back();
				a->pop_back();
				o = a->back();
				a->pop_back();
				// ensure they are numbers
				if (o->IsNumber() && g->IsNumber()) {
					// make a reference object
					value = PDFObject::makeReference(o->GetInt32(), g->GetInt32(), doc);
				}
				g->Release();
				o->Release();
			}
		}

		// push the object onto the array
		if (!value) break;
		{
			a->push_back(value);
		}
	}
	// give up the goods
	return d;
}


OpaqueBPositionIO::OpaqueBPositionIO(BPositionIO *io)
	: PDFOpaque(), m_io(io)
{
}


OpaqueBPositionIO::~OpaqueBPositionIO()
{
	delete m_io;
}
