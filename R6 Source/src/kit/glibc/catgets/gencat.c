/* Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1996.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <argp.h>
#include <ctype.h>
#include <endian.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <locale.h>
#include <libintl.h>
#include <limits.h>
#include <nl_types.h>
#include <obstack.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "version.h"

#include "catgetsinfo.h"


#define SWAPU32(w) \
  (((w) << 24) | (((w) & 0xff00) << 8) | (((w) >> 8) & 0xff00) | ((w) >> 24))

struct message_list
{
  int number;
  const char *message;

  const char *fname;
  size_t line;
  const char *symbol;

  struct message_list *next;
};


struct set_list
{
  int number;
  int deleted;
  struct message_list *messages;
  int last_message;

  const char *fname;
  size_t line;
  const char *symbol;

  struct set_list *next;
};


struct catalog
{
  struct set_list *all_sets;
  struct set_list *current_set;
  size_t total_messages;
  char quote_char;
  int last_set;

  struct obstack mem_pool;
};


/* If non-zero force creation of new file, not using existing one.  */
static int force_new;

/* Name of output file.  */
static const char *output_name;

/* Name of generated C header file.  */
static const char *header_name;

/* Name and version of program.  */
static void print_version (FILE *stream, struct argp_state *state);
void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

#define OPT_NEW 1

/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{
  { "header", 'H', N_("NAME"), 0,
    N_("Create C header file NAME containing symbol definitions") },
  { "new", OPT_NEW, NULL, 0,
    N_("Do not use existing catalog, force new output file") },
  { "output", 'o', N_("NAME"), 0, N_("Write output to file NAME") },
  { NULL, 0, NULL, 0, NULL }
};

/* Short description of program.  */
static const char doc[] = N_("Generate message catalog.\
\vIf INPUT-FILE is -, input is read from standard input.  If OUTPUT-FILE\n\
is -, output is written to standard output.\n");

/* Strings for arguments in help texts.  */
static const char args_doc[] = N_("\
-o OUTPUT-FILE [INPUT-FILE]...\n[OUTPUT-FILE [INPUT-FILE]...]");

/* Prototype for option handler.  */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Function to print some extra text in the help message.  */
static char *more_help (int key, const char *text, void *input);

/* Data structure to communicate with argp functions.  */
static struct argp argp =
{
  options, parse_opt, args_doc, doc, NULL, more_help
};


/* Wrapper functions with error checking for standard functions.  */
extern void *xmalloc (size_t n);

/* Prototypes for local functions.  */
static void error_print (void);
static struct catalog *read_input_file (struct catalog *current,
					const char *fname);
static void write_out (struct catalog *result, const char *output_name,
		       const char *header_name);
static struct set_list *find_set (struct catalog *current, int number);
static void normalize_line (const char *fname, size_t line, char *string,
			    char quote_char);
static void read_old (struct catalog *catalog, const char *file_name);


int
main (int argc, char *argv[])
{
  struct catalog *result;
  int remaining;

  /* Set program name for messages.  */
  error_print_progname = error_print;

  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");

  /* Set the text message domain.  */
  textdomain (PACKAGE);

  /* Initialize local variables.  */
  result = NULL;

  /* Parse and process arguments.  */
  argp_parse (&argp, argc, argv, 0, &remaining, NULL);

  /* Determine output file.  */
  if (output_name == NULL)
    output_name = remaining < argc ? argv[remaining++] : "-";

  /* Process all input files.  */
  setlocale (LC_CTYPE, "C");
  if (remaining < argc)
    do
      result = read_input_file (result, argv[remaining]);
    while (++remaining < argc);
  else
    result = read_input_file (NULL, "-");

  /* Write out the result.  */
  if (result != NULL)
    write_out (result, output_name, header_name);

  exit (EXIT_SUCCESS);
}


/* Handle program arguments.  */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'H':
      header_name = arg;
      break;
    case OPT_NEW:
      force_new = 1;
      break;
    case 'o':
      output_name = arg;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


