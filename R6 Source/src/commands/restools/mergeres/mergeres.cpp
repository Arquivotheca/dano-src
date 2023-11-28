#include <stdio.h>
#include <Application.h>
#include <Resources.h>
#include <stdlib.h>
#include <File.h>
#include <Entry.h>
#include <string.h>


int
main(
	int argc,
	char *argv[])
{
	BApplication app("application/x-mw-mergeres");
	int nullname = 0;
	int replace_duplicates = 0;

	{
	if (argc < 3) {
		fprintf(stderr, "usage: mergeres [-noname] [-replace] source ... destination\n");
		goto err;
	}
	entry_ref dest;
	if (get_ref_for_path(argv[argc-1], &dest)) {
		fprintf(stderr, "mergeres: can't find %s\n", argv[argc-1]);
		goto err;
	}
	BFile outFile(&dest, O_RDWR);
	if (outFile.InitCheck()) {
		fprintf(stderr, "mergeres: can't open %s\n", argv[argc-1]);
		goto err;
	}
	BResources out(&outFile);
	for (int ix=1; ix<argc-1; ix++) {
		if (!strcmp(argv[ix], "-noname")) {
			nullname = 1;
			continue;
		}
		if (!strcmp(argv[ix], "-replace")) {
			replace_duplicates = 1;
			continue;
		}
		entry_ref ref;
		if (get_ref_for_path(argv[ix], &ref)) {
			fprintf(stderr, "mergeres: can't find %s\n", argv[ix]);
			continue;
		}
		BFile inFile(&ref, O_RDONLY);
		if (inFile.InitCheck()) {
			fprintf(stderr, "mergeres: can't open %s\n", argv[ix]);
			continue;
		}
		BResources in(&inFile);
		long id;
		ulong type;
		const char *name = NULL;
		size_t size;
		for (int rix=0; in.GetResourceInfo(rix, &type, &id, &name, &size); rix++) {
			void *data = in.FindResource(type, id, &size);
			char tstr[5];
			memcpy(tstr, &type, 4);
			tstr[4] = 0;
			if (!data) {
				fprintf(stderr, "mergeres: can't find resource %s:%s:%d\n",
					argv[ix], tstr, id);
				continue;
			}
			if (!replace_duplicates) {
				while (out.HasResource(type, id)) {
					id++;
				}
			}
			else
			{
				out.RemoveResource(type, id);
			}
			if (nullname)
				name = NULL;
			if (out.AddResource(type, id, data, size, name)) {
				fprintf(stderr, "mergeres: can't add resource %s:%d\n",
					tstr, id);
			}
			free(data);
			name = NULL;
		}
	}
	app.PostMessage(B_QUIT_REQUESTED);
	app.Run();
	}
	return 0;

err:
	app.PostMessage(B_QUIT_REQUESTED);
	app.Run();
	return 1;
}
