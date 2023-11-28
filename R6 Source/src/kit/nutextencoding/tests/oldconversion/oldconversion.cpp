
#include <UTF8.h>
#include <textencoding/TextEncodingNames.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

using namespace B::TextEncoding;

uint32 conversion_for(const char *name) {
	uint32 enc = B_NO_CONVERSION;
	if (strcasecmp(name, ISO1Name) == 0)
		enc = B_ISO1_CONVERSION;

	else if (strcasecmp(name, ISO2Name) == 0)
		enc = B_ISO2_CONVERSION;
	
	else if (strcasecmp(name, ISO3Name) == 0)
		enc = B_ISO3_CONVERSION;
	
	else if (strcasecmp(name, ISO4Name) == 0)
		enc = B_ISO4_CONVERSION;
	
	else if (strcasecmp(name, ISO5Name) == 0)
		enc = B_ISO5_CONVERSION;
	
	else if (strcasecmp(name, ISO6Name) == 0)
		enc = B_ISO6_CONVERSION;
	
	else if (strcasecmp(name, ISO7Name) == 0)
		enc = B_ISO7_CONVERSION;
	
	else if (strcasecmp(name, ISO8Name) == 0)
		enc = B_ISO8_CONVERSION;
	
	else if (strcasecmp(name, ISO9Name) == 0)
		enc = B_ISO9_CONVERSION;
	
	else if (strcasecmp(name, ISO10Name) == 0)
		enc = B_ISO10_CONVERSION;
	
	else if (strcasecmp(name, ISO13Name) == 0)
		enc = B_ISO13_CONVERSION;
	
	else if (strcasecmp(name, ISO14Name) == 0)
		enc = B_ISO14_CONVERSION;
	
	else if (strcasecmp(name, ISO15Name) == 0)
		enc = B_ISO15_CONVERSION;
	
	else if (strcasecmp(name, SJISName) == 0)
		enc = B_SJIS_CONVERSION;
	
	else if (strcasecmp(name, EUCName) == 0)
		enc = B_EUC_CONVERSION;
	
	else if (strcasecmp(name, JISName) == 0)
		enc = B_JIS_CONVERSION;
	
	else if (strcasecmp(name, KOI8RName) == 0)
		enc = B_KOI8R_CONVERSION;
	
	else if (strcasecmp(name, EUCKRName) == 0)
		enc = B_EUC_KR_CONVERSION;
	
	else if (strcasecmp(name, Big5Name) == 0)
		enc = B_BIG5_CONVERSION;
	
	else if (strcasecmp(name, GBKName) == 0)
		enc = B_GBK_CONVERSION;
	
	else if (strcasecmp(name, MSWinName) == 0)
		enc = B_MS_WINDOWS_CONVERSION;
	
	else if (strcasecmp(name, CP1250Name) == 0)
		enc = B_MS_WINDOWS_1250_CONVERSION;
	
	else if (strcasecmp(name, CP1251Name) == 0)
		enc = B_MS_WINDOWS_1251_CONVERSION;
	

	else if (strcasecmp(name, CP1253Name) == 0)
		enc = B_MS_WINDOWS_1253_CONVERSION;
	

	else if (strcasecmp(name, CP1254Name) == 0)
		enc = B_MS_WINDOWS_1254_CONVERSION;
	
	else if (strcasecmp(name, CP1255Name) == 0)
		enc = B_MS_WINDOWS_1255_CONVERSION;
	
	else if (strcasecmp(name, CP1256Name) == 0)
		enc = B_MS_WINDOWS_1256_CONVERSION;
	
	else if (strcasecmp(name, CP1257Name) == 0)
		enc = B_MS_WINDOWS_1257_CONVERSION;
	
	else if (strcasecmp(name, CP1258Name) == 0)
		enc = B_MS_WINDOWS_1258_CONVERSION;
	
	else if (strcasecmp(name, MSDosName) == 0)
		enc = B_MS_DOS_CONVERSION;
	
	else if (strcasecmp(name, CP737Name) == 0)
		enc = B_MS_DOS_737_CONVERSION;
	
	else if (strcasecmp(name, CP775Name) == 0)
		enc = B_MS_DOS_775_CONVERSION;
	
	else if (strcasecmp(name, CP850Name) == 0)
		enc = B_MS_DOS_850_CONVERSION;
	
	else if (strcasecmp(name, CP852Name) == 0)
		enc = B_MS_DOS_852_CONVERSION;
	
	else if (strcasecmp(name, CP855Name) == 0)
		enc = B_MS_DOS_855_CONVERSION;
	
	else if (strcasecmp(name, CP857Name) == 0)
		enc = B_MS_DOS_857_CONVERSION;
	
	else if (strcasecmp(name, CP860Name) == 0)
		enc = B_MS_DOS_860_CONVERSION;
	
	else if (strcasecmp(name, CP861Name) == 0)
		enc = B_MS_DOS_861_CONVERSION;
	
	else if (strcasecmp(name, CP862Name) == 0)
		enc = B_MS_DOS_862_CONVERSION;
	
	else if (strcasecmp(name, CP863Name) == 0)
		enc = B_MS_DOS_863_CONVERSION;
	
	else if (strcasecmp(name, CP864Name) == 0)
		enc = B_MS_DOS_864_CONVERSION;
	
	else if (strcasecmp(name, CP865Name) == 0)
		enc = B_MS_DOS_865_CONVERSION;
	
	else if (strcasecmp(name, CP866Name) == 0)
		enc = B_MS_DOS_866_CONVERSION;
	
	else if (strcasecmp(name, CP869Name) == 0)
		enc = B_MS_DOS_869_CONVERSION;
	
	else if (strcasecmp(name, CP874Name) == 0)
		enc = B_MS_DOS_874_CONVERSION;
	
	else if (strcasecmp(name, MacRomanName) == 0)
		enc = B_MAC_ROMAN_CONVERSION;
	
	else if (strcasecmp(name, MacCenteuroName) == 0)
		enc = B_MAC_CENTEURO_CONVERSION;
	
	else if (strcasecmp(name, MacCroatianName) == 0)
		enc = B_MAC_CROATIAN_CONVERSION;
	
	else if (strcasecmp(name, MacCyrillicName) == 0)
		enc = B_MAC_CYRILLIC_CONVERSION;
	
	else if (strcasecmp(name, MacGreekName) == 0)
		enc = B_MAC_GREEK_CONVERSION;
	
	else if (strcasecmp(name, MacHebrewName) == 0)
		enc = B_MAC_HEBREW_CONVERSION;
	
	else if (strcasecmp(name, MacIcelandName) == 0)
		enc = B_MAC_ICELAND_CONVERSION;
	
	else if (strcasecmp(name, MacTurkishName) == 0)
		enc = B_MAC_TURKISH_CONVERSION;
	
	else if (strcasecmp(name, MacExpertName) == 0)
		enc = B_MAC_EXPERT_CONVERSION;

	return enc;
}