static char *
more_help (int key, const char *text, void *input)
{
  switch (key)
    {
    case ARGP_KEY_HELP_EXTRA:
      /* We print some extra information.  */
      return strdup (gettext ("\
Report bugs using the `glibcbug' script to <bugs@gnu.org>.\n"));
    default:
      break;
    }
  return (char *) text;
}

/* Print the version information.  */
static void
print_version (FILE *stream, struct argp_state *state)
{
  fprintf (stream, "gencat (GNU %s) %s\n", PACKAGE, VERSION);
  fprintf (stream, gettext ("\
Copyright (C) %s Free Software Foundation, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "1996, 1997");
  fprintf (stream, gettext ("Written by %s.\n"), "Ulrich Drepper");
}


/* The address of this function will be assigned to the hook in the
   error functions.  */
static void
error_print ()
{
  /* We don't want the program name to be printed in messages.  Emacs'
     compile.el does not like this.  */
}


static struct catalog *
read_input_file (struct catalog *current, const char *fname)
{
  FILE *fp;
  char *buf;
  size_t len;
  size_t line_number;

  if (strcmp (fname, "-") == 0 || strcmp (fname, "/dev/stdin") == 0)
    {
      fp = stdin;
      fname = gettext ("*standard input*");
    }
  else
    fp = fopen (fname, "r");
  if (fp == NULL)
    {
      error (0, errno, gettext ("cannot open input file `%s'"), fname);
      return current;
    }

  /* If we haven't seen anything yet, allocate result structure.  */
  if (current == NULL)
    {
      current = (struct catalog *) xmalloc (sizeof (*current));

      current->all_sets = NULL;
      current->total_messages = 0;
      current->last_set = 0;
      current->current_set = find_set (current, NL_SETD);

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free
      obstack_init (&current->mem_pool);
    }

  buf = NULL;
  len = 0;
  line_number = 0;
  while (!feof (fp))
    {
      int continued;
      int used;
      size_t start_line = line_number + 1;
      char *this_line;

      do
	{
	  int act_len;

	  act_len = getline (&buf, &len, fp);
	  if (act_len <= 0)
	    break;
	  ++line_number;

	  /* It the line continued?  */
	  if (buf[act_len - 1] == '\n')
	    {
	      --act_len;
	      continued = buf[act_len - 1] == '\\';
	      if (continued)
		--act_len;
	    }
	  else
	    continued = 0;

	  /* Append to currently selected line.  */
	  obstack_grow (&current->mem_pool, buf, act_len);
	}
      while (continued);

      obstack_1grow (&current->mem_pool, '\0');
      this_line = (char *) obstack_finish (&current->mem_pool);

      used = 0;
      if (this_line[0] == '$')
	{
	  if (isspace (this_line[1]))
	    /* This is a comment line.  Do nothing.  */;
	  else if (strncmp (&this_line[1], "set", 3) == 0)
	    {
	      int cnt = sizeof ("set");
	      int set_number;
	      const char *symbol = NULL;
	      while (isspace (this_line[cnt]))
		++cnt;

	      if (isdigit (this_line[cnt]))
		{
		  set_number = atol (&this_line[cnt]);

		  /* If the given number for the character set is
		     higher than any we used for symbolic set names
		     avoid clashing by using only higher numbers for
		     the following symbolic definitions.  */
		  if (set_number > current->last_set)
		    current->last_set = set_number;
		}
	      else
		{
		  /* See whether it is a reasonable identifier.  */
		  int start = cnt;
		  while (isalnum (this_line[cnt]) || this_line[cnt] == '_')
		    ++cnt;

		  if (cnt == start)
		    {
		      /* No correct character found.  */
		      error_at_line (0, 0, fname, start_line,
				     gettext ("illegal set number"));
		      set_number = 0;
		    }
		  else
		    {
		      /* We have found seomthing that looks like a
			 correct identifier.  */
		      struct set_list *runp;

		      this_line[cnt] = '\0';
		      used = 1;
		      symbol = &this_line[start];

		      /* Test whether the identifier was already used.  */
		      runp = current->all_sets;
		      while (runp != 0)
			if (runp->symbol != NULL
			    && strcmp (runp->symbol, symbol) == 0)
			  break;
			else
			  runp = runp->next;

		      if (runp != NULL)
			{
			  /* We cannot allow duplicate identifiers for
			     message sets.  */
			  error_at_line (0, 0, fname, start_line,
					 gettext ("duplicate set definition"));
			  error_at_line (0, 0, runp->fname, runp->line,
					 gettext ("\
this is the first definition"));
			  set_number = 0;
			}
		      else
			/* Allocate next free message set for identifier.  */
			set_number = ++current->last_set;
		    }
		}

	      if (set_number != 0)
		{
		  /* We found a legal set number.  */
		  current->current_set = find_set (current, set_number);
		  if (symbol != NULL)
		      used = 1;
		  current->current_set->symbol = symbol;
		  current->current_set->fname = fname;
		  current->current_set->line = start_line;
		}
	    }
	  else if (strncmp (&this_line[1], "delset", 6) == 0)
	    {
	      int cnt = sizeof ("delset");
	      size_t set_number;
	      while (isspace (this_line[cnt]))
		++cnt;

	      if (isdigit (this_line[cnt]))
		{
		  size_t set_number = atol (&this_line[cnt]);
		  struct set_list *set;

		  /* Mark the message set with the given number as
		     deleted.  */
		  set = find_set (current, set_number);
		  set->deleted = 1;
		}
	      else
		{
		  /* See whether it is a reasonable identifier.  */
		  int start = cnt;
		  while (isalnum (this_line[cnt]) || this_line[cnt] == '_')
		    ++cnt;

		  if (cnt == start)
		    {
		      error_at_line (0, 0, fname, start_line,
				     gettext ("illegal set number"));
		      set_number = 0;
		    }
		  else
		    {
		      const char *symbol;
		      struct set_list *runp;

		      this_line[cnt] = '\0';
		      used = 1;
		      symbol = &this_line[start];

		      /* We have a symbolic set name.  This name must
			 appear somewhere else in the catalogs read so
			 far.  */
		      set_number = 0;
		      for (runp = current->all_sets; runp != NULL;
			   runp = runp->next)
			{
			  if (strcmp (runp->symbol, symbol) == 0)
			    {
			      runp->deleted = 1;
			      break;
			    }
			}
		      if (runp == NULL)
			/* Name does not exist before.  */
			error_at_line (0, 0, fname, start_line,
				       gettext ("unknown set `%s'"), symbol);
		    }
		}
	    }
	  else if (strncmp (&this_line[1], "quote", 5) == 0)
	    {
	      int cnt = sizeof ("quote");
	      while (isspace (this_line[cnt]))
		++cnt;
	      /* Yes, the quote char can be '\0'; this means no quote
		 char.  */
	      current->quote_char = this_line[cnt];
	    }
	  else
	    {
	      int cnt;
	      cnt = 2;
	      while (this_line[cnt] != '\0' && !isspace (this_line[cnt]))
		++cnt;
	      this_line[cnt] = '\0';
	      error_at_line (0, 0, fname, start_line,
			     gettext ("unknown directive `%s': line ignored"),
			     &this_line[1]);
	    }
	}
      else if (isalnum (this_line[0]) || this_line[0] == '_')
	{
	  const char *ident = this_line;
	  int message_number;

	  do
	    ++this_line;
	  while (this_line[0] != '\0' && !isspace (this_line[0]));;
	  this_line[0] = '\0';	/* Terminate the identifier.  */

	  do
	    ++this_line;
	  while (isspace (this_line[0]));
	  /* Now we found the beginning of the message itself.  */

	  if (isdigit (ident[0]))
	    {
	      struct message_list *runp;

	      message_number = atoi (ident);

	      /* Find location to insert the new message.  */
	      runp = current->current_set->messages;
	      while (runp != NULL)
		if (runp->number == message_number)
		  break;
		else
		  runp = runp->next;
	      if (runp != NULL)
		{
		  /* Oh, oh.  There is already a message with this
		     number is the message set.  */
		  error_at_line (0, 0, fname, start_line,
				 gettext ("duplicated message number"));
		  error_at_line (0, 0, runp->fname, runp->line,
				 gettext ("this is the first definition"));
		  message_number = 0;
		}
	      ident = NULL;	/* We don't have a symbol.  */

	      if (message_number != 0
		  && message_number > current->current_set->last_message)
		current->current_set->last_message = message_number;
	    }
	  else if (ident[0] != '\0')
	    {
	      struct message_list *runp;
	      runp = current->current_set->messages;

	      /* Test whether the symbolic name was not used for
		 another message in this message set.  */
	      while (runp != NULL)
		if (runp->symbol != NULL && strcmp (ident, runp->symbol) == 0)
		  break;
		else
		  runp = runp->next;
	      if (runp != NULL)
		{
		  /* The name is already used.  */
		  error_at_line (0, 0, fname, start_line,
				 gettext ("duplicated message identifier"));
		  error_at_line (0, 0, runp->fname, runp->line,
				 gettext ("this is the first definition"));
		  message_number = 0;
		}
	      else
		/* Give the message the next unused number.  */
		message_number = ++current->current_set->last_message;
	    }
	  else
	    message_number = 0;

	  if (message_number != 0)
	    {
	      struct message_list *newp;

	      used = 1;	/* Yes, we use the line.  */

	      /* Strip quote characters, change escape sequences into
		 correct characters etc.  */
	      normalize_line (fname, start_line, this_line,
			      current->quote_char);

	      newp = (struct message_list *) xmalloc (sizeof (*newp));
	      newp->number = message_number;
	      newp->message = this_line;
	      /* Remember symbolic name; is NULL if no is given.  */
	      newp->symbol = ident;
	      /* Remember where we found the character.  */
	      newp->fname = fname;
	      newp->line = start_line;

	      /* Find place to insert to message.  We keep them in a
		 sorted single linked list.  */
	      if (current->current_set->messages == NULL
		  || current->current_set->messages->number > message_number)
		{
		  newp->next = current->current_set->messages;
		  current->current_set->messages = newp;
		}
	      else
		{
		  struct message_list *runp;
		  runp = current->current_set->messages;
		  while (runp->next != NULL)
		    if (runp->next->number > message_number)
		      break;
		    else
		      runp = runp->next;
		  newp->next = runp->next;
		  runp->next = newp;
		}
	    }
	  ++current->total_messages;
	}
      else
	{
	  size_t cnt;

	  cnt = 0;
	  /* See whether we have any non-white space character in this
	     line.  */
	  while (this_line[cnt] != '\0' && isspace (this_line[cnt]))
	    ++cnt;

	  if (this_line[cnt] != '\0')
	    /* Yes, some unknown characters found.  */
	    error_at_line (0, 0, fname, start_line,
			   gettext ("malformed line ignored"));
	}

      /* We can save the memory for the line if it was not used.  */
      if (!used)
	obstack_free (&current->mem_pool, this_line);
    }

  if (fp != stdin)
    fclose (fp);
  return current;
}


static void
write_out (struct catalog *catalog, const char *output_name,
	   const char *header_name)
{
  /* Computing the "optimal" size.  */
  struct set_list *set_run;
  size_t best_total, best_size, best_depth;
  size_t act_size, act_depth;
  struct catalog_obj obj;
  struct obstack string_pool;
  const char *strings;
  size_t strings_size;
  u_int32_t *array1, *array2;
  size_t cnt;
  int fd;

  /* If not otherwise told try to read file with existing
     translations.  */
  if (!force_new)
    read_old (catalog, output_name);

  /* Initialize best_size with a very high value.  */
  best_total = best_size = best_depth = UINT_MAX;

  /* We need some start size for testing.  Let's start with
     TOTAL_MESSAGES / 5, which theoretically provides a mean depth of
     5.  */
  act_size = 1 + catalog->total_messages / 5;

  /* We determine the size of a hash table here.  Because the message
     numbers can be chosen arbitrary by the programmer we cannot use
     the simple method of accessing the array using the message
     number.  The algorithm is based on the trivial hash function
     NUMBER % TABLE_SIZE, where collisions are stored in a second
     dimension up to TABLE_DEPTH.  We here compute TABLE_SIZE so that
     the needed space (= TABLE_SIZE * TABLE_DEPTH) is minimal.  */
  while (act_size <= best_total)
    {
      size_t deep[act_size];

      act_depth = 1;
      memset (deep, '\0', act_size * sizeof (size_t));
      set_run = catalog->all_sets;
      while (set_run != NULL)
	{
	  struct message_list *message_run;

	  message_run = set_run->messages;
	  while (message_run != NULL)
	    {
	      size_t idx = (message_run->number * set_run->number) % act_size;

	      ++deep[idx];
	      if (deep[idx] > act_depth)
		{
		  act_depth = deep[idx];
		  if (act_depth * act_size > best_total)
		    break;
		}
	      message_run = message_run->next;
	    }
	  set_run = set_run->next;
	}

      if (act_depth * act_size <= best_total)
	{
	  /* We have found a better solution.  */
	  best_total = act_depth * act_size;
	  best_size = act_size;
	  best_depth = act_depth;
	}

      ++act_size;
    }

  /* let's be prepared for an empty message file.  */
  if (best_size == UINT_MAX)
    {
      best_size = 1;
      best_depth = 1;
    }

  /* OK, now we have the size we will use.  Fill in the header, build
     the table and the second one with swapped byte order.  */
  obj.magic = CATGETS_MAGIC;
  obj.plane_size = best_size;
  obj.plane_depth = best_depth;

  /* Allocate room for all needed arrays.  */
  array1 =
    (u_int32_t *) alloca (best_size * best_depth * sizeof (u_int32_t) * 3);
  memset (array1, '\0', best_size * best_depth * sizeof (u_int32_t) * 3);
  array2
    = (u_int32_t *) alloca (best_size * best_depth * sizeof (u_int32_t) * 3);
  obstack_init (&string_pool);

  set_run = catalog->all_sets;
  while (set_run != NULL)
    {
      struct message_list *message_run;

      message_run = set_run->messages;
      while (message_run != NULL)
	{
	  size_t idx = (((message_run->number * set_run->number) % best_size)
			* 3);
	  /* Determine collision depth.  */
	  while (array1[idx] != 0)
	    idx += best_size * 3;

	  /* Store set number, message number and pointer into string
	     space, relative to the first string.  */
	  array1[idx + 0] = set_run->number;
	  array1[idx + 1] = message_run->number;
	  array1[idx + 2] = obstack_object_size (&string_pool);

	  /* Add current string to the continuous space containing all
	     strings.  */
	  obstack_grow0 (&string_pool, message_run->message,
			 strlen (message_run->message));

	  message_run = message_run->next;
	}

      set_run = set_run->next;
    }
  strings_size = obstack_object_size (&string_pool);
  strings = obstack_finish (&string_pool);

  /* Compute ARRAY2 by changing the byte order.  */
  for (cnt = 0; cnt < best_size * best_depth * 3; ++cnt)
    array2[cnt] = SWAPU32 (array1[cnt]);

  /* Now we can write out the whole data.  */
  if (strcmp (output_name, "-") == 0
      || strcmp (output_name, "/dev/stdout") == 0)
    fd = STDOUT_FILENO;
  else
    {
      fd = creat (output_name, 0666);
      if (fd < 0)
	error (EXIT_FAILURE, errno, gettext ("cannot open output file `%s'"),
	       output_name);
    }

  /* Write out header.  */
  write (fd, &obj, sizeof (obj));

  /* We always write out the little endian version of the index
     arrays.  */
#if __BYTE_ORDER == __LITTLE_ENDIAN
  write (fd, array1, best_size * best_depth * sizeof (u_int32_t) * 3);
  write (fd, array2, best_size * best_depth * sizeof (u_int32_t) * 3);
#elif __BYTE_ORDER == __BIG_ENDIAN
  write (fd, array2, best_size * best_depth * sizeof (u_int32_t) * 3);
  write (fd, array1, best_size * best_depth * sizeof (u_int32_t) * 3);
#else
# error Cannot handle __BYTE_ORDER byte order
#endif

  /* Finally write the strings.  */
  write (fd, strings, strings_size);

  if (fd != STDOUT_FILENO)
    close (fd);

  /* If requested now write out the header file.  */
  if (header_name != NULL)
    {
      int first = 1;
      FILE *fp;

      /* Open output file.  "-" or "/dev/stdout" means write to
	 standard output.  */
      if (strcmp (header_name, "-") == 0
	  || strcmp (header_name, "/dev/stdout") == 0)
	fp = stdout;
      else
	{
	  fp = fopen (header_name, "w");
	  if (fp == NULL)
	    error (EXIT_FAILURE, errno,
		   gettext ("cannot open output file `%s'"), header_name);
	}

      /* Iterate over all sets and all messages.  */
      set_run = catalog->all_sets;
      while (set_run != NULL)
	{
	  struct message_list *message_run;

	  /* If the current message set has a symbolic name write this
	     out first.  */
	  if (set_run->symbol != NULL)
	    fprintf (fp, "%s#define %sSet %#x\t/* %s:%Zu */\n",
		     first ? "" : "\n", set_run->symbol, set_run->number - 1,
		     set_run->fname, set_run->line);
	  first = 0;

	  message_run = set_run->messages;
	  while (message_run != NULL)
	    {
	      /* If the current message has a symbolic name write
		 #define out.  But we have to take care for the set
		 not having a symbolic name.  */
	      if (message_run->symbol != NULL)
		if (set_run->symbol == NULL)
		  fprintf (fp, "#define AutomaticSet%d%s %#x\t/* %s:%Zu */\n",
			   set_run->number, message_run->symbol,
			   message_run->number, message_run->fname,
			   message_run->line);
		else
		  fprintf (fp, "#define %s%s %#x\t/* %s:%Zu */\n",
			   set_run->symbol, message_run->symbol,
			   message_run->number, message_run->fname,
			   message_run->line);

	      message_run = message_run->next;
	    }

	  set_run = set_run->next;
	}

      if (fp != stdout)
	fclose (fp);
    }
}


static struct set_list *
find_set (struct catalog *current, int number)
{
  struct set_list *result = current->all_sets;

  /* We must avoid set number 0 because a set of this number signals
     in the tables that the entry is not occupied.  */
  ++number;

  while (result != NULL)
    if (result->number == number)
      return result;
    else
      result = result->next;

  /* Prepare new message set.  */
  result = (struct set_list *) xmalloc (sizeof (*result));
  result->number = number;
  result->deleted = 0;
  result->messages = NULL;
  result->next = current->all_sets;
  current->all_sets = result;

  return result;
}


/* Normalize given string *in*place* by processing escape sequences
   and quote characters.  */
static void
normalize_line (const char *fname, size_t line, char *string, char quote_char)
{
  int is_quoted;
  char *rp = string;
  char *wp = string;

  if (quote_char != '\0' && *rp == quote_char)
    {
      is_quoted = 1;
      ++rp;
    }
  else
    is_quoted = 0;

  while (*rp != '\0')
    if (*rp == quote_char)
      /* We simply end the string when we find the first time an
	 not-escaped quote character.  */
	break;
    else if (*rp == '\\')
      {
	++rp;
	if (quote_char != '\0' && *rp == quote_char)
	  /* This is an extension to XPG.  */
	  *wp++ = *rp++;
	else
	  /* Recognize escape sequences.  */
	  switch (*rp)
	    {
	    case 'n':
	      *wp++ = '\n';
	      ++rp;
	      break;
	    case 't':
	      *wp++ = '\t';
	      ++rp;
	      break;
	    case 'v':
	      *wp++ = '\v';
	      ++rp;
	      break;
	    case 'b':
	      *wp++ = '\b';
	      ++rp;
	      break;
	    case 'r':
	      *wp++ = '\r';
	      ++rp;
	      break;
	    case 'f':
	      *wp++ = '\f';
	      ++rp;
	      break;
	    case '\\':
	      *wp++ = '\\';
	      ++rp;
	      break;
	    case '0' ... '7':
	      {
		int number = *rp++ - '0';
		while (number <= (255 / 8) && *rp >= '0' && *rp <= '7')
		  {
		    number *= 8;
		    number += *rp++ - '0';
		  }
		*wp++ = (char) number;
	      }
	      break;
	    default:
	      /* Simply ignore the backslash character.  */
	      break;
	    }
      }
    else
      *wp++ = *rp++;

  /* If we saw a quote character at the beginning we expect another
     one at the end.  */
  if (is_quoted && *rp != quote_char)
    error (0, 0, fname, line, gettext ("unterminated message"));

  /* Terminate string.  */
  *wp = '\0';
  return;
}


static void
read_old (struct catalog *catalog, const char *file_name)
{
  struct catalog_info old_cat_obj;
  struct set_list *set = NULL;
  int last_set = -1;
  size_t cnt;

  old_cat_obj.status = closed;
  old_cat_obj.cat_name = file_name;
  old_cat_obj.nlspath = NULL;
  __libc_lock_init (old_cat_obj.lock);

  /* Try to open catalog, but don't look through the NLSPATH.  */
  __open_catalog (&old_cat_obj);

  if (old_cat_obj.status != mmapped && old_cat_obj.status != malloced)
    if (errno == ENOENT)
      /* No problem, the catalog simply does not exist.  */
      return;
    else
      error (EXIT_FAILURE, errno, gettext ("while opening old catalog file"));

  /* OK, we have the catalog loaded.  Now read all messages and merge
     them.  When set and message number clash for any message the new
     one is used.  */
  for (cnt = 0; cnt < old_cat_obj.plane_size * old_cat_obj.plane_depth; ++cnt)
    {
      struct message_list *message, *last;

      if (old_cat_obj.name_ptr[cnt * 3 + 0] == 0)
	/* No message in this slot.  */
	continue;

      if (old_cat_obj.name_ptr[cnt * 3 + 0] - 1 != (u_int32_t) last_set)
	{
	  last_set = old_cat_obj.name_ptr[cnt * 3 + 0] - 1;
	  set = find_set (catalog, old_cat_obj.name_ptr[cnt * 3 + 0] - 1);
	}

      last = NULL;
      message = set->messages;
      while (message != NULL)
	{
	  if ((u_int32_t) message->number >= old_cat_obj.name_ptr[cnt * 3 + 1])
	    break;
	  last = message;
	  message = message->next;
	}

      if (message == NULL
	  || (u_int32_t) message->number > old_cat_obj.name_ptr[cnt * 3 + 1])
	{
	  /* We have found a message which is not yet in the catalog.
	     Insert it at the right position.  */
	  struct message_list *newp;

	  newp = (struct message_list *) xmalloc (sizeof(*newp));
	  newp->number = old_cat_obj.name_ptr[cnt * 3 + 1];
	  newp->message =
	    &old_cat_obj.strings[old_cat_obj.name_ptr[cnt * 3 + 2]];
	  newp->fname = NULL;
	  newp->line = 0;
	  newp->symbol = NULL;
	  newp->next = message;

	  if (last == NULL)
	    set->messages = newp;
	  else
	    last->next = newp;

	  ++catalog->total_messages;
	}
    }
}
