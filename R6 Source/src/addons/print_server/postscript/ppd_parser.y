%{

#define P  /* printf */
#define PP P

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "parser_interface.h"

/* #define FREE(x) {fprintf(stderr, "\tFREE(0x%x): [%s]\n", x, x); free(x); } */
#define FREE(x) free(x)

extern int yylineno;
extern char* yytext;

int invoke_on = 0;	
%}

%union {
	char *string;
	int ival;
	double dval;
}

%token OPENUI
%token CLOSEUI
%token OPENGROUP
%token CLOSEGROUP
%token OPENSUBGROUP
%token CLOSESUBGROUP
%token ORDERDEPS
%token NONUIORDERDEPS
%token QUERYORDERDEPS
%token UICONSTRAINT
%token NONUICONSTRAINT
%token INCLUDE
%token END

%token JCLOPENUI
%token JCLCLOSEUI

%token <string> DEFAULT
%token TRUE
%token FALSE
%token UNKNOWN


%token <string> ENCODING
%token <string> SECTION

%token <string> KEYWORD
%token <string> ID
%token <string> OQSTRING
%token <string> MQSTRING
%token <string> CQSTRING
%token <string> QSTRING
%token <string> EQSTRING
%token <string> STRING
%token <string> IMAGEABLE
%token <string> PAPERDIMEN
%token <string> UITYPE
%token REGION
%token <ival> INT
%token <dval> REAL
%token <string> QUERY
%token NL
%token <string> TRANSLATION

%type <dval> num
%type <string> boolean

%type <string> word

%type <string> stringrule
%type <string> option
%type <string> qstring
%type <string> translation
%type <string> string_thing
%type <string> oqstring

%%

start:	/* empty */
	| list { P("parser: Done.\n"); }
	;

list:	nv
	|	list nv
	|	invocation
	|	list invocation
	|	quoteval
	|	list quoteval
	| 	symbolval
	| 	list symbolval
	|	stringval
	|	list stringval 
	|	query
	| 	list query
 	|	imageable_area
 	|	list imageable_area
 	|	paper_dimension
 	|	list paper_dimension

	|	openui
	|	list openui
	|	closeui
	|	list closeui
	|	opengroup
	|	list opengroup
	|	closegroup
	|	list closegroup
	|	opensubgroup
	|	list opensubgroup
	|	closesubgroup
	|	list closesubgroup
	|	orderdeps
	|	list orderdeps
	|	nonui_orderdeps
	|	list nonui_orderdeps
	|	query_orderdeps
	|	list query_orderdeps
	|	ui_constraint
	|	list ui_constraint
	|	nonui_constraint
	|	list nonui_constraint
	|	include
	|	list include

	
	|	jclopenui
	|	list jclopenui
	|	jclcloseui
	|	list jclcloseui
	
	
	|	default
	|	list default
	;

/*
**		Structure Keywords
*/

openui: OPENUI KEYWORD translation ':' UITYPE { P("parser: openui.\n"); open_ui($2, $3, $5); FREE($2); if($3) FREE($3); FREE($5); }
	;

closeui: CLOSEUI ':' KEYWORD		{ P("parser: closeui.\n"); close_ui($3); FREE($3); }
	;

opengroup:	OPENGROUP ':' ID translation { P("parser: opengroup.\n"); open_group($3, $4); FREE($3); if($4) FREE($4); }
	;
	
closegroup: CLOSEGROUP ':' ID				{ P("parser: closegroup.\n"); close_group($3); FREE($3); }
	|		CLOSEGROUP ':' ID translation	{ P("parser: closegroup.\n"); close_group($3); FREE($3); FREE($4); }
	;

opensubgroup:	OPENSUBGROUP ':' ID translation { /* nothing yet */ FREE($3); if($4) FREE($4);}
	;
	
closesubgroup: CLOSESUBGROUP ':' ID			{ /* nothing yet */ FREE($3); }
	;


orderdeps:	ORDERDEPS ':' num SECTION KEYWORD	{ /* nothing yet */ FREE($4); FREE($5);}
	|		ORDERDEPS ':' num SECTION KEYWORD ID	{ /* nothing yet */ FREE($4); FREE($5); FREE($6); }
	;


nonui_orderdeps:	NONUIORDERDEPS ':' num SECTION KEYWORD 	{ /* nothing yet */ FREE($4); FREE($5);}
	| 				NONUIORDERDEPS ':' num SECTION KEYWORD ID { /* nothing yet */ FREE($4); FREE($5); FREE($6);}
	;	 

query_orderdeps:	QUERYORDERDEPS ':' num KEYWORD { /* nothing yet */ FREE($4);}
	|				QUERYORDERDEPS ':' num KEYWORD ID { /* nothing yet */ FREE($4); FREE($5);}
	;

	
