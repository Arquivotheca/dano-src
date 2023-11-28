// query.cpp
//
// A shell utility for somewhat emulating the Tracker's "Find By Formula"
// functionality.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <Path.h>
#include <Query.h>
#include <Entry.h>
#include <String.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <SupportDefs.h>
#include <getopt.h>
#include <sys/utsname.h>

// characters to backslash escape in filenames
static const char* METACHARS = "()[]*\'\"\\^? \t\n\r|&!<>;~$@";

// option flags
bool o_all_volumes = false;			// query on all volumes
bool o_no_escape = false;			// don't escape meta characters

void
usage(void)
{
	printf("usage:  query [ -e ] [ -a || -v volume ] expression\n");
	printf("        -e         don't escape meta-characters\n");
	printf("        -a         perform the query on all volumes\n");
	printf("        -v <file>  perform the query on just one volume;\n");
	printf("                   <file> can be any file on that volume.\n");
	printf("                   defaults to the boot volume.\n");
	printf(" hint:  query \'name=foo\' will find a file named \"foo\"\n");
	exit(0);
}

void
perform_query(BVolume *v, const char *predicate)
{
	BQuery query;
	status_t ret;
	bool cheated = false;
	int hits = 0;
	
	// Set up the volume and predicate for the query.
	query.SetVolume(v);
	query.SetPredicate(predicate);

	ret = query.Fetch();
	if (ret == B_BAD_VALUE) {
		/* this is so people can do 'query foo' when they
		 * really mean 'query name=foo'.
		 */
		cheated = true;
		char *cp = (char *)malloc(strlen(predicate) + 6);
		if (cp) {
			sprintf(cp, "name=%s", predicate);
			query.SetPredicate(cp);
			free(cp);
			ret = query.Fetch();
		}
	}
	if (ret < B_OK) {
		printf("query: bad query expression\n");
		return;
	}
	
	BEntry e;
	BPath p;
	while (query.GetNextEntry(&e) == B_OK) {
		e.GetPath(&p);
		BString s = p.Path();
		if (!o_no_escape)
		{
			s.CharacterEscape(METACHARS, '\\');
		}
		printf("%s\n", s.String());

		hits++;
	}

	if (hits == 0 && cheated) {
		fprintf(stderr, "query: correct syntax is \'query name=filename\'.\n");
		fprintf(stderr, "       tried searching for a file called \"%s\""
						" but came up empty.\n", predicate);
	}
	
	return;
}

int
main(int32 argc, char **argv)
{
	// Make sure we have the minimum number of arguments.
	if (argc < 2) usage();	

	// Which volume do we make the query on?
	char volume_path[B_FILE_NAME_LENGTH];
	// Default to the volume of the current working directory.
	strcpy(volume_path, ".");
	
	// Parse command-line arguments.
	int32 opt;
	while ((opt = getopt(argc, argv, "eav:")) != -1) {
		switch(opt) {
		case 'e':
			o_no_escape = true;
			break;
			
		case 'a':
			o_all_volumes = true;
			break;
		
		case 'v':
			strncpy(volume_path, optarg, B_FILE_NAME_LENGTH);
			break;
		
		default:
			usage();
			break;
		}
	}

	if (argc == optind) {
		printf("query: you must specify a query string.\n");
		return 0;
	}

	BVolume query_volume;
	
	if (!o_all_volumes) {
		// Find the volume that the query should be performed on,
		// and set the query to it.
		BEntry e(volume_path);
		if (e.InitCheck() != B_OK) {
			printf("query: %s is not a valid file\n", volume_path);
			exit(0);
		}
		e.GetVolume(&query_volume);

		if (!query_volume.KnowsQuery())
			printf("query: volume containing %s is not query-enabled\n", volume_path);
		else
			perform_query(&query_volume, argv[optind]);
		
		exit(0);
	}			

	// Okay, we want to query all the disks -- so iterate over
	// them, one by one, running the query.
	BVolumeRoster volume_roster;
	while (volume_roster.GetNextVolume(&query_volume) == B_OK) {
		// We don't print errors here -- this will catch /pipe and
		// other filesystems we don't care about.
		if (query_volume.KnowsQuery())
			perform_query(&query_volume, argv[optind]);
	}

	exit(0);	
}
