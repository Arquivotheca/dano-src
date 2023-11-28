/* Test for string function add boundaries of usable memory.
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#define _GNU_SOURCE 1

/* Make sure we don't test the optimized inline functions if we want to
   test the real implementation.  */
#undef __USE_STRING_INLINES

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/param.h>

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

int
main (int argc, char *argv[])
{
  int size = sysconf (_SC_PAGESIZE);
  char *adr, *dest;
  int result = 0;

  adr = (char *) mmap (NULL, 3 * size, PROT_READ|PROT_WRITE,
		       MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  dest = (char *) mmap (NULL, 3*size, PROT_READ|PROT_WRITE,
			MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if (adr == MAP_FAILED || dest == MAP_FAILED)
    {
      if (errno == ENOSYS)
        puts ("No test, mmap not available.");
      else
        {
          printf ("mmap failed: %m");
          result = 1;
        }
    }
  else
    {
      int inner, middle, outer;

      mprotect(adr, size, PROT_NONE);
      mprotect(adr+2*size, size, PROT_NONE);
      adr += size;

      mprotect(dest, size, PROT_NONE);
      mprotect(dest+2*size, size, PROT_NONE);
      dest += size;

      memset (adr, 'T', size);

      /* strlen test */
      for (outer = size - 1; outer >= MAX (0, size - 128); --outer)
        {
          for (inner = MAX (outer, size - 64); inner < size; ++inner)
	    {
	      adr[inner] = '\0';

	      if (strlen (&adr[outer]) != (size_t) (inner - outer))
		{
		  printf ("strlen flunked for outer = %d, inner = %d\n",
			  outer, inner);
		  result = 1;
		}

	      adr[inner] = 'T';
	    }
        }

      /* strchr test */
      for (outer = size - 1; outer >= MAX (0, size - 128); --outer)
        {
	  for (middle = MAX (outer, size - 64); middle < size; ++middle)
	    {
	      for (inner = middle; inner < size; ++inner)
		{
		  char *cp;
		  adr[middle] = 'V';
		  adr[inner] = '\0';

		  cp = strchr (&adr[outer], 'V');

		  if ((inner == middle && cp != NULL)
		      || (inner != middle
			  && (cp - &adr[outer]) != middle - outer))
		    {
		      printf ("strchr flunked for outer = %d, middle = %d, "
			      "inner = %d\n", outer, middle, inner);
		      result = 1;
		    }

		  adr[inner] = 'T';
		  adr[middle] = 'T';
		}
	    }
        }

      /* strrchr test */
      for (outer = size - 1; outer >= MAX (0, size - 128); --outer)
        {
	  for (middle = MAX (outer, size - 64); middle < size; ++middle)
	    {
	      for (inner = middle; inner < size; ++inner)
		{
		  char *cp;
		  adr[middle] = 'V';
		  adr[inner] = '\0';

		  cp = strrchr (&adr[outer], 'V');

		  if ((inner == middle && cp != NULL)
		      || (inner != middle
			  && (cp - &adr[outer]) != middle - outer))
		    {
		      printf ("strrchr flunked for outer = %d, middle = %d, "
			      "inner = %d\n", outer, middle, inner);
		      result = 1;
		    }

		  adr[inner] = 'T';
		  adr[middle] = 'T';
		}
	    }
        }

      /* strcpy test */
      for (outer = size - 1; outer >= MAX (0, size - 128); --outer)
        {
          for (inner = MAX (outer, size - 64); inner < size; ++inner)
	    {
	      adr[inner] = '\0';

	      if (strcpy (dest, &adr[outer]) != dest
		  || strlen (dest) != (size_t) (inner - outer))
		{
		  printf ("strcpy flunked for outer = %d, inner = %d\n",
			  outer, inner);
		  result = 1;
		}

	      adr[inner] = 'T';
	    }
        }

      /* stpcpy test */
      for (outer = size - 1; outer >= MAX (0, size - 128); --outer)
        {
          for (inner = MAX (outer, size - 64); inner < size; ++inner)
	    {
	      adr[inner] = '\0';

	      if ((stpcpy (dest, &adr[outer]) - dest) != inner - outer)
		{
		  printf ("stpcpy flunked for outer = %d, inner = %d\n",
			  outer, inner);
		  result = 1;
		}

	      adr[inner] = 'T';
	    }
        }
    }

  return result;
}
