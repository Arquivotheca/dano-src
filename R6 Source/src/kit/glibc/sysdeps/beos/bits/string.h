#ifndef _STRING_H
# error "Never include <bits/string.h> directly; use <string.h> instead."
#endif

extern int strncasecmp(const char *str1, const char *str2, size_t nchars);	
extern const char *const sys_siglist[];
extern const char *strsignal(int sig);
extern char *stpcpy(char *dest, const char *src);

#define __NO_STRING_INLINES		/* don't want those nasty macros in string2.h */
