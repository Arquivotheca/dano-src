#include <stdarg.h>

void
syslog(int pri, const char *fmt, ...)
{
}

void
vsyslog(int pri, register const char *fmt, va_list ap)
{
}

void
openlog (const char *ident, int logstat, int logfac)
{
}

void
closelog ()
{
}

/* setlogmask -- set the log mask level */
int
setlogmask(int pmask)
{
	return 0;
}
