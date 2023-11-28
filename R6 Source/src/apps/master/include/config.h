/* include/config.h.  Generated automatically by configure.  */
/*
 * PostgreSQL configuration-settings file.
 *
 * config.h.in is processed by configure to produce config.h.
 *
 * If you want to modify any of the tweakable settings in the first part
 * of this file, you can do it in config.h.in before running configure,
 * or in config.h afterwards.  Of course, if you edit config.h, then your
 * changes will be overwritten the next time you run configure.
 *
 * $Id: config.h.in,v 1.113 2000/05/12 13:58:25 scrappy Exp $
 */

#ifndef CONFIG_H
#define CONFIG_H

/*
 * Default runtime limit on number of backend server processes per postmaster;
 * this is just the default setting for the postmaster's -N switch.
 * (Actual value is now set by configure script.)
 */
#define DEF_MAXBACKENDS 32

/*
 * Hard limit on number of backend server processes per postmaster.
 * Increasing this costs about 32 bytes per process slot as of v 6.5.
 */
#define MAXBACKENDS	(DEF_MAXBACKENDS > 1024 ? DEF_MAXBACKENDS : 1024)

/*
 * Default number of buffers in shared buffer pool (each of size BLCKSZ).
 * This is just the default setting for the postmaster's -B switch.
 * Perhaps it ought to be configurable from a configure switch.
 * NOTE: default setting corresponds to the minimum number of buffers
 * that postmaster.c will allow for the default MaxBackends value.
 */
#define DEF_NBUFFERS (DEF_MAXBACKENDS > 8 ? DEF_MAXBACKENDS * 2 : 16)

/*
 * Size of a disk block --- currently, this limits the size of a tuple.
 * You can set it bigger if you need bigger tuples.
 */
/* currently must be <= 32k bjm */
#define BLCKSZ	8192

/*
 * RELSEG_SIZE is the maximum number of blocks allowed in one disk file.
 * Thus, the maximum size of a single file is RELSEG_SIZE * BLCKSZ;
 * relations bigger than that are divided into multiple files.
 *
 * CAUTION: RELSEG_SIZE * BLCKSZ must be less than your OS' limit on file
 * size.  This is typically 2Gb or 4Gb in a 32-bit operating system.  By
 * default, we make the limit 1Gb to avoid any possible integer-overflow
 * problems within the OS.  A limit smaller than necessary only means we
 * divide a large relation into more chunks than necessary, so it seems
 * best to err in the direction of a small limit.  (Besides, a power-of-2
 * value saves a few cycles in md.c.)
 *
 * CAUTION: you had best do an initdb if you change either BLCKSZ or
 * RELSEG_SIZE.
 */
#define RELSEG_SIZE	(0x40000000 / BLCKSZ)

/*
 * As soon as the backend blocks on a lock, it waits this number of seconds
 * before checking for a deadlock.
 * We don't check for deadlocks just before sleeping because a deadlock is
 * a rare event, and checking is an expensive operation.
 */
#define DEADLOCK_CHECK_TIMER 1

/*
 * Maximum number of columns in an index and maximum number of arguments
 * to a function. They must be the same value.
 *
 * The minimum value is 9 (btree index creation has a 9-argument function).
 *
 * There is no maximum value, though if you want to pass more than 32 
 * arguments to a function, you will have to modify 
 * pgsql/src/backend/utils/fmgr/fmgr.c and add additional entries 
 * to the 'case' statement for the additional arguments.
 */
#define INDEX_MAX_KEYS		16
#define FUNC_MAX_ARGS		INDEX_MAX_KEYS

/*
 * Enables debugging print statements in the date/time support routines.
 * Particularly useful for porting to a new platform/OS combination.
 */
/* #define DATEDEBUG */

/*
 * defining unsafe floats's will make float4 and float8
 * ops faster at the cost of safety, of course!        
 */
/* #define UNSAFE_FLOATS */

/*
 * Define this to make libpgtcl's "pg_result -assign" command process C-style
 * backslash sequences in returned tuple data and convert Postgres array
 * attributes into Tcl lists.  CAUTION: this conversion is *wrong* unless
 * you install the routines in contrib/string/string_io to make the backend
 * produce C-style backslash sequences in the first place.
 */
/* #define TCL_ARRAYS */

/*
 * User locks are handled totally on the application side as long term
 * cooperative locks which extend beyond the normal transaction boundaries.
 * Their purpose is to indicate to an application that someone is `working'
 * on an item.  Define this flag to enable user locks.  You will need the
 * loadable module user-locks.c to use this feature.
 */
#define USER_LOCKS

/* Genetic Query Optimization (GEQO):
 * 
 * The GEQO module in PostgreSQL is intended for the solution of the
 * query optimization problem by means of a Genetic Algorithm (GA).
 * It allows the handling of large JOIN queries through non-exhaustive
 * search.
 * For further information see README.GEQO <utesch@aut.tu-freiberg.de>.
 */
