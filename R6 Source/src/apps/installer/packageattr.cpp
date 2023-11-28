#include <String.h>
#include <stdlib.h>
#include <stdio.h>
#include "OptionalPackage.h"
#include <StorageKit.h>
#include <Application.h>
#include <TranslationUtils.h>

int ARGC;
char **ARGV;

void usage()
{
	BString docs;
	docs << "Usage: " << ARGV[0] << " <directory> <name> <description> <group>\n";
	docs << "       <on by default> <always on> <size> [icon file]\n";
	docs << "All of the above options, except for the icon file, must be given.\n";
	docs << "The directory option should be the name of the directory on which to\n";
	docs << "set attributes.  The name, description, and group options should be\n";
	docs << "strings.  The on by default and always on options should be either\n";
	docs << "0 or 1.  The size should be a string representation of the number of\n";
	docs << "bytes needed to install all the files in the package, including their\n";
	docs << "attributes.  If specified, the icon should be a 21x15-pixel 8-bit \n";
	docs << "image file of a format supported by a Translation Kit plugin on your\n";
	docs << "machine.\n"; 
	fprintf(stdout, docs.String());
}

int main(int argc, char *argv[])
{
	BApplication *app = new BApplication("application/x-vnd.Be.packageattr");
	BDirectory dir;
	BString name, desc, group, iconname;
	int32 on, alwayson;
	off_t size;
	BBitmap *icon(NULL);
	
	ARGC = argc;
	ARGV = argv;

	// check for right number of args
	if (!(argc == 8 || argc == 9)) {
		fprintf(stdout, "ERROR: Incorrect number of parameters.\n");
		usage();
		exit(-1);
	}

	// get directory to use
	dir.SetTo(ARGV[1]);
	BEntry entry;	
	if (dir.GetEntry(&entry) != B_OK) {
		fprintf(stdout, "ERROR: directory argument specified is bad.\n");
		usage();
		exit(-2);
	}

	// get name, desc, & group
	name << ARGV[2];
	desc << ARGV[3];
	group << ARGV[4];
	
	// get on and always on
	on = atoi(ARGV[5]);
	alwayson = atoi(ARGV[6]);

	// get size
	size = atoi(ARGV[7]);
	
	// get icon, if specified
	if (ARGC == 9) {
		iconname << ARGV[8];
		icon = BTranslationUtils::GetBitmap(iconname.String());
	}
	
	// write to disk
	OptionalPackage *pkg = new OptionalPackage(ARGV[1], name.String(), desc.String(),
		group.String(), (on != 0), (alwayson != 0), size, icon);
	pkg->WriteToDirectory(&dir);
	
	return(0);
}

