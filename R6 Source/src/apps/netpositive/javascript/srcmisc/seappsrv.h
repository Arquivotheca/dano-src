/* seappsrv.h  Application services 
 */

/* (c) COPYRIGHT 1993-98           NOMBAS, INC.
 *                                 64 SALEM ST.
 *                                 MEDFORD, MA 02155  USA
 * 
 * ALL RIGHTS RESERVED
 * 
 * This software is the property of Nombas, Inc. and is furnished under
 * license by Nombas, Inc.; this software may be used only in accordance
 * with the terms of said license.  This copyright notice may not be removed,
 * modified or obliterated without the prior written permission of Nombas, Inc.
 * 
 * This software is a Trade Secret of Nombas, Inc.
 * 
 * This software may not be copied, transmitted, provided to or otherwise made
 * available to any other person, company, corporation or other entity except
 * as specified in the terms of said license.
 * 
 * No right, title, ownership or other interest in the software is hereby
 * granted or transferred.
 * 
 * The information contained herein is subject to change without notice and
 * should not be construed as a commitment by Nombas, Inc.
 */

/* ConsoleIO */
struct ConsoleIO {
   ulong (_FAR_ *Write)(jseContext jsecontext, const jsechar * data, ulong length);
   ulong (_FAR_ *Read)(jseContext jsecontext, jsechar * buffer, ulong length);
   jsechar * (_FAR_ *Gets)(jseContext jsecontext, jsechar * buffer, ulong length);
   slong (_FAR_ *Getch)(jseContext jsecontext);
   ulong (_FAR_ *Kbhit)(jseContext jsecontext);
   int (_FAR_ *Ungetc)(jseContext jsecontext, int c);
};

#define  CONSOLEIO_NAME   UNISTR("ConsoleIO")
void AddStandardService_ConsoleIO(jseContext jsecontext);
#define CONSOLEIO_CONTEXT  ((struct ConsoleIO *)jseGetSharedData(jsecontext,CONSOLEIO_NAME))

/* Interpret */
#define INTERPRET_NAME    UNISTR("Interpret")
typedef jsebool (*InterpretFunction)(jsebool,jseContext, const jsechar *,
          const jsechar *, const void *, jseNewContextSettings, int,
          jseContext, jseVariable *);
void AddStandardService_Interpret(jseContext jsecontext);
#define INTERPRET_CONTEXT  ((InterpretFunction)jseGetSharedData(jsecontext,INTERPRET_NAME))

/* Suspend */
#define SUSPEND_NAME      UNISTR("Suspend")
typedef jsebool (*SuspendFunction)(jseContext, ulong );
   /* most implementations would never return false */
void AddStandardService_Suspend(jseContext jsecontext);
#define SUSPEND_CONTEXT    ((SuspendFunction)jseGetSharedData(jsecontext,SUSPEND_NAME))

/* Spawn */
#define SPAWN_NAME        UNISTR("Spawn")
typedef jsebool (*SpawnFunction)(jseContext jsecontext, int mode, const jsechar * command,
                              jsechar **argv, int *result); 
void AddStandardService_Spawn(jseContext jsecontext);
#define SPAWN_CONTEXT      ((SpawnFunction) jseGetSharedData(jsecontext,SPAWN_NAME))

/* Environment */
struct Environment {
   const jsechar * (*GetEnv)(jseContext jsecontext, const jsechar * name);
   int    (*PutEnv)(jseContext jsecontext, const jsechar * name, const jsechar * value);
   void   (*ReSync)(jseContext jsecontext);
};
/* If NULL is passed as a parameter to GetEnv, then we want the entire list of the 
 * environment.  This is in the form of a malloc'ed array of 'NAME=VALUE' strings.
 * The names themselves will not be freed, but the array will.  Also note that the
 * return value from GetEnv will not be freed either.
 */

