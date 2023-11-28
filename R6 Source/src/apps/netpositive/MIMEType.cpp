// ===========================================================================
//	MIMEType.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "MIMEType.h"
#include "MessageWindow.h"

#include <Bitmap.h>
#include <Mime.h>
#include <ctype.h>

// ===========================================================================

bool 		MimeType::mMimesInited = false;
MimeType*	MimeType::mTextType = NULL;
MimeType*	MimeType::mDefaultType = NULL;
MimeType*	MimeType::mHTMLType = NULL;
CLinkedList	MimeType::mMimeList;

#ifdef ADFILTER
extern const char *kAdFilterMimeType;
extern const char *kAdFilterMIMETypeShortDesc;
extern const char *kAdFilterMIMETypeLongDesc;
extern const char *kAdFilterSiteAttr;
extern const char *kAdFilterSiteAttrName;
extern const char *kAdFilterFilterAttr;
extern const char *kAdFilterFilterAttrName;
extern const char *kAdFilterActionAttr;
extern const char *kAdFilterActionAttrName;
extern const char *kAdFilterAttrAttr;
extern const char *kAdFilterAttrAttrName;
extern const char *kAdFilterTagAttr;
extern const char *kAdFilterTagAttrName;
#endif
extern const char *kApplicationSig;
extern const char *kPasswordFTPAttr;
extern const char *kPasswordFTPAttrName;
extern const char *kPasswordUserAttr;
extern const char *kPasswordUserAttrName;
extern const char *kPasswordAuthAttr;
extern const char *kPasswordAuthAttrName;
extern const char *kPasswordRealmAttr;
extern const char *kPasswordRealmAttrName;
extern const char *kPasswordMimeType;
extern const char *kPasswordMIMETypeShortDesc;
extern const char *kPasswordMIMETypeLongDesc;
extern const char *kFontMimeType;
extern const char *kFontMIMETypeShortDesc;
extern const char *kFontMIMETypeLongDesc;
extern const char *kFontFontAttr;
extern const char *kFontFontAttrName;
extern const char *kFontMapsToAttr;
extern const char *kFontMapsToAttrName;

enum {
	kForceLargeIcon = 0x1,
	kForceMiniIcon = 0x2,
	kForceShortDescription = 0x4,
	kForceLongDescription = 0x8,
	kForcePreferredApp = 0x10,
	kForceAttrInfo = 0x20
};

// ===========================================================================
 
/*
bool LooksLikeText(const char *data, int dataCount)
{
//	Check for a PEFF

	if (data[0] == 'J' && data[1] == 'o' && data[2] == 'y' && data[3] == '!')
		return false;
		
//	Check the first bit to ensure its text

	dataCount = MIN(1024,dataCount);
	for (int i = 0; i < dataCount; i++)
		if (data[i] == 0)
			return false;
	return true;
}
*/

bool LooksLikeText(const char *data, int len)
{
	int j;
//	Check for a PEFF

	if (data[0] == 'J' && data[1] == 'o' && data[2] == 'y' && data[3] == '!')
		return false;
		
	for(j=0; j < len; j++) {
		if (isprint(data[j]) == 0 && isspace(data[j]) == 0 && data[j] != '\t') {

			/* if it's not plain ascii, then try to see if it's UTF-8 */

			if ((data[j] & 0xc0) != 0xc0)
				break;
			
			if (j+1 >= len) {
				j = len;
				break;
			}

			if ((data[j] & 0x20) == 0) {         /* 2-byte utf */
				if ((data[j+1] & 0xc0) != 0x80)
					break;
				j++;
			} else {                             /* 3 or 4 byte utf */
				if ((data[j] & 0x30) == 0x20) {
					if (j+2 >= len) {
						j = len;
						break;
					}

					if ((data[j+1] & 0xc0) != 0x80 ||
						(data[j+2] & 0xc0) != 0x80)
						break;
					j += 2;
				} else if ((data[j] & 0x30) == 0x30) {
					if ((data[j] & 0x08) != 0)
						break;
					
					if (j+3 >= len) {
						j = len;
						break;
					}
							
					if ((data[j+1] & 0xc0) != 0x80 ||
						(data[j+2] & 0xc0) != 0x80 ||
						(data[j+3] & 0xc0) != 0x80)
						break;

					j += 3;
				}
			}
		}
	}
	
	if (j < len)
		return false;
	else
		return true;
}


