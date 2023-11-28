#ifndef _BOOT_VIDEO_H
#define _BOOT_VIDEO_H

void clear_screen(void);
void invert(int x1, int x2, int y);
void print_at(int x, int y, const char *s);
void center(int y, const char *s);

#define COLOR_TAG 1

#define LOBLUE	"\001\001"
#define LOGREEN	"\001\002"
#define LOCYAN	"\001\003"
#define LORED	"\001\004"
#define PURPLE	"\001\005"
#define BROWN	"\001\006"
#define GREY	"\001\007"
#define LOGREY	"\001\010"
#define BLUE	"\001\011"
#define GREEN	"\001\012"
#define CYAN	"\001\013"
#define RED		"\001\014"
#define PINK	"\001\015"
#define YELLOW	"\001\016"
#define WHITE	"\001\017"

void console_print(const char *s);

#endif
