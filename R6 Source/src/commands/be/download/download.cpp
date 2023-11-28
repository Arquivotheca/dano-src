#include <stdio.h>
#include <www/Protocol.h>
#include <www/ResourceCache.h>

#define BUF_SIZE 0x10000

int main(int argc, char **argv)
{
	int fd = -1;
	char buf[BUF_SIZE];
	int bufOffset = 0;
	
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <url> <local file>\n", argv[0]);
		return 1;
	}		

	InitWebSettings();
		// From www/ResourceCache.h (kit/www/Settings.cpp)
		// Sets up links to proxy bindernodes, among other things

	URL source(argv[1]);
	if (!source.IsValid()) {
		fprintf(stderr, "Invalid URL format\n");
		return 1;
	}

	fd = open(argv[2], O_CREAT | O_TRUNC | O_RDWR, 0644);
	if (fd < 0) {
		perror("opening output file");
		return 1;
	}
	
	Protocol *protocol = 0;
	ssize_t contentLength = 0x7ffffffful;
	char contentType[B_MIME_TYPE_LENGTH];
	for (;;) {
		protocol = Protocol::InstantiateProtocol(source.GetScheme());
		if (protocol == 0) {
			fprintf(stderr, "Unimplemented scheme\n");
			return 1;
		}

		BMessage error;
		if (protocol->Open(source, source, &error, true) < 0) {
			printf("Error opening URL\n");
			return 1;
		}

		contentLength = protocol->GetContentLength();
		bigtime_t delay;
		if (protocol->GetRedirectURL(source, &delay)) {
			delete protocol;
			protocol = 0;
			char tmp[1024];
			source.GetString(tmp, 1024);
			fprintf(stderr, "  redirecting to \"%s\"\n", tmp);
			continue;
		}

		break;
	}
	
	printf("Loading");
	size_t totalRead = 0;
	for (;;) {
		printf(".");
		fflush(stdout);
		if (bufOffset == BUF_SIZE) {
			write(fd, buf, BUF_SIZE);
			bufOffset = 0;
		}

		int sizeRead = protocol->Read(buf + bufOffset, MIN(BUF_SIZE - bufOffset,
			contentLength - totalRead));
		if (sizeRead <= 0) {
			write(fd, buf, bufOffset);
			break;
		}

		bufOffset += sizeRead;
		totalRead += sizeRead;
	}
	
	delete protocol;
	close(fd);
	return 0;
}
