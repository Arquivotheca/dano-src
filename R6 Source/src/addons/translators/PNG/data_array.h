/*--------------------------------------------------------------------*\
  File:      data_array.h
  Creator:   Matt Bogosian <mbogosian@usa.net>
  Copyright: (c)1998, Matt Bogosian. All rights reserved.
  Description: Header file describing some convenient data array
      functions.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#include <stddef.h>


#ifndef _LIBMBOGOSIAN_DATA_ARRAY_H
#define _LIBMBOGOSIAN_DATA_ARRAY_H


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-= Definitions, Enums, Typedefs, Consts =-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#ifndef DECL_SPEC
#if defined __BEOS__ && defined EXPORT_SYMBOLS
#define DECL_SPEC _EXPORT
#elif defined __BEOS__ && defined IMPORT_SYMBOLS
#define DECL_SPEC _IMPORT
#else
#define DECL_SPEC
#endif
#endif


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=- Function Prototypes =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*\
  Function name: arrayLength
  Defined in:    data_array.h
  Arguments:     const void * const * const a_array - a null-
                     terminated array of objects.
  Returns:       size_t - the number of elements in the array.
  Throws:        none
  Description: Function to return the number of elements (not
      including the null terminator) in the array.
\*--------------------------------------------------------------------*/

size_t arrayLength(const void * const * const a_array);

/*--------------------------------------------------------------------*\
  Function name: deleteArguments
  Defined in:    data_array.cpp
  Arguments:     const char * const * const a_args - a null-
                     terminated array of strings that was allocated
                     using replicateArguments().
  Returns:       none
  Throws:        none
  Description: Function to delete a set of arguments allocated by
      replicateArguments().
\*--------------------------------------------------------------------*/

void deleteArguments(const char * const * const a_args);

/*--------------------------------------------------------------------*\
  Function name: replicateArguments
  Defined in:    data_array.cpp
  Arguments:     const size_t a_argc - the number of strings in the
                     array.
                 const char * const * const a_argv - an array of
                     strings.
  Returns:       const char * const * - a replication of those
                     strings.
  Throws:        none
  Description: Function to replicate a given array of strings such
      the replication is null-terminated (though the given array need
      not be).
\*--------------------------------------------------------------------*/

const char * const *replicateArguments(const size_t a_argc, const char * const * const a_argv);

/*--------------------------------------------------------------------*\
  Function name: shiftArray
  Defined in:    data_array.cpp
  Arguments:     size_t * const a_argc - the number of command line
                     arguments.
                 const void ** const a_argv - the command line
                     arguments.
                 const size_t a_which - the argument to remove.
  Returns:       const void * - the argument which was removed (null
                     if none).
  Throws:        none
  Description: Function to remove an element from a given null-
      terminated array. Note: this function does NOT depend on a_argc
      for an accurate count. It merely decrements it if a shiftable
      argument was found. In other words, a_argv could point to a
      null-terminated array of three items and *a_argc could be 50. In
      that case, if you called this function to remove the second
      item, it would, and it would decrement *a_argc by one.
\*--------------------------------------------------------------------*/

const void *shiftArray(size_t * const a_argc, const void ** const a_argv, size_t a_which);

/*--------------------------------------------------------------------*\
  Function name: shiftArray
  Defined in:    data_array.cpp
  Arguments:     size_t * const a_argc - the number of command line
                     arguments.
                 const void ** const a_argv - the command line
                     arguments.
                 const void * const a_which - the argument to remove.
  Returns:       const void * - the argument which was removed (null
                     if none).
  Throws:        none
  Description: Function to remove an element from a given null-
      terminated array. Note: this function does NOT depend on a_argc
      for an accurate count. It merely decrements it if a shiftable
      argument was found. In other words, a_argv could point to a
      null-terminated array of three items and *a_argc could be 50. In
      that case, if you called this function to remove the second
      item, it would, and it would decrement *a_argc by one.
\*--------------------------------------------------------------------*/

const void *shiftArray(size_t * const a_argc, const void ** const a_argv, const void * const a_which);


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
inline size_t arrayLength(const void * const * const a_array)
//====================================================================
{
	size_t len(0);
	
	while (a_array[len] != NULL)
	{
		len++;
	}
	
	return len;
}


#endif    // _LIBMBOGOSIAN_DATA_ARRAY_H
