
/* kludge to avoid multiple files in build system */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <private/storage/write_res.h>


const char * output_file = "xres.output.rsrc";

static void
usage()
{
	fprintf(stderr, "\nusage: xres [ -r ] [ -l ] [ -o file ] [ cmd ] [ resourcefile ] ...\n");
	fprintf(stderr, "Default output file name is '%s'.\n", output_file);
	fprintf(stderr, "\nEach time a new output file is specified, any resourced \n");
	fprintf(stderr, "gathered from earlier input files are written to that file \n");
	fprintf(stderr, "and then flushed. I e the output file will be the merge of \n");
	fprintf(stderr, "all files after it on the line until the next output file.\n");
	fprintf(stderr, "This is exciting because 'xres' accepts input files in \n");
	fprintf(stderr, "both PEF and PE (PPC and X86) formats!\n");
	fprintf(stderr, "An alternate output file option name is --output\n\n");
	fprintf(stderr, "If '-l' is given, resources in input files are listed \n");
	fprintf(stderr, "to stdout rather than copied to the output file.\n");
	fprintf(stderr, "An alternate option name is --list\n\n");
	fprintf(stderr, "If '-r' is given, the file name has a '.rsrc' suffix silently appended.\n");
	fprintf(stderr, "'cmd' can be one of:\n");
	fprintf(stderr, "An alternate option name is --rsrc\n\n");
	fprintf(stderr, "-d type[:id]  do not copy resource(s) of type type (and id id).\n");
	fprintf(stderr, "-a type:id[:name] [datafile|-s string]  add resource of type type, id id, \n");
	fprintf(stderr, "      name name, with data taken from datafile (or the string string).\n");
	fprintf(stderr, "-x type[:id]  copy only (extract) resource(s) of type type (and id id)\n");
	fprintf(stderr, "--  do not treat further names starting with '-' as options.\n");
}

typedef struct _delete_items {
	struct _delete_items * next;
	unsigned int type;
	int id;
	int id_counts;
} delete_items;

delete_items * g_delete_items;
delete_items * g_extract_items;

static void
add_delete_item(
	unsigned int type,
	int id,
	int id_counts)
{
	delete_items * i = (delete_items *)malloc(sizeof(delete_items));
	if (!i) {
		fprintf(stderr, "delete resource: out of memory\n");
		exit(2);
	}
	i->type = type;
	i->id = id;
	i->id_counts = id_counts;
	i->next = g_delete_items;
	g_delete_items = i;
}


static void
add_extract_item(
	unsigned int type,
	int id,
	int id_counts)
{
	delete_items * i = (delete_items *)malloc(sizeof(delete_items));
	if (!i) {
		fprintf(stderr, "extract resource: out of memory\n");
		exit(2);
	}
	i->type = type;
	i->id = id;
	i->id_counts = id_counts;
	i->next = g_delete_items;
	g_extract_items = i;
}


static void
do_delete(
	res_map * map)
{
	delete_items * di = g_delete_items;
	while (di != NULL) {
		if (di->id_counts) {
			remove_resource_id(map, di->type, di->id);
		}
		else {
			void * cookie = NULL;
			unsigned int type;
			int id;
			const void * data;
			int size;
			const char * name;
			while (iterate_resources(map, &cookie, &type, &id, &data, &size, &name) == 0) {
				if (type == di->type) {
					remove_resource_id(map, type, id);
				}
			}
		}
		di = di->next;
	}
}


static void
do_extract(
	res_map * map)
{
	void * cookie = NULL;
	unsigned int type;
	int id;
	const void * data;
	int size;
	const char * name;

	if (g_extract_items == NULL) {
		return;
	}
	while (iterate_resources(map, &cookie, &type, &id, &data, &size, &name) == 0) {
		delete_items * di = g_extract_items;
		int keep = 0;
		while (di != NULL) {
			if ((type == di->type) && (!di->id_counts || (di->id == id))) {
				keep = 1;
				break;
			}
			di = di->next;
		}
		if (!keep) {
			remove_resource_id(map, type, id);
		}
	}
}


