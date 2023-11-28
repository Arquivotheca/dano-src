// xtou	by Hiroshi Lockheimer

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SupportDefs.h>
#include <UTF8.h>


const uint32	kSrcBufferSize = 256;
const uint32	kDstBufferSize = 512;

const char		*kEncodingSuffixes[] = { ".iso1",
										 ".iso2",
										 ".iso3",
										 ".iso4",
										 ".iso5",
										 ".iso6",
										 ".iso7",
										 ".iso8",
										 ".iso9",
										 ".iso10",
										 ".mac",
										 ".sjis",
										 ".euc",
										 ".jis",
										 ".win",
										 ".uni",
										 ".koi8r",
										 ".win1251",
										 ".dos866",
										 ".dos",
										 ".euckr"
										};


void HelpAndExit();
//void CreateMappingsFile();

int
main(
	int		argc,
	char	*argv[])
{
	if (argc == 1)
		HelpAndExit();

	uint32	conversion = B_ISO1_CONVERSION;
	bool	toUTF8 = true;
	bool	forceNewline = false;

	for (int32 i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 't':
					toUTF8 = false;
					// fall thru

				case 'f':
				{
					if ((i + 1) > argc)
						HelpAndExit();

					const char *encoding = argv[++i];

					if (strcmp("iso1", encoding) == 0)
						conversion = B_ISO1_CONVERSION;
					else if (strcmp("iso2", encoding) == 0)
						conversion = B_ISO2_CONVERSION;
					else if (strcmp("iso3", encoding) == 0)
						conversion = B_ISO3_CONVERSION;
					else if (strcmp("iso4", encoding) == 0)
						conversion = B_ISO4_CONVERSION;
					else if (strcmp("iso5", encoding) == 0)
						conversion = B_ISO5_CONVERSION;
					else if (strcmp("iso6", encoding) == 0)
						conversion = B_ISO6_CONVERSION;
					else if (strcmp("iso7", encoding) == 0)
						conversion = B_ISO7_CONVERSION;
					else if (strcmp("iso8", encoding) == 0)
						conversion = B_ISO8_CONVERSION;
					else if (strcmp("iso9", encoding) == 0)
						conversion = B_ISO9_CONVERSION;
					else if (strcmp("mac", encoding) == 0)
						conversion = B_MAC_ROMAN_CONVERSION;
					else if (strcmp("sjis", encoding) == 0)
						conversion = B_SJIS_CONVERSION;
					else if (strcmp("euc", encoding) == 0)
						conversion = B_EUC_CONVERSION;
					else if (strcmp("jis", encoding) == 0)
						conversion = B_JIS_CONVERSION;
					else if (strcmp("win", encoding) == 0)
						conversion = B_MS_WINDOWS_CONVERSION;
					else if (strcmp("uni", encoding) == 0)
						conversion = B_UNICODE_CONVERSION;
					else if (strcmp("koi8r", encoding) == 0)
						conversion = B_KOI8R_CONVERSION;
					else if (strcmp("win1251", encoding) == 0)
						conversion = B_MS_WINDOWS_1251_CONVERSION;
					else if (strcmp("dos866", encoding) == 0)
						conversion = B_MS_DOS_866_CONVERSION;
					else if (strcmp("dos", encoding) == 0)
						conversion = B_MS_DOS_CONVERSION;
					else if (strcmp("euckr", encoding) == 0)
						conversion = B_EUC_KR_CONVERSION;
					else
						HelpAndExit();
					continue;
				}

				case 'n':
					forceNewline = true;
					continue;
			}
		}

		FILE *srcFile = fopen(argv[i], "r");

		if (srcFile == NULL) 
			fprintf(stderr, "Error opening \"%s\"\n", argv[i]);
		else {
			const char *suffix = (toUTF8) ? ".utf8" : 
											kEncodingSuffixes[conversion];

			size_t	srcFileLen = strlen(argv[i]);
			char	*dstFileName = (char *)malloc(srcFileLen + 
												  strlen(suffix) + 
												  1);

			strcpy(dstFileName, argv[i]);
			strcpy(dstFileName + srcFileLen, suffix);

			FILE *dstFile = fopen(dstFileName, "w");

			if (dstFile == NULL) 
				fprintf(stderr, "Error creating \"%s\"\n", dstFileName);
			else {
				char	src[kSrcBufferSize];
				int32	numRead;
				int32	state = 0;
				while ( (numRead = fread(src, sizeof(char), 
										 kSrcBufferSize, srcFile)) > 0 ) {
					int32	srcLen = numRead;
					char	dst[kDstBufferSize];
					int32	dstLen = kDstBufferSize;

					if (toUTF8)
						convert_to_utf8(conversion, 
										src, 
										&srcLen, 
										dst, 
										&dstLen,
										&state);  	
					else
						convert_from_utf8(conversion,
										  src,
										  &srcLen,
										  dst,
										  &dstLen,
										  &state);

					if (srcLen < 1)
						break;

					if (forceNewline) {
						for (int32 n = 0; n < dstLen; n++) {
							if (dst[n] == '\r')
								dst[n] = '\n';
						}
					}

					fwrite(dst, sizeof(char), dstLen, dstFile);
					fseek(srcFile, srcLen - numRead, SEEK_CUR);
				}

				fclose(dstFile);
			}

			fclose(srcFile);
			free(dstFileName);
		}	
	}

	exit(0);
}


