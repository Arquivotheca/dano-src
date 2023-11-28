/* Implementation of the locale program according to POSIX 9945-2.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.

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
#include <argz.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <langinfo.h>
#include <libintl.h>
#include <limits.h>
#include <locale.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "localeinfo.h"


/* If set print the name of the category.  */
static int show_category_name;

/* If set print the name of the item.  */
static int show_keyword_name;

/* Print names of all available locales.  */
static int do_all;

/* Print names of all available character maps.  */
static int do_charmaps = 0;

/* Name and version of program.  */
static void print_version (FILE *stream, struct argp_state *state);
void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{
  { NULL, 0, NULL, 0, N_("System information:") },
  { "all-locales", 'a', NULL, OPTION_NO_USAGE,
    N_("Write names of available locales") },
  { "charmaps", 'm', NULL, OPTION_NO_USAGE,
    N_("Write names of available charmaps") },
  { NULL, 0, NULL, 0, N_("Modify output format:") },
  { "category-name", 'c', NULL, 0, N_("Write names of selected categories") },
  { "keyword-name", 'k', NULL, 0, N_("Write names of selected keywords") },
  { NULL, 0, NULL, 0, NULL }
};

/* Short description of program.  */
static const char doc[] = N_("Get locale-specific information.");

/* Strings for arguments in help texts.  */
static const char args_doc[] = N_("NAME\n[-a|-m]");

/* Prototype for option handler.  */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Function to print some extra text in the help message.  */
static char *more_help (int key, const char *text, void *input);

/* Data structure to communicate with argp functions.  */
static struct argp argp =
{
  options, parse_opt, args_doc, doc, NULL, more_help
};


/* We don't have these constants defined because we don't use them.  Give
   default values.  */
#define CTYPE_MB_CUR_MIN 0
#define CTYPE_MB_CUR_MAX 0
#define CTYPE_HASH_SIZE 0
#define CTYPE_HASH_LAYERS 0
#define CTYPE_CLASS 0
#define CTYPE_TOUPPER_EB 0
#define CTYPE_TOLOWER_EB 0
#define CTYPE_TOUPPER_EL 0
#define CTYPE_TOLOWER_EL 0

/* Definition of the data structure which represents a category and its
   items.  */
struct category
{
  int cat_id;
  const char *name;
  size_t number;
  struct cat_item
  {
    int item_id;
    const char *name;
    enum { std, opt } status;
    enum value_type value_type;
    int min;
    int max;
  } *item_desc;
};

/* Simple helper macro.  */
#define NELEMS(arr) ((sizeof (arr)) / (sizeof (arr[0])))

/* For some tricky stuff.  */
#define NO_PAREN(Item, More...) Item, ## More

/* We have all categories defined in `categories.def'.  Now construct
   the description and data structure used for all categories.  */
#define DEFINE_ELEMENT(Item, More...) { Item, ## More },
#define DEFINE_CATEGORY(category, name, items, postload, in, check, out)      \
    static struct cat_item category##_desc[] =				      \
      {									      \
        NO_PAREN items							      \
      };

#include "categories.def"
#undef DEFINE_CATEGORY

