#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OS.h>
#include <signal.h>
#include <Debug.h>

#include "ush.h"


extern command commands[];



// Lookup a command (argv[0]) and dispatch it 
//
status_t
execute(int argc, char **argv)
{
	int i;
		
	for(i=0;commands[i].func;i++){
		if(!strcmp(argv[0],commands[i].name)) {
			argc--; argv++;
			if(argc < commands[i].minargs){
				fprintf(stderr,"%s: requires at least %d argument%s\n",
						commands[i].name,commands[i].minargs,commands[i].minargs>1?"s":"");
				return 1;
			} else {
				return commands[i].func(argc,argv);
			}
		}
	}

	// fallback
	return do_run(argc, argv);	
}

status_t 
execute_stmt(int argc, char **argv, context *ctxt)
{
	char *x;
	int skip = ctxt->skip[ctxt->depth];
	
	if(argc < 1) return 0;

	if((argc == 1) && ((x = strchr(argv[0],'=')) != NULL)){
		if(!skip) putenv(argv[0]);
		return 0;
	}
	
	if(!strcmp(argv[0],"while")){
		if(ctxt->interactive){
			fprintf(stderr,"while not available to interactive shells\n");
			exit(1);
		}
		ctxt->depth++;
		ctxt->posn[ctxt->depth] = ctxt->cur_posn;
		if(skip){
			ctxt->skip[ctxt->depth] = 1;
		} else {
			if(execute(argc-1,argv+1)){
				ctxt->skip[ctxt->depth] = 1;
			} else {
				ctxt->skip[ctxt->depth] = 0;
			}
		}
		return 0;
	}
	
	if(!strcmp(argv[0],"wend")){
		if(ctxt->depth){
			if(!ctxt->skip[ctxt->depth]){
				/* not skipping, repeat our while loop */
				fseek(ctxt->fp, ctxt->posn[ctxt->depth], SEEK_SET);
			}
			ctxt->depth--;
			return 0;
		} else {
			fprintf(stderr,"too many \"wend\"'s\n");
			exit(1);
		}
	}
	
	if(!strcmp(argv[0],"if")){
		ctxt->depth++;
		
		if(skip){
			ctxt->skip[ctxt->depth] = 1;
		} else {
			if(execute(argc-1,argv+1)){
				ctxt->skip[ctxt->depth] = 1;
			} else {
				ctxt->skip[ctxt->depth] = 0;
			}
		}
		return 0;
	}	
	
	if(!strcmp(argv[0],"fi")){
		if(ctxt->depth){
			ctxt->depth--;
			return 0;
		} else {
			fprintf(stderr,"too many \"fi\"'s\n");
			exit(1);
		}
	}
	
	if(!strcmp(argv[0],"else")){
		if(ctxt->depth){
			if(!ctxt->skip[ctxt->depth-1]){
				ctxt->skip[ctxt->depth] = !skip;
			}
			return 0;
		} else {
			fprintf(stderr,"stray \"else\"\n");
			exit(1);
		}
	}

	if(skip) {
		return 0;
	} else {
		ctxt->last_result = execute(argc, argv);
		return ctxt->last_result;
	}
}

#define sSPACE       0
#define sTOKEN       1
#define sQUOTED      2
#define sESCAPE      3
#define sEXPAND      4
#define sEXPAND2     5

