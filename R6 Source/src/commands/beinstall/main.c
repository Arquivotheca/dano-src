#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>

#include "main.h"

const char *COPYRIGHT_STRING="Copyright (c) 2001; Be, Incorporated. All Rights Reserved.\n";

const char *VERSION_STRING="0.1";
/* Version history:
 *  0.1 - intial version based on copyattr and create_dir.
*/

const char *argv0;

static void help(void)
{
	printf("%s - Version %s\n", argv0, VERSION_STRING);
	printf(COPYRIGHT_STRING);
	printf("\n");
	printf(
		"Usage:\n"
		"  -l, --dereference-links : copy symlinks as files\n"
		"  -d, --directory : all arguments are directories to create\n"
		"  -D     : create leading directories if needed\n"
		"  -g, --group=GROUP : installed files/dirs are set to group GROUP\n"
		"  -o, --owner=OWNER : installed files/dirs are set to owner OWNER\n"
		"  -m, --mode=MODE : installed files/dirs are set to permission mode MODE (in octal)\n"
		"  -e, --ignore-attributes : ignore Be's extended attributes\n"
		"  -p, --perserve-timestamps : preserve the time of the source file\n"
		"  -s, --strip : silently ignored\n"
		"  -f, --force : delete the target if it already exists\n"
		"  -v, --verbose : show what we're doing\n"
		"      --version : show version text\n"
		"      -- : no further options\n"
		"  -h, --help : show this help text]\n"
	);
	exit(0);
}

void error(const char *text)
{
	perror(text);
}

int main(int argc, char **argv)
{
	static struct beinstall_options options;
	static struct option lopts[] = {
		{"backup", no_argument, NULL, 'b'}, /* unsupported: error */
		{"dereference-links", no_argument, NULL, 'l'},
		{"directory", no_argument, NULL, 'd'},
		{"force", no_argument, NULL, 'f'},
		{"group", required_argument, NULL, 'g'},
		{"ignore-attributes", no_argument, NULL, 'e'},
		{"mode", required_argument, NULL, 'm'},
		{"owner", required_argument, NULL, 'o'},
		{"preserve-timestamps", no_argument, NULL, 'p'},
		{"recursive", no_argument, NULL, 'r'},
		{"strip", no_argument, NULL, 's'}, /* unsupported: ignored */
		{"suffix", required_argument, NULL, 'S'}, /* unsupported: error */
		{"verbose", no_argument, NULL, 'v'},
		{"version-control", required_argument, NULL, 'V'}, /* unsupported: error */
		{"version", no_argument, NULL, 0},
		{"help", no_argument, NULL, 'h'}
	};
	int opt;
	int optidx;
	int err = 0;

	if(argc==0){
		help();
	}
	memset(&options, sizeof(options), '\0');
	argv0 = argv[0];	
	while((opt = getopt_long(argc, argv, "bldfg:em:o:prsS:vV:hD", lopts, &optidx)) != -1) {
		switch(opt) {
			case 'b': /* backup */
			case 'V': /* version-control */
			case 'S': /* suffix */
				error("Backups are unsupported.\n");
				break;
			case 's': /* strip */
				/* strip is silently unsupported */
				break;
			case 'l': /* dereference-links */
				options.dereference_links = 1;
				break;
			case 'f': /* force */
				options.force = 1;
				break;
			case 'e': /* ignore-attributes */
				options.ignore_attributes = 1;
				break;
			case 'p': /* preserve-timestamps */
				options.preserve_timestamps = 1;
				break;
			case 'v': /* verbose */
				options.verbose = 1;
				break;
			case 'r': /* recursive */
				options.recursive = 1;
				break;
			case 'd': /* directory */
				options.install_mode = BEINSTALL_MODE_DIRECTORY;
				break;
			case 'D': /* -D */
				options.directories_if_needed = 1;
				break;
			case 'g': /* group */
				options.pgroup = (unsigned int)strtol(optarg, NULL, 0);
				options.use_pgroup = 1;
				break;
			case 'o': /* owner */
				options.powner = (unsigned int)strtol(optarg, NULL, 0);
				options.use_powner = 1;
				break;
			case 'm': /* mode */
				options.pmode = (unsigned int)strtol(optarg, NULL, 0) & 07777;
				options.use_pmode = 1;
				break;
			case 'h': /* help */
				help();
				break;
			case 0:
				if(!strcmp(lopts[optidx].name, "version")) {
					printf("%s - Version %s\n", argv[0], VERSION_STRING);
					printf(COPYRIGHT_STRING);
					exit(0);
				}
				break;
		}
	}
	if(options.install_mode != BEINSTALL_MODE_DIRECTORY) {
		if(optind + 1 >= argc) {
			error("When not using -d, you need two or more non-option arguments");
		}
		options.dest = argv[argc-1];
		argc--;
	} else if(optind >= argc) {
		options.dest = NULL;
		error("When using -d, you need at least one non-option argument");
	}
	options.sources = (char**)malloc(sizeof(char*) * (argc - optind + 1));
	optidx = 0;
	if(argc - optind > 1 && options.install_mode == BEINSTALL_MODE_FILE) {
		options.install_mode = BEINSTALL_MODE_FILES;
	}
	while(optind < argc) {
		options.sources[optidx++] = argv[optind++];
	}
	options.sources[optidx++] = NULL;

	if(options.install_mode != BEINSTALL_MODE_DIRECTORY) {
		err = copyattr_start(&options);
	} else {
		optidx = 0;
		while(options.sources[optidx]!=NULL){
			err = create_directory(options.sources[optidx++], &options, NO_MODE_OVERRIDE, 0);
		}
		if(err) fprintf(stderr, "%s: %s\n", options.sources[optidx-1], strerror(err));
	}
	return(err);
}
