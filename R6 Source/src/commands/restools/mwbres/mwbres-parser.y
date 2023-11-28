%{ 
#include <alloca.h>
#include "comptab.h"
#include <stdio.h>
#define YYDEBUG 1
int yylex();					
int yyerror(const char *);
int yyparse(void);
extern char yytext[];
%}
%token	INCLUDE
%token	ENUM
%token	DEFINE
%token	UNDEF
%token	IFDEF
%token	IFNDEF
%token	ELSE
%token	ENDIF
%token	RESOURCE
%token	PAD
%token	READ
%token	IMPORT
%token	IDENTIFIER
%token	INTEGER
%token	RCHARACTER
%token	BRACKETSTRING
%token	QUOTESTRING
%token MESSAGE
%token STRING
%token INT32
%token POINT
%token RECT
%token DATA

%%
input_file:
	  resource_file
	| blank_file
	;
		 
resource_file:
	  clause resource_file
	| clause
	;

blank_file:
	;

clause:
	  include_clause
			{	do_include($1);	}
	| define_clause
			{	do_define($1);	}
	| undef_clause
			{	do_undef($1);	}
	| ifdef_clause
			{	do_ifdef($1);	}
	| else_clause
			{	do_else($1);	}
	| endif_clause
			{	do_endif($1);	}
	| resource_clause
			{	do_resource($1);	}
	| enum_clause
			{	do_enum($1);	}
	;

include_clause:
	  INCLUDE  file_name 
			{	$$ = make_include((struct cstring *)$2); }
	;
	
file_name:
	  BRACKETSTRING
			{	$$ = make_string(yytext, "<>");	}
	| QUOTESTRING
			{	$$ = make_string(yytext, "\"\"");	}
	;

define_clause:
	  DEFINE identifier_or_value
	  		{	$$ = make_define((struct cname *)$2, NULL);	}
	| DEFINE identifier_or_value identifier_or_value
			{	$$ = make_define((struct cname *)$2, $3);	}
	;

identifier_or_value:
	  IDENTIFIER
			{	$$ = make_name(yytext);	}
	| INTEGER
			{	$$ = make_numeric(yytext, 1);	}
	| RCHARACTER
			{	$$ = make_numeric_char(yytext);	}
	| QUOTESTRING
			{	$$ = make_string(yytext, "\"\"");	}
	;

undef_clause:
	  UNDEF identifier
			{	$$ = make_undef((struct cname *)$2);	}
	;

ifdef_clause:
	  IFDEF '(' identifier ')'
			{	$$ = make_ifdef((struct cname *)$3, 0); }
	| IFDEF identifier
			{	$$ = make_ifdef((struct cname *)$2, 0);	}
	| IFNDEF '(' identifier ')'
			{	$$ = make_ifdef((struct cname *)$3, 1); }
	| IFNDEF identifier
			{	$$ = make_ifdef((struct cname *)$2, 1);	}
	;

else_clause:
	  ELSE
			{	$$ = make_else();	}
	;

endif_clause:
	  ENDIF
			{	$$ = make_endif();	}
	;

resource_clause:
	  RESOURCE resource_head data_block
			{	$$ = add_to_resource((struct cresource *)$2, (struct cdata *)$3);	}
	;

resource_head:
	  '(' character_value ',' integer_value ',' string_value ')'
			{	$$ = make_resource($2, $4, $6);	}
	;

character_value:
	  RCHARACTER
			{	$$ = make_numeric_char(yytext);	}
	| identifier
			{	$$ = get_name_value((struct cname *)$1);	}
	;

integer_value:
	  INTEGER
			{	$$ = make_numeric(yytext, 1);	}
	| identifier
			{	$$ = get_name_value((struct cname *)$1);	}
	;

string_value:
	  QUOTESTRING
			{	$$ = make_string(yytext, "\"\"");	}
 	| identifier
			{	$$ = get_name_value((struct cname *)$1);	}
	;
	