#define ENVIRONMENT_NAME  UNISTR("Environment")
void AddStandardService_Environment(jseContext jsecontext);
#define ENVIRONMENT_CONTEXT ((struct Environment *)jseGetSharedData(jsecontext,ENVIRONMENT_NAME))
#define PutEnvironment(name,value)  (ENVIRONMENT_CONTEXT ? ENVIRONMENT_CONTEXT->PutEnv(jsecontext,name,value) : 0)
#define GetEnvironment(name) (ENVIRONMENT_CONTEXT ? ENVIRONMENT_CONTEXT->GetEnv(jsecontext,name) : NULL)

/* MacCWD */
#define MACCWD_NAME       UNISTR("MacCwd")
#define MACCWD_SIZE       1024
void AddStandardService_MacCWD(jseContext jsecontext);
#define MACCWD_CONTEXT     ((jsechar *)jseGetSharedData(jsecontext,MACCWD_NAME))

/* Errno */
#define ERRNO_NAME        UNISTR("Errno")
#define AddStandardService_Errno(jsecontext)  jseSetSharedData(jsecontext,ERRNO_NAME,(void *)&errno)
#define GetErrno(jsecontext) ( *((sword32 *)jseGetSharedData(jsecontext,ERRNO_NAME)) )
#define SetErrno(jsecontext,val)  ( *((sword32 *)jseGetSharedData(jsecontext,ERRNO_NAME)) = (sword32) val)

struct std_file_ {
   jsechar * filename;  /* null if we don't know what the filename is */
   ulong ref;
   const jsechar * OpenMode;     /* file open modes, initialized first time only     */
   const jsechar * ReOpenMode;   /* used when re-opening a file that was redirected  */
   long ReOpenOffset;         /* used for std_in to reopen and reset to previous location */
   /* maybe add file mode (r or t) and position, although stdxxx seems to have problems with this on CON: */
   jsechar * VariableName; /* initialize first time only */
   FILE *fp;  /* Initialized first time only */
#  if defined(__JSE_CON32__)
      /* It seems that the standard file handles can get screwed up by Watcom's
       * freopen() such that even using the 'standard' method of getting to them
       * returned wacked out handles.
       */
      HANDLE saved;
      DWORD standard_value;  /* STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, & STD_ERROR_HANDLE */
#  endif
   struct std_file_ *Previous;  /* allocate and save previous value here */
};

   enum { stdin_idx = 0, stdout_idx = 1, stderr_idx = 2, std_idx_count = 3 };
   /* fields to keep track of where stdxxx may be redirected.  If NULL then goes to standard
    * console, else name is allocated here.  The routines in this structure see to it the stdxxx files
    * always point somewhere, and if file is closed then they revert to parents file or to
    * console.
    */
    
struct RedirectionInfo {
   struct std_file_ std_file[std_idx_count];
   void (*Open)(jseContext jsecontext,FILE * fp,const jsechar *name,ulong ref);
   void (*Close)(jseContext jsecontext,FILE *fp);
   uint (*Find)(jseContext jsecontext,FILE*fp);
};
#define REDIRECTION_NAME  UNISTR("REDIRECTION_INFO")

#define RedirectionInfoFromContext(jsecontext) \
   ((struct RedirectionInfo *)jseGetSharedData(jsecontext,REDIRECTION_NAME))
/* These functions are found in srcapp\filelist.c */
#define jseGetStdFileName(jsecontext,fp) \
   (RedirectionInfoFromContext(jsecontext)->std_file \
    [RedirectionInfoFromContext(jsecontext)->Find(jsecontext,fp)].filename)
#define jseStdFileReopened(jsecontext,fp) \
   (jseGetStdFileName(jsecontext,fp) != NULL)
void AddStandardService_FileRedirection(jseContext jsecontext);

/* jseMainWindow */
struct jseMainWindow;
#define JSE_MAIN_WINDOW_NAME   UNISTR("jseMainWindow")
#define jseMainWindowFromContext(jsecontext) \
   ((struct jseMainWindow *)jseGetSharedData(jsecontext,JSE_MAIN_WINDOW_NAME))
