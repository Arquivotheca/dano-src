#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

//#define dbg_main

#include "comptab.h" //%%% rmd DR 22 July
#include "filechain.h"

#if defined(__cplusplus)
#define EX extern //"C"
#else
#define EX extern
#endif

EX int yyparse(void);
#if defined(__cplusplus)
extern "C" int yywrap(void);
#else
EX int yywrap(void);
#endif
EX int yyerror(const char *err); 
EX int yy_line_no;
EX char yytext[];

extern filechain *g_file_chain;
extern FILE *yyin;

extern int mergeflag;
int errors = 0;

#if DEBUG
int verbose = 1;
#else
int verbose = 0;
#endif

extern int yy_current_file_line();
extern const char *yy_current_file_name();
int emit_resources(char *);
void usage_error( const char * arg, ...);

void
usage_error(
	const char * arg,
	...)
{
	va_list args;
	va_start(args, arg);
	vfprintf(stderr, arg, args);
	va_end(args);
	fprintf(stderr, "\nusage: mwbres [-merge] [-o filename.rsrc] source.r\n"
);
	exit(1);
}


char *output_name = "output.rsrc";

int
main(
	int argc,
	char *argv[])
{
	int ix;
	int name_rsrc = 0;

#if DEBUG_MEM
char * av[] = {
	"mwbres2", "-o", "foo.rsrc", "about.r", NULL
};
	argv = av;
	argc = 4;
	putenv("MALLOC_DEBUG=11");
	putenv("MALLOC_DEBUG_CHECK_FREQUENCY=1");
#endif

/* to run the program say 
	mwbres [-merge] [-o filename.rsrc] [-DNAME[=VALUE]] source.r
*/
	mergeflag = 0;

	struct cname temp_name;
	temp_name.type = kcname;
/* predefined values */
	temp_name.string = "__BERES__";
	(void)add_name_int(&temp_name, 200, prio_define);
	temp_name.string = "__BEOS__";
	(void)add_name_int(&temp_name, 1, prio_define);
#if __POWERPC__
	temp_name.string = "__POWERPC__";
	(void)add_name_int(&temp_name, 1, prio_define);
#elif __arm__
	temp_name.string = "__arm__";
	(void)add_name_int(&temp_name, 1, prio_define);
#elif __INTEL__
	temp_name.string = "__INTEL__";
	(void)add_name_int(&temp_name, 1, prio_define);
#else
#error  Unknown arch
#endif
#if __BIG_ENDIAN__
	temp_name.string = "__BIG_ENDIAN__";
	(void)add_name_int(&temp_name, 1, prio_define);
#else
	temp_name.string = "__LITTLE_ENDIAN__";
	(void)add_name_int(&temp_name, 1, prio_define);
#endif
	time_t now;
	time(&now);
	temp_name.string = "__CDATE__";
	(void)add_name_string(&temp_name, ctime(&now), prio_define);

	for (ix=1; ix<argc; ix++)
	{
	/* -o outputfile specifies what file to create/merge into */
	/* -merge specifies that resources in output file should be preserved instead of truncated */
	/* -- is convention to terminate option list to allow file names starting with -
 	*/     
		if (!strcmp(argv[ix], "-r")) 
			name_rsrc = 1;
		else if (!strcmp(argv[ix], "-o")) {
			if (ix == argc-1)
				usage_error("-o requires argument");
			else if (name_rsrc) {
				static char output_name_rsrc[1024];
				strcpy(output_name_rsrc, argv[++ix]);
				strcat(output_name_rsrc, ".rsrc");
				output_name = output_name_rsrc;
			}
			else
				output_name = argv[++ix];
		}
		else if (!strcmp(argv[ix], "-merge"))
			mergeflag = 1;
		else if (!strncmp(argv[ix], "-D", 2)) {
			char * name = &argv[ix][2];
			if (!*name) {
				usage_error("-D requires name");
			}
			char * eq = strchr(name, '=');
			if (eq) {
				eq++;
				char * out;
				int l = strtol(eq, &out, 10);
				if (out && !*out) {
					temp_name.string = name;
					(void)add_name_int(&temp_name, l, prio_define);
				}
				else {
					temp_name.string = name;
					(void)add_name_string(&temp_name, eq, prio_define);
				}
			}
			else {
				temp_name.string = name;
				(void)add_name_int(&temp_name, 1, prio_define);
			}
		}
		else if (!strcmp(argv[ix], "--"))
			break;
		else if (argv[ix][0] == '-')
			usage_error("unknown option %s", argv[ix]);
		else
			break;  /* this is the file */
	}

if (ix == argc)
	usage_error("input file missing");
if (ix != argc-1)
	usage_error("extra arguments found! (first is %s)", argv[ix+1]);

	/* freopen basically changes the file associated with stdin to be argv[ix].
	While running the mwbres2 program, the 2nd argument will be this file indicated
	by argv[ix].	*/
	
	FILE *in = freopen(argv[ix], "r", stdin);
		
	#ifdef dbg_main
	printf("test file is at %p \n", in);
	#endif
	
	if (!in) {
		fprintf(stderr, "E '%s' line 0: cannot open file\n", argv[ix]);
	} 
	else 
		{
			//init the  g_file_chain
			filechain *init = (filechain *)malloc(sizeof(filechain));
			init->prev = NULL;
			init->file = in;
			init->line = yy_current_file_line();
			
			init->curname = strdup(argv[ix]);
			
			g_file_chain = init;
			//g_file_chain init is over.
			
			if (!yyparse()) {
				if (verbose) fprintf(stderr, "I '%s' accepted\n", argv[ix]);
				emit_resources(output_name);
				}/* if (!yyparse() */
		}//else
	return errors ? 1 : 0;
}

/* When lexer encounters EOF, it calls yywrap(). If yywrap() returns
   1, scanner reports a zero token to report EOF. lex (not flex)
   defines yywrap()
   as a macro, so here it need not be undefined if it's to be redefined to
   continue scanning on EOF */

int
yywrap()
{
	static int done = 1;
	
	if (g_file_chain->prev == NULL) 
			return done;
	else
		{
			//printf("Inside yywrap's else part\n");
			filechain *tmp = g_file_chain;
			g_file_chain = g_file_chain->prev;
			free(tmp);
			yyin = g_file_chain->file;
			//yy_current_file_name is now g_file_chain->curname;(taken care of, in yy_current...()
			return !done;
		}
}

int
yyerror(
	 const char *err)
{
	#ifdef ddbug
	printf("Inside yyerror\n");
	#endif
	
	printf("Line %d # %s near '%s'\n",yy_line_no, err, yytext);
	errors++;
	return 0;
}

int foo_bar();
int yylex();

//#define ldbug 1

int
foo_bar()
{
	int ret = yylex();
	
	#ifdef ldbug
	printf("yylex returns %d\n", ret);
	#endif
	
	return ret;
}
