%{

#include "ArrayParserState.h"
#include "array_scanner.h"

#include <Debug.h>
#include <ctype.h>

#define yywrap()	1

#define YY_DECL int yylex YY_PROTO(( void* lvalp, ArrayParserState* pState ))

#define YY_NO_UNPUT

%}


letter		[a-zA-Z]
bindigit	[01]
octdigit	{bindigit}|[2-7]
digit		{octdigit}|[89]
hexdigit	{digit}|[a-fA-F]
alnum		{letter}|{digit}
exponent	[eE][+-]?{digit}+
real		({digit}+\.{digit}*|{digit}*\.{digit}+)
ident		[a-zA-Z_][a-zA-Z0-9_]*

%x string
%x hexstring
%x comment
%x cc
%x incl
%x define
%option yylineno
%option nostdinit
%option never-interactive
%option batch

%%
														YYSTYPE& yylval = *(YYSTYPE*)lvalp;
														char* d = 0;

	/* C type keywords */
char													return CHAR;
int														return INT;
short													return SHORT;
long													return LONG;
signed													return SIGNED;
unsigned												return UNSIGNED;
"\#define"												return DEFINE;

	/* common typedefs as keywords */
uchar													return UCHAR;
ushort													return USHORT;
ulong													return ULONG;
int8													return INT8;
uint8													return UINT8;
int16													return INT16;
uint16													return UINT16;
int32													return INT32;
uint32													return UINT32;
int64													return INT64;
uint64													return UINT64;

	/* symbolic types */
{ident}													{
															yylval.SetString(yytext);
															return IDENTIFIER;
														}

\"														BEGIN(string); yylval.SetString("");
<string>\"												BEGIN(INITIAL); return STRING;
<string>\n												{
															pState->Error(B_BAD_VALUE, "Strings can not span multiple lines.");
															REJECT;
														}
<string>\\0[bB]{bindigit}+								yylval.StringObject() << (char)strtol(yytext + 3, &d, 2);
<string>\\{octdigit}{3}									yylval.StringObject() << (char)strtol(yytext + 1, &d, 8);
<string>\\0[dD]{digit}{3}								yylval.StringObject() << (char)strtol(yytext + 3, &d, 10);
<string>\\0[xX]{hexdigit}{2}							yylval.StringObject() << (char)strtol(yytext + 3, &d, 16);
<string>\\${hexdigit}{2}								yylval.StringObject() << (char)strtol(yytext + 2, &d, 16);
<string>\\t												yylval.StringObject() << (char)'\t';
<string>\\n												yylval.StringObject() << (char)'\n';
<string>\\r												yylval.StringObject() << (char)'\r';
<string>\\b												yylval.StringObject() << (char)0x08;
<string>\\?												yylval.StringObject() << (char)0x7F;
<string>\\f												yylval.StringObject() << (char)0x0C;
<string>\\v												yylval.StringObject() << (char)0x0B;
<string>\\.												yylval.StringObject() << (char)yytext[1];
<string>[^\\\"\n]+										yylval.StringObject() << yytext;
\'														BEGIN(cc); yylval.SetNumber(0);
<cc>\'													BEGIN(INITIAL); return NUMBER;
<cc>\\t													yylval.SetNumber( (yylval.Number() << 8) | '\t' );
<cc>\\n													yylval.SetNumber( (yylval.Number() << 8) | '\n' );
<cc>\\r													yylval.SetNumber( (yylval.Number() << 8) | '\r' );
<cc>\\b													yylval.SetNumber( (yylval.Number() << 8) | 0x08 );
<cc>\\?													yylval.SetNumber( (yylval.Number() << 8) | 0x7F );
<cc>\\f													yylval.SetNumber( (yylval.Number() << 8) | 0x0C );
<cc>\\v													yylval.SetNumber( (yylval.Number() << 8) | 0x0B );
<cc>\\.													yylval.SetNumber( (yylval.Number() << 8) | yytext[1] );
<cc>.													yylval.SetNumber( (yylval.Number() << 8) | yytext[0] );
"/*"													BEGIN(comment);
<comment>[^*\n]*
<comment>"*"+[^*/\n]*
<comment>\n
<comment>"*"+"/"										BEGIN(INITIAL);
"//"[^\n]*
0[xX]{hexdigit}{1,8}									yylval.SetNumber(strtoul(yytext + 2, &d, 16)); return NUMBER;
${hexdigit}{1,8}										yylval.SetNumber(strtoul(yytext + 1, &d, 16)); return NUMBER;
0{octdigit}{1,12}										yylval.SetNumber(strtoul(yytext, &d, 8)); return NUMBER;
0[bB]{bindigit}{1,32}									yylval.SetNumber(strtoul(yytext + 2, &d, 2)); return NUMBER;
[-+]?{digit}+											yylval.SetNumber(strtoul(yytext, &d, 10)); return NUMBER;
<<EOF>>													return EOS;
[ \n\r\t]+
.														return yytext[0];
%%

status_t StartScanningString(const char* data, size_t size)
{
	YY_BUFFER_STATE old = YY_CURRENT_BUFFER;
	yy_delete_buffer(old);
	YY_BUFFER_STATE buf = yy_scan_bytes(data, size);
	if( buf ) {
		yy_switch_to_buffer(buf);
		return B_OK;
	}
	return B_NO_MEMORY;
}

status_t StopScanningString()
{
	YY_BUFFER_STATE old = YY_CURRENT_BUFFER;
	yy_delete_buffer(old);
	return B_OK;
}