static struct category category[] =
  {
#define DEFINE_CATEGORY(category, name, items, postload, in, check, out)      \
    [category] = { _NL_NUM_##category, name, NELEMS (category##_desc),	      \
		   category##_desc },
#include "categories.def"
#undef DEFINE_CATEGORY
  };
#define NCATEGORIES NELEMS (category)


/* Automatically set variable.  */
extern const char *__progname;

/* helper function for extended name handling.  */
extern void locale_special (const char *name, int show_category_name,
			    int show_keyword_name);

/* Prototypes for local functions.  */
static void write_locales (void);
static void write_charmaps (void);
static void show_locale_vars (void);
static void show_info (const char *name);


int
main (int argc, char *argv[])
{
  int remaining;

  /* Set initial values for global variables.  */
  show_category_name = 0;
  show_keyword_name = 0;

  /* Set locale.  Do not set LC_ALL because the other categories must
     not be affected (according to POSIX.2).  */
  setlocale (LC_CTYPE, "");
  setlocale (LC_MESSAGES, "");

  /* Initialize the message catalog.  */
  textdomain (PACKAGE);

  /* Parse and process arguments.  */
  argp_parse (&argp, argc, argv, 0, &remaining, NULL);

  /* `-a' requests the names of all available locales.  */
  if (do_all != 0)
    {
      setlocale (LC_COLLATE, "");
      write_locales ();
      exit (EXIT_SUCCESS);
    }

  /* `m' requests the names of all available charmaps.  The names can be
     used for the -f argument to localedef(1).  */
  if (do_charmaps != 0)
    {
      write_charmaps ();
      exit (EXIT_SUCCESS);
    }

  /* Specific information about the current locale are requested.
     Change to this locale now.  */
  setlocale (LC_ALL, "");

  /* If no real argument is given we have to print the contents of the
     current locale definition variables.  These are LANG and the LC_*.  */
  if (remaining == argc && show_keyword_name == 0 && show_category_name == 0)
    {
      show_locale_vars ();
      exit (EXIT_SUCCESS);
    }

  /* Process all given names.  */
  while (remaining <  argc)
    show_info (argv[remaining++]);

  exit (EXIT_SUCCESS);
}


/* Handle program arguments.  */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'a':
      do_all = 1;
      break;
    case 'c':
      show_category_name = 1;
      break;
    case 'm':
      do_charmaps = 1;
      break;
    case 'k':
      show_keyword_name = 1;
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
  fprintf (stream, "locale (GNU %s) %s\n", PACKAGE, VERSION);
  fprintf (stream, gettext ("\
Copyright (C) %s Free Software Foundation, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "1995, 1996, 1997");
  fprintf (stream, gettext ("Written by %s.\n"), "Ulrich Drepper");
}


/* Simple action function which prints arguments as strings.  */
static void
print_names (const void *nodep, VISIT value, int level)
{
  if (value == postorder || value == leaf)
    puts (*(char **) nodep);
}


/* Write the names of all available locales to stdout.  We have some
   sources of the information: the contents of the locale directory
   and the locale.alias file.  To avoid duplicates and print the
   result is a reasonable order we put all entries is a search tree
   and print them afterwards.  */
static void
write_locales (void)
{
  void *all_data = NULL;
  DIR *dir;
  struct dirent *dirent;
  char *alias_path;
  size_t alias_path_len;
  char *entry;

#define PUT(name) tsearch ((name), &all_data, \
			   (int (*) (const void *, const void *)) strcoll)

  dir = opendir (LOCALEDIR);
  if (dir == NULL)
    {
      error (1, errno, gettext ("cannot read locale directory `%s'"),
	     LOCALEDIR);
      return;
    }

  /* `POSIX' locale is always available (POSIX.2 4.34.3).  */
  PUT ("POSIX");
  /* And so is the "C" locale.  */
  PUT ("C");

  /* Now we can look for all files in the directory.  */
  while ((dirent = readdir (dir)) != NULL)
    if (strcmp (dirent->d_name, ".") != 0
	&& strcmp (dirent->d_name, "..") != 0)
      {
	mode_t mode;
#ifdef _DIRENT_HAVE_D_TYPE
	if (dirent->d_type != DT_UNKNOWN)
	  mode = DTTOIF (dirent->d_type);
	else
#endif
	  {
	    struct stat st;
	    char buf[sizeof (LOCALEDIR) + strlen (dirent->d_name) + 1];

	    stpcpy (stpcpy (stpcpy (buf, LOCALEDIR), "/"), dirent->d_name);

	    if (stat (buf, &st) < 0)
	      continue;
	    mode = st.st_mode;
	  }

	if (S_ISDIR (mode))
	  PUT (strdup (dirent->d_name));
      }

  closedir (dir);

  /* Now read the locale.alias files.  */
  if (argz_create_sep (LOCALE_ALIAS_PATH, ':', &alias_path, &alias_path_len))
    error (1, errno, gettext ("while preparing output"));

  entry = NULL;
  while ((entry = argz_next (alias_path, alias_path_len, entry)))
    {
      static const char aliasfile[] = "/locale.alias";
      FILE *fp;
      char full_name[strlen (entry) + sizeof aliasfile];

      stpcpy (stpcpy (full_name, entry), aliasfile);
      fp = fopen (full_name, "r");
      if (fp == NULL)
	/* Ignore non-existing files.  */
	continue;

      while (! feof (fp))
	{
	  /* It is a reasonable approach to use a fix buffer here
	     because
	     a) we are only interested in the first two fields
	     b) these fields must be usable as file names and so must
	        not be that long  */
	  char buf[BUFSIZ];
	  char *alias;
	  char *value;
	  char *cp;

	  if (fgets (buf, BUFSIZ, fp) == NULL)
	    /* EOF reached.  */
	    break;

	  cp = buf;
	  /* Ignore leading white space.  */
	  while (isspace (cp[0]))
	    ++cp;

	  /* A leading '#' signals a comment line.  */
	  if (cp[0] != '\0' && cp[0] != '#')
	    {
	      alias = cp++;
	      while (cp[0] != '\0' && !isspace (cp[0]))
		++cp;
	      /* Terminate alias name.  */
	      if (cp[0] != '\0')
		*cp++ = '\0';

	      /* Now look for the beginning of the value.  */
	      while (isspace (cp[0]))
		++cp;

	      if (cp[0] != '\0')
		{
		  value = cp++;
		  while (cp[0] != '\0' && !isspace (cp[0]))
		    ++cp;
		  /* Terminate value.  */
		  if (cp[0] == '\n')
		    {
		      /* This has to be done to make the following
			 test for the end of line possible.  We are
			 looking for the terminating '\n' which do not
			 overwrite here.  */
		      *cp++ = '\0';
		      *cp = '\n';
		    }
		  else if (cp[0] != '\0')
		    *cp++ = '\0';

		  /* Add the alias.  */
		  PUT (strdup (alias));
		}
	    }

	  /* Possibly not the whole line fits into the buffer.
	     Ignore the rest of the line.  */
	  while (strchr (cp, '\n') == NULL)
	    {
	      cp = buf;
	      if (fgets (buf, BUFSIZ, fp) == NULL)
		/* Make sure the inner loop will be left.  The outer
		   loop will exit at the `feof' test.  */
		*cp = '\n';
	    }
	}

      fclose (fp);
    }

  twalk (all_data, print_names);
}


/* Write the names of all available character maps to stdout.  */
static void
write_charmaps (void)
{
  void *all_data = NULL;
  DIR *dir;
  struct dirent *dirent;

  dir = opendir (CHARMAP_PATH);
  if (dir == NULL)
    {
      error (1, errno, gettext ("cannot read character map directory `%s'"),
	     CHARMAP_PATH);
      return;
    }

  /* Now we can look for all files in the directory.  */
  while ((dirent = readdir (dir)) != NULL)
    if (strcmp (dirent->d_name, ".") != 0
	&& strcmp (dirent->d_name, "..") != 0)
      {
	char *buf = NULL;
	mode_t mode;

#ifdef _DIRENT_HAVE_D_TYPE
	if (dirent->d_type != DT_UNKNOWN)
	  mode = DTTOIF (dirent->d_type);
	else
#endif
	  {
	    struct stat st;

	    buf = alloca (sizeof (CHARMAP_PATH) + strlen (dirent->d_name) + 1);

	    stpcpy (stpcpy (stpcpy (buf, CHARMAP_PATH), "/"), dirent->d_name);

	    if (stat (buf, &st) < 0)
	      continue;
	    mode = st.st_mode;
	  }

	if (S_ISREG (mode))
	  {
	    FILE *fp;

	    PUT (strdup (dirent->d_name));

	    /* Read the file and learn about the code set name.  */
	    if (buf == NULL)
	      {
		buf = alloca (sizeof (CHARMAP_PATH)
			      + strlen (dirent->d_name) + 1);

		stpcpy (stpcpy (stpcpy (buf, CHARMAP_PATH), "/"),
			dirent->d_name);
	      }

	    fp = fopen (buf, "r");
	    if (fp != NULL)
	      {
		char *name = NULL;

		while (!feof (fp))
		  {
		    char junk[BUFSIZ];

		    if (fscanf (fp, " <code_set_name> %as", &name) == 1)
		      break;

		    while (fgets (junk, sizeof junk, fp) != NULL
			   && strchr (junk, '\n') == NULL)
		      continue;
		  }

		fclose (fp);

		if (name != NULL)
		  PUT (name);
	      }
	  }
      }

  closedir (dir);

  twalk (all_data, print_names);
}


/* We have to show the contents of the environments determining the
   locale.  */
static void
show_locale_vars (void)
{
  size_t cat_no;
  const char *lcall = getenv ("LC_ALL");
  const char *lang = getenv ("LANG") ? : "POSIX";

  void get_source (const char *name)
    {
      char *val = getenv (name);

      if ((lcall ?: "")[0] != '\0' || val == NULL)
	printf ("%s=\"%s\"\n", name, (lcall ?: "")[0] ? lcall : lang);
      else
	printf ("%s=%s\n", name, val);
    }

  /* LANG has to be the first value.  */
  printf ("LANG=%s\n", lang);

  /* Now all categories in an unspecified order.  */
  for (cat_no = 0; cat_no < NCATEGORIES; ++cat_no)
    get_source (category[cat_no].name);

  /* The last is the LC_ALL value.  */
  printf ("LC_ALL=%s\n", lcall ? : "");
}


/* Some of the "string" we print contain non-printable characters.  We
   encode them here.  */
static void
print_escaped (const char *string)
{
  const unsigned char *ch;

  ch = string;
  while ('\0' != *ch)
    {
      if (isprint (*ch))
	putchar (*ch);
      else
	printf("<0x%02x>", *ch);
      ++ch;
    }
}


/* Show the information request for NAME.  */
static void
show_info (const char *name)
{
  size_t cat_no;

  void print_item (struct cat_item *item)
    {
      if (show_keyword_name != 0)
	printf ("%s=", item->name);

      switch (item->value_type)
	{
	case string:
	  if (show_keyword_name)
	    putchar ('"');
	  print_escaped (nl_langinfo (item->item_id) ? : "");
	  if (show_keyword_name)
	    putchar ('"');
	  break;
	case stringarray:
	  {
	    int cnt;
	    const char *val;

	    if (show_keyword_name)
	      putchar ('"');

	    for (cnt = 0; cnt < item->max - 1; ++cnt)
	      {
		val = nl_langinfo (item->item_id + cnt);
		if (val != NULL)
		  print_escaped (val);
		putchar (';');
	      }

	    val = nl_langinfo (item->item_id + cnt);
	    if (val != NULL)
	      print_escaped (val);

	    if (show_keyword_name)
	      putchar ('"');
	  }
	  break;
	case stringlist:
	  {
	    int first = 1;
	    const char *val = nl_langinfo (item->item_id) ? : "";

	    while (*val != '\0')
	      {
		printf ("%s%s%s%s", first ? "" : ";",
			show_keyword_name ? "\"" : "", val,
			show_keyword_name ? "\"" : "");
		val = strchr (val, '\0') + 1;
		first = 0;
	      }
	  }
	  break;
	case byte:
	  {
	    const char *val = nl_langinfo (item->item_id);

	    if (val != NULL)
	      printf ("%d", *val == CHAR_MAX ? -1 : *val);
	  }
	  break;
	case bytearray:
	  {
	    const char *val = nl_langinfo (item->item_id);
	    int cnt = val ? strlen (val) : 0;

	    while (cnt > 1)
	      {
		printf ("%d;", *val == CHAR_MAX ? -1 : *val);
                --cnt;
		++val;
	      }

	    printf ("%d", cnt == 0 || *val == CHAR_MAX ? -1 : *val);
	  }
	  break;
	case word:
	  {
	    unsigned int val =
	      (unsigned int) (unsigned long int) nl_langinfo (item->item_id);
	    printf ("%d", val);
	  }
	  break;
	default:
	}
      putchar ('\n');
    }

  for (cat_no = 0; cat_no < NCATEGORIES; ++cat_no)
    {
      size_t item_no;

      if (strcmp (name, category[cat_no].name) == 0)
	/* Print the whole category.  */
	{
	  if (show_category_name != 0)
	    puts (category[cat_no].name);

	  for (item_no = 0; item_no < category[cat_no].number; ++item_no)
	    print_item (&category[cat_no].item_desc[item_no]);

	  return;
	}

      for (item_no = 0; item_no < category[cat_no].number; ++item_no)
	if (strcmp (name, category[cat_no].item_desc[item_no].name) == 0)
	  {
	    if (show_category_name != 0)
	      puts (category[cat_no].name);

	    print_item (&category[cat_no].item_desc[item_no]);
	    return;
	  }
    }

  /* The name is not a standard one.
     For testing and perhaps advanced use allow some more symbols.  */
  locale_special (name, show_category_name, show_keyword_name);
}