void
HelpAndExit()
{
	fprintf(stderr, "Usage : xtou [OPTION] file1 [file2...]\n"
					"  -f      Encoding specifies source      (x -> UTF-8)\n"
					"  -t      Encoding specifies destination (UTF-8 -> x)\n"
					"  -n      Convert carriage-returns to newlines\n"
					"Encodings:\n"
					"  iso1    ISO 8859-1\n"
					"  iso2    ISO 8859-2\n"
					"  iso3    ISO 8859-3\n"
					"  iso4    ISO 8859-4\n"
					"  iso5    ISO 8859-5\n"
					"  iso6    ISO 8859-6\n"
					"  iso7    ISO 8859-7\n"
					"  iso8    ISO 8859-8\n"
					"  iso9    ISO 8859-9\n"
					"  mac     Mac Roman\n"
					"  sjis    Shift-JIS\n"
					"  euc     EUC\n"
					"  jis     JIS\n"
					"  win     MS-Windows\n"
					"  uni     Unicode\n"
					"  koi8r   Russian KOI8-R\n"
					"  win1251 Russian Windows\n"
					"  dos866  Russian DOS\n"
					"  dos     MS-DOS\n"
					"  euckr   EUC Korean\n");

	exit(0);
}


/*
void
CreateMappingsFile()
{
	const int32 NUM_CHARS = 0xFFFF;

	uint16 *table = (uint16 *)malloc(sizeof(uint16) * NUM_CHARS);
	for (int32 i = 0; i < NUM_CHARS; i++)
		table[i] = 0;
	for (int32 i = 0; i < 256; i++)
		table[i] = i;

	FILE *srcFile = fopen("/boot/SHIFTJIS.TXT", "r");
	while (true) {
		char buffer[200];
		fgets(buffer, 200, srcFile);
		if (feof(srcFile)) 
			break;
			
		if (strlen(buffer) != 0) {
			int32 index = 0;
			int32 unicode = 0;
			sscanf(buffer, "0x%04X	0x%04X\n", &index, &unicode);
			table[index] = unicode;
		}
	}
	fclose(srcFile);

	FILE	*dstFile = fopen("/boot/SHIFTJIS.MAP", "w");
	int32	i = 0;
	while (i < 256) {
		fprintf(dstFile, "0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X,\n", table[i], table[i + 1], table[i + 2], table[i + 3], table[i + 4], table[i + 5], table[i + 6], table[i + 7], table[i + 8], table[i + 9], table[i + 10], table[i + 11], table[i + 12], table[i + 13], table[i + 14], table[i + 15]);
		i += 16;
	}

	fprintf(dstFile, "-------------------------------\n");
	i = 0x8100;
	while (i < 0xA000) {
		fprintf(dstFile, "0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X,\n", table[i], table[i + 1], table[i + 2], table[i + 3], table[i + 4], table[i + 5], table[i + 6], table[i + 7], table[i + 8], table[i + 9], table[i + 10], table[i + 11], table[i + 12], table[i + 13], table[i + 14], table[i + 15]);
		i += 16;
	}

	fprintf(dstFile, "-------------------------------\n");
	i = 0xE000;
	while (i < 0xF000) {
		fprintf(dstFile, "0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X,\n", table[i], table[i + 1], table[i + 2], table[i + 3], table[i + 4], table[i + 5], table[i + 6], table[i + 7], table[i + 8], table[i + 9], table[i + 10], table[i + 11], table[i + 12], table[i + 13], table[i + 14], table[i + 15]);
		i += 16;
	}

	fclose(dstFile);

	free(table);

	exit(0);
}
*/
