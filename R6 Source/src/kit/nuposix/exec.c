#include	<OS.h>
#include	<image.h>
#include	<unistd.h>
#include    <sys/types.h>
#include    <dirent.h>
#include    <sys/stat.h>
#include	<fcntl.h>
#include	<errno.h>
#include    <stdlib.h>
#include    <stdarg.h>
#include    <string.h>
#undef	TRUE
#undef	FALSE
#include	<setjmp.h>

#include "priv_syscalls.h"

#include <Debug.h>

extern char **environ;

#if __ELF__
int
__execve(const char *com, char *const argv[], char *const envp[])
#else
int
execve(const char *com, char * const argv[], char * const envp[])
#endif
{
	int			argc;
	int			envc;
	int			err;

	((char **)argv)[0] = (char *)com;
	for(argc=0; argv[argc]; argc++)
		;
	for(envc=0; envp[envc]; envc++)
		;

	err = _kexec_image_(argc, (char **)argv, envc, (char **)envp);
	errno = err;
	return -1;
}

#if __ELF__
#pragma weak execve = __execve
#endif


int
execv(const char *path, char *const *argv)
{
	return execve(path, argv, environ);
}

extern char **environ;

int
execl(const char *path, const char *arg, ...)
{
	int nargs=0;
	char *str, **argv;
	va_list ap;
		
	if (arg != NULL) {
		va_start(ap, arg);
		do {
			str = va_arg(ap, char *);
			nargs++;
		} while (str != NULL);
		va_end(ap);
	}
	
	nargs++;  /* account for the first argument, "arg" */
	argv = (char **)malloc(nargs * sizeof(char *));
	if (argv == NULL) {
		errno = ENOMEM;
		return -1;
	}

	nargs = 0;
	argv[nargs++] = (char *)arg;

	if (arg != NULL) {
		va_start(ap, arg);
		do {
			argv[nargs] = va_arg(ap, char *);
		} while (argv[nargs++] != NULL);
	
		va_end(ap);
	}

	return execve(path, argv, environ);
}


static int 
find_path(char *file, char *path)
{
	const char *pathstr = getenv("PATH");
	const char *ptr, *start;
	struct stat st;
	
	if (*file == '/') {
		strcpy(path, file);
		return 1;
	}

	if (pathstr == NULL || *pathstr == '\0')   /* an empty path? */
		pathstr = "/bin:./";

	for(ptr = pathstr; *ptr; ) {
		start = ptr;
		while(*ptr && *ptr != ':')
			ptr++;

		strncpy(path, start, (ptr - start));
		path[(ptr - start)] = '\0';    /* make sure it's null terminated */
		if (ptr != start && *(ptr-1) != '/')
			strcat(path, "/");

		strcat(path, file);
		if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
			return 1;
		}

		if (*ptr == ':')    /* advance over the colon */
			ptr++;


	}

	return 0;
}

int
execlp(const char *file, const char *arg, ...)
{
	int nargs=0;
	char *str, **argv;
	char path[PATH_MAX];
	va_list ap;

	if (find_path((char *)file, path) == 0) {
		errno = ENOENT;
		return -1;
	}
		
	if (arg != NULL) {
		va_start(ap, arg);
		do {
			str = va_arg(ap, char *);
			nargs++;
		} while (str != NULL);
		va_end(ap);
	}
	
	nargs++;  /* account for the argument "arg" */
	argv = (char **)malloc(nargs * sizeof(char *));
	if (argv == NULL) {
		errno = ENOMEM;
		return -1;
	}

	nargs = 0;
	argv[nargs++] = (char *)arg;

	if (arg != NULL) {
		va_start(ap, arg);
		do {
			argv[nargs] = va_arg(ap, char *);
		} while (argv[nargs++] != NULL);
		
		va_end(ap);
	}

	return execve(path, argv, environ);
}

int
execle(const char *path, const char *arg , ...)
{
	int nargs=0;
	char *str, **argv;
	va_list ap;
	char **envp;

	if (arg != NULL) {
		va_start(ap, arg);
		do {
			str = va_arg(ap, char *);
			nargs++;
		} while (str != NULL);
		va_end(ap);
	}
	
	nargs++;  /* account for the argument "arg" */
	argv = (char **)malloc(nargs * sizeof(char *));
	if (argv == NULL) {
		errno = ENOMEM;
		return -1;
	}

	nargs = 0;
	argv[nargs++] = (char *)arg;

	va_start(ap, arg);
	if (arg != NULL) {
		do {
			argv[nargs] = va_arg(ap, char *);
		} while (argv[nargs++] != NULL);
	}

	envp = (char **)va_arg(ap, char *);
	va_end(ap);

	return execve(path, argv, envp);
}


int
exect(const char *path, char *const *argv)
{
	char **envp = argv;

	while(*envp != NULL)
		envp++;
	
	envp++;  /* environment pointer comes after argv array */
	
	return execve(path, argv, envp);
}


int
execvp(const char *file, char *const *argv)
{
	char path[PATH_MAX];

	if (find_path((char *)file, path))
		return execve(path, argv, environ);
	else {
		errno = ENOENT;
		return -1;
	}
}
