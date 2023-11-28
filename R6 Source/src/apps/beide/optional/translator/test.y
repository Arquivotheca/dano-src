%{ 
int yylex();					
int yyerror(const char *);
int yyparse(void);
extern char yytext[];
%}
%token	IFDEF
%token	IFNDEF
%token	IDENTIFIER

%%

ifdef_clause:
	  IFDEF identifier
			{	$$ = make_ifdef((struct cname *)$2, 0);	}
	| IFNDEF identifier
			{	$$ = make_ifdef((struct cname *)$2, 1);	}
	;

identifier:
	  IDENTIFIER
			{	$$ = make_name(yytext);		}
	;

