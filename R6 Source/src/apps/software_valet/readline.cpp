#include <stdio.h>
#include <SupportDefs.h>

status_t readline(FILE *f, char *buf, int bufsize);

status_t readline(FILE *f, char *buf, int bufsize)
{
	status_t readErr = B_NO_ERROR;
	char *c = buf;
	char *max = buf + bufsize;

	while (1) {
		if (c >= max) {
			// buffer full
			return c - buf;
		}
		int ch = getc(f);
		
		if (ch < 0) {
			// EOF
			if (c - buf) {
				*c = 0;
				return c - buf;
			}
			return ch;
		}
		if (ch == '\r') {			
			// read ahead one character
			int eh = getc(f);
			
			if (eh == '\n') {
				// we got extra linefeed, crlf
				// end of line
				*c = 0;
				// return line-length
				return c - buf;
			}
			else {
				// no extra line feed
				// next char is for next call
				// end of line
				*c = 0;
				if (eh >= 0) {
					// push back extra character
					ungetc(eh, f);
					// return line-length
					return c - buf;
				}
				else if (eh < 0 && c - buf) {
					// eof w/ partial line
					return c - buf;
				}
				else {
					// eof
					return eh;
				}
			}
		}
		else if (ch == '\n') {
			// end of line
			*c = 0;
			return c - buf;
		}
		*c++ = ch;
	}
}
