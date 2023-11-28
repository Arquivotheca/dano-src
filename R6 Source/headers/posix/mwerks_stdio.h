#ifndef _STDIO_H_
# error "Never include mwerks_stdio.h directly! Include stdio.h instead."
#endif

#ifndef _MWERKS_STDIO_H_
#define _MWERKS_STDIO_H_

#define _IONBF	0
#define _IOLBF	1
#define _IOFBF	2
#define BUFSIZ			4096

__extern_c_start

enum __file_kinds {
	__closed_file,
	__disk_file,
	__console_file,
	__string_file,
	__unavailable_file
};

enum __open_modes {
	__must_exist,
	__create_if_necessary,
	__create_or_truncate
};

enum __io_modes {
	__read				= 1,
	__write				= 2,
	__read_write		= 3,
	__append			= 4
};


typedef struct {
	unsigned char	open_mode;
	unsigned char	io_mode;
	unsigned char	buffer_mode;
	unsigned char	file_kind;
	unsigned char	binary_io;
} __file_modes;

typedef struct {
	unsigned char	_io_state;
	unsigned char	_free_buffer;
	unsigned char	_eof;
	unsigned char	_error;
} __file_state;

enum __io_states {
	__neutral,
	__writing,
	__reading,
	__rereading
};

typedef unsigned long	__file_handle;

typedef struct _FILE FILE;

enum __io_results {
     __no_io_error,
     __io_error,
     __io_EOF
};

typedef void (* __idle_proc)  (void);
typedef int  (* __pos_proc)   (__file_handle file, fpos_t * position, int mode, __idle_proc idle_proc);     /* mm 970708 */
typedef int  (* __io_proc)    (__file_handle file, unsigned char * buff, size_t * count, __idle_proc idle_proc);
typedef int	 (* __close_proc) (__file_handle file);

#define __ungetc_buffer_size	2

struct _FILE {
	__file_handle		handle;
	__file_modes		mode;
	__file_state		_state;			/* Be-mani 980107 */
	unsigned char		char_buffer;
	unsigned char		char_buffer_overflow;
	unsigned char		ungetc_buffer[__ungetc_buffer_size];
	fpos_t          	position;		/* bds 131097 */
	unsigned char *		buffer;
	unsigned long		buffer_size;
	unsigned char *		_buffer_ptr;	/* Be-mani 980107 */
	unsigned long		_buffer_len;	/* Be-mani 980107 */
	unsigned long		buffer_alignment;
	unsigned long		saved_buffer_len;
	fpos_t				buffer_pos;		/* bds 131097 */
	__pos_proc			position_proc;
	__io_proc			read_proc;
	__io_proc			write_proc;
	__close_proc		close_proc;
	__idle_proc			idle_proc;
};

# define __files _files			/* Be-mani 980107a necessary for
								 * backward compatibility. */

#define stdin  	(&__files[0])
#define stdout	(&__files[1])
#define stderr	(&__files[2])

extern FILE		__files[];


int		__get_char(FILE * file);
int		__put_char(int c,FILE * file);

FILE 	*__handle_open  (__file_handle handle, const char * mode);
FILE 	*__handle_reopen(__file_handle handle, const char * mode, FILE * file);

int		__vfscanf(FILE * file,    const char * format, va_list arg);
int		__vsscanf(const char * s, const char * format, va_list arg);

void	__set_idle_proc(FILE * file, __idle_proc idle_proc);

#ifdef __cplusplus

inline int getc(FILE * file)
	{ return file->_buffer_len-- ? *file->_buffer_ptr++ : __get_char(file); }

inline int putc(int c, FILE *file)
	{ return file->_buffer_len-- ? (*file->_buffer_ptr++ = (unsigned char)c) : __put_char(c, file); }

inline int getchar()
	{ return getc(stdin); }

inline int putchar(int c)
	{ return putc(c, stdout); }

inline int feof(FILE * file)
	{ return file->_state._eof; }

inline int ferror(FILE * file)
	{ return file->_state._error; }

#else

#define getc(file)		((file)->_buffer_len-- ? (int)  *(file)->_buffer_ptr++      : __get_char(file))
#define putc(c, file)	((file)->_buffer_len-- ? (int) (*(file)->_buffer_ptr++ = c) : __put_char(c, file))
#define getchar()		getc(stdin)
#define putchar(c)		putc(c, stdout)
#define feof(file)		((file)->_state._eof)
#define ferror(file)	((file)->_state._error)

#endif /* C++ */

__extern_c_end

#endif /* _MWERKS_STDIO_H_ */
