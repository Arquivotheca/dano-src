/* unixfunc.c Needed functions for Unix
 */

/* (c) COPYRIGHT 1993-98           NOMBAS, INC.
 *                                 64 SALEM ST.
 *                                 MEDFORD, MA 02155  USA
 * 
 * ALL RIGHTS RESERVED
 * 
 * This software is the property of Nombas, Inc. and is furnished under
 * license by Nombas, Inc.; this software may be used only in accordance
 * with the terms of said license.  This copyright notice may not be removed,
 * modified or obliterated without the prior written permission of Nombas, Inc.
 * 
 * This software is a Trade Secret of Nombas, Inc.
 * 
 * This software may not be copied, transmitted, provided to or otherwise made
 * available to any other person, company, corporation or other entity except
 * as specified in the terms of said license.
 * 
 * No right, title, ownership or other interest in the software is hereby
 * granted or transferred.
 * 
 * The information contained herein is subject to change without notice and
 * should not be construed as a commitment by Nombas, Inc.
 */

#ifdef __JSE_UNIX__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pwd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>

#include "unixfunc.h"
#include "utilstr.h"

/* ---------------------------------------------------------------------- */

int getch()
{
   return getchar();
#if 0
   int ch;
   tty_cbreak(0);
   ch = getchar();
   tty_reset(0);
   return ch;
#endif
}

int getche()
{
   int ch = getch();
   putc(ch,stdout);
   return ch;
}

int kbhit()
{
   struct timeval tv;
   int ret;

#  if defined(_AIX)
   int mask = 1<<0;
// seb 98.11.12
#  elif defined(__JSE_BEOS__)
   typedef struct fd_set { int fds_bits[1]; } fd_set;
   fd_set mask;
#  else
#  ifdef __linux__
   /* on some linuxes, use 'struct fd_set' */
   fd_set mask;
#  else
   struct fd_set mask;
#  endif
  
   FD_ZERO(&mask);
   FD_SET(0,&mask);
#endif

   tv.tv_sec = 0;
   tv.tv_usec = 1;

   ret = select(1,&mask,NULL,NULL,&tv)>0;
#if 0
   tty_cbreak(0);
   ret = select(1,&mask,NULL,NULL,&tv)>0;
   tty_reset(0);
#endif
   return ret;
}

/* ---------------------------------------------------------------------- */

/* The algorithm: go through and convert any ~/./.. into the correct
 * values by the appropriate action (based on if this is the first
 * path component or not - two paths (he he))
 */

int MakeFullPath(char *buffer,const char *path,unsigned int buflen)
{
   const char *start = path;
   char *buffer_start = buffer;


   /* If we don't start off with a path-specifier, start with the current
    * directory.
    */
   if( *path!='.' && *path!='/' && *path!='~' )
   {
      buffer[0] = '\0'; getcwd(buffer,200);
      buffer += strlen(buffer); *buffer++ = '/';
   }

   while( *path )
   {
      /* Tilde conversion - note, something like ../../~root/joe is the same
       * as ~root/joe
       */
      if( *path=='~' && (start==path || path[-1]=='/') )
      {
         path++; buffer = buffer_start;

         /* Something like ~/ ... */
         if( *path=='\0' || *path=='/' )
         {
            char *homedir = (char *) getenv("HOME");
            if( !homedir )
            {
               struct passwd *entry;

               entry = getpwuid (getuid ());
               if( entry ) homedir = entry->pw_dir;
            }
            strcpy(buffer,homedir); buffer += strlen(buffer);
         } else {
            /* We have something like ~rich/ */
            char u_name[257];
            struct passwd *entry;
            char *name;

            name = u_name;
            while( *path && *path!='/' )
            {
               *name++ = *path++;
            }
            *name = '\0';

            if( entry = getpwnam(u_name) )
            {
               strcpy(buffer,entry->pw_dir); buffer += strlen(buffer);
               endpwent();
            } else {
               sprintf(buffer,"~%s",u_name);
               buffer += strlen(buffer);
            }
            endpwent ();
         }
         continue;
      }

      /* convert path-relative specifiers */
      if( *path=='.' && (path==start || path[-1]=='/') &&
          (path[1]=='\0' || path[1]=='/' || (path[1]=='.' && path[2]=='/')) )
      {
         /* It is a relative path. Start by filling in the cwd if needed */
         if( buffer==buffer_start )
         {
            buffer[0] = '\0'; getcwd(buffer,200);
            buffer += strlen(buffer);
         }
         *path ++;
         if( *path=='.' )
         {
            path++;
            /* Here we must go up the directory chain one entry. */
            buffer--;
            while( buffer>buffer_start && buffer[-1]!='/' ) buffer--;
            buffer--;
         }

         continue;
      }

      /* Any other character is just copied to the buffer. */
      *buffer++ = *path++;
   }
   *buffer = '\0';
   return 1;
}