bool SmellsLikeHTML(const char *data, unsigned int len)
{
	unsigned int i;
	unsigned int j;
	const char *html_tags[] = { "<HTML", "<HEAD", "<TITLE", "<BODY", 
                                "<TABLE", "<!--", "<META", "<CENTER" };
	/* NOTE: the inner-loop below assumes that all entries in the above
             table begin with a '<' character. */

	for(i=0; i < len; i++) {
		for(j=0; j < sizeof(html_tags)/sizeof(char *); j++) {
			if (data[i] == '<' && (i + strlen(html_tags[j])) < len && 
				strncasecmp(&data[i], html_tags[j], strlen(html_tags[j])) == 0)
				return true;
		}
	}

	return false;
}


// This code is stolen from the Tracker, in Tracker.cpp.
bool MimeType::InstallMimeIfNeeded(const char *type, const uchar *largeIconBits,
	const uchar *miniIconBits, const char *shortDescription, const char *
	longDescription, const char *preferredAppSignature, BMessage *attrInfo,
	uint32 forceMask)
{
	// used by InitMimeTypes - checks if a metamime of a given <type> is
	// installed and if it has all the specified attributes; if not, the
	// whole mime type is installed and all attributes are set; nulls can
	// be passed for attributes that don't matter; returns true if anything
	// had to be changed
	BBitmap largeIcon(BRect(0, 0, 31, 31), B_COLOR_8_BIT);
	BBitmap miniIcon(BRect(0, 0, 15, 15), B_COLOR_8_BIT);
	char tmp[B_MIME_TYPE_LENGTH];
	BMessage tmpmsg;

	BMimeType mime(type);
	bool installed = mime.IsInstalled();
		
	if (!installed
		|| (largeIconBits && ((forceMask & kForceLargeIcon)
			|| mime.GetIcon(&largeIcon, B_LARGE_ICON) != B_NO_ERROR))
		|| (miniIconBits && ((forceMask & kForceMiniIcon)
			|| mime.GetIcon(&miniIcon, B_MINI_ICON) != B_NO_ERROR))
		|| (shortDescription && ((forceMask & kForceShortDescription)
			|| mime.GetShortDescription(tmp) != B_NO_ERROR))
		|| (longDescription && ((forceMask & kForceLongDescription)
			|| mime.GetLongDescription(tmp) != B_NO_ERROR))
		|| (preferredAppSignature && ((forceMask & kForcePreferredApp)
			|| mime.GetPreferredApp(tmp) != B_NO_ERROR))
		|| (attrInfo && ((forceMask & kForceAttrInfo)
			|| mime.GetAttrInfo(&tmpmsg) != B_NO_ERROR))) {
	
		if (!installed)
			mime.Install();

		if (largeIconBits) {
			largeIcon.SetBits(largeIconBits, largeIcon.BitsLength(), 0, B_COLOR_8_BIT);
			mime.SetIcon(&largeIcon, B_LARGE_ICON);
		}

		if (miniIconBits) {
			miniIcon.SetBits(miniIconBits, largeIcon.BitsLength(), 0, B_COLOR_8_BIT);
			mime.SetIcon(&miniIcon, B_MINI_ICON);
		}

		if (shortDescription)
			mime.SetShortDescription(shortDescription);
		
		if (longDescription)
			mime.SetLongDescription(longDescription);
		
		if (preferredAppSignature)
			mime.SetPreferredApp(preferredAppSignature);
			
		if (attrInfo)
			mime.SetAttrInfo(attrInfo);	

		return true;
	}
	return false;
}

//==================================================================
//	Init the built in list of mime types

