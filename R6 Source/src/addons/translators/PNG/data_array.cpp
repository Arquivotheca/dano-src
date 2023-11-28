/*--------------------------------------------------------------------*\
  File:      data_array.cpp
  Creator:   Matt Bogosian <mbogosian@usa.net>
  Copyright: (c)1998, Matt Bogosian. All rights reserved.
  Description: Source file containing some convenient data array
      functions.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#include "data_array.h"

#include <malloc.h>
#include <string.h>


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
void deleteArguments(const char * const * const a_args)
//====================================================================
{
	char **args(const_cast<char **>(a_args));
	
	if (args != NULL)
	{
		free(args[0]);
		free(args);
	}
}

//====================================================================
const char * const *replicateArguments(const size_t a_argc, const char * const * const a_argv)
//====================================================================
{
	// Here we store the option's arguments in one string in the
	// format "ARG1\0ARG2\0...ARGn\0" and we create a list of pointers
	// to places in that string, with a null pointer at the end of the
	// list
	size_t count(0), len(0);
	char *arg_buf(NULL);
	
	if (a_argv != NULL)
	{
		// Count the number of arguments and get the total length
		// (including the terminating '\0' characters)
		while (count < a_argc
			&& a_argv[count] != NULL)
		{
			len += strlen(a_argv[count++]) + 1;
		}
		
		// Try to allocation space for the string and for the pointers
		if (len > 0
			&& (arg_buf = static_cast<char *>(malloc(sizeof (char) * len))) == NULL)
		{
			return NULL;
		}
	}
	
	// Note: this attempts to create a null-terminated list even if
	// a_argv is null
	char **args;
	
	if ((args = static_cast<char **>(malloc(sizeof (char *) * (count + 1)))) == NULL)
	{
		free(arg_buf);
		return NULL;
	}
	
	// Copy the arguments into the string and point them (this won't
	// do anything if a_argv was null and hence, count is 0)
	for (size_t i(0);
		i < count;
		i++)
	{
		args[i] = arg_buf;
		strcpy(args[i], a_argv[i]);
		arg_buf += strlen(arg_buf) + 1;
	}
	
	// Initialize the final pointer to null
	args[count] = NULL;
	
	return args;
}

//====================================================================
const void *shiftArray(size_t * const a_argc, const void ** const a_argv, const size_t a_which)
//====================================================================
{
	const void *arg(NULL);
	
	for (size_t i(0);
		a_argv[i] != NULL;
		i++)
	{
		// Move everything down if we've hit the item in question
		if (i >= a_which)
		{
			// Save it first
			if (i == a_which)
			{
				arg = a_argv[i];
			}
			
			a_argv[i] = a_argv[i + 1];
		}
	}
	
	// Decrement the argument counter if necessary
	if (arg != NULL)
	{
		(*a_argc)--;
	}
	
	return arg;
}

//====================================================================
const void *shiftArray(size_t * const a_argc, const void ** const a_argv, const void * const a_which)
//====================================================================
{
	const void *arg(NULL);
	
	for (size_t i(0);
		a_argv[i] != NULL;
		i++)
	{
		if (arg == NULL
			&& a_argv[i] == a_which)
		{
			arg = a_argv[i];
		}
		
		// Move everything down if we've hit the item in question
		if (arg != NULL)
		{
			a_argv[i] = a_argv[i + 1];
		}
	}
	
	// Decrement the argument counter if necessary
	if (arg != NULL)
	{
		(*a_argc)--;
	}
	
	return arg;
}