ui_constraint:		UICONSTRAINT ':' KEYWORD KEYWORD	{ P("parser: ui_constraint.\n"); add_ui_constraint($3, NULL, $4, NULL); FREE($3); FREE($4);}
	|				UICONSTRAINT ':' KEYWORD word KEYWORD	{ P("parser: ui_constraint.\n"); add_ui_constraint($3, $4, $5, NULL); FREE($3); FREE($4); FREE($5);}
	|				UICONSTRAINT ':' KEYWORD KEYWORD word	{ P("parser: ui_constraint.\n"); add_ui_constraint($3, NULL, $4, $5); FREE($3); FREE($4); FREE($5); }
	|				UICONSTRAINT ':' KEYWORD word KEYWORD word	{ P("parser: ui_constraint.\n");add_ui_constraint($3, $4, $5, $6);  FREE($3); FREE($4); FREE($5); FREE($6);}
	;

nonui_constraint:	NONUICONSTRAINT ':' KEYWORD KEYWORD	{ /* nothing yet */  FREE($3); FREE($4);}
	|				NONUICONSTRAINT ':' KEYWORD word KEYWORD	{ /* nothing yet */ FREE($3); FREE($4); FREE($5); }
	|				NONUICONSTRAINT ':' KEYWORD KEYWORD word	{ /* nothing yet */  FREE($3); FREE($4); FREE($5);}
	|				NONUICONSTRAINT ':' KEYWORD word KEYWORD word	{ /* nothing yet */  FREE($3); FREE($4); FREE($5); FREE($6);}
	;

include:	INCLUDE qstring		{ /* nothing yet */ }
	;


/*
**		General Information Keywords
*/

/*
**		Basic Device Capability Keywords
*/

/*
**		System Management Keywords
*/


jclopenui: JCLOPENUI KEYWORD translation ':' UITYPE { P("parser: jcl_open_ui.\n");jcl_open_ui($2, $3, $5);  FREE($2); if($3) FREE($3); FREE($5);}
	;

jclcloseui: JCLCLOSEUI ':' KEYWORD		{ P("parser: jcl_close_ui.\n"); jcl_close_ui($3); FREE($3); }
	;


word:		boolean		{ $$ = $1; }
	|		ID			{ $$ = $1; }
	|		REAL		{ char str[64]; sprintf(str, "%f", $1); $$=strdup(str); }
	|		INT			{ char str[64]; sprintf(str, "%d", $1); $$=strdup(str); }
	;

boolean:	TRUE					{ $$ = strdup("True"); }
	|		FALSE					{ $$ = strdup("False"); }
	|		UNKNOWN					{ $$ = strdup("Unknown"); }
	;

default:	DEFAULT ':' option translation	{ PP("DEFAULT! [%s][%s][%s]\n", $1, $3, $4);
												set_default($1, $3, $4);  FREE($1); FREE($3); if($4) FREE($4);}
	|		DEFAULT ':' boolean	{ PP("DEFAULT! [%s][%s]\n", $1, $3);
									set_default($1, $3, $3); FREE($1); FREE($3); }
	|		DEFAULT ':' num		{ PP("parser: default INT: %s.\n", $1);  FREE($1);}
	;


num:		REAL					{ $$ = $1; }
	|		INT						{ $$ = $1; }
	;


nv:		KEYWORD ':' 							{ P("parser: No value: %s\n", $1); FREE($1);}
	;

imageable_area: IMAGEABLE ID translation ':' qstring	{ imageable_area($2, $3, $5); FREE($2); if($3) FREE($3); }
	;

paper_dimension: PAPERDIMEN ID translation ':' qstring	{ paper_dimension($2, $5);  FREE($2); if($3) FREE($3); }
	;
 

query:	QUERY ':' qstring		{ P("parser: Query string.\n"); FREE($1);}
	|	QUERY ':' EQSTRING		{ P("parser: Query Empty string.\n");}
	|	QUERY ':' oqstring mqstrings cqstring  { P("parser: Long Query string.\n"); FREE($1);}
	|	QUERY ':' oqstring mqstrings cqstring END { P("parser: Long Query string.\n"); FREE($1);}
	;


quoteval:	left_quoteval right_invocation 		{ }
	;

left_quoteval:	KEYWORD ':' {	P("parser: Left Quoted value: %s.\n", $1);
								start_invocation($1, NULL, NULL);
								invoke_on = 1;
								FREE($1);
							}
	;


invocation:	left_invocation right_invocation  	{ }
	;						