#define GEQO

/*
 * Define this if you want psql to _always_ ask for a username and a password
 * for password authentication.
 */
/* #define PSQL_ALWAYS_GET_PASSWORDS */

/*
 * Define this if you want to allow the lo_import and lo_export SQL functions
 * to be executed by ordinary users.  By default these functions are only
 * available to the Postgres superuser.  CAUTION: these functions are
 * SECURITY HOLES since they can read and write any file that the Postgres
 * backend has permission to access.  If you turn this on, don't say we
 * didn't warn you.
 */
/* #define ALLOW_DANGEROUS_LO_FUNCTIONS */

/*
 * Use btree bulkload code: 
 * this code is moderately slow (~10% slower) compared to the regular
 * btree (insertion) build code on sorted or well-clustered data.  on
 * random data, however, the insertion build code is unusable -- the
 * difference on a 60MB heap is a factor of 15 because the random
 * probes into the btree thrash the buffer pool.
 *
 * Great thanks to Paul M. Aoki (aoki@CS.Berkeley.EDU)
 */
#define FASTBUILD /* access/nbtree/nbtsort.c */

/*
 * TBL_FREE_CMD_MEMORY: free memory allocated for a user query inside
 * transaction block after this query is done. 
 */
#define TBL_FREE_CMD_MEMORY

/*
 * ELOG_TIMESTAMPS: adds a timestamp with the following format to elog
 * messages:  yymmdd.hh:mm:ss.mmm [pid] message
 */
/* #define ELOG_TIMESTAMPS */

/*
 * USE_SYSLOG: use syslog for elog and error messages printed by tprintf
 * and eprintf. This must be activated with the syslog flag in pg_options
 * (syslog=0 for stdio, syslog=1 for stdio+syslog, syslog=2 for syslog).
 * For information see backend/utils/misc/trace.c (Massimo Dal Zotto).
 */
/* #define USE_SYSLOG */

/* Debug #defines */
/* #define IPORTAL_DEBUG  */
/* #define HEAPDEBUGALL  */
/* #define ISTRATDEBUG  */
/* #define FASTBUILD_DEBUG */
/* #define ACLDEBUG */
/* #define RTDEBUG */
/* #define GISTDEBUG */
/* #define OMIT_PARTIAL_INDEX */
/* #define NO_BUFFERISVALID   */
/* #define NO_SECURITY        */
/* #define OLD_REWRITE        */

/*
 * MAXPGPATH: standard size of a pathname buffer in Postgres (hence,
 * maximum usable pathname length is one less).
 *
 * We'd use a standard system header symbol for this, if there weren't
 * so many to choose from: MAXPATHLEN, _POSIX_PATH_MAX, MAX_PATH, PATH_MAX
 * are all defined by different "standards", and often have different
 * values on the same platform!  So we just punt and use a reasonably
 * generous setting here.
 */
#define MAXPGPATH		1024

/*
 * DEFAULT_MAX_EXPR_DEPTH: default value of max_expr_depth SET variable.
 */
#define DEFAULT_MAX_EXPR_DEPTH	10000


/*
 *------------------------------------------------------------------------
 * Everything past here is set by the configure script.
 *------------------------------------------------------------------------
 */

/* Set to 1 if you want to USE_LOCALE */
/* #undef USE_LOCALE */

/* Set to 1 if you want CYR_RECODE (cyrillic recode) */
/* #undef CYR_RECODE */

/* Set to 1 if you want to use multibyte characters */
/* #undef MULTIBYTE */

/* Set to 1 if you want to Enable ASSERT CHECKING */
/* #undef USE_ASSERT_CHECKING */

/* 
 * DEF_PGPORT is the TCP port number on which the Postmaster listens by
 * default.  This can be overriden by command options, environment variables,
 * and the postconfig hook. (now set by configure script)
 */ 
#define DEF_PGPORT "5432" 

/* Define const as empty if your compiler doesn't grok const. */
/* #undef const */

/* Define as your compiler's spelling of "inline", or empty if no inline. */
/* #undef inline */

/* Define signed as empty if your compiler doesn't grok "signed char" etc */
/* #undef signed */

/* Define volatile as empty if your compiler doesn't grok volatile. */
/* #undef volatile */

/* Define if your cpp understands the ANSI stringizing operators in macros */
#define HAVE_STRINGIZE 1

/* Set to 1 if you have <arpa/inet.h> */
#define HAVE_ARPA_INET_H 1

/* Set to 1 if you have <crypt.h> */
/* #undef HAVE_CRYPT_H */

/* Set to 1 if you have <dld.h> */
/* #undef HAVE_DLD_H */

