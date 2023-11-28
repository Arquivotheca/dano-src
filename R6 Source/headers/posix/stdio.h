#ifndef _STDIO_H_
#define _STDIO_H_

#include <be_setup.h>

#include <size_t.h>
#include <null.h>
#include <va_list.h>
#include <wchar_t.h>

typedef long long fpos_t;

#if __GNUC__
#include <gnu_stdio.h>
#elif __POWERPC__
#include <mwerks_stdio.h>
#endif

#ifdef EOF
# undef EOF
#endif
#define EOF				-1
#define FOPEN_MAX		128	
#define STREAM_MAX 		FOPEN_MAX
#define FILENAME_MAX	256

#define L_ctermid  		32
#define L_cuserid  		32
#define	L_tmpnam		512

#define TMP_MAX			32767

#ifndef SEEK_SET
#define SEEK_SET	0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR	1
#endif
#ifndef SEEK_END
#define SEEK_END	2
#endif


__extern_c_start

FILE	*popen(const char *cmd, const char *type);
int     pclose(FILE *fp);

int 	remove(const char *name);
int 	rename(const char *old_name, const char *new_name);

char	*tempnam(char *dir, char *pfx);
char 	*tmpnam(char *name);
FILE 	*tmpfile(void);

FILE 	*fopen  (const char *name, const char *mode);
FILE 	*freopen(const char *name, const char *mode, FILE *file);

FILE 	*fdopen(int fd, const char *type);
int		fileno(FILE *fd);

int 	fclose(FILE *file);
int 	fflush(FILE *file);

int  	printf(const char *format, ...);
int  	fprintf(FILE *file, const char *format, ...);
int 	sprintf(char *s, const char *format, ...);
int 	vfprintf(FILE *file, const char *format, va_list arg);
int 	vprintf(const char *format, va_list arg);
#if __GNUC__
int asprintf (char **string_ptr, const char *format, ...);
#endif

int  	scanf (const char *format, ...);
int  	fscanf (FILE *file, const char *format, ...);
int 	sscanf (const char *s, const char *format, ...);
int 	vsprintf(char *s, const char *format, va_list arg);

int		(getchar)(void);
int		(getc)(FILE *file);

int		(putchar)(int c);
int		(putc)(int c, FILE *file);

char 	*gets(char *s);
char 	*fgets(char *s, int n, FILE *file);
int 	fgetc(FILE *file);

int 	puts(const char *s);
int 	(fputc)(int c, FILE *file);
int 	fputs(const char *s, FILE *file);

int 	(ungetc)(int c, FILE *file);

size_t	fread(void *ptr, size_t memb_size, size_t num_memb, FILE *file);
size_t	fwrite(const void *ptr, size_t memb_size, size_t num_memb, FILE *file);

int		fgetpos(FILE *file, fpos_t *pos);
long	ftell(FILE *file);
fpos_t  _ftell(FILE *);

int		fsetpos(FILE *file, const fpos_t *pos);
int		fseek(FILE *file, long offset, int mode);
int     _fseek(FILE *, fpos_t, int);

void 	setbuf (FILE *file, char *buff);
int  	setvbuf(FILE *file, char *buff, int mode, size_t size);
int 	setbuffer(FILE *stream, char *buf, size_t size);
int 	setlinebuf(FILE *stream);

void	rewind(FILE *file);
void	clearerr(FILE *file);

void	perror(const char *s);

int		(feof)(FILE *file);
int		(ferror)(FILE *file);

__extern_c_end

#endif /* _STDIO_H_ */