left_invocation: KEYWORD option translation ':'	{ P("left invoke 1\n"); start_invocation($1, $2, $3); invoke_on = 1; FREE($1); FREE($2); if($3) { FREE($3); } }
	|	KEYWORD boolean translation ':'	{ P("left invoke 2\n"); start_invocation($1, $2, $3); invoke_on = 1;  FREE($1); FREE($2); if($3) { FREE($3);} }
	|	KEYWORD num translation ':'	{ char str[64]; sprintf(str, "%d", $2); P("left invoke 3\n"); start_invocation($1, str, $3); invoke_on = 1; FREE($1); if($3) { FREE($3); } }
	;
	
right_invocation: string_thing	{ P("parser: evaluating right_invocation.\n"); invoke_on = 0; }
	;


string_thing:	qstring	translation					{ $$ = $1; if($2) FREE($2);}
	|			EQSTRING 							{ $$ = $1; }
	|			EQSTRING END						{ $$ = $1; }
	|			qstring translation END				{ $$ = $1; if($2) FREE($2);}
	|			oqstring mqstrings cqstring			{ $$ = $1; }
	|			oqstring mqstrings cqstring END		{ $$ = $1; }
	;

	
qstring:	QSTRING				{ $$ = $1; P("parser: in qstring, invoke_on = %d\n", invoke_on);
									if(invoke_on) {
										add_invocation_line($1);
									} else {
										FREE($1);
									}
								}
	;


oqstring:	OQSTRING			{ $$ = $1;
									if(invoke_on) {
										add_invocation_line($1);
									} else {
										FREE($1);
									}
								}
	;

mqstrings:	/* empty */
	|		mqstrings mqstring	{ P("parser: mqstring MQSTRING.\n"); }
	;

mqstring:	MQSTRING			{	if(invoke_on) {
										add_invocation_line($1);
									} else {
										FREE($1);
									}
								}
	;
	
cqstring:	CQSTRING			{ 	if(invoke_on) {
										add_invocation_line($1);
									} else {
										FREE($1);
									}
								}
	;

stringval:	KEYWORD ':' stringrule		{ P("parser: String value, no option.\n"); FREE($1); FREE($3);}
	|		KEYWORD option translation ':' stringrule	{ FREE($1); FREE($2); if($3) { FREE($3); }; FREE($5);}
	;


stringrule:	ID translation					{ P("parser: stringrule: ID.\n"); $$ = $2; FREE($1);}	
	|		stringrule ID translation		{ P("parser: stringrule ID.\n"); $$ = $3; FREE($1); FREE($2);}
	|		INT					{  char str[256]; P("parser: stringrule: INT.\n");sprintf(str, "%d", $1); $$ = strdup(str);}
	|		stringrule INT		{  char str[256]; P("parser: stringrule INT.\n"); sprintf(str, "%d", $2); $$ = strdup(str); FREE($1);}
	|		REAL				{  char str[256]; P("parser: stringrule: REAL.\n"); sprintf(str, "%f", $1); $$ = strdup(str);}
	|		stringrule REAL		{  char str[256]; P("parser: stringrule REAL.\n"); sprintf(str, "%f", $2); $$ = strdup(str); FREE($1);}
	|		boolean				{  $$ = $1; }
	|		stringrule qstring	{  $$ = $1; }
	;

	
	
symbolval:	KEYWORD ':' '^' ID 		{ P("parser: Symbol val, no option.\n"); FREE($1); FREE($4); }
	|		KEYWORD option translation ':' '^' ID 		{ P("parser: Symbol val, option.\n"); FREE($1); FREE($2); if($3) {FREE($3); }; FREE($6); }
	;	


option:		ID					{ $$ = $1; P("parser: option: id: %s\n", $1); }
	|		'^' ID				{ $$ = $2; P("parser: option: ^id: %s\n", $2); }
	|		KEYWORD				{ $$ = $1; P("parser: option: *id: %s\n", $1); }
	;


translation:	/* empty */		{ $$ = NULL; }
	|		TRANSLATION			{ $$ = $1;P("parser: got translation: %s\n", $1); }
	;


	
	
%%

extern FILE *yyin;
int yyerror(const char*);


int yyerror(const char* err)
{
	printf("*** PPD %s while parsing the string:\n***\t%s\n*** on or near line %d.\n",
			err, yytext, yylineno);

	return 0;
}

#ifdef PARSETEST

int main(int argc, char **argv)
{
	int rv;
	FILE *file;
	if(argc != 2)
	{
		printf("Usage: %s <filename>\n", argv[0]);
		exit (0);
	}
	
	file = fopen(argv[1], "r");
	if(!file)
	{
		printf("Couldn't open file: %s.\n", argv[1]);
		exit(0);
	}
	
	yyin = file;
	rv = yyparse();

	if(rv == 0)
		printf("%s: Parsed okay.\n", argv[1]);
	else
		printf("%s: Parse failed.\n", argv[1]);

	return 0;
}

#endif