//	This should probably be broken out into a parser which returns a
//	"parsed command" structure, and a separate function which runs
//	the actual parsed command.
//	That will be necessary to support pipes.
status_t
parse(char *line, context *ctxt)
{
	char token[4096];
	char *tokens[MAXTOKENS+1];
	int redirectInto = -1;
	int redirectOutof = -1;
	int redirectAppend = 0;
	char *x= NULL;
	char *var= NULL;
	int i, n, state,last;
	status_t res = 0;
	int save0 = -1;
	int save1 = -1;
	int save2 = -1;

	/* State machine variables summary:
	   char* line: Input line from script or tty. Increments with the
	           parser state.
	   char* x: Next character in current token storage to be copied from
	           line ("*x = *line" style).
	   char* var: Points to the beginning of the nearest parsed
	           shell variable in the current token.  The variable is
	           read into the token first, then expanded in-place.
	   char* token: Current token as copied from line.  (x, var
	           point into this array.)
	   char** tokens: Array of tokens (shell words) as read from line.
	 */

	n = 0;
	last = state = sSPACE;
	
	while(*line) {
		switch(state){
		case sSPACE:
			if(*line == '#') goto done;
			
			if(isspace(*line)) break;
			
			if (*line == '>') {
				redirectInto = n;
				if (line[1] == '>')
				{
					line++;
					redirectAppend = 1;
				}
				break;
			}
			if (*line == '<') {
				redirectOutof = n;
				break;
			}
			x = token;
			state = sTOKEN;
			// fall through
			
		case sTOKEN:
			if(*line == '$') {
				var = x;
				last = sTOKEN;
				state = sEXPAND;
				break;
			}
			if(*line == '"') {
				state = sQUOTED;
				break;
			}
			if(*line == '\\'){
				last = sTOKEN;
				state = sESCAPE;
				break;
			}
			if(isspace(*line)) {
				*x = 0;

				/* HACK alert: supporting unquoted-var-expansion
				   (see discussion below)
				
				   Goal 1: allow empty tokens ("")
				   
				   Goal 2: allow expansion of an unquoted variable to
				   possibly change the number of words in the input
				   (Bourne shell allows arbitrary word changes)
				   
				   Compromise: allow an unquoted shell variable to
				   add either 0 or 1 words to the input.
				   We do this by throwing away the new token if
				   (a) the expansion was unquoted and (b) the token
				   is empty. */
				if (last == sQUOTED || token != x) {
					tokens[n] = strdup(token);
					n++;
					if(n == MAXTOKENS) goto done;
				}
								
				state = sSPACE;
			} else {
				*x = *line; x++;
			}
			break;
		
		case sEXPAND:
			if(*line == '?'){
				sprintf(var, "%d", ctxt->last_result);
				x += strlen(var);
				state = last;
				break;
			}
			if(isdigit(*line)){
				// handle argument vars
				state = last;
				break;
			}
			
			if(isalpha(*line) || (*line == '_')){
				*x = *line;
				x++;
				state = sEXPAND2;
				break;
			}
			
			*x = '$'; x++;
			state = last;
			continue;
			
		case sEXPAND2:
			if(isalnum(*line) || (*line == '_')){
				*x = *line;
				x++;
				break;
			} else {
				char *env;
				*x = 0;
				env = getenv(var);
				if(env){
					/* HACK alert: unquoted-variable-expansion is broken
					
					   This is where ush's very simplistic variable
					   expansion starts to cause trouble.  We always
					   extract the contents of an expanded variable
					   into a *single* token, as if it were in quotes.
					   To be truly Bourne-esque, we should extract
					   each word of $FOO into a new token if $FOO is
					   introduced without quotes in the input stream.
					   
					   (Further reading: sh(1), EXPANSION section;
					   pay particular attention to $FOO, "$FOO",
					   word arity of expression expansions, etc.) */
					strcpy(var,env);            /* <- hack here */
					x = var + strlen(var); /* walk x to end of expanded text in current token */
				} else {
					x = var;
				}
				state = last;
				continue;
			}
			
		case sQUOTED:
			if(*line == '$') {
				var = x;		/* note: var points into x's token string */
				/* HACK alert:
				
				   To get around ush's broken unquoted-variable-expansion
				   (see HACK alert above), we keep track of the fact that
				   we entered a $var state from within a quoted state.
				   This way, we can simulate the difference between $foo
				   and "$foo" (if -z $foo, then the former does not
				   introduce new tokens, whereas the latter will introduce
				   an empty token). */
				last = sQUOTED;
				state = sEXPAND;
				break;
			}
			if(*line == '\\'){
				last = sQUOTED;
				state = sESCAPE;
				break;
			}
			if(*line == '"'){
				last = sQUOTED;
				state = sTOKEN;
			} else {
				*x = *line;
				x++;
			}
			break;			
			
		case sESCAPE:
			switch(*line){
			case 'n': *x = '\n'; break;
			case 'r': *x = '\r'; break;
			case 't': *x = '\t'; break;
			case '"': *x = '"'; break;
			case '\'': *x = '\''; break;
			default: *x = *line;
			}
			x++;
			state = last;
			break;
		}
		line++;
	}
done:
	if (redirectInto >= 0)
	{
		if ((redirectInto < 1) ||
			((redirectInto != n-1) &&
				((redirectOutof == -1) ||
				(redirectInto != n-2))))
		{
			fprintf(stderr, "parse error: redirect and argument must be last\n");
			res = -1;
		}
	}
	if (!res && redirectOutof >= 0)
	{
		if ((redirectOutof < 1) ||
			((redirectOutof != n-1) &&
				((redirectInto == -1) ||
				(redirectOutof != n-2))))
		{
			fprintf(stderr, "parse error: redirect and argument must be last\n");
			res = -1;
		}
	}
	fflush(stderr);
	fflush(stdout);
	fflush(stdin);
	if (!res && redirectInto > 0)
	{
		int one;
		save1 = dup(1);
		save2 = dup(2);
		close(1);
		close(2);
		/*	open() and dup() are GUARANTEED to use the lowest-available file descriptor!	*/
		one = open(tokens[redirectInto], O_WRONLY|O_CREAT|(redirectAppend ? O_APPEND : O_TRUNC), 0666);
		if (one < 0)
		{
			dup(save1);
			dup(save2);
			close(save1);
			close(save2);
			perror(tokens[redirectInto]);
			res = -1;
		}
		else
		{
			ASSERT(one == 1);
			dup(one);	//	into 2
		}
		free(tokens[redirectInto]);
		n--;
	}
	if (!res && redirectOutof > 0)
	{
		int zero;
		save0 = dup(0);
		close(0);
		/*	open() and dup() are GUARANTEED to use the lowest-available file descriptor!	*/
		zero = open(tokens[redirectOutof], O_RDONLY);
		if (zero < 0)
		{
			dup(save0);
			close(save0);
			/*	if we re-directed output, too, then clean that up	*/
			if (save1 > -1)
			{
				close(1);
				close(2);
				dup(save1);
				dup(save2);
				close(save1);
				close(save2);
			}
			perror(tokens[redirectOutof]);
			res = -1;
		}
		else
		{
			ASSERT(zero == 0);
		}
		free(tokens[redirectOutof]);
		n--;
	}
	tokens[n] = NULL;

	if (!res)
	{
		res = execute_stmt(n, tokens, ctxt);
	}
	fflush(stdin);
	fflush(stderr);
	fflush(stdout);

	if (save0 > -1)
	{
		close(0);
		dup(save0);
		close(save0);
	}
	if (save1 > -1)
	{
		close(1);
		dup(save1);
		close(save1);
	}
	if (save2 > -1)
	{
		close(2);
		dup(save2);
		close(save2);
	}

	for(i=0;i<n;i++) free(tokens[i]);

	return res;
}

