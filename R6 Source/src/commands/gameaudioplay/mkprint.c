
#include <stdio.h>
#include <ctype.h>


char output[16000];
char * ptr = output;
typedef struct typeinf {
	char * type;
	char * ctrl;
} typeinf;
typeinf types[] = {
	{ "int", "%d" },
	{ "short", "%d" },
	{ "long", "%ld" },
	{ "int32", "%ld" },
	{ "int16", "%d" },
	{ "off_t", "%Ld" },
	{ "bigtime_t", "%Ld" },
	{ "time_t", "%ld" },
	{ "size_t", "0x%lx" },
	{ "void", "%p" },
	{ "uint32", "0x%lx" },
	{ "uint16", "0x%x" },
	{ "char", "%s" },
	{ NULL, NULL },
};

int
main()
{
	char line[256];
	char type[128];
	char var[128];
	int ix;
	fprintf(stderr, "paste struct member declarations here:\n");
	while (1) {
		char * ff, * gg;
		line[0] = 0;
		fgets(line, 256, stdin);
		if (!line[0]) break;
		if (2 != sscanf(line, " %s %s", type, var)) {
			fprintf(stderr, "ignoring line: %s", line);
			sprintf(ptr, "// %s", line);
			ptr += strlen(ptr);
			continue;
		}
		ff = var;
		while (*ff && !isalpha(*ff)) {
			ff++;
		}
		if (!*ff) {
			fprintf(stderr, "no alpha? bad line: %s", line);
			sprintf(ptr, "// %s", line);
			ptr += strlen(ptr);
			continue;
		}
		gg = ff;
		while (*gg && (isalpha(*gg) || isdigit(*gg) || (*gg == '_'))) {
			gg++;
		}
		*gg = 0;
		for (ix=0; types[ix].type != NULL; ix++) {
			if (!strcmp(types[ix].type, type)) {
				sprintf(ptr, "\tprintf(\"%s=%s\\n\", X->%s); // %s", ff, types[ix].ctrl, ff, line);
				ptr += strlen(ptr);
				goto good;
			}
		}
		fprintf(stderr, "unknown type: %s line: %s", type, line);
	good:
		continue;
	}
	sprintf(ptr, "\tprintf(\"\\n\");\n");
	fwrite(output, strlen(output), 1, stdout);
	return 0;
}
