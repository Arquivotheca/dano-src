
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Path.h>
#include <File.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <String.h>
#include <print/RawPrintJob.h>

#include "PrintEnv.h"


static void usage(const char *name)
{
	fprintf(stderr, "usage: %s [-p printer] [filename] ...  # print files\n",				name);
	fprintf(stderr, "       %s -l                           # list available printers\n",	name);
	fprintf(stderr, "       %s --help                       # this help message\n",			name);
	exit(1);
}

void probe()
{
	BPath settings;
	find_directory(B_USER_PRINTERS_DIRECTORY, &settings);

	// And then watch the jobs for each printers
	status_t err;
	BEntry an_entry;
	BDirectory dir(settings.Path());
	while ((err = dir.GetNextEntry(&an_entry)) >= 0)
	{ // TODO: Should make sure it is actually a printer
		char name[256];
		if (an_entry.GetName(name) == B_OK)
			printf("%s\n", name);
	}
}

int main(int argc, char **argv)
{
    const char *printer = NULL;
	char *myname = argv[0];
	int list = 0;

	// Get the default printer
	BString default_printer;
	if (get_default_printer(default_printer) == B_OK)
		printer = default_printer.String();
	

	for (argc--, argv++; argc > 0 && **argv == '-'; argc--, argv++) {
		if (strcmp(argv[0], "-p") == 0) {
			printer = argv[1];
			argc--, argv++;
		} else if (strcmp(argv[0], "-l") == 0) {
			list++;
		} else if (strcmp(argv[0], "--help") == 0) {
			usage(myname);
		} else {
			usage(myname);
		}
	}
	if (list) {
		printf("probing available printers...\n");
		probe();
		exit(0);
	}
	if (printer == NULL) {
		fprintf(stderr, "No printer specified.\n");
		exit(1);
	}

    for ( ;argc > 0; argv++, argc--)
    {
    	status_t err;
    	BFile file(argv[0], B_READ_ONLY);
    	if ((err = file.InitCheck()) != B_OK) {
    		fprintf(stderr, "Can't open file %s (%s)\n", argv[0], strerror(err));
			exit(1);
		}

    	BRawPrintJob job(printer, argv[0]);
    	if ((err = job.InitCheck()) != B_OK) {
    		fprintf(stderr, "Can't open printer %s (%s)\n", printer, strerror(err));
			exit(1);
		}
		
		if ((err = job.BeginJob()) != B_OK) {
			fprintf(stderr, "Error creating the spoolfile on %s (%s)\n", printer, strerror(err));
			exit(1);
		}

				size_t read, sent;
				size_t length = 65536;
				void *buffer = (char *)malloc(length);
				if (buffer)	{
					do {
						if ((read = file.Read(buffer, length)) > 0)
							sent = job.Print(buffer, read);
					} while ((read == length) && (sent == read));
					if (read < 0)		err = read;
					else if (sent < 0)	err = sent;
					free(buffer);
				} else {
					err = B_NO_MEMORY;
				}

		if ((err = job.CommitJob()) != B_OK) {
			fprintf(stderr, "Error closing the spoolfile on %s (%s)\n", printer, strerror(err));
			exit(1);
		}
	}
	return (0);
}