int main(int argc, char *argv[]) {
	if (argc != 5) {
		printf("usage: oldconversion inFile outFile conversionName\n");
		return B_ERROR;
	}
	FILE *in = fopen(argv[1], "r");
	FILE *out = fopen(argv[2], "w+");
	
	uint32 enc = conversion_for(argv[3]);
	
	if (!in || !out || enc == B_NO_CONVERSION) {
		printf("no in, out or codec\n");
		fclose(in); fclose(out);
		return B_ERROR;
	}
	
	bool toUnicode = (strcasecmp(argv[4], "to") == 0);
	
	char inBuf[256];
	int32 inSize = 0;
	char outBuf[256];
	int32 outSize = 0;
	struct stat s;
	fstat(fileno(in), &s);	
	off_t leftToRead = 	s.st_size;

	int32 bytesRead = 0;
	int32 state = 0;
	while (leftToRead > 0) {
		fpos_t pos = s.st_size - leftToRead;
		fsetpos(in, &pos);
		printf("leftToRead: %Ld pos: %Ld fS-lTR: %Ld\n", leftToRead, _ftell(in), s.st_size - leftToRead);
		bytesRead = fread(inBuf, sizeof(char), 256, in);
		inSize = bytesRead;
		printf("bytesRead: %ld\n", bytesRead);
		if (inSize > 0) {
			outSize = 256;
			status_t status = B_OK;
			if (toUnicode)
				status = convert_to_utf8(enc, inBuf, &inSize, outBuf, &outSize, &state);
			else
				status = convert_from_utf8(enc, inBuf, &inSize, outBuf, &outSize, &state);
			printf("status: %s (%ld)\n", strerror(status), status);
			if (status == B_OK) {
				fwrite(outBuf, sizeof(char), outSize, out);
			}
			leftToRead -= inSize;
		}
		else
			break;
	}
	
	fclose(in);
	fclose(out);

};