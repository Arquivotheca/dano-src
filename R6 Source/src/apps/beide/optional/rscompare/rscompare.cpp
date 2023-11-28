// ---------------------------------------------------------------------------
/*
	rscompare.cpp
	
	rscompare is a resource file compare tool

	It can be used to verify that resource files don't change, 
	or that a build is complete. Usage is simple:
		rstool <filename1> <filename2>

	It will list resources that are in one file but not the other, 
	both by ID and name. It will also compare the contents of the resources 
	that match up, and list the resources that are different between the files.
	
*/
// ---------------------------------------------------------------------------


#include <Resources.h>
#include <File.h>
#include <stdio.h>

static void
getcode(
	type_code		code,
	char *			out)
{
	char hex[] = "0123456789abcdef";

	for (int ix=0; ix<4; ix++) {
		unsigned char ch = (code >> 24);
		if ((ch < 32) || (ch == '\\') || (ch > 127)) {
			*(out++) = '\\';
			*(out++) = 'x';
			*(out++) = hex[(ch>>4)&0xf];
			*(out++) = hex[ch&0xf];
		} else {
			*(out++) = ch;
		}
		code <<= 8;
	}
	*(out++) = 0;
}


static int
list_missing(
	BResources &		res1,
	BResources &		res2,
	const char *		file)
{
	type_code type;
	int32 id;
	const char * name;
	size_t size;
	char code[20];
	int errs = 0;
	for (int ix=0; res1.GetResourceInfo(ix, &type, &id, &name, &size); ix++) {
		if (!res2.HasResource(type, id)) {
			getcode(type, code);
			printf("[%s]: missing '%s' id %ld\n", file, code, id);
			errs++;
		}
		if (name && name[0] && !res2.HasResource(type, name)) {
			getcode(type, code);
			printf("[%s]: missing '%s' named '%s'\n", file, code, name);
			errs++;
		}
	}
	return errs;
}


static int
list_differences(
	BResources &		res1,
	const char *		file1,
	BResources &		res2,
	const char *		file2)
{
	type_code type;
	int32 id;
	const char * name;
	size_t size1;
	char code[20];
	int errs = 0;
	for (int ix=0; res1.GetResourceInfo(ix, &type, &id, &name, &size1); ix++) {
		size_t size2;
		const void * data1 = res1.LoadResource(type, id, &size1);
		if (!data1) {
			getcode(type, code);
			fprintf(stderr, "Out of memory '%s' %ld\n", code, id);
			continue;
		}
		const void * data2 = res2.LoadResource(type, id, &size2);
		if (data2) {
			if (size1 != size2) {
				getcode(type, code);
				printf("[%s] vs [%s]: resource '%s' %ld is sized %ld/%ld\n", file1, file2, code, id, size1, size2);
				errs++;
			} else if (memcmp(data1, data2, size1)) {
				getcode(type, code);
				printf("[%s] vs [%s]: resource '%s' %ld differs\n", file1, file2, code, id);
				errs++;
			}
		}
	}
	return errs;
}


int
main(
	int		argc,
	char *	argv[])
{
	if (argc != 3) {
		fprintf(stderr, "usage: rscompare <file1> <file2>\n");
		return -1;
	}

	BFile file1(argv[1], O_RDONLY);
	if (file1.InitCheck()) {
		fprintf(stderr, "rscompare: [%s] not found\n", argv[1]);
		return -1;
	}

	BFile file2(argv[2], O_RDONLY);
	if (file2.InitCheck()) {
		fprintf(stderr, "rscompare: [%s] not found\n", argv[2]);
		return -1;
	}

	BResources res1;
	if (res1.SetTo(&file1)) {
		fprintf(stderr, "rscompare: [%s] not a resource file\n", argv[1]);
		return -1;
	}

	BResources res2;
	if (res2.SetTo(&file2)) {
		fprintf(stderr, "rscompare: [%s] not a resource file\n", argv[2]);
		return -1;
	}

	int errs = 0;
	errs += list_missing(res1, res2, argv[2]);
	errs += list_missing(res2, res1, argv[1]);
	errs += list_differences(res1, argv[1], res2, argv[2]);

	printf("ResCompare done: %d errors\n", errs);

	return 0;
}
