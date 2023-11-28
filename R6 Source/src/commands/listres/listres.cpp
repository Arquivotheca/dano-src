#include <stdio.h>
#include <string.h>

#include <File.h>
#include <Resources.h>

int
main(int argc, char **argv)
{
	BFile			file;
	BResources		rfile;
	int32			index;
	type_code	    	res_type;
	int32			rsrc_id;
	const char		*res_name;
	size_t			res_size;
	status_t		err;
	long			i;
	

	if (argc == 1) {
		fprintf(stderr, "usage: listres 'filename' ['filename' ...]\n");
		return 1;
	}

	for(i=1; i<argc; i++) {
		file.SetTo(argv[i], O_RDONLY);
		err = rfile.SetTo(&file);
		if (err) {
			fprintf(stderr, "listres: error for '%s' (%s)\n", argv[i], strerror(err));
			continue;
		}
		printf("file %s\n", argv[i]);
		printf("  Type      Id     Size                 Name\n");
		printf("  ------ ----- -------- --------------------\n");
		index = 0;
		while (rfile.GetResourceInfo(index++, &res_type, &rsrc_id, &res_name, &res_size)) {
			printf("  '%c%c%c%c' %5d %8d %20s\n",
				(res_type >> 24) & 0xff,
				(res_type >> 16) & 0xff,
				(res_type >> 8) & 0xff,
				(res_type >> 0) & 0xff,
				rsrc_id,
				res_size,
				res_name ? res_name : "(no name)");
		}
	}
}
