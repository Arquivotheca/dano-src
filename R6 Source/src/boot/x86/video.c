#include <stdlib.h>

#include <support/SupportDefs.h>

#include "video.h"

void invert(int x1, int x2, int y)
{
	uchar *p;

	p = (uchar *)0xb8000 + y * 160 + x1 * 2 + 1;
	for (;x1<=x2;x1++,p+=2)
		*p = *p ^ 0x77;
}

void print_at(int x, int y, const char *s)
{
	char current_color = 7;
	uchar *p;
	
	p = (uchar *)0xb8000 + y * 160 + x * 2;
	
	while (*s) {
		if (*s == COLOR_TAG) {
			s++;
			current_color = *(s++);
		} else {
			*(p++) = *(s++);
			*(p++) = current_color;
		}
	}
}

static int tstrlen(const char *s)
{
	int n = 0;
	while (*s) {
		if (*(s++) == COLOR_TAG)
			s++;
		else
			n++;
	}
	return n;
}

void center(int y, const char *s)
{
	char t[81];
	int len = tstrlen(s);

	memset(t, ' ', 80);
	t[80] = 0;
	strcpy(t + 39 - len / 2, s);

	print_at(0, y, t);
//	print_at(39 - len/2, y, s);
}

/* console routines */

static int	curpos = 0xb8000, curcol = 0, currow = 0;

void clear_screen(void)
{
	curpos = 0xb8000; curcol = currow = 0;
	memset((void *)0xb8000, 0, 80*25*2);
}

static void
scroll(int lines)
{
	char buffer[80*25*2];
	memcpy(buffer, (char *)0xb8000, 80*25*2);
	memcpy((char *)0xb8000, buffer + lines*80*2, 80*2*(25-lines));
	memset((char *)0xb8000 + 80*2*(25 - lines), 0, 80*2*lines);
}

static void
print_char(char c)
{
	if (c == '\n') {
		curcol = 0, currow++;
		curpos = 0xb8000 + 160 * currow;
	} else {
		*(unsigned short *)curpos = c + 0x700;
		curpos += 2;
		if (++curcol == 80)
			curcol = 0, currow++;
	}
	if (currow > 24) {
		scroll(1);
		currow--;
		curpos -= 160;
	}
}

void
console_print(const char *s)
{
	while (*s) print_char(*(s++));
}