/* ---------------------------------------------------------------------- */

static struct termios save_termios;
static int ttysavefd = -1;
static enum { TTY_RESET, TTY_RAW, TTY_CBREAK } ttystate = TTY_RESET;

int tty_cbreak(int fd)
{
   struct termios buf;

   if( tcgetattr(fd,&save_termios)<0 ) return -1;
   buf = save_termios;
   buf.c_lflag &= ~(ECHO|ICANON);
   buf.c_cc[VMIN] = 1;
   buf.c_cc[VTIME] = 0;
   if( tcsetattr(fd,TCSAFLUSH,&buf)<0 ) return -1;
   ttystate = TTY_CBREAK;
   ttysavefd = fd;
   return 0;
}

int tty_reset(int fd)
{
   if( ttystate!=TTY_CBREAK && ttystate!=TTY_RAW ) return 0;
   if( tcsetattr(fd,TCSAFLUSH,&save_termios)<0 ) return -1;
   ttystate = TTY_RESET;
   return 0;
}


/*
 * Reads the termcap.
 */
char *get_termcap(char *str)
{
   char *val;
   static int warning = 0;
   char buffer[4];
   static char returnval[4192];
   int i;


   /* Special case coding for the screen size. Hopefully, this will always work */

   /* Use getenv, cause it can be updated by X on the fly */
   if( !stricmp(str,"li") && (val = getenv("LINES")) ) return val;
   if( !stricmp(str,"co") && (val = getenv("COLUMNS")) ) return val;

   /* Otherwise, we simply search the termcap settings. */
   val = getenv("TERMCAP");
   if( !val )
   {
      if( warning==0 )
      {
         fprintf(stderr,"TERMCAP not set; Please set it to improve performance.\n");
         warning = 1;
      }
      return "";
   }

   buffer[0] = ':'; buffer[1] = str[0]; buffer[2] = str[1]; buffer[3] = '\0';
   val = strstr(val,buffer);
   if( !val ) return "";

   /* Found a match, copy it to returnval and then return it. */
   val += 4;
   for( i=0;val[i] && val[i]!=':';i++ )
      returnval[i] = val[i];
   returnval[i] = '\0';
   return returnval;
}

void termcap_print(char *str)
{
   while( *str )
   {
      if( *str=='\\' )
      {
         switch( *++str )
         {
            case 'E': putchar(27); break;
            case 'n': putchar(10); break;
            case 'r': putchar(13); break;
            case 't': putchar(9); break;
            case 'b': putchar(8); break;
            case 'f': putchar(12); break;
            default: putchar(*str); break;
         }
      } else {
         putchar(*str);
      }
      str++;
   }
   fflush(stdout);
}


#ifdef __sun__

double difftime(time_t t1,time_t t2)
{
   if( t1>t2 ) return (double)(t1-t2); else return (double)(t2-t1);
}

#endif

#else
   ALLOW_EMPTY_FILE
#endif
