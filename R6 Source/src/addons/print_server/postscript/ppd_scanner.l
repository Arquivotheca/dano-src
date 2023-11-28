%{
#include <stdio.h>
#include <string.h>
#include "ppd_parser.h"

#define P /* printf */

/* #define STRDUP(x) (Z=strdup(x));fprintf(stderr, "STRDUP: 0x%x = stdup(%s)\n", Z, x); */
#define STRDUP(x) strdup(x)

void strip_trailing_quote(char*);

void* Z;		/* for STRDUP debugging only */
int yylineno = 1;
int colon_cnt = 0;

%}

%s LONGSTRING
%s TRANS
%s COLSTOP
%s UI

%s DSC_SECTION
%s LENCODE

%%

%{
	/*		
	**		Structure Keywords
	*/
%}

^"*OpenUI"		{ P("lexer: Open UI.\n"); BEGIN UI; return OPENUI; }
^"*CloseUI"		{ P("lexer: Close UI.\n"); BEGIN 0; return CLOSEUI; }
^"*OpenUIEFI"	{ P("lexer: Open UIEFI.\n"); BEGIN UI; return OPENUI; }
^"*CloseUIEFI"	{ P("lexer: Close UIEFI.\n"); BEGIN 0; return CLOSEUI; }

"Boolean"		{ P("lexer: boolean.\n"); yylval.string = STRDUP(yytext); return UITYPE; }
"PickOne"		{ P("lexer: PickOne.\n"); yylval.string = STRDUP(yytext); return UITYPE; }
"PickMany"		{ P("lexer: PickMany.\n"); yylval.string = STRDUP(yytext); return UITYPE; }

^"*OpenGroup"		{ P("lexer: Open Group.\n"); return OPENGROUP; }
^"*CloseGroup"		{ P("lexer: Close Group.\n"); return CLOSEGROUP; }

^"*OpenSubGroup"		{ P("lexer: Open Sub-Group.\n"); return OPENSUBGROUP; }
^"*CloseSubGroup"		{ P("lexer: Close Sub-Group.\n"); return CLOSESUBGROUP; }

^"*OrderDependency"			{ P("lexer: Order dependency.\n"); BEGIN DSC_SECTION; return ORDERDEPS; }
^"*NonUIOrderDependency"	{ P("lexer: Non-UI Order dependency.\n"); BEGIN DSC_SECTION; return NONUIORDERDEPS; }

<DSC_SECTION>"Prolog"			{ yylval.string = STRDUP(yytext); BEGIN 0; return SECTION; }
<DSC_SECTION>"DocumentSetup"	{ yylval.string = STRDUP(yytext); BEGIN 0; return SECTION; }
<DSC_SECTION>"PageSetup"		{ yylval.string = STRDUP(yytext); BEGIN 0; return SECTION; }
<DSC_SECTION>"AnySetup"			{ yylval.string = STRDUP(yytext); BEGIN 0; return SECTION; }
<DSC_SECTION>"JCLSetup"			{ yylval.string = STRDUP(yytext); BEGIN 0; return SECTION; }

^"*QueryOrderDependency"	{ P("lexer: Query Order dependency.\n"); return QUERYORDERDEPS; }

^"*UIConstraints"			{ P("lexer: UI Constraints.\n"); return UICONSTRAINT; }
^"*UIConstraintsEFI"		{ P("lexer: UI Constraints EFI.\n"); return UICONSTRAINT; }

^"*NonUIConstraints"		{ P("lexer: Non-UI Constraints.\n"); return NONUICONSTRAINT; }
^"*NonUIConstraintsEFI"		{ P("lexer: Non-UI Constraints EFI.\n"); return NONUICONSTRAINT; }

^"*Include"					{ P("lexer: Include.\n"); return INCLUDE; }

^"*JCLOpenUI"				{ P("lexer: JCL open UI.\n"); BEGIN UI; return JCLOPENUI; }

^"*JCLCloseUI"				{ P("lexer: JCL close UI.\n"); BEGIN 0; return JCLCLOSEUI; }


^"*Default"[A-Za-z0-9]+		{ P("lexer: Default.\n");
								yylval.string = STRDUP(yytext+8);
								return DEFAULT;}

