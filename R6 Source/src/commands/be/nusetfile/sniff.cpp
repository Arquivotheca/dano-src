// ===========================================================================
//	MimeType.cpp
// 	©1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include <Mime.h>
#include "sniff.h"

// ===========================================================================

bool 		TSniffer::mMimesInited = false;
TSniffer*	TSniffer::mTextType = NULL;
TSniffer*	TSniffer::mDefaultType = NULL;
BList		TSniffer::mMimeList;

// ===========================================================================
 
bool LooksLikeText(const char *data, int dataCount)
{
//	Check for a PEFF

	if (data[0] == 'J' && data[1] == 'o' && data[2] == 'y' && data[3] == '!')
		return false;
		
//	Check the first bit to ensure its text

	dataCount = dataCount < 1024 ? dataCount : 1024;
	for (int i = 0; i < dataCount; i++)
		if (data[i] == 0)
			return false;
#if 0 
		if (data[i] < 32) {
			switch (data[i]) {
				case '\r':
				case '\n':
				case '\t':
					break;
				default:
					pprint("LooksLikeText failed: %d at %d",((Byte*)data)[i],i);
					return false;
			}
		}
#endif

	return true;
}

//==================================================================
//	Init the built in list of mime types

void TSniffer::InitMimes()
{
	if (mMimesInited)
		return;
	mMimesInited = true;

//	Images
	
	mMimeList.AddItem(new TSniffer("image/gif","GIF Image",".gif","GIF8"));						// 'GIF8'
	mMimeList.AddItem(new TSniffer("image/jpeg","JPEG Image",".jpg",
		"\377\330\377\340"));		// FFD8FFE0
	mMimeList.AddItem(new TSniffer("image/png","PNG Image",".png","\211PNG"));					// 0x89 'PNG'
	mMimeList.AddItem(new TSniffer("image/tiff","TIFF Image",".tif","MM*"));						// 'MM'
	mMimeList.AddItem(new TSniffer("image/tiff","TIFF Image",".tif","II*"));						// 'MM'

//	HTML

	mMimeList.AddItem(new TSniffer("text/html","HTML File",".htm","<"));
	//mMimeList.AddItem(new TSniffer("text/text","Text File",".txt"));

//	Movies and Sound

	mMimeList.AddItem(new TSniffer("video/quicktime","QuickTime Movie",
		".mov","mdat",4));
	mMimeList.AddItem(new TSniffer("video/quicktime","QuickTime Movie",
		".mov","moov",4));
	mMimeList.AddItem(new TSniffer("video/x-msvideo","Microsoft Movie",
		".avi","AVI ",8));
	mMimeList.AddItem(new TSniffer("video/mpeg","MPEG System Movie",
		".mpg","/000/000/001/272"));	// 0x000001BA - system stream
	mMimeList.AddItem(new TSniffer("video/mpeg","MPEG Movie",
		".mpg","/000/000/001/263"));			// 0x000001B3 - video stream
	mMimeList.AddItem(new TSniffer("audio/wav","WAVE Sound File",
		".wav","WAV ",8));

//	Postscript

	mMimeList.AddItem(new TSniffer("application/x-postscript","Postscript File",
		".ps","%!"));
	mMimeList.AddItem(new TSniffer("application/pdf","PDF File",
		".pdf","%PDF"));

//	Compressed

	mMimeList.AddItem(new TSniffer("application/mac-binhex40",
		"Macintosh Binhex File",".hqx"));
	mMimeList.AddItem(new TSniffer("application/x-macbinary","Macintosh File",
		".bin"));

	mMimeList.AddItem(new TSniffer("application/zip","Zip File",".zip","PK"));
	mMimeList.AddItem(new TSniffer("application/gzip","GZip File",
		".gz","\037\213"));			// 0x1F8B
	mMimeList.AddItem(new TSniffer("application/compress","Compress File",
		".z","\037\235"));	// 0x1F9D
	
//	Be Stuff

	mMimeList.AddItem(new TSniffer("text/plain","Source Code File",".c"));
	mMimeList.AddItem(new TSniffer("text/plain","Source Code File",".h"));

	mMimeList.AddItem(new TSniffer(B_APPLICATION_MIME_TYPE,
		"Be Application",NULL,"Joy!peff"));
	mMimeList.AddItem(new TSniffer("application/x-be-resource",
		"Be Resource file",NULL,"Joy!resf"));
	mMimeList.AddItem(new TSniffer("application/x-be-sym-file",
		"Symbol File for Be executable",".xMAP"));

//	Default
	
	mTextType = new TSniffer("text/plain","Text File",".txt");
	mDefaultType = new TSniffer("application/octet-stream","Generic File",
		".dat");
}

//==================================================================

TSniffer::TSniffer(const char* mimeType, const char* desc,
	const char* suffixes, const char* sig, int sigOffset) :
	mMimeType(mimeType),mDescription(desc),mSuffixes(suffixes),
	mSig(sig),mSigOffset(sigOffset)
{
}

TSniffer::~TSniffer()
{
}

//	Match the mimes suffix to the one in the filename

bool TSniffer::MatchSuffix(const char *fileSuffix)
{
	if (!mSuffixes)
		return false;
	return strstr(fileSuffix, mSuffixes) != 0;	// Match that sig!
}

//	Match the mimes signature

bool TSniffer::MatchSignature(void* data, int dataCount)
{
	if (!mSig)
		return false;
	return strncmp((char*)data + mSigOffset, mSig, strlen(mSig)) == 0;
}

const char* TSniffer::GetMimeType()
{
	return mMimeType;
}

const char* TSniffer::GetDescription()
{
	return mDescription;
}

//==================================================================
//	Determine mime type based on filename, file contents

TSniffer* TSniffer::MatchMime(const char *filename, void *data, int dataCount)
{
	InitMimes();
	TSniffer* m;
	
//	Match signature first

	if (data && dataCount) {
		int i = 0;
		while (m = (TSniffer*)mMimeList.ItemAt(i++)) {
			if (m->MatchSignature(data,dataCount))
				return m;
		}
	}
	
//	Try and match suffix

	char *name = strdup(filename);
	const char* c = strchr(name,'.');
	if (c != NULL) {
		const char* fileSuffix;
		while (c) {
			fileSuffix = c;
			c = strchr(c + 1,'.');		// Find the last period
		}
		int i = 0;
		while (m = (TSniffer*)mMimeList.ItemAt(i++)) {
			if (m->MatchSuffix(fileSuffix))
				return m;
		}
	}

//	Hmm... try and match to text
	
	if (data && dataCount) {
		if (LooksLikeText((const char*)data,dataCount))
			return mTextType;
	}

//	No Match, return the default type

//+	pprint("MatchMime Failed for '%s",filename);
//+	if (dataCount)
//+		pprintHex(data,min(dataCount,16));

	return mDefaultType;
}


