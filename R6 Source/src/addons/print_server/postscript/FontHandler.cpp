#include "FontHandler.h"

#include <Path.h>
#include <Font.h>

#include <shared_fonts.h>

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <ctype.h>

#include "ttf2pt1.h"

#define P //printf

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

extern int32 convert_tt(const char*, const char*);

FontHandler::FontHandler()
{
	fConversionType = FF_Type1;
	fFontFile = NULL;
	memset(fFontAliases, 0, sizeof(fFontAliases));
}

FontHandler::~FontHandler()
{
	BFont *font;
	for(int32 i=fFontsUsed.CountItems()-1; i >= 0; i--){
		font = (BFont*)fFontsUsed.RemoveItem(i);		
		delete font;
	}

	delete fFontFile;

	if(fFontFilePath != NULL) {
		unlink(fFontFilePath.String());
	}
}

void
FontHandler::UsesFont(BFont *font)
{
	fFontsUsed.AddItem(font);
	
	
	int32 ct = fFontsUsed.CountItems();
	for(int32 i=0; i < ct; i++){
		((BFont*)fFontsUsed.ItemAt(i))->PrintToStream();
	}
	printf("---------------------------\n");
}

char*
FontHandler::GetFontAlias(BFont *font)
{
	BString alias;
	
	font_family inFamily, curFamily;
	font_style inStyle, curStyle;
	
	font->GetFamilyAndStyle(&inFamily, &inStyle);
	
	int32 fontCount = fFontsUsed.CountItems();
	for(int32 i=0; i < fontCount; i++) {
		((BFont*)fFontsUsed.ItemAt(i))->GetFamilyAndStyle(&curFamily,
															&curStyle);
		if((strcmp(inFamily, curFamily) == 0) &&
			(strcmp(inStyle, curStyle) == 0)){
			P("\t...found match in position %ld\n", i);
			BString alias = "F";
			alias << i;
			return (strdup(alias.String()));
		}
	}

	/* font isn't being used... */
	P("\t...no match found, adding new alias F%ld\n", fontCount);
	UsesFont(new BFont(font));
	alias = "F";
	alias << fontCount;
	return (strdup(alias.String()));
	
}

void
FontHandler::SetConversionType(int32 type)
{
	fConversionType = type;
}


const char*
FontHandler::FontFilePath()
{
	return fFontFilePath.String();
}

status_t
FontHandler::PrepareFontFile()
{
	status_t err;
	char *alias;
	
	err = OpenFontFile();
	if(err != B_OK){
		return err;
	}	

	font_file_info		*info_bis;	

	int32 idx;
	int32 font_file_ct = _count_font_files_(0);
	P("font_file_ct = %ld\n", font_file_ct);
	for(int j=0; j < font_file_ct; j++){
		_get_nth_font_file_(j, &info_bis);
		P("Checking on font [%s, %s]\n", info_bis->family, info_bis->style);

		if((info_bis->file_type == FC_BITMAP_FONT) ||
			(info_bis->file_type == FC_BAD_FILE)) {
			// skip tuned fonts	& bad files
			continue;
		}
		
		if(IsFontInUse(info_bis->family, info_bis->style, &idx)){			
			P("\t...it's in use, idx = %ld, size = %f.\n", idx, info_bis->size);
			BPath path;
			GetPathForFont(info_bis->dir_id, info_bis->filename, &path);
			if(info_bis->file_type == FC_TRUETYPE_FONT) {
				alias = ConvertTrueTypeFont(path);
				fFontAliases[idx] = alias;
				P("\t...it's TT, alias = [%s]\n", alias);
			} else {
				alias = HandleType1Font(path);
				fFontAliases[idx] = alias;
				P("\t...it's T1, alias = [%s]\n", alias);
			}
		}
	}

	return B_OK;
}

status_t
FontHandler::OpenFontFile()
{
	status_t err;
	
	fFontFilePath = tmpnam(NULL);
	if(fFontFilePath.Length() == 0) {
		return B_ERROR;
	}
	
	fFontFile = new BFile(fFontFilePath.String(),
							B_CREATE_FILE | B_READ_WRITE);
	if((err = fFontFile->InitCheck()) != B_OK) {
		fFontFilePath = (char*)NULL;
		return err;
	}

	P("In OpenFontFile(), path is: %s\n", fFontFilePath.String());

	return err;
}

bool
FontHandler::IsFontInUse(const char *family, const char *style, int32 *idx)
{
	BFont *font;
	font_family cur_fam;
	font_style cur_style;
	
	bool isUsed = false;
	int32 fontCount = fFontsUsed.CountItems();
	for(int32 i=0; i < fontCount; i++) {
		font = (BFont*)fFontsUsed.ItemAt(i);
		font->GetFamilyAndStyle(&cur_fam,&cur_style);
		if((strcmp(family, cur_fam) == 0) &&
			(strcmp(style, cur_style) == 0)){
				isUsed = true;
				*idx = i;
				break;
		}		
	}
	
	return isUsed;
}

