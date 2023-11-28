//
//	WORDEntry.cpp
//

#include <algobase.h>
#include <TranslationDefs.h>
#include <TranslatorAddOn.h>
#include "ImportWORD.h"
#include "ExportWORD.h"

#define AUTHOR	"©1999-2000 Gobe Software, Inc."
#define VERSION	"Version 2.0.1"
#pragma export on
char translatorName[] = "Gobe MS-WORD Translator";
long translatorVersion = 201;
char translatorInfo[] = VERSION;
#define WORD_TYPE	'MSWD'


translation_format inputFormats[] = 
{
	// For import (these mimetypes are used to filter the open dialog.  The * is interpreted correctly by Squirrel).
	// No need to have both WORD_TYPE and ASCII_TYPE, since text/* covers them both
	{WORD_TYPE, SQUIRREL_TRANSLATOR_NEW_TEXT, 0.9, 1.0, "application/msword", "Microsoft Word"},
	{WORD_TYPE, SQUIRREL_TRANSLATOR_NEW_TEXT, 0.9, 1.0, "application/x-MacOS-WDBN", "Microsoft Word"},

	// For export
	{SQUIRREL_TEXT_NEW_STREAM, SQUIRREL_TRANSLATOR_NEW_TEXT, 1.0, 1.0, "", "Gobe Productive WP stream"},
	{0, 0, 0, 0, "", ""},
};

translation_format outputFormats[] = 
{
	// For import (text -> what internal Format?)
	{SQUIRREL_TEXT_NEW_STREAM, SQUIRREL_TRANSLATOR_NEW_TEXT, 1.0, 1.0, "", "Gobe Productive WP stream"},

	// For export (Squirrel -> what external Format?)
	{WORD_TYPE, SQUIRREL_TRANSLATOR_NEW_TEXT, 0.9, 1.0, "application/msword", "Microsoft Word"},
	{0, 0, 0, 0, "", ""},
};

bool outputIsImport[] = { true, false, false };

long Identify(BPositionIO *inSpec, const translation_format* format, BMessage* ioExtension, translator_info* outInfo, uint32 wantType)
{
	long rtn = B_OK;
	
	if (!outInfo)
		return B_NO_TRANSLATOR;

	outInfo->quality = 0.0;

	if (ioExtension)
		GetConfigMessage(ioExtension);
		
	bool mustBeImport = false;	
	if (wantType)
	{
		size_t x;
		for (x = 0; x < sizeof(outputFormats) / sizeof(translation_format); x++)
		{
			if (outputFormats[x].type == wantType)
			{
				mustBeImport = outputIsImport[x];
				break;
			}
		}

		if (x >= sizeof(outputFormats) / sizeof(translation_format))
			return B_NO_TRANSLATOR;
	}	
	
	TBlockStreamReader reader(inSpec);
	if (reader.OpenStream() == B_NO_ERROR)
	{
		if (mustBeImport)
			return B_NO_TRANSLATOR;

		//IFDEBUG(fprintf(stderr, "WORD Identify (Export)\n"));
		// it's an export; pick the correct input format based on the TMessageStream type
		if (!format)
		{
			for (size_t x = 0; x < sizeof(inputFormats) / sizeof(translation_format); x++)
			{
				if (inputFormats[x].type == (uint) reader.MinorKind())
				{
					format = &inputFormats[x];
					break;
				}
			}
			
			if (!format)
				rtn = B_NO_TRANSLATOR;
		}
	}
	else
	{
		//IFDEBUG(fprintf(stderr, "WORD Identify (Import)\n"));

		TOLEReader *oleReader = new TOLEReader(inSpec);		
		if (oleReader->Entries() && getOleEntry(oleReader, "WordDocument"))
		{
			if (!format)
			{
				//IFDEBUG(fprintf(stderr, "WORD Identify - WORD text - assign format.\n"));
				format = &inputFormats[0];
			}
			else				
				IFDEBUG(fprintf(stderr, "WORD Identify - WORD text - has format.\n"));
		}
		else
			rtn = B_NO_TRANSLATOR;
			
		delete oleReader;
	}
	
	if (rtn == B_OK)
	{
		outInfo->type = format->type;
		outInfo->group = format->group;
		outInfo->quality = format->quality;
		outInfo->capability = format->capability;
		strcpy(outInfo->name, format->name);
		strcpy(outInfo->MIME, format->MIME);
	}
	
	return rtn;
}


long Translate(
		BPositionIO *			inStream,
		const translator_info *	inInfo,
		BMessage *				ioExtension,
		uint32					outFormat,
		BPositionIO *			outStream)
{	
	IFDEBUG(fprintf(stderr, "WORD Translate\n"));
	long rtnCode = B_NO_TRANSLATOR;
	
	switch(inInfo->type)
	{
		// Imports:
		case WORD_TYPE:
		{
			TBlockStreamWriter writer(outStream);
			int32 minorKind = WORDPROCESSING_MINOR_KIND;
			rtnCode = writer.CreateStream(COMPOUND_DOCUMENT_KIND, minorKind);
			if (!rtnCode)
			{
				IFDEBUG(fprintf(stderr, "WORD Translate Import\n"));
				TImportWORD	wordImport(inStream, &writer, ioExtension);
				
				rtnCode = wordImport.DoTranslate();
				
			}
			break;	
		}
	
		// Exports
		case SQUIRREL_TEXT_NEW_STREAM:
		{
			IFDEBUG(fprintf(stderr, "RTF Translate Export\n"));
			TBlockStreamReader reader(inStream);
			rtnCode = reader.OpenStream();
			if (!rtnCode)
			{
				IFDEBUG(fprintf(stderr, "WORD Translate Export\n"));
				TExportWORD	wordExport(&reader, outStream);	
				rtnCode = wordExport.DoTranslate();
			}
			break;	
		}
	}
	
	IFDEBUG(fprintf(stderr, "WORD End Translate\n"));
	return rtnCode;
}

status_t GetConfigMessage(BMessage * ioExtension)
{
	ioExtension->AddBool("supports_tables", true);
	ioExtension->AddBool("supports_graphics", false);
	ioExtension->AddBool("supports_status", true);
	return B_OK;
}


status_t MakeConfig(	/*	optional	*/
				BMessage * ioExtension,	/*	can be NULL	*/
				BView * * outView,
				BRect * outExtent)
{
	//	ignore config

	*outView = new BView(BRect(0,0,200,150), "Gobe MS-WORD Translator Settings", B_FOLLOW_NONE, B_WILL_DRAW);
	rgb_color gray = { 222, 222, 222, 0 };
	(*outView)->SetViewColor(gray);

	BStringView *str = new BStringView(BRect(10,10,190,34), "title", translatorName);
	str->SetFont(be_bold_font);
	(*outView)->AddChild(str);

	str = new BStringView(BRect(10,44,190,68), "info", VERSION " " __DATE__);
	str->SetFont(be_plain_font);
	(*outView)->AddChild(str);

	str = new BStringView(BRect(10,69,190,93), "author", AUTHOR);
	str->SetFont(be_plain_font);
	(*outView)->AddChild(str);

	outExtent->Set(0,0,200,150);

	return B_OK;
}

#pragma export off
