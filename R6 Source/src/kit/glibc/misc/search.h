/* Declarations for System V style searching functions.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#ifndef _SEARCH_H
#define	_SEARCH_H 1

#include <features.h>

#define __need_size_t
#define __need_NULL
#include <stddef.h>

__BEGIN_DECLS

#if defined __USE_SVID || defined __USE_XOPEN_EXTENDED
/* Prototype structure for a linked-list data structure.
   This is the type used by the `insque' and `remque' functions.  */

struct qelem
  {
    struct qelem *q_forw;
    struct qelem *q_back;
    char q_data[1];
  };


/* Insert ELEM into a doubly-linked list, after PREV.  */
extern void insque __P ((void *__elem, void *__prev));

/* Unlink ELEM from the doubly-linked list that it is in.  */
extern void remque __P ((void *__elem));
#endif


/* For use with hsearch(3).  */
#ifndef __COMPAR_FN_T
# define __COMPAR_FN_T
typedef int (*__compar_fn_t) __PMT ((__const __ptr_t, __const __ptr_t));

# ifdef	__USE_GNU
typedef __compar_fn_t comparison_fn_t;
# endif
#endif

/* Action which shall be performed in the call the hsearch.  */
typedef enum
  {
    FIND,
    ENTER
  }
ACTION;

typedef struct entry
  {
    char *key;
    char *data;
  }
ENTRY;

/* Opaque type for internal use.  */
struct _ENTRY;

/* Family of hash table handling functions.  The functions also
   have reentrant counterparts ending with _r.  The non-reentrant
   functions all work on a signle internal hashing table.  */

/* Search for entry matching ITEM.key in internal hash table.  If
   ACTION is `FIND' return found entry or signal error by returning
   NULL.  If ACTION is `ENTER' replace existing data (if any) with
   ITEM.data.  */
extern ENTRY *hsearch __P ((ENTRY __item, ACTION __action));

/* Create a new hashing table which will at most contain NEL elements.  */
extern int hcreate __P ((size_t __nel));

/* Destroy current internal hashing table.  */
extern void __hdestroy __P ((void));
extern void hdestroy __P ((void));

#ifdef __USE_GNU
/* Data type for reentrant functions.  */
struct hsearch_data
  {
    struct _ENTRY *table;
    unsigned int size;
    unsigned int filled;
  };

/* Reentrant versions which can handle multiple hashing tables at the
   same time.  */
extern int hsearch_r __P ((ENTRY __item, ACTION __action, ENTRY **__retval,
			   struct hsearch_data *__htab));
extern int hcreate_r __P ((size_t __nel, struct hsearch_data *htab));
extern void hdestroy_r __P ((struct hsearch_data *htab));
#endif


/* The tsearch routines are very interesting. They make many
   assumptions about the compiler.  It assumes that the first field
   in node must be the "key" field, which points to the datum.
   Everything depends on that.  */
/* For tsearch */
typedef enum
{
  preorder,
  postorder,
  endorder,
  leaf
}
VISIT;

/* Search for an entry matching the given KEY in the tree pointed to
   by *ROOTP and insert a new element if not found.  */
extern void *__tsearch __PMT ((__const void *__key, void **__rootp,
			       __compar_fn_t compar));
extern void *tsearch __PMT ((__const void *__key, void **__rootp,
			     __compar_fn_t compar));

/* Search for an entry matching the given KEY in the tree pointed to
   by *ROOTP.  If no matching entry is available return NULL.  */
extern void *__tfind __PMT ((__const void *__key, void *__const *__rootp,
			     __compar_fn_t compar));
extern void *tfind __PMT ((__const void *__key, void *__const *__rootp,
			   __compar_fn_t compar));

/* Remove the element matching KEY from the tree pointed to by *ROOTP.  */
extern void *__tdelete __PMT ((__const void *__key, void **__rootp,
			       __compar_fn_t compar));
extern void *tdelete __PMT ((__const void *__key, void **__rootp,
			     __compar_fn_t compar));

#ifndef __ACTION_FN_T
# define __ACTION_FN_T
typedef void (*__action_fn_t) __PMT ((__const void *__nodep,
				      VISIT __value,
				      int __level));
#endif

/* Walk through the whole tree and call the ACTION callback for every node
   or leaf.  */
extern void __twalk __PMT ((__const void *__root, __action_fn_t action));
extern void twalk __PMT ((__const void *__root, __action_fn_t action));

#ifdef __USE_GNU
/* Callback type for function to free a tree node.  If the keys are atomic
   data this function should do nothing.  */
typedef void (*__free_fn_t) __P ((void *__nodep));

/* Destroy the whole tree, call FREEFCT for each node or leaf.  */
extern void __tdestroy __PMT ((void *__root, __free_fn_t freefct));
extern void tdestroy __PMT ((void *__root, __free_fn_t freefct));
#endif


/* Perform linear search for KEY by comparing by COMPAR in an array
   [BASE,BASE+NMEMB*SIZE).  */
extern void *lfind __PMT ((__const void *__key, __const void *__base,
			   size_t *__nmemb, size_t __size,
			   __compar_fn_t __compar));

/* Perform linear search for KEY by comparing by COMPAR function in
   array [BASE,BASE+NMEMB*SIZE) and insert entry if not found.  */
extern void *lsearch __PMT ((__const void *__key, void *__base,
			     size_t *__nmemb, size_t __size,
			     __compar_fn_t __compar));

__END_DECLS

#endif /* search.h */
