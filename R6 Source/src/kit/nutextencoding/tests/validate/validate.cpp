
#include <textencoding/BTextEncoding.h>
#include <stdio.h>

using namespace B::TextEncoding;

int main(int argc, char * argv[]) {

	if (argc != 4) {
		printf("usage: validate Name ConversionFile ConversionFormat\n");
		return -1;
	}
	
	FILE *file = fopen(argv[2], "r");
	if (!file) {
		printf("can't open %s!\n", argv[2]);
		return -1;
	}
	
	
	BTextEncoding enc(argv[1]);
	if (!enc.HasCodec()) {
		printf("no %s codec found!\n", argv[1]);
	}
	
	
	const char *fmt = argv[3];
	uint32 format = 0;
	if (strcasecmp(fmt, "0xU\\t0xC") == 0)
		format = 1;
	else if (strcasecmp(fmt, "C\\tU") == 0)
		format = 2;
	else if (strcasecmp(fmt, "U\\tC") == 0)
		format = 3;
	else if (strcasecmp(fmt, "C:U") == 0)
		format = 4;
	else
		format = 0;	
	
	bool pass = true;
	fseek(file, 0L, SEEK_SET);	
	char line[256];
	while (fgets(line, 256, file) != NULL) {
		uint16 charcode = 0xffff;
		uint16 unicode = 0xffff;
		ssize_t cnt = 0;
		if (format == 1)
			cnt = sscanf(line, "%hi\t%hi", &unicode, &charcode);
		else if (format == 2) {
			char C[16];
			char U[16]; 
			strcpy(C, "0x"); 
			strcpy(U, "0x");
			
			cnt = sscanf(line, "%s\t%s", &C[2], &U[2]);
			sscanf(C, "%hi", &charcode);
			sscanf(U, "%hi", &unicode);
		}
		else if (format == 3) {
			char C[16];
			char U[16]; 
			strcpy(C, "0x"); 
			strcpy(U, "0x");
			
			cnt = sscanf(line, "%s\t%s", &U[2], &C[2]);
			sscanf(C, "%hi", &charcode);
			sscanf(U, "%hi", &unicode);
		}
		else if (format == 4) {
			char C[16];
			char U[16]; 
			strcpy(C, "0x"); 
			strcpy(U, "0x");
			
			cnt = sscanf(line, "%4c:%s", &C[2], &U[2]);
			sscanf(C, "%hi", &charcode);
			sscanf(U, "%hi", &unicode);
		}
		else
			cnt = sscanf(line, "%hi\t%hi", &charcode, &unicode);
	
		if (unicode != 0xffff && charcode != 0xffff) {
			uint16 uni = enc.UnicodeFor(charcode);
			if (unicode != uni) {
				printf("problem: 0x%04x->0x%04x not 0x%04x as expected\n", charcode, uni, unicode);
				pass = false;
			}
		}
	
	}

	printf("%s:%s\n", argv[1], ((pass) ? "PASSED" : "FAILED"));
	return (pass) ? B_OK : B_ERROR;

};