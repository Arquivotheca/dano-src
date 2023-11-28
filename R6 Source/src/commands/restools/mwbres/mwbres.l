%{
#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include "mwbres-parser.h"
#include "yy_funcs.h"
#include "filechain.h"

void panic(const char *fmt, ...);
int yyerror(const char *);

int yylex();
#define YY_ALWAYS_INTERACTIVE 1

//filechain defn moved to filechain.h

filechain *g_file_chain;
int yy_line_no = 1; 

FILE *
yy_include_file(
	FILE * to_include,
	const char * name)
{
	//printf("Inside yy_include_file(...)\n");
	//printf(" the yyin now is %p\n", yyin);
	
	filechain *newchain = (filechain *)malloc(sizeof(filechain));
	int ix;
	filechain *test = g_file_chain;
	for (ix=0; ix<32; ix++)
	{
		if (!test) break;
		test = test->prev;
	}
	if (test)	/*	too deep include chain?	*/
	{
		fclose(to_include);
		panic("too deep include file nesting");
		return NULL;
	}
	
	newchain->prev = g_file_chain;
		
	yyin = to_include; //make the included file be the input file to be parsed now.

	newchain->file = yyin;
	
	newchain->curname = (char *)malloc(1+strlen(name));
	strcpy(newchain->curname, name);

	newchain->line = yy_line_no;
	
	yy_line_no = 0;	/*	since we'll see newline after #include!	*/
	
	g_file_chain = newchain;
	
	//printf("About to exit yy_include_file\n");
	
	return yyin;
	
}

const char *
yy_current_file_name()
{
	if (g_file_chain)
		return g_file_chain->curname;
	return NULL;
}

int
yy_current_file_line()
{
	return yy_line_no;
}

#if 0
<T>message						{ return MESSAGE; }
<T>string						{ return STRING; }
<T>int32						{ return INT32; }
<T>point						{ return POINT; }
<T>rect							{ return RECT; }
<T>data							{ return DATA; }
#endif

%}
%state T
%state CCOMMENT
%state CPPCOMMENT
%%
								{ BEGIN T; }
<T>\/\*							{ BEGIN CCOMMENT; }
<CCOMMENT>\*\/					{ BEGIN T; }
<CCOMMENT>.						{ /* eat comment */ }
<CCOMMENT>[\n]					{ yy_line_no++; }
<T>\/\/							{ BEGIN CPPCOMMENT; }
<CPPCOMMENT>.					{ /* eat comment */ }
<CPPCOMMENT>[\n]				{ yy_line_no++; BEGIN T; }
<T>"#"[ \t]*include				{ return INCLUDE; }
<T>"#"[ \t]*define				{ return DEFINE; }
<T>"#"[ \t]*undef				{ return UNDEF; }
<T>"#"[ \t]*ifdef				{ return IFDEF; }
<T>"#"[ \t]*if[ \t]+defined		{ return IFDEF; }
<T>"#"[ \t]*ifndef				{ return IFNDEF; }
<T>"#"[ \t]*if[ \t]+\!defined	{ return IFNDEF; }
<T>"#"[ \t]*else				{ return ELSE; }
<T>"#"[ \t]*endif				{ return ENDIF; }
<T>enum							{ return ENUM; }
<T>resource						{ return RESOURCE; }
<T>pad							{ return PAD; }
<T>read							{ return READ; }
<T>import						{ return IMPORT; }
<T>[_a-zA-Z][_a-zA-Z0-9]*		{ return IDENTIFIER; }
<T>0x[0-9a-fA-F]+					{ return INTEGER; }
<T>-?[0-9]+						{ return INTEGER; }
<T>"'".{1,4}"'"					{ return RCHARACTER; }
<T>"<".{1,1023}">"				{ return BRACKETSTRING; }
<T>\"([^"\\]|(\\.)){0,1023}\"	{ return QUOTESTRING; }
<T>[ \t\r]						{ /* ignore */ }
<T>[\n]							{ yy_line_no++; }
<T>.							{ return yytext[0]; }
