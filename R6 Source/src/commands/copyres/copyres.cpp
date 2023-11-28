#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <File.h>
#include <Resources.h>

int
main(int argc, char **argv)
{
	status_t		err;
	BFile			file;
	BResources		sfile, dfile;
	int				i;
	void			*buf;
	bool			created;
	type_code		type;
	int32			id;
	size_t			size;
	const char		*name;
	

	if (argc != 3) {
		fprintf(stderr, "usage: copyres 'source file' 'dest file'\n");
		return 1;
	}

	file.SetTo(argv[1], O_RDONLY);
	err = sfile.SetTo(&file);
	if (err) {
		fprintf(stderr, "copyres: problem with file '%s' (%s)\n", argv[1], strerror(err));
		return 1;
	}
		
	created = FALSE;
	err = file.SetTo(argv[2], O_RDWR);
	if (err) {
		err = file.SetTo(argv[2], O_CREAT | O_TRUNC | O_RDWR);
		created = TRUE;
	}
	if (err) {
		fprintf(stderr, "copyres: problem with file '%s' (1. %s)\n", argv[2], strerror(err));
		return 1;
	}
	err = dfile.SetTo(&file, created);
	if (err) {
		fprintf(stderr, "copyres: problem with file '%s' (2. %s)\n", argv[2], strerror(err));
		return 1;
	}

	for(i=0; sfile.GetResourceInfo(i, &type, &id, &name, &size); i++) {
		buf = sfile.FindResource(type, id, &size);
		if (buf == NULL) {
			fprintf(stderr, "problem reading resource type %c%c%c%c, id %d\n",
				type >> 24,	type >> 16,	type >> 8, type, id);
			return 1;
		}
		if (dfile.AddResource(type, id, buf, size, name) != B_NO_ERROR) {
			fprintf(stderr, "problem reading resource type %c%c%c%c, id %d\n",
				type >> 24,	type >> 16,	type >> 8, type, id);
			free(buf);
			return 1;
		}
		free(buf);
	}
	return 0;
}