void MimeType::InitMimes()
{
	if (mMimesInited) return;
	mMimesInited = true;
	
	BMessage attrInfo;
	
#ifdef ADFILTER
	attrInfo.AddString("attr:name", kAdFilterSiteAttr);
	attrInfo.AddString("attr:public_name", kAdFilterSiteAttrName);
	attrInfo.AddInt32("attr:type", B_STRING_TYPE);
	attrInfo.AddBool("attr:viewable", true);
	attrInfo.AddBool("attr:editable", true);
	attrInfo.AddInt32("attr:width", 100);
	attrInfo.AddInt32("attr:alignment", B_ALIGN_LEFT);
	
	attrInfo.AddString("attr:name", kAdFilterFilterAttr);
	attrInfo.AddString("attr:public_name", kAdFilterFilterAttrName);
	attrInfo.AddInt32("attr:type", B_STRING_TYPE);
	attrInfo.AddBool("attr:viewable", true);
	attrInfo.AddBool("attr:editable", true);
	attrInfo.AddInt32("attr:width", 200);
	attrInfo.AddInt32("attr:alignment", B_ALIGN_LEFT);
	
	attrInfo.AddString("attr:name", kAdFilterActionAttr);
	attrInfo.AddString("attr:public_name", kAdFilterActionAttrName);
	attrInfo.AddInt32("attr:type", B_STRING_TYPE);
	attrInfo.AddBool("attr:viewable", true);
	attrInfo.AddBool("attr:editable", true);
	attrInfo.AddInt32("attr:width", 100);
	attrInfo.AddInt32("attr:alignment", B_ALIGN_LEFT);

	attrInfo.AddString("attr:name", kAdFilterTagAttr);
	attrInfo.AddString("attr:public_name", kAdFilterTagAttrName);
	attrInfo.AddInt32("attr:type", B_STRING_TYPE);
	attrInfo.AddBool("attr:viewable", true);
	attrInfo.AddBool("attr:editable", true);
	attrInfo.AddInt32("attr:width", 75);
	attrInfo.AddInt32("attr:alignment", B_ALIGN_LEFT);

	attrInfo.AddString("attr:name", kAdFilterAttrAttr);
	attrInfo.AddString("attr:public_name", kAdFilterAttrAttrName);
	attrInfo.AddInt32("attr:type", B_STRING_TYPE);
	attrInfo.AddBool("attr:viewable", true);
	attrInfo.AddBool("attr:editable", true);
	attrInfo.AddInt32("attr:width", 75);
	attrInfo.AddInt32("attr:alignment", B_ALIGN_LEFT);
	InstallMimeIfNeeded(kAdFilterMimeType, NULL, NULL, kAdFilterMIMETypeShortDesc, kAdFilterMIMETypeLongDesc, kApplicationSig, &attrInfo, false);
	
	attrInfo.MakeEmpty();
#endif
	
	attrInfo.AddString("attr:name", kPasswordFTPAttr);
	attrInfo.AddString("attr:public_name", kPasswordFTPAttrName);
	attrInfo.AddInt32("attr:type", B_BOOL_TYPE);
	attrInfo.AddBool("attr:viewable", true);
	attrInfo.AddBool("attr:editable", true);
	attrInfo.AddInt32("attr:width", 20);
	attrInfo.AddInt32("attr:alignment", B_ALIGN_LEFT);

	attrInfo.AddString("attr:name", kPasswordUserAttr);
	attrInfo.AddString("attr:public_name", kPasswordUserAttrName);
	attrInfo.AddInt32("attr:type", B_STRING_TYPE);
	attrInfo.AddBool("attr:viewable", true);
	attrInfo.AddBool("attr:editable", true);
	attrInfo.AddInt32("attr:width", 100);
	attrInfo.AddInt32("attr:alignment", B_ALIGN_LEFT);

	attrInfo.AddString("attr:name", kPasswordAuthAttr);
	attrInfo.AddString("attr:public_name", kPasswordAuthAttrName);
	attrInfo.AddInt32("attr:type", B_STRING_TYPE);
	attrInfo.AddBool("attr:viewable", true);
	attrInfo.AddBool("attr:editable", true);
	attrInfo.AddInt32("attr:width", 100);
	attrInfo.AddInt32("attr:alignment", B_ALIGN_LEFT);

	attrInfo.AddString("attr:name", kPasswordRealmAttr);
	attrInfo.AddString("attr:public_name", kPasswordRealmAttrName);
	attrInfo.AddInt32("attr:type", B_STRING_TYPE);
	attrInfo.AddBool("attr:viewable", true);
	attrInfo.AddBool("attr:editable", true);
	attrInfo.AddInt32("attr:width", 100);
	attrInfo.AddInt32("attr:alignment", B_ALIGN_LEFT);
	
	InstallMimeIfNeeded(kPasswordMimeType, NULL, NULL, kPasswordMIMETypeShortDesc, kPasswordMIMETypeLongDesc, kApplicationSig, &attrInfo, false);

	attrInfo.MakeEmpty();
	
	attrInfo.AddString("attr:name", kFontFontAttr);
	attrInfo.AddString("attr:public_name", kFontFontAttrName);
	attrInfo.AddInt32("attr:type", B_STRING_TYPE);
	attrInfo.AddBool("attr:viewable", true);
	attrInfo.AddBool("attr:editable", true);
	attrInfo.AddInt32("attr:width", 100);
	attrInfo.AddInt32("attr:alignment", B_ALIGN_LEFT);

	attrInfo.AddString("attr:name", kFontMapsToAttr);
	attrInfo.AddString("attr:public_name", kFontMapsToAttrName);
	attrInfo.AddInt32("attr:type", B_STRING_TYPE);
	attrInfo.AddBool("attr:viewable", true);
	attrInfo.AddBool("attr:editable", true);
	attrInfo.AddInt32("attr:width", 100);
	attrInfo.AddInt32("attr:alignment", B_ALIGN_LEFT);

	InstallMimeIfNeeded(kFontMimeType, NULL, NULL, kFontMIMETypeShortDesc, kFontMIMETypeLongDesc, kApplicationSig, &attrInfo, false);

	typedef struct {
		const char *type;
		const char *name;
		const char *extension;
		const char *sig;
		int			sigOffset;
	} MimeData;
	
	char starcode_pkg[] = { 0x41, 0x6c, 0x42, 0x1a,0xff,0x0a,0xd,00 };

	const MimeData kMimeData[] = {
//	Images
		
		{"image/gif",					"GIF Image",				".gif",			"GIF8",				0},		// 'GIF8'
		{"image/jpeg",					"JPEG Image",				".jpg",			"\xff\xd8\xff",		0},		// FFD8FF
		{"image/png",					"PNG Image",				".png",			"\211PNG",			0},		// 0x89 'PNG'
		{"image/tiff",					"TIFF Image",				".tif",			"MM*",				0},		// 'MM'
		{"image/tiff",					"TIFF Image",				".tif",			"II*",				0},		// 'MM'
	
//	HTML
	
		//{"text/html",					"HTML File",				".htm",			"<",				0},
		//{"text/text",					"Text File",				".txt"			,0,					0},
	
//	Movies and Sound
	
		{"audio/x-wav",					"WAVE Sound File",			".wav",			"WAV ",				8},
		{"audio/x-midi",				"Midi Data",				".midi",		"MThd",				0},
	
//	Postscript
	
		{"application/x-postscript",	"Postscript File",			".ps",			"%!",				0},
		{"application/x-pdf",			"PDF File",					".pdf",			"%PDF",				0},
	
//	Compressed/Archive
	
		{"application/x-tar",			"Tar Archive",				".tar",			"ustar",			257},
		{"application/x-mac-binhex40",	"Macintosh Binhex File",	".hqx",			0,					0},
		{"application/x-macbinary",		"Macintosh File",			".bin",			0,					0},
	
		{"application/zip",				"Zip File",					".zip",			"PK",				0},
		{"application/x-gzip",			"GZip File",				".gz",			"\037\213",			0},		// 0x1F8B
		{"application/x-compress",		"Compress File",			".z",			"\037\235",			0},		// 0x1F9D
		{"application/x-scode-UPkg",	"StarCode Installer Package", ".pkg", starcode_pkg, 			0},
		
		{"application/x-shockwave-flash", "Shockwave Flash",		".swf",			"FWS\0x03",			0 },
		
//	Be Stuff
	
	//	{"application/x-Be-Executable","Be Application",NULL,"Joy!peff",0},
	//	{"application/x-Be-Resource","Be Resource file",NULL,"Joy!resf",0},
	
		{""}
	};
	
//	Default
	
	mTextType = new MimeType("text/plain","Text File",".txt");
	mDefaultType = new MimeType("application/octet-stream","Generic File",".dat");
	mHTMLType = new MimeType("text/html","HTML File",".htm");

	
	const MimeData *data = kMimeData;
	while (*data->type) {
		mMimeList.Add(new MimeType(data->type, data->name, data->extension, data->sig, data->sigOffset));
		data++;
	}

}