int
main(
	int argc,
	char * argv[])
{
	int ix;
	res_map * cur_map = NULL;
	int input_endian;
	int listflag = 0;
	int ignoreopts = 0;
	int name_rsrc = 0;

	for (ix=1; ix<argc; ix++) {
		if (ignoreopts) {
			goto do_file;
		}
		if (!strcmp(argv[ix], "--help") || !strcmp(argv[ix], "-?")) {
			usage();
			return 1;
		}
		else if (!strcmp(argv[ix], "-l") || !strcmp(argv[ix], "--list")) {
			listflag = 1;
		}
		else if (!strcmp(argv[ix], "-r") || !strcmp(argv[ix], "--rsrc")) {
			name_rsrc = 1;
		}
		else if (!strcmp(argv[ix], "-o") || !strcmp(argv[ix], "--output")) {
			if (!argv[ix+1]) {
				fprintf(stderr, "error: --output requires an argument\n");
				return 1;
			}
			else if (listflag) {
				fprintf(stderr, "warning: output file is ignored when --list is in effect.\n");
				ix++;
			}
			else {
				if (cur_map) {
					int fd = open(output_file, O_RDWR | O_CREAT, 0666);
					if (fd < 0) {
						fprintf(stderr, "error: cannot create %s\n", output_file);
						return 1;
					}
					if (position_at_map(fd, 1, &input_endian) < 0) {
						fprintf(stderr, "error: output file %s is not a resource file\n", output_file);
						return 1;
					}
					do_delete(cur_map);
					do_extract(cur_map);
					if (write_resource_file(cur_map, fd, input_endian, NULL) < 0) {
						fprintf(stderr, "error: problem writing resource file %s\n", output_file);
						return 1;
					}
					close(fd);
					dispose_resource_map(cur_map);
					cur_map = NULL;
				}
				if (name_rsrc) {
					static char saved_output_file_name[1024];
					strcpy(saved_output_file_name, argv[ix+1]);
					strcat(saved_output_file_name, ".rsrc");
					output_file = saved_output_file_name;
				}
				else {
					output_file = argv[ix+1];
				}
				ix++;
			}
		}
		else if (!strcmp(argv[ix], "-d") || !strcmp(argv[ix], "--delete")) {
			char * col;
			unsigned int type = 0;
			int id = 0;
			int id_counts = 0;
			ix++;
			if (!argv[ix]) {
				fprintf(stderr, "--delete: option requires an argument\n");
				return 1;
			}
			if (strlen(argv[ix]) < 4) {
				fprintf(stderr, "--delete type code must be four characters\n");
				return 1;
			}
			col = argv[ix];
			type = (type << 8) | ((unsigned char)*(col++));
			type = (type << 8) | ((unsigned char)*(col++));
			type = (type << 8) | ((unsigned char)*(col++));
			type = (type << 8) | ((unsigned char)*(col++));
			if (*col) {
				char * end;
				if (*col == ':') {
					col++;
				}
				else {
					fprintf(stderr, "--delete requires type:id\n");
					return 1;
				}
				if (*col) {
					end = col;
					id = strtol(col, &end, 10);
					if (*end) {
						fprintf(stderr, "--delete id is not a valid id\n");
						return 1;
					}
					id_counts = 1;
				}
			}
			add_delete_item(type, id, id_counts);
		}
		else if (!strcmp(argv[ix], "-x") || !strcmp(argv[ix], "--extract")) {
			char * col;
			unsigned int type = 0;
			int id = 0;
			int id_counts = 0;
			ix++;
			if (!argv[ix]) {
				fprintf(stderr, "--extract: option requires an argument\n");
				return 1;
			}
			if (strlen(argv[ix]) < 4) {
				fprintf(stderr, "--extract type code must be four characters\n");
				return 1;
			}
			col = argv[ix];
			type = (type << 8) | ((unsigned char)*(col++));
			type = (type << 8) | ((unsigned char)*(col++));
			type = (type << 8) | ((unsigned char)*(col++));
			type = (type << 8) | ((unsigned char)*(col++));
			if (*col) {
				char * end;
				if (*col == ':') {
					col++;
				}
				else {
					fprintf(stderr, "--extract requires type:id\n");
					return 1;
				}
				if (*col) {
					end = col;
					id = strtol(col, &end, 10);
					if (*end) {
						fprintf(stderr, "--extract id is not a valid id\n");
						return 1;
					}
					id_counts = 1;
				}
			}
			add_extract_item(type, id, id_counts);
		}
		else if (!strcmp(argv[ix], "-a")) {
			unsigned int type = 0;
			int id;
			char * name;
			char * col;
			ix++;
			if (!argv[ix] || !argv[ix+1]) {
				fprintf(stderr, "--add requires type:id and datafile\n");
				return 1;
			}
			if ((strlen(argv[ix]) < 6) || argv[ix][4] != ':') {
				fprintf(stderr, "--add requires type:id as first argument\n");
				return 1;
			}
			name = argv[ix];
			type = (type << 8) | ((unsigned char)*(name++));
			type = (type << 8) | ((unsigned char)*(name++));
			type = (type << 8) | ((unsigned char)*(name++));
			type = (type << 8) | ((unsigned char)*(name++));
			id = strtol(name+1, &name, 10);
			if (*name && *name != ':') {
				fprintf(stderr, "--add id is malformed\n");
				return 1;
			}
			if (*name) name++;
			ix++;
			if (!strcmp(argv[ix], "-s") || !strcmp(argv[ix], "--string")) {
				ix++;
				if (!argv[ix]) {
					fprintf(stderr, "--string requires an argument\n");
					return 1;
				}
				add_resource(&cur_map, type, id, argv[ix], strlen(argv[ix])+1, name);
			}
			else {
				int df = open(argv[ix], O_RDONLY);
				off_t size;
				void * data;
				size_t limit = 4*1024*1024;
				if (df < 0) {
					fprintf(stderr, "%s: cannot open: %s\n", argv[ix], strerror(errno));
					return 1;
				}
				size = lseek(df, 0, 2);
				if (size < 0) {
					fprintf(stderr, "%s: cannot find end of file\n", argv[ix]);
					close(df);
					return 1;
				}
				if (size > limit) {
					fprintf(stderr, "%s: data file too large (%Ld bytes; limit %d)\n", 
						argv[ix], size, limit);
					close(df);
					return 1;
				}
				lseek(df, 0, 0);
				data = malloc(size);
				if (!data) {
					fprintf(stderr, "%s: out of memory (%d bytes)\n", argv[ix], size);
					close(df);
					return 1;
				}
				if (read(df, data, size) != size) {
					fprintf(stderr, "%s: error reading file\n", argv[ix]);
					close(df);
					free(data);
					return 1;
				}
				close(df);
				add_resource(&cur_map, type, id, data, size, name);
				free(data);
			}
		}
		else if (!strcmp(argv[ix], "--")) {
			ignoreopts = 1;
		}
		else {
			int fd;
	do_file:
			fd = open(argv[ix], O_RDONLY);
			if (fd < 0) {
				fprintf(stderr, "error: cannot open %s\n", argv[ix]);
				return 1;
			}
			if (position_at_map(fd, 0, &input_endian) <= 0) {
				fprintf(stderr, "error: %s is not a resource file\n", argv[ix]);
				return 1;
			}
			if (read_resource_file(&cur_map, fd, input_endian, NULL) < 0) {
				fprintf(stderr, "error: problem reading resources from %s\n", argv[ix]);
				return 1;
			}
			close(fd);
			if (listflag) {
				void * cookie = NULL;
				unsigned int type;
				int id;
				const void * data;
				int size;
				const char * name;
				fprintf(stdout, "\n%s resources (%s format):\n", 
					argv[ix], input_endian ? "little-endian" : "big-endian");
				fprintf(stdout, "\n type           ID        size  name\n");
				fprintf(stdout,   "------ ----------- -----------  --------------------\n");
				while (!iterate_resources(cur_map, &cookie, &type, &id, &data, &size, &name)) {
					fprintf(stdout, "'%c%c%c%c' %11d %11d  %s\n", 
						type>>24, type>>16, type>>8, type, id, size, 
						(name && *name) ? name : "(no name)");
				}
				dispose_resource_map(cur_map);
				cur_map = NULL;
			}
		}
	}
	if (cur_map) {
		int fd = open(output_file, O_RDWR | O_CREAT, 0666);
		if (fd < 0) {
			fprintf(stderr, "error: cannot create %s\n", output_file);
			return 1;
		}
		if (position_at_map(fd, 1, &input_endian) < 0) {
			fprintf(stderr, "error: output file %s is not a resource file\n", output_file);
			return 1;
		}
		do_delete(cur_map);
		do_extract(cur_map);
		if (write_resource_file(cur_map, fd, input_endian, NULL) < 0) {
			fprintf(stderr, "error: problem writing resource file %s\n", output_file);
			return 1;
		}
		close(fd);
		dispose_resource_map(cur_map);
		cur_map = NULL;
	}
	else if (!listflag) {
		fprintf(stderr, "warning: nothing written to %s [use --help for help]\n", output_file);
	}
	return 0;
}

