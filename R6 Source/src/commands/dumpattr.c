
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <fs_attr.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>



enum {
	output = 0,
	input = 1
};

int direction = output;
int noheader = 0;

FILE * infile;
FILE * outfile;

static void
usage()
{
	fprintf(stderr, "usage: dumpattr [ options ] file [ attr ... ]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "-u      --undump            read attribute value(s) from stdin and write to file\n");
	fprintf(stderr, "-i file --input=file        use file instead of stdin\n");
	fprintf(stderr, "-o file --output=file       use file instead of stdout\n");
	fprintf(stderr, "-n      --noheader          do not add header\n");
	fprintf(stderr, "--                          end of options\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "If no attr given, all are assumed. When a single attribute is given, the \n");
	fprintf(stderr, "informational header is omitted from both input/output.\n");
	exit(1);
}


void
get_print_type(
	uint32 type,
	char * out)
{
	uchar * ch = (uchar *)&type;
	if ((ch[0] > 31) && (ch[0] < 127) && 
			(ch[1] > 31) && (ch[1] < 127) && 
			(ch[2] > 31) && (ch[2] < 127) && 
			(ch[3] > 31) && (ch[3] < 127)) {
		sprintf(out, "\'%c%c%c%c\'", type>>24, type>>16, type>>8, type);
	}
	else {
		sprintf(out, "0x%08lx", type);
	}
}

void
get_print_name(
	const char * name,
	char * out)
{
	assert(strlen(name) < 256);
	while (*name) {
		if ((*(uchar *)name > 31) && (*(uchar *)name != '\\')) {
			*(out++) = *name;
		}
		else {
			switch (*name) {
			case 10:
				sprintf(out, "\\n");
				break;
			case 13:
				sprintf(out, "\\r");
				break;
			case '\\':
				sprintf(out, "\\");
				break;
			default:
				sprintf(out, "\\x%02x", (uchar)*name);
			}
			out += strlen(out);
		}
		name++;
	}
	*out = 0;
}

void
unget_name(
	const char * in,
	char * out)
{
	char * temp;
	char xx[3];
	while (*in && (*(uchar *)in >= 32)) {
		if (*in == '\\') {
			switch (in[1]) {
			case '\\':
				*(out++) = '\\';
				in += 2;
				break;
			case 'n':
				*(out++) = 10;
				in += 2;
				break;
			case 'r':
				*(out++) = 13;
				in += 2;
				break;
			case 'x':
				if (!in[2] || !in[3]) {
					fprintf(stderr, "runtime warning: string ends in backslash hex\n");
					in += strlen(in);
				}
				else {
					xx[0] = in[2];
					xx[1] = in[3];
					in += 4;
					xx[2] = 0;
					*(out++) = strtol(xx, &temp, 16);
				}
				break;
			case 0:
				fprintf(stderr, "runtime warning: string ends in backslash escape\n");
				in++;
				break;
			default:
				fprintf(stderr, "runtime warning: unknown backslash escape '%c'\n", in[1]);
				*(out++) = in[1];
				in += 2;
				break;
			}
		}
		else {
			*(out++) = *(in++);
		}
	}
	*out = 0;
}

int
dump_one_attr(
	int fd,
	const char * name)
{
	attr_info ai;
	char str1[30];
	char str2[300];
	char b[1024];
	char * buf = b;
	off_t togo, where = 0;
	ssize_t rd;
	int err;
	if (fs_stat_attr(fd, name, &ai) < 0) {
		fprintf(stderr, "runtime error: fs_stat_attr(%d, %s) failed\n", fd, name);
		return errno;
	}
	if (!noheader) {
		get_print_type(ai.type, str1);
		get_print_name(name, str2);
		fprintf(outfile, "%s %Ld %s\n", str1, ai.size, str2);
	}
	if (ai.size > 1024) {
		if (ai.size > 65536) {
			buf = malloc(65536);
		}
		else {
			buf = malloc(ai.size);
		}
	}
	if (!buf) return B_NO_MEMORY;
	togo = ai.size;
	while (togo > 0) {
		if (togo > 65536) {
			rd = 65536;
		}
		else {
			rd = togo;
		}
		if (rd != (err = fs_read_attr(fd, name, ai.type, where, buf, rd))) {
			fprintf(stderr, "runtime error: fs_read_attr(%d, %s) failed (%d)\n", fd, name, err);
			return -1;
		}
		fwrite(buf, rd, 1, outfile);
		togo -= rd;
		where += rd;
	}
	if (buf != b) free(buf);
	if (!noheader) {
		fprintf(outfile, "\n");		/* always terminate with a newline */
	}
	return 0;
}

int
dump_some_attrs(
	int fd,
	const char ** names)
{
	int err;
	int cnt = 0;
	while (*names) {
		err = dump_one_attr(fd, *names);
		if (err < 0) return err;
		names++;
		cnt++;
	}
	if ((cnt > 1) && noheader) {
		fprintf(stderr, "usage warning: the data is not delineated by headers, and there were %d attributes\n", cnt);
	}
	return 0;
}

int
dump_all_attrs(
	int fd)
{
	DIR * d = fs_fopen_attr_dir(fd);
	struct dirent * de;
	int err;
	int cnt = 0;
	if (d == 0) {
		err = errno;
		fprintf(stderr, "runtime error: fs_fopen_attr_dir(%d) failed\n", fd);
		return err;
	}
	while ((de = fs_read_attr_dir(d)) != 0) {
		err = dump_one_attr(fd, de->d_name);
		if (err < 0) return err;
		cnt++;
	}
	fs_close_attr_dir(d);
	if ((cnt > 1) && noheader) {
		fprintf(stderr, "usage warning: the data is not delineated by headers, and there were %d attributes\n", cnt);
	}
	return 0;
}

int
undump_one_attr(
	int fd,
	const char * name,
	uint32 type,
	off_t size)
{
	int err, rd;
	off_t togo, where;
	char * buf, b[1024];

	buf = b;
	if (size > 1024) {
		if (size > 65536) {
			buf = malloc(65536);
		}
		else {
			buf = malloc(size);
		}
	}
	if (!buf) return B_NO_MEMORY;

	/* remove the attr so there's a clean slate */
	fs_remove_attr(fd, name);

	/* copy the data */
	togo = size;
	while (togo > 0) {
		if (togo > 65536) {
			rd = 65536;
		}
		else {
			rd = togo;
		}
		if (fread(buf, 1, rd, infile) != rd) {
			fprintf(stderr, "runtime error: cannot read %d bytes from input file, attr '%s'\n",
					rd, name);
		}
		if (rd != (err = fs_write_attr(fd, name, type, where, buf, rd))) {
			fprintf(stderr, "runtime error: fs_write_attr(%d, %s) failed (%d)\n", fd, name, err);
			return err;
		}
		togo -= rd;
		where += rd;
	}
	if (buf != b) free(buf);
	return 0;
}

int
name_match(
	const char * name,
	const char ** names)
{
	if (!names) return 1;
	while (*names) {
		if (!strcmp(*names, name)) return 1;
		names++;
	}
	return 0;
}

int
skip_data(
	off_t size)
{
	if (infile == stdin) {
		char buf[4096];
		while (size > 0) {
			int tr;
			if (size > 4096)
				tr = 4096;
			else
				tr = size;
			if (tr != fread(buf, 1, tr, infile)) {
				fprintf(stderr, "runtime error: cannot read past skipped attribute data\n");
				return -1;
			}
			size -= tr;
		}
	}
	else {
		if (fseek(infile, size, 1) < 0) {
			fprintf(stderr, "runtime error: cannot seek input file to skip attribute\n");
			return -1;
		}
	}
	return 0;
}

int
undump_some_attrs(
	int fd,
	const char ** names)
{
	char hdr[1024];
	char name[256];
	char type_c[4];
	char * p;
	uint32 type;
	off_t size;
	int err;

	while (!feof(infile) && !ferror(infile)) {
		hdr[0] = 0;
		fgets(hdr, 1024, infile);
		if (!hdr[0]) {
			// done
			break;
		}
		if (hdr[0] == '\'') {
			if (5 != sscanf(hdr, "\'%c%c%c%c\' %Ld", &type_c[0],
					&type_c[1], &type_c[2], &type_c[3], &size)) {
				goto error;
			}
			type = (type_c[0]<<24)+(type_c[1]<<16)+(type_c[2]<<8)+type_c[3];
		}
		else if (hdr[0] == '0') {
			if (2 != sscanf(hdr, "0x%lx %Ld", &type, &size)) {
				goto error;
			}
		}
		else {
error:
			fprintf(stderr, "runtime error: expected header line in infile, got '%s'\n",
					hdr);
			return -1;
		}
		p = strchr(hdr, ' ');
		if (p) p = strchr(p+1, ' ');
		if (p) p++;
		if (!p || !*p || (*p == '\n')) goto error;
		unget_name(p, name);
		/* filter names */
		if (name_match(name, names)) {
			err = undump_one_attr(fd, name, type, size);
		}
		else {
			err = skip_data(size);
		}
		if (err < 0) {
			return err;
		}
		if (fgetc(infile) != '\n') {
			fprintf(stderr, "runtime error: attribute data was not properly terminated\n");
			return -1;
		}
	}
	return ferror(infile) ? -1 : 0;
}

int
undump_all_attrs(
	int fd)
{
	return undump_some_attrs(fd, NULL);
}


int
main(
	int argc,
	const char * argv[])
{
	int fd;
	int err;
	if ((argc < 2) || !strcmp(argv[1], "--help")) {
		usage();
	}
	argc--;
	argv++;
	infile = stdin;
	outfile = stdout;
	while (argv[0] && (argv[0][0] == '-')) {
		if (!strcmp(argv[0], "--")) {
			break;
		}
		else if (!strcmp(argv[0], "--undump")) {
			direction = input;
		}
		else if (!strcmp(argv[0], "--noheader")) {
			noheader = 1;
		}
		else if (!strncmp(argv[0], "--input=", 8)) {
			if (infile != stdin) {
				fprintf(stderr, "usage error: multiple input files specified\n");
				exit(2);
			}
			infile = fopen(&argv[0][8], "r");
			if (!infile) {
				fprintf(stderr, "runtime error: cannot open '%s'\n", argv[0]);
				exit(3);
			}
		}
		else if (!strncmp(argv[0], "--output=", 9)) {
			if (outfile != stdout) {
				fprintf(stderr, "usage error: multiple output files specified\n");
				exit(2);
			}
			outfile = fopen(&argv[0][9], "w");
			if (!outfile) {
				fprintf(stderr, "runtime error: cannot create '%s'\n", argv[0]);
				exit(3);
			}
		}
		else if (argv[0][1] != '-') {
			//	old-skool options
			const char * fn;
			const char * ch;
			for (ch = &argv[0][1]; *ch; ch++) {
				switch (*ch) {
				case 'u':
					direction = input;
					break;
				case 'n':
					noheader = 1;
					break;
				case 'i':
					if (ch[1]) {
						fn = ch+1;
						ch += strlen(ch)-1;
					}
					else if (argv[1]) {
						fn = argv[1];
						argv++;
						argc--;
					}
					else {
						fprintf(stderr, "usage error: -i with no input file name\n");
						exit(2);
					}
					if (infile != stdin) {
						fprintf(stderr, "usage error: multiple input files specified\n");
						exit(2);
					}
					infile = fopen(fn, "r");
					if (!infile) {
						fprintf(stderr, "runtime error: cannot open '%s'\n", fn);
						exit(2);
					}
					break;
				case 'o':
					if (ch[1]) {
						fn = ch+1;
						ch += strlen(ch)-1;
					}
					else if (argv[1]) {
						fn = argv[1];
						argv++;
						argc--;
					}
					else {
						fprintf(stderr, "usage error: -o with no output file name\n");
						exit(2);
					}
					if (outfile != stdout) {
						fprintf(stderr, "usage error: multiple output files specified\n");
						exit(2);
					}
					outfile = fopen(fn, "w");
					if (!outfile) {
						fprintf(stderr, "runtime error: cannot open '%s'\n", fn);
						exit(2);
					}
					break;
				default:
					fprintf(stderr, "usage error: unknown option '%c'\n", *ch);
					usage();
					break;
				}
			}
		}
		else {
			fprintf(stderr, "usage error: unknown option '%s'\n", argv[0]);
			usage();
		}
		argc--;
		argv++;
	}
	if (!argv[0]) {
		fprintf(stderr, "usage error: no file specified (dumpattr --help for more info)\n");
		exit(2);
	}
	if (noheader && (direction == input)) {
		fprintf(stderr, "warning: specifying --noheader with --undump has no effect\n");
	}
	if ((infile != stdin) && (direction == output)) {
		fprintf(stderr, "warning: specifying input file without --undump has no effect\n");
	}
	if ((outfile != stdout) && (direction == input)) {
		fprintf(stderr, "warning: specifying output file with --undump has no effect\n");
	}
	fd = open(argv[0], (direction == input) ? O_RDWR | O_CREAT : O_RDONLY, 0666);
	if (fd < 0) {
		fprintf(stderr, "runtime error: %s opening '%s'\n", strerror(errno), argv[0]);
		exit(3);
	}
	argv++;
	argc--;
	if (direction == output) {
		if (*argv) {
			if (argv[1]) {
				dump_some_attrs(fd, argv);
			}
			else {
				dump_one_attr(fd, *argv);
			}
		}
		else {
			dump_all_attrs(fd);
		}
	}
	else {
		if (*argv) {
			undump_some_attrs(fd, argv);
		}
		else {
			undump_all_attrs(fd);
		}
	}
	close(fd);
	if (direction == output) {
		fflush(outfile);
	}
	if (infile != stdin) {
		fclose(stdin);
	}
	if (outfile != stdout) {
		fclose(stdout);
	}
	return (err < 0) ? 3 : 0;
}