BString MimeType::RepairMimeType(const BString& type)
{
	typedef struct {
		const char *badMimeType;
		const char *goodMimeType;
	} MimeRepairEntry;
	
	const MimeRepairEntry kMimeRepairTable[] = {
		{"application/gzip",			"application/x-gzip"},
		{""}
	};
	
	const MimeRepairEntry *entry = kMimeRepairTable;
	
	BString fixedType = type;
	
	// Some MIME types have a semicolon followed by crap.  I'm not sure what's
	// up with this, but hack it off at the semicolon.
	int32 semicolonPos = fixedType.FindFirst(';');
	if (semicolonPos >= 0)
		fixedType.Truncate(semicolonPos);
		
	if (fixedType.Length() == 0)
		fixedType = "application/octet-stream";
		
	while (*entry->badMimeType) {
		if (strcasecmp(fixedType.String(), entry->badMimeType) == 0) {
			pprint("Changing MIME type %s to %s", fixedType.String(), entry->goodMimeType);
			fixedType = entry->goodMimeType;
			return fixedType;
		}
		entry++;
	}
	return fixedType;
}

//==================================================================

MimeType::MimeType(const char* mimeType, const char* desc, const char* suffixes, const char* sig, int sigOffset) :
	mMimeType(mimeType),mDescription(desc),mSuffixes(suffixes),mSig(sig),mSigOffset(sigOffset)
{
}