status_t runfile(FILE *f, int interactive)
{
	context ctxt;
	status_t res= B_OK;
	char buffer[4098];
	int l;

	if(interactive) {
		init_readline(fileno(f));
	}

	ctxt.depth = 0;
	ctxt.skip[0] = 0;
	ctxt.last_result = 0;
	ctxt.interactive = interactive;
	ctxt.fp = f;

	for(;;){
		if(interactive) {
			const char *prompt = getenv("PROMPT");
			if(!prompt) prompt = "% ";
			fflush(stdout);
			readline(fileno(f),fileno(stdout),prompt,buffer);
		} else {
			ctxt.cur_posn = ftell(ctxt.fp);
			if(fgets(buffer, 4096, ctxt.fp) == NULL) break;
		}
		l = strlen(buffer);
		buffer[l] = ' ';
		buffer[l+1] = 0;
		res= parse(buffer,&ctxt);
	}
	return res;
}

int main(int argc, char **argv)
{
	int fd;	
	status_t res;
	thread_id thid = find_thread(NULL);
	
	argc--;
	argv++;

	fd = open("/dev/null",O_RDONLY);
	if(fd == 0){
		putenv("STATUS=REPLACED_FDS");
		fd = open("/dev/null",O_WRONLY);
		fd = open("/dev/null",O_WRONLY);
	} else {
		close(fd);
	}

	/* ignore ^C and lead our process group to greater glory */
	signal( SIGHUP, SIG_IGN);
	signal( SIGQUIT, SIG_IGN);
	
	setpgid( thid, thid);	
	ioctl(0, 'pgid', thid);
	ioctl(1, 'pgid', thid);
	ioctl(2, 'pgid', thid);	
			
	signal( SIGHUP, SIG_IGN);
	signal( SIGQUIT, SIG_IGN);
	
	while(argc){
		if(argv[0][0] == '-'){
			if(!strcmp(argv[0],"-c")){
				if(argc > 1){
					int l = strlen(argv[1]);
					char *tmp = malloc(l+2);
					if(tmp){
						context ctxt;
						ctxt.depth = 0;
						ctxt.skip[0] = 0;
						ctxt.last_result = 0;
						strcpy(tmp,argv[1]);
						tmp[l] = ' ';
						tmp[l+1] = 0;
						return parse(tmp,&ctxt);
					}
				}
				return 1;
			}
		} else {
			int fd;
			FILE *f = NULL;
			
			fd = open(argv[0], O_RDONLY | O_CLOEXEC);
			if(fd >= 0) 
				f = fdopen(fd,"r");
			if(f){
				res = runfile(f,0);
				fclose(f);
				return res;
			} else {
				fprintf(stderr,"cannot open \"%s\"\n",argv[0]);
				if(fd >= 0) close(fd);
				return 1;
			}
		}
		argv++;
		argc--;
	}

	return runfile(stdin,1);
}


