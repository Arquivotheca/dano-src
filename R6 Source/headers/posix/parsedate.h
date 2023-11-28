/*
   This is the header file for the routine parsedate().  The routine
   parsedate() returns the integer representation of a human readable
   ascii string such as:  Mon, June 10th, 1993 10:00:03 am GMT

   The parsedate() routine understands many, many date formats, some
   complicated, some not.  It also understands casual formats such as
   "next friday", "9 pm last tuesday", or "this saturday" and even
   formats that are relative to the current time like: "+15 min" or
   "+3:00" to indicate 15 minutes from now and 3 hours from now.

   The specifications of the functions declared in this header file
   are:

   Name:   time_t parsedate(const char *datestr, time_t now);
   Input:  char *datestr - pointer to the string containing the date 
           time_t now    - the time with which the datestring should 
		                   be parsed with respect to.  Specify a -1
						   to indicate the current time (this is the
						   usual usage).
   Return: time_t  - number of seconds since Jan 1, 1970 00:00 GMT
                     or -1 if datestr is an unrecognized date format
   

   Name:   void set_dateformats(char *table[]);
   Input:  char *table - pointer to an array of strings containing date
                         formats.  The last entry must be a NULL. The
						 format of these strings is described below.
   Return: None
   
   Name:   const char **get_dateformats();
   Input:  None
   Return: char **  - pointer to the array of strings currently in use.
                      This table as a NULL entry to mark the end.
   
   
   Other Instructions:
   
   The default list of date formats that parsedate understands
   is quite exhaustive (the result of culling the Date: line from
   80,000 usenet messages as well as a few hand crafted formats
   for convenience).  It will be rare that you should need to specify
   your own date formats with set_dateformats().

   If you need to specify your own date formats, you can do so with
   the function set_dateformats().  The table of formats have the
   following syntax (which is the same as the Unix "date" command):

	   A/a = Weekday
	   B/b = Month name
	   d   = Day of month as decimal
	   H/I = Hours
	   m   = Month as a decimal
	   M   = Minute as a decimal
	   p   = Meridian ie am/pm
	   S   = Seconds as a decimal
	   Y/y = Year
	   Z/z = Time Zone
	   T   = Time Unit eg 1 sec/ 2 hrs/etc.
	   :   = Must have a colon
	   ,   = Must have a comma
	   .   = Must have a period
	   -   = Must have a dash (or a "/")
	   []  = brackets mean the enclosed field is optional
	         note: brackets only work around a single field
   
   Longer patterns should come before shorter patterns since the first
   match that is found is used.  For example, a format such as "H:M:S"
   should come before the pattern "H:M" or else "H:M:S" would never be
   matched.

   An example format string is:

      "[A][,] B d[,] [Y] H:M:S [p] [Z]",

   Which handles date strings such as: Mon, June 10th, 1993 10:00:03 am GMT

   A simpler example is something like:

       "y-m-d"

   which is the specification for strings like:  1994-01-27

*/   

#ifndef _PARSEDATE_H
#define _PARSEDATE_H

#ifndef __time__
#include <time.h>
#endif
#if __BEOS__
#include <BeBuild.h>
#else
#endif

#ifdef __cplusplus
extern "C" {
#endif

time_t       parsedate_etc(const char *s, time_t now, int *flags);

/*
   The flags passed back from parsedate tell you about how it parsed
   the string.  Other flags may be defined in the future that will
   affect how parsedate() behaves.
*/   
#define PARSEDATE_RELATIVE_TIME    0x0001  /* the string was relative */


time_t       parsedate(const char *s, time_t now);
void         set_dateformats(const char *table[]);
const char **get_dateformats();

#ifdef __cplusplus
}
#endif

#endif /* _PARSEDATE_H */