void
FontHandler::GetPathForFont(int32 dir_id, const char *filename, BPath *path)
{
	font_folder_info	*info;

	_count_font_folders_(); // must call this before _get_nth_...

	_get_nth_font_folder_(dir_id, &info);
	path->SetTo(info->pathname, filename);
}	


char*
FontHandler::ConvertTrueTypeFont(BPath fontfile)
{
	if(fConversionType == FF_Type1) {
		return ConvertTTtoType1(fontfile);
	}
	
	return NULL;
}

char*
FontHandler::ConvertTTtoType1(BPath fontfile)
{
	BString cnvFont = tmpnam(NULL);
	if(cnvFont.Length() == 0) {
		return NULL;
	}
	
	ttf2pt1* ttconv = new ttf2pt1;
	ttconv->convert_tt(fontfile.Path(), cnvFont.String());	
	delete ttconv;
	
	/* get the name of this font */
	cnvFont << ".pfa";
	BFile file(cnvFont.String(), B_READ_ONLY);
	if(file.InitCheck() != B_OK) {
		return NULL;
	}


	char fontNameBuf[256];
	file.ReadAttr("FontName", B_STRING_TYPE, 0, fontNameBuf, 256);

	AppendToFontFile(cnvFont.String());
	unlink(cnvFont.String());
	return (strdup(fontNameBuf));
}

void
FontHandler::AppendToFontFile(const char *font)
{
	int32 fd = open(font, O_RDONLY);
	if(fd < 0) {
		return;
	}

	status_t rv;
	
	int32 len;
	char buf[1024];
	while((len = read(fd, buf, sizeof(buf))) > 0) {
		rv = fFontFile->Write(buf, len);
		if(rv < 0 || rv != len) {
			P("Write returned: %ld\n", rv);
		}
	}	

	close(fd);
}

char*
FontHandler::HandleType1Font(BPath path)
{
	status_t isPFB = IsPFB(path);
	if(isPFB == 1) {
		BPath pfaPath;
		if(ConvertPFBtoPFA(path, pfaPath) != B_OK) {
			return NULL;
		}
		path = pfaPath;
	}

	char *font = ParseT1forFontName(path.Path());
	AppendToFontFile(path.Path());

	if(isPFB) {
		unlink(path.Path());
	}
		
	return(font);
}


const int32 PS_BUFFER_SIZE = 128;

static char*
extract_def(char *buffer, int32 *index) {
	char		*res;
	int32		i, j;
	
	i = *index;
	while ((buffer[i] != '/') && (i<(2*PS_BUFFER_SIZE))) i++;

	if (i == (2*PS_BUFFER_SIZE)) {
		return NULL;
	}

	j = i+1;
	while ((isalnum(buffer[j]) || buffer[j] == '-' || buffer[j] == '_')
			&& (j<(2*PS_BUFFER_SIZE))) {
		j++;
	}

	if (j == (2*PS_BUFFER_SIZE)) {
		return NULL;
	}
	res = (char*)malloc(j-i);
	if(res == NULL) {
		return NULL;
	}
	
	memcpy(res, buffer+i+1, j-i-1);
	res[j-i-1] = 0;
	*index = j+1;
	return res;
}

char*
FontHandler::ParseT1forFontName(const char *font)
{
	const char FontName[] = "FontName";
	char *fontName = NULL;
	char buffer[2*PS_BUFFER_SIZE+1];
	
	FILE* fp = fopen(font, "rb");
	if(fp == 0L) {
		return NULL;
	}
	
	if (fseek(fp, 0L, SEEK_SET) != 0) {
		goto exit; 	// fontName is NULL
	}
	if (fread(buffer+PS_BUFFER_SIZE, 1, PS_BUFFER_SIZE, fp) != PS_BUFFER_SIZE) {
		goto exit; 	// fontName is NULL
	}
	buffer[2*PS_BUFFER_SIZE] = 0;

	int32 i, j;
	for(;;) {
		memcpy(buffer, buffer+PS_BUFFER_SIZE, PS_BUFFER_SIZE);
		if (fread(buffer+PS_BUFFER_SIZE, 1, PS_BUFFER_SIZE, fp) != PS_BUFFER_SIZE) {
			goto exit; 	// fontName is NULL
		}
		i = 0;
		while (i < PS_BUFFER_SIZE) {
			if ((buffer[i] == '/') && isalpha(buffer[i+1])) {

				j = i+1;
				while (isalpha(buffer[j]) && (j < (i+PS_BUFFER_SIZE))) j++;

				if (strncmp(buffer+i+1, FontName, j-i-1) == 0) {
					fontName = extract_def(buffer, &j);
					goto exit;	// handles both error & non-error
				}
				
				i = j;
				
			}
			else i++;
		}
	}

exit:	

	fclose(fp);
	return fontName;
}

