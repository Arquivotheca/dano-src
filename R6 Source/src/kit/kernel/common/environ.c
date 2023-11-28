/* ++++++++++
	environ.c
	Copyright (C) 1992-3 Be Incorporated.  All Rights Reserved.
 	Some more sleazy glue to emulate some of UNIX weirdness.

	Mod History
	09 dec 93	elr	cobbled together for the first time
+++++ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* this is the environment pointer it gets filled in by start in crt0.s */

char	**environ = NULL;

static long count_env_vars(char **);
static void copy_env_to_heap();

/*
 * Search an environment for a variable.
 */

static char *
_getenvguts(char **envp, const char *ev)
{
int	len_of_string;
	
	len_of_string = strlen(ev);

    if (envp == NULL)
		return NULL;

	while (*envp) {
		if ((strncmp(ev, *envp, len_of_string) == 0) &&
			((*envp)[len_of_string] == '=')) {
			return( *envp + len_of_string + 1);
		}
		envp++;
	}
	return(NULL);
}

/*
 * This will be the getenv call. Search the environment for the given
 * string item.
 */

char *
getenv(const char *ev)
{
	return _getenvguts (environ, ev);
}

static int _first_time_ = 1;


int
putenv(const char *string)
{
	long	c;
	long	s;
	char	**e;
	char	*n;
	char	*old;
	char	*value;
	char	*name;


	if (_first_time_) {
		_first_time_ = 0;
		copy_env_to_heap();
	}

	n = strchr(string, (int) '=');
	if (!n)
		return -1;
	
	value = n+1;	/* value is one char past the '=' */
	
	name = malloc((n - string + 1) * sizeof(char));
	strncpy(name, string, n - string);
	name[n - string] = '\0'; 

	old = getenv(name);

	if (old == NULL) {
		/* if the env variable doesn't exist yet... */

		e = environ;
		c = count_env_vars(e);			/* this includes the terminating NULL */
		
		/* alloc new char array, one element larger to hold new entry */
		s = sizeof(char *) * c;
		e = (char **) malloc(s + sizeof(char *));

		/* copy old elements into new array */
		memcpy(e, environ, s);

		/* alloc space for new string, and add at 2nd last element of array  */
		n = (char *) malloc(strlen(string) + sizeof(char));
		strcpy(n, string);
		e[c-1] = n;

		/* place the terminating NULL at end of array */
		e[c] = NULL;

		/* free old array and set up new array */
		free(environ);
		environ = e;
	} else {
		/* else, the env variable already exists so we must replace value */

		/* find the entry in environ that represents the var */
		s = strlen(name);
		e = environ;
		while (*e) {
			if (!strncmp(string, *e, s) && ((*e)[s] == '=')) {
				free(*e);						/* free old entry */
				*e = (char *) malloc(strlen(string) + sizeof(char));
				strcpy(*e, string);
				break;
			}
			e++;
		}
	}
	free(name);
	return(0);
}

void copy_env_to_heap()
{
	char	**new_env;
	long	c = count_env_vars(environ);
	char	**env;
	char	**p;
	char	*var;
	char	*new_var;

	p = new_env = (char **) malloc(c * sizeof(char *));

	/* now copy each var */
	for (env = environ; *env; env++, p++) {
		var = *env;
		new_var = (char *) malloc(strlen(var) + sizeof(char));
		strcpy(new_var, var);
		*p = new_var;
	}
	*p = NULL;			/* add the terminating NULL */
	environ = new_env;
}

long count_env_vars(char **env)
{
	long c;
	/* determine the length of environ (the # of variables) */
	for (c = 0; *env; env++)
		c++;
	
	/* add 1 extra for the terminating NULL */
	return ++c;
}
