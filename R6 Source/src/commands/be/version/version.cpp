#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <AppFileInfo.h>
#include <Application.h>

#include "verinfo.h"

int usage(int exitcode);

int main(int argc, char *argv[])
{
	BApplication	app("application/x-Be.vnd.version");
	status_t		err = 0;

//	char n[5];  /* use this later on for -n */
	int optc;
	int errorcount = 0;
	
	version_info info;
	version_kind kind = B_APP_VERSION_KIND;


/* If non-zero, print the version string in the format used by setversion.  */
	int numerical_output = 0;
	
/* If non-zero, print the verbose version string.  */
	int long_version = 0;

/* If non-zero, print the version of this command on standard output and exit.  */
	int show_own_version =  0;

/* If non-zero, display usage information and exit.  */
	int show_help = 0;
	
	struct option const longopts[] =
	{
		{"long", no_argument, &long_version, 1},
		{"numerical", no_argument, &numerical_output, 1},
		{"system", no_argument, (int *)&kind,  B_SYSTEM_VERSION_KIND },
		{"help", no_argument, &show_help, 1},
		{"version", no_argument, &show_own_version, 1},
		{NULL, 0, NULL, 0}
	};
	
		
	while ( (optc = getopt_long(argc,argv,"hlns",longopts,(int * )0)) != EOF)
	{
		switch (optc)
			{
			case 'h':
				usage (0);
			case 'l':
				long_version = 1;
				break;
			case 'n':
				numerical_output = 1;
				break;
			case 's':
				kind = B_SYSTEM_VERSION_KIND;
				break;
/*
todo:  add ability to select which field(s) -n gives you. 
*/

			default:
				break;
			}
	};
	
	if ( show_help ) usage (0);
	
	if ( show_own_version ) {
		BFile file(argv[0],B_READ_ONLY);
		BAppFileInfo appFileInfo(&file);

		err = appFileInfo.GetVersionInfo(&info, kind);
		if (err == B_OK) printf("%s\n", info.short_info);
		exit (0);
	}
	
	for (int32 i = optind; i < argc; i++ )
	{
		BFile file(argv[i],B_READ_ONLY);
		BAppFileInfo appFileInfo(&file);

		err = appFileInfo.GetVersionInfo(&info, kind);
				
		if (err == B_OK) {
			if( long_version )
			{
				printf("%s\n", info.long_info);
			} else if( numerical_output )
			{
				printf("%lu %lu %lu %s %lu\n", info.major, info.middle, info.minor, variety[info.variety], info.internal );
			} else
			{
				printf("%s\n", info.short_info);
			};
		} else if ( err == B_NO_INIT ) {
			errorcount++;
			printf("Couldn't get file info!\n");
		} else {
			errorcount++;
			printf("Version unknown!\n");
		};
	}
	return errorcount ? 1 : 0;

}


int usage(int exitcode)
{	
	printf("usage: version [OPTION] FILNAME [FILENAME2, ...]\n");
	printf("Returns the version of a file.\n\n");

	printf("\t-h, --help\t\tthis usage message\n");
	printf("\t-l, --long\t\tprint long version information of FILENAME\n");
	printf("\t-n, --numerical\t\tprint in numerical mode\n\t\t\t\t(Major miDdle miNor Variety Internal)\n");
	printf("\t-s, --system\t\tprint system version instead of app version\n");	
	printf("\t--version\t\tprint version information for this command\n");	

	exit (exitcode);
}