status_t
FontHandler::WriteFontAliases(BDataIO *toFile)
{
	BString str;
	
	for(int32 i=0; i < 128; i++) {
		if(fFontAliases[i] == 0) {
			continue;
		}
		
		P("Writing Font Alias #%ld\n", i);
		str = "/F";
		str << i;
		str << " /" << fFontAliases[i];
		str << " def\n";

		P("\t...alias = %s\n", str.String());
		toFile->Write(str.String(), str.Length());
		
		free(fFontAliases[i]);
	}
	return B_OK;
}

status_t
FontHandler::IsPFB(BPath font)
{
	uchar buffer[1];
	status_t rv = 0;

	FILE* fp = fopen(font.Path(), "rb");
	if(fp == 0L) {
		return B_ERROR;
	}
	
	if (fseek(fp, 0L, SEEK_SET) != 0) {
		rv = B_ERROR;
		goto exit;
	}

	if (fread(buffer, 1, sizeof(buffer), fp) != sizeof(buffer)) {
		rv = B_ERROR;
		goto exit;
	}

	if(buffer[0] != '%%') {
		rv = 1;
	} else {
		rv = 0;
	}
	
exit:
	fclose(fp);
	return rv;
}

status_t
FontHandler::ConvertPFBtoPFA(const BPath pfbPath, BPath& pfaPath)
{
	const int16 BUF_SIZE = 4096;
	const uint8 FLG_ASCII = 1;
	const uint8 FLG_BINARY = 2;
	const uint8 FLG_EOF = 3;
	status_t rv = B_OK;

	BString pfaString = tmpnam(NULL);
	if(pfaString.Length() == 0) {
		return B_ERROR;
	} else {
		pfaPath.SetTo(pfaString.String());
	}
	
	FILE *pfaFP = fopen(pfaString.String(), "wa");
	if(pfaFP == 0L) {
		return B_ERROR;
	}
	
	FILE *pfbFP = fopen(pfbPath.Path(), "rb");
	if(pfbFP == 0L) {
		fclose(pfaFP);
		return B_ERROR;
	}

	if (fseek(pfbFP, 0L, SEEK_SET) != 0) {
		rv = B_ERROR;
		goto exit;
	}
	
	uchar  buf[BUF_SIZE];
	uchar  type;
	uint32 length, nread;
	int16  nbytes, rc;


	for ( ; ; ) {
		if ((rc = fread((char *) buf, 1, 6, pfbFP)) < 2) {
			rv = B_ERROR;
			goto exit;			
		}

		if (buf[0] != 128) {
			rv = B_ERROR;
			goto exit;			
		}

		if ((type = buf[1]) == FLG_EOF) {
			rv = B_OK;
			goto exit;
		}		

		if (rc != 6) {
			rv = B_ERROR;
			goto exit;			
		}

		length = little4(&buf[2]);
		for (nread = 0; nread < length; nread += rc) {
			nbytes = min(BUF_SIZE, length - nread);
			if (!(rc = fread((char *) buf, 1, nbytes, pfbFP))) {
				rv = B_ERROR;
				goto exit;			
			}

			switch (type)
			{
			 case FLG_ASCII:
				outascii(pfaFP, buf, (uint32)rc);
				break;
			 case FLG_BINARY:
				outbinary(pfaFP, buf, (uint32)rc);
				break;
			 default:
			 {
				rv = B_ERROR;
				goto exit;
				break;
			 }
			}
		}
	}		

exit:
	fclose(pfaFP);
	fclose(pfbFP);
	return rv;
}

void
FontHandler::outbinary(FILE *fout, uchar *buf, uint32 len)
{
	uint32 i;
	static const int8 BYTES_PER_LINE = 32;
	
	for (i = 0; i < len; i++) {
		fprintf(fout, "%02x", buf[i]);
		if (!((i + 1) % BYTES_PER_LINE))
			fprintf(fout, "\n");
	}

	if (i % BYTES_PER_LINE)
		fprintf(fout, "\n");
}

void
FontHandler::outascii(FILE *fout, uchar *buf, uint32 len)
{
	uint32 i;

	for (i = 0; i < len; i++) {
		if (buf[i] == '\r')
			putc('\n', fout);
		else
			putc((char) buf[i], fout);
   }
}

uint32
FontHandler::little4(uchar *buf)
{
   return (uint32) buf[0] +
         ((uint32) buf[1] << 8) +
         ((uint32) buf[2] << 16) +
         ((uint32) buf[3] << 24);
}
