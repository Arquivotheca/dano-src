
#include <textencoding/BTextEncoding.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
using namespace B::TextEncoding;

int main(int argc, char *argv[]) {
	if (argc != 5) {
		printf("usage: convert_test inFile outFile conversionName\n");
		return B_ERROR;
	}
	FILE *in = fopen(argv[1], "r");
	FILE *out = fopen(argv[2], "w+");
	
	printf("in: %s out:%s build codec: %s\n", argv[1], argv[2], argv[3]);
	BTextEncoding enc(argv[3]);
	
	
	if (!in || !out || !enc.HasCodec()) {
		printf("no in, out or codec\n");
		printf("in: %x out:%x has codec %d\n", in, out, enc.HasCodec());
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
	conversion_info info;
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
				status = enc.ConvertToUnicode(inBuf, &inSize, outBuf, &outSize, info);
			else
				status = enc.ConvertFromUnicode(inBuf, &inSize, outBuf, &outSize, info);
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