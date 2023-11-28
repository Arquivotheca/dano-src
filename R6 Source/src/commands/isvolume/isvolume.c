/*
	This program will report whether or not a given
	volume matches certain criteria such as whether
	it is read-only, has attributes, etc.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fs_info.h>

extern int _kstatfs_(dev_t dev, long *pos, int fd, const char *path,
					struct fs_info *fs);

static void
usage(void)
{
	fprintf(stderr, "Usage: isvolume {-OPTION} [volumename]\n");
	fprintf(stderr, "   Where OPTION is one of:\n");
	fprintf(stderr, "           -readonly   - volume is read-only\n");
	fprintf(stderr, "           -query      - volume supports queries\n");
	fprintf(stderr, "           -attribute  - volume supports attributes\n");
	fprintf(stderr, "           -mime       - volume supports MIME information\n");
	fprintf(stderr, "           -shared     - volume is shared\n");
	fprintf(stderr, "           -persistent - volume is backed on permanent storage\n");
	fprintf(stderr, "           -removable  - volume is on removable media\n");
	fprintf(stderr, "   If the option is true for the named volume, 'yes' is printed\n");
	fprintf(stderr, "   and if the option is false, 'no' is printed. Multiple options\n");
	fprintf(stderr, "   can be specified in which case all of them must be true.\n\n");
	fprintf(stderr, "   If no volume is specified, the volume of the current directory is assumed.\n");
	exit(0);
}

int
main(int argc, char **argv)
{
	fs_info		info;
	int         flag_mask = 0, i;	
	char 		*volname = NULL;
	dev_t		dev = -1;
	struct stat st;

	if (argc < 2)
		usage();

	for(i=1; i < argc; i++) {
		if (strcmp(argv[i], "-readonly") == 0)
			flag_mask |= B_FS_IS_READONLY;
		else if (strcmp(argv[i], "-query") == 0)
			flag_mask |= B_FS_HAS_QUERY;
		else if (strcmp(argv[i], "-attribute") == 0)
			flag_mask |= B_FS_HAS_ATTR;
		else if (strcmp(argv[i], "-mime") == 0)
			flag_mask |= B_FS_HAS_MIME;
		else if (strcmp(argv[i], "-shared") == 0)
			flag_mask |= B_FS_IS_SHARED;
		else if (strcmp(argv[i], "-persistent") == 0)
			flag_mask |= B_FS_IS_PERSISTENT;
		else if (strcmp(argv[i], "-removable") == 0)
			flag_mask |= B_FS_IS_REMOVABLE;
		else if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0)
			usage();
		else if (argv[i][0] == '-') {
			fprintf(stderr, "%s: option %s is not understood (use --help for help)\n", 
					argv[0], argv[i]);
			
			exit(0);
		} else {
			break;    /* it's a volume name */
		}
	}

	if (flag_mask == 0) {
		fprintf(stderr, "%s: no volume flags specified! (use --help for help)\n", 
				argv[0]);
		exit(1);
	}

	if (i < argc)
		volname = &argv[i][0];
	else
		volname = "./";

	/* first try it as a volume name... */
	if (_kstatfs_(-1, NULL, -1, volname, &info) != 0) {
		/* ok, that failed, try to get the dev_t for the name specified */
		if (stat(volname, &st) == 0 || _kstatfs_(st.st_dev, NULL, -1, NULL, &info) != 0) {
			fprintf(stderr, "%s: can't get information about volume: %s\n", &argv[0][0], volname);
			exit(1);
		}
	}

#if 0   /* debugging */
	printf("volume %s: info.flags 0x%x flag mask 0x%x\n", volname, 
		   info.flags, flag_mask);
	printf("%c%c%c%c%c%c%c\n", 
		(info.flags & B_FS_HAS_QUERY) ? 'Q' : '-',
		(info.flags & B_FS_HAS_ATTR) ? 'A' : '-',
		(info.flags & B_FS_HAS_MIME) ? 'M' : '-',
		(info.flags & B_FS_IS_SHARED) ? 'S' : '-',
		(info.flags & B_FS_IS_PERSISTENT) ? 'P' : '-',
		(info.flags & B_FS_IS_REMOVABLE) ? 'R' : '-',
		(info.flags & B_FS_IS_READONLY) ? '-' : 'W');
#endif

	if ((info.flags & flag_mask) == flag_mask) {
		printf("yes\n");
		return 0;
	} else {
		printf("no\n");
		return 1;
	}
}
