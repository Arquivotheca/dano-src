#include	<string.h>

/* Function: strupr
   Input:    char *     - String to convert
   Output:   char *     - Converted string
   Converts the string to uppercase.
   Added by Trevor Phillips, 25/6/98, for Solaris port. */
char *mystrupr(char * strng)
{
   unsigned int i, len;
	 len = strlen(strng);
   for ( i = 0 ; i < len ; i++ )
   {
      if (strng[i]>='a' && strng[i]<='z')
      {
         strng[i]+='A'-'a';
      }
   }
   return strng;
}