/* Set to 1 if you have <endian.h> */
#define HAVE_ENDIAN_H 1

/* Set to 1 if you have <float.h> */
#define HAVE_FLOAT_H 1

/* Set to 1 if you have <fp_class.h> */
/* #undef HAVE_FP_CLASS_H */

/* Set to 1 if you have <getopt.h> */
#define HAVE_GETOPT_H 1

/* Set to 1 if you have <history.h> */
/* #undef HAVE_HISTORY_H */

/* Set to 1 if you have <ieeefp.h> */
/* #undef HAVE_IEEEFP_H */

/* Set to 1 if you have <limits.h> */
#define HAVE_LIMITS_H 1

/* Set to 1 if you have <netdb.h> */
#define HAVE_NETDB_H 1

/* Set to 1 if you have <netinet/in.h> */
#define HAVE_NETINET_IN_H 1

/* Set to 1 if you have <readline.h> */
/* #undef HAVE_READLINE_H */

/* Set to 1 if you have <readline/history.h> */
/* #undef HAVE_READLINE_HISTORY_H */

/* Set to 1 if you have <readline/readline.h> */
/* #undef HAVE_READLINE_READLINE_H */

/* Set to 1 if  you have <sys/select.h> */
/* #undef HAVE_SYS_SELECT_H */

/* Set to 1 if you have <termios.h> */
#define HAVE_TERMIOS_H 1

/* Set to 1 if  you have <values.h> */
/* #undef HAVE_VALUES_H */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* default path for the location of the odbcinst.ini file */
/* #undef ODBCINST */

/* Define if you have the setproctitle function.  */
/* #undef HAVE_SETPROCTITLE */

/* Define if you have the stricmp function.  */
/* #undef HAVE_STRICMP */

/* Set to 1 if you have libreadline and it includes history functions */
/* #undef HAVE_HISTORY_IN_READLINE */

/*
 * Block of parameters for the ODBC code.
 */

/* Set to 1 if you have <pwd.h> */
#define HAVE_PWD_H 1

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the dl library (-ldl).  */
/* #undef HAVE_LIBDL */

/*
 * End parameters for ODBC code.
 */

/* Set to 1 if you gettimeofday(a,b) vs gettimeofday(a) */
#define HAVE_GETTIMEOFDAY_2_ARGS 1
#ifndef HAVE_GETTIMEOFDAY_2_ARGS
# define gettimeofday(a,b) gettimeofday(a)
#endif

/* Set to 1 if you have snprintf() in the C library */
#define HAVE_SNPRINTF 1

/* Set to 1 if your standard system headers declare snprintf() */
#define HAVE_SNPRINTF_DECL 1

/* Set to 1 if you have vsnprintf() in the C library */
#define HAVE_VSNPRINTF 1

/* Set to 1 if your standard system headers declare vsnprintf() */
#define HAVE_VSNPRINTF_DECL 1

/* Set to 1 if you have strerror() */
#define HAVE_STRERROR 1

/*
 *	Set to 1 if you have isinf().
 *	These are all related to port/isinf.c 
 */
/* #undef HAVE_FPCLASS */
/* #undef HAVE_FP_CLASS */
/* #undef HAVE_FP_CLASS_H */
/* #undef HAVE_FP_CLASS_D */
/* #undef HAVE_CLASS */

#define HAVE_ISINF 1
#ifndef HAVE_ISINF
int isinf(double x);
#endif

/* Set to 1 if you have gethostname() */
#define HAVE_GETHOSTNAME 1
#ifndef HAVE_GETHOSTNAME
int  gethostname(char *name, int namelen);
#endif

/* Set to 1 if struct tm has a tm_zone member */
#define HAVE_TM_ZONE 1

/* Set to 1 if you have int timezone.
 * NOTE: if both tm_zone and a global timezone variable exist,
 * using the tm_zone field should probably be preferred,
 * since global variables are inherently not thread-safe.
 */
#define HAVE_INT_TIMEZONE 1

/* Set to 1 if you have cbrt() */
#define HAVE_CBRT 1

/* Set to 1 if you have inet_aton() */
/* #undef HAVE_INET_ATON */
#ifndef HAVE_INET_ATON
# ifdef HAVE_ARPA_INET_H
#  ifdef HAVE_NETINET_IN_H
#   include <sys/types.h>
#   include <netinet/in.h>
#  endif
#  include <arpa/inet.h>
# endif
extern int  inet_aton(const char *cp, struct in_addr * addr);
#endif

/* Set to 1 if you have fcvt() */
#define HAVE_FCVT 1

/* Set to 1 if you have rint() */
#define HAVE_RINT 1 

/* Set to 1 if you have finite() */
#define HAVE_FINITE 1

/* Set to 1 if you have memmove() */
#define HAVE_MEMMOVE 1

/* Set to 1 if you have sigsetjmp() */
#define HAVE_SIGSETJMP 1

