/*
 * This is a program to print out any ascii strings it may find in a binary program.
 *
 * It is purposely very simple.  The normal strings program "knows" about
 * executables and libraries and doesn't print out _all_ the strings in
 * such files.  I just want something that will print out any string greater
 * than a certain length.  I can use grep to filter out junk.
 *
 * (written a long time ago but we don't have one so I'm "donating" it)
 *
 * Dominic Giampaolo
 * dbg@be.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define NULL_STATE  0
#define IN_STRING   1


char *progname=NULL;
long   min_str_len=5;


void
get_strings(char *fname)
{
  FILE *fp;
  char buff[256], *ptr;
  int state, ch;
  long curr_len=0;
  
  fp = fopen(fname, "rb");
  if (fp == NULL)
   {
     fprintf(stderr, "%s: Can't open %s\n", progname, fname);
     return;
   }

  state = NULL_STATE;
  while((ch = getc(fp)) != EOF)
   {
     if (state == NULL_STATE)
      {
	if (isprint(ch) || ch == '\t')
	 {
	   state = IN_STRING;
	   curr_len = 0;
	   buff[curr_len++] = ch;
	   buff[curr_len]   = '\0';           /* null terminate */
	 }
      }
     else if (state == IN_STRING)
      {
	if (isprint(ch) || ch == '\t')
	   {
	     if (min_str_len == 1 && curr_len == 1)
	      {
		printf("%c", buff[0]);
		curr_len = 2;
	      }
	     
	     if (curr_len < min_str_len)
	      {
		buff[curr_len++] = ch;
		if (curr_len >= min_str_len)
		 {
		   buff[curr_len] = '\0';     /* null terminate */
		   printf("%s", buff);
		 }
	      }
	     else
	       printf("%c", ch);
	   }
	else
	 {
	   if (curr_len >= min_str_len) /* if we actually printed anything */
	     printf("\n");
	   curr_len = 0;
	   state = NULL_STATE;
	 }
      }
   }

  fclose(fp);
}



void
usage(char *pname)
{
  fprintf(stderr, "Usage: %s [-l min_str_len] fname...\n", pname);
  exit(1);
}



main(int argc, char **argv)
{
  int i,min_arg=1;

  progname = argv[0];

  if (argc == 1)
    usage(progname);

  if (argv[1] && strcmp(argv[1], "-l") == 0)  /* get the min string length */
   {
     if (argv[2] == NULL || !isdigit(argv[2][0]))
      {
	fprintf(stderr, "%s: Argument to -l option should be a number (was %s).\n",
		progname, argv[2]);
	exit(5);
      }

     min_str_len = strtol(argv[2], NULL, 0);
     min_arg = 3;
   }

  for (i=min_arg; i < argc; i++)
   {
     get_strings(argv[i]);
   }

  exit(0);
}