<COLSTOP>[^:\t]*		{ P("lexer: translation string: %s\n", yytext);
						yylval.string = STRDUP(yytext); BEGIN 0;
						return TRANSLATION; }

<TRANS>[^\n]*		{ P("lexer: translation string: %s\n", yytext);
						yylval.string = STRDUP(yytext); BEGIN 0;
						return TRANSLATION; }

"/"				{ P("lexer: translation start.\n");
					if(colon_cnt) {
						P("lexer: In TRANS start state (colons = %d).\n", colon_cnt);
						BEGIN TRANS;					
					} else {
						P("lexer: In COLSTOP start state (colons = %d).\n", colon_cnt);					
						BEGIN COLSTOP;
					}
				}


^"*%".*			;

[ \t]+			;

^"*End"			{ P("lexer: got *End\n"); BEGIN 0; return END; }

"True"				{ P("lexer: True.\n"); return TRUE;}

"False"				{ P("lexer: False.\n"); return FALSE;}

"Unknown"			{ P("lexer: Unknown.\n"); return UNKNOWN;}

[-+]?[0-9]+			{ P("lexer: int: %s\n", yytext);
					yylval.ival = atoi(yytext); return INT;	}

[-+]?[0-9]+\.[0-9]*	|
[-+]?[0-9]*\.[0-9]+	{ P("lexer: real: %s\n", yytext);
					yylval.dval = atof(yytext); return REAL; }



^"*?"[A-Za-z][A-Za-z0-9._=&,]+	{ P("lexer: query start.\n"); 
								yylval.string = STRDUP(yytext+2); return QUERY; }


\*[A-Za-z0-9][A-Za-z0-9._=&,]+(-[A-Za-z0-9._]+)*		{ P("lexer: keyword: %s\n", yytext);
											yylval.string = STRDUP(yytext+1); return KEYWORD; }

[A-Za-z0-9][A-Za-z0-9._=&,\(\)%+-]*(-[A-Za-z0-9._]+)*		{ P("lexer: identifier: %s\n", yytext);
											yylval.string = STRDUP(yytext);
											return ID; }

\"[^\"\n]+\"	{ P("lexer: quote string: %s\n", yytext);
					yylval.string = STRDUP(yytext+1);
					strip_trailing_quote(yylval.string);
					P("yylval.string = [%s]\n", yylval.string);
					return QSTRING; }					

\"[\t ]*\"		{ P("lexer: empty quote string.\n");
					yylval.string = NULL; return EQSTRING; }
					
<LONGSTRING>[^\"\n]*\"[ \t]*\n	{ P("lexer: close quote: [%s]\n", yytext);
								yylval.string = STRDUP(yytext);
								strip_trailing_quote(yylval.string);		
								BEGIN 0;
								yylineno++; colon_cnt = 0;
								return CQSTRING; }

<LONGSTRING>[^\"\n]*\n	{ P("lexer: middle of string: [%s]\n", yytext);
							yylval.string = STRDUP(yytext);
							yylineno++; colon_cnt = 0;
							return MQSTRING; }					

\"[^\"\n]*\n	{ P("lexer: open quote: [%s]\n", yytext);
					yylval.string = STRDUP(yytext+1); BEGIN LONGSTRING;
					yylineno++; colon_cnt = 0;
					return OQSTRING; }


":"				{ P("lexer: found ':'\n"); colon_cnt++; return yytext[0]; }


\n				{ yylineno++; colon_cnt = 0; P("lexer: colon_cnt reset to %d\n", colon_cnt);}

.				{ P("lexer: found '%c'\n", yytext[0]); return yytext[0]; }

%%

void
strip_trailing_quote(char *str)
{
	int i;
	const int len = strlen(str);
	for (i=len-1 ; i>=0 ; i--)
	{
		if (str[i] == '"')
		{
			str[i] = '\0';
			return;
		}
	}

// Same as (?)
//	char *p = strrchr(str, '"');
//	if (p) *p=0;
}

int yywrap()
{
	//fprintf(stderr, "In yywrap()...\n");
	if(YY_CURRENT_BUFFER){
		YY_CURRENT_BUFFER->yy_is_our_buffer = 1;
		yy_delete_buffer(YY_CURRENT_BUFFER);
		YY_CURRENT_BUFFER = NULL;
	}
	return 1;
}
