/*	yy_funcs.h	*/

#if !defined(YY_FUNCS_H)
#define YY_FUNCS_H

#include <stdio.h>

extern FILE * yy_include_file(FILE * to_include, const char * filename);
/*	returns NULL for main file, or file name string for include file	*/
extern const char * yy_current_file_name();
extern int yy_current_file_line();
extern int yyerror(const char *str);

#endif