data_block:
	  '{' data_items '}'
			{	$$ = make_data_block($2); } 
 	;
 	
data_items:
	  data_item
	  		{ $$ = make_data_items($1, NULL); }
	| data_item ',' data_items
			{ $$ = make_data_items($1, $3); }
	;
	
data_item:
	  INTEGER		
	  		{ $$ = make_cdata(make_numeric(yytext, 1)); }
	| RCHARACTER
			{ $$ = make_cdata(make_numeric_char(yytext)); }
	| QUOTESTRING
			{ $$ = make_cdata(make_string(yytext, "\"\"")); }
	| identifier
			{ $$ = make_cdata(get_name_value((struct cname *)$1)); }		
	| read_item
			{ $$ = $1; } 
	| import_item
			{ $$ = $1; }
	| pad_item
			{ $$ = $1; }
	| data_block 
			{ $$ = $1; }
/*
	| message_item
			{ $$ = flatten_message($1); }
*/
	;

read_item:
	  READ '(' file_name ')'
	  		{ $$ = do_read($3); }
	;
	
import_item:
	  IMPORT '(' file_name ',' character_value ',' INTEGER { $$ = make_numeric(yytext, 1); } ')' 
	  		{ $$ = do_import($3, $5, $8); }
	| IMPORT '(' file_name ',' character_value ',' QUOTESTRING { $$ = make_string(yytext, "\"\""); } ')'
			{ $$ = do_import($3, $5, $8); }
	| IMPORT '(' file_name ',' character_value ',' identifier ')'
			{ $$ = do_import($3, $5, $7); }
	;
	
pad_item:
	  PAD '(' integer_value ')' 
			{ $$ = do_pad ($3); }

/*
message_item:
	  MESSAGE '(' integer_value ')' '{' { $$ = make_message($3) } message_data '}'
	;

message_data:
	  message_data_item ',' message_data
	| message_data_item
	;

message_data_item:
	  STRING '(' string_value ')' '{' string_value '}'
	  	{ struct cnumeric val; val.type = cnumeric; val.isinteger = 1; val.value.intval = B_STRING_TYPE;
	  	  add_message_data(&val, $3, $6); }
	| INT32 '(' string_value ')' '{' integer_value '}'
	  	{ struct cnumeric val; val.type = cnumeric; val.isinteger = 1; val.value.intval = B_INT32_TYPE;
	  	  add_message_data(&val, $3, $6); }
	| POINT '(' string_value ')' '{' point_value '}'
	  	{ struct cnumeric val; val.type = cnumeric; val.isinteger = 1; val.value.intval = B_POINT_TYPE;
	  	  add_message_data(&val, $3, $6); }
	| RECT '(' string_value ')' '{' rect_value '}'
	  	{ struct cnumeric val; val.type = cnumeric; val.isinteger = 1; val.value.intval = B_RECT_TYPE;
	  	  add_message_data(&val, $3, $6); }
	| DATA '(' integer_value ',' string_value ')' data_block
	  	{ add_message_data($3, $5, $7); }
	;
*/
 
enum_clause:
	  ENUM '{' enum_declarators '}' ';'
			{	$$ = $3;	}
	| ENUM IDENTIFIER '{' enum_declarators '}' ';'
			{	$$ = $4;	}
	;

enum_declarators:
	  enum_declarator
			{	$$ = $1;	}
	| enum_declarator ',' enum_declarators
			{	$$ = add_to_enum((struct cenum *)$1, (struct cenum *)$3);	}
	;

enum_declarator:
	  identifier
			{	$$ = make_enum((struct cname *)$1, NULL);	}
	| identifier '=' integer_value
			{	$$ = make_enum((struct cname *)$1, $3);	}
	| identifier '=' RCHARACTER
			{	$$ = make_enum((struct cname *)$1, make_numeric_char(yytext));	}
	;

identifier:
	  IDENTIFIER
			{	$$ = make_name(yytext);		}
	;