MimeType::~MimeType()
{
}

//	Match the mimes suffix to the one in the filename

bool MimeType::MatchSuffix(const char *fileSuffix)
{
	if (mSuffixes.Length() == 0)
		return false;
	return strstr(fileSuffix,mSuffixes.String()) != 0;	// Match that sig!
}

//	Match the mimes signature

bool MimeType::MatchSignature(const void* data, int)
{
	if (mSig.Length() == 0 || data == 0)
		return false;
	return strncmp((char*)data + mSigOffset,mSig.String(),mSig.Length()) == 0;
}

const char* MimeType::GetMimeType()
{
	return mMimeType.String();
}

const char* MimeType::GetDescription()
{
	return mDescription.String();
}

//==================================================================
//	Determine mime type based on filename, file contents

MimeType* MimeType::MatchMime(const char *filename, const void *data, int dataCount)
{
	InitMimes();
	MimeType* m;
	
//	Match signature first

	if (data && dataCount){
		for (m = (MimeType*)mMimeList.First(); m; m = (MimeType*)m->Next()) {
			if (m->MatchSignature(data,dataCount))
				return m;
		}
		if (SmellsLikeHTML((const char*)data,dataCount))
			return mHTMLType;
		//	Hmm... try and match to text

		if (LooksLikeText((const char*)data,dataCount))
			return mTextType;
	}		
		
//	Try and match suffix
	BString name(filename);
	name.ToLower();
	if (name.Length()) {
		const char* c = strchr(name.String(),'.');
		if (c != NULL) {
			const char* fileSuffix;
			while (c) {
				fileSuffix = c;
				c = strchr(c + 1,'.');		// Find the last period
			}
			for (m = (MimeType*)mMimeList.First(); m; m = (MimeType*)m->Next()) {
				if (m->MatchSuffix(fileSuffix))
					return m;
			}
		}
	}

	
//	No Match, return the default type

	pprint("MatchMime Failed for '%s",filename);
	if (dataCount)
		pprintHex(data,MIN(dataCount,16));

	return mDefaultType;
}


