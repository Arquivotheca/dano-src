#ifndef _FONTHANDLER_H_
#define _FONTHANDLER_H_

#include <List.h>
#include <File.h>
#include <String.h>
#include <stdio.h>

class BPath;
class BFont;

enum { FF_Type1, FF_Type3, FF_Type42 };

class FontHandler
{
 public:
								FontHandler();
								~FontHandler();
								
	void						UsesFont(BFont*);

	void						SetConversionType(int32);
	status_t					PrepareFontFile();

	char*						GetFontAlias(BFont*);
	status_t					WriteFontAliases(BDataIO *toFile);
	
	const char*					FontFilePath();
								
 private:

	status_t					OpenFontFile();
	bool						IsFontInUse(const char *family, const char *style,
											int32 *idx);

	char*						ConvertTrueTypeFont(BPath);
	char*						ConvertTTtoType1(BPath);
	char*						HandleType1Font(BPath);
	char*						ParseT1forFontName(const char *font);

	status_t					IsPFB(BPath);
	status_t					ConvertPFBtoPFA(const BPath, BPath&);
	void						outbinary(FILE*, uchar*, uint32);
	void						outascii(FILE*, uchar*, uint32);
	uint32						little4(uchar*);
	
	void						GetPathForFont(int32, const char*, BPath*);
	void						AppendToFontFile(const char*);
	
	BFile						*fFontFile;
	BString						fFontFilePath;

	BList						fFontsUsed;
	int32						fConversionType;

	char*						fFontAliases[128];
};

#endif