/*
 * When there is no sigsetjmp, its functionality is provided by plain
 * setjmp. Incidentally, nothing provides setjmp's functionality in
 * that case.
 */
#ifndef HAVE_SIGSETJMP
# define sigjmp_buf jmp_buf
# define sigsetjmp(x,y)	setjmp(x)
# define siglongjmp longjmp
#endif

/* Set to 1 if you have sysconf() */
#define HAVE_SYSCONF 1

/* Set to 1 if you have getrusage() */
#define HAVE_GETRUSAGE 1

/* Set to 1 if you have waitpid() */
#define HAVE_WAITPID 1

/* Set to 1 if you have setsid() */
#define HAVE_SETSID 1

/* Set to 1 if you have sigprocmask() */
#define HAVE_SIGPROCMASK 1

/* Set to 1 if you have sigprocmask() */
#define HAVE_STRCASECMP 1
#ifndef HAVE_STRCASECMP
extern int  strcasecmp(char *s1, char *s2);
#endif

/* Set to 1 if you have strtol() */
#define HAVE_STRTOL 1

/* Set to 1 if you have strtoul() */
#define HAVE_STRTOUL 1

/* Set to 1 if you have strdup() */
#define HAVE_STRDUP 1
#ifndef HAVE_STRDUP
extern char *strdup(char const *);
#endif

/* Set to 1 if you have random() */
#define HAVE_RANDOM 1
#ifndef HAVE_RANDOM
extern long random(void);
#endif

/* Set to 1 if you have srandom() */
#define HAVE_SRANDOM 1
#ifndef HAVE_SRANDOM
extern void srandom(unsigned int seed);
#endif

/* Set to 1 if you have libreadline.a */
/* #undef HAVE_LIBREADLINE */

/* Set to 1 if you have libhistory.a */
/* #undef HAVE_LIBHISTORY */

/* Set to 1 if your libreadline defines rl_completion_append_character */
/* #undef HAVE_RL_COMPLETION_APPEND_CHARACTER */

/* Set to 1 if your libreadline has filename_completion_function */
/* #undef HAVE_FILENAME_COMPLETION_FUNCTION */

/* Set to 1 if your readline headers actually declare the above */
/* #undef HAVE_FILENAME_COMPLETION_FUNCTION_DECL */

/* Set to 1 if you have getopt_long() (GNU long options) */
#define HAVE_GETOPT_LONG 1


/*
 * On architectures for which we have not implemented spinlocks (or
 * cannot do so), we use System V semaphores.  We also use them for
 * long locks.  For some reason union semun is never defined in the
 * System V header files so we must do it ourselves.
 */
/* Set to 1 if you have union semun */
/* #undef HAVE_UNION_SEMUN */

/* Set to 1 if you have F_SETLK option for fcntl() */
/* #undef HAVE_FCNTL_SETLK */

/* Set to 1 if type "long int" works and is 64 bits */
/* #undef HAVE_LONG_INT_64 */

/* Set to 1 if type "long long int" works and is 64 bits */
#define HAVE_LONG_LONG_INT_64 1

/* Define this as the appropriate snprintf format for 64-bit ints, if any */
#define INT64_FORMAT "%lld"

/* These must be defined as the alignment requirement (NOT the size) of
 * each of the basic C data types (except char, which we assume has align 1).
 * MAXIMUM_ALIGNOF is the largest alignment requirement for any C data type.
 * ALIGNOF_LONG_LONG_INT need only be defined if HAVE_LONG_LONG_INT_64 is.
 */
#define ALIGNOF_SHORT 2
#define ALIGNOF_INT 4
#define ALIGNOF_LONG 4
#define ALIGNOF_LONG_LONG_INT 4
#define ALIGNOF_DOUBLE 4
#define MAXIMUM_ALIGNOF 4

/* Define as the base type of the last arg to accept */
#define SOCKET_SIZE_TYPE size_t

/* Define if POSIX signal interface is available */
/* #undef USE_POSIX_SIGNALS */

/* Define if C++ compiler accepts "using namespace std" */
#define HAVE_NAMESPACE_STD 1

/* Define if C++ compiler accepts "#include <string>" */
#define HAVE_CXX_STRING_HEADER 1


/*
 * Pull in OS-specific declarations (using link created by configure)
 */

#include "os.h"

/*
 * The following is used as the arg list for signal handlers.  Any ports
 * that take something other than an int argument should change this in
 * the port specific makefile.  Note that variable names are required
 * because it is used in both the prototypes as well as the definitions.
 * Note also the long name.  We expect that this won't collide with
 * other names causing compiler warnings.
 */ 

#ifndef       SIGNAL_ARGS
#  define SIGNAL_ARGS int postgres_signal_arg
#endif


#endif /* CONFIG_H */
