#include <Debug.h>
#include <image.h>
#include <signal.h>
#include <unistd.h>

#include "XpandoMatic.h"
#include "XpandoWindow.h"

//--------------------------------------------------------------------

static int
is_meta(char ch)
{
    if (ch == '['  || ch == ']'  || ch == '*'  || ch == '"' || ch == '\'' ||
        ch == '?'  || ch == '^'  || ch == '\\' || ch == ' ' || ch == '\t' ||
        ch == '\n' || ch == '\r' || ch == '('  || ch == ')' || ch == '{'  ||
        ch == '}'  || ch == '!'  || ch == '$'  || ch == '%' || ch == '~'  ||
        ch == '`'  || ch == '@'  || ch == '#'  || ch == '&' || ch == '>'  ||
        ch == '<'  || ch == ';'  || ch == '|')
        return 1;

    return 0;
}


static char *
escape_meta_chars(char *str)
{
    int len, num_meta = 0;
    char *tmp, *ptr;

    for(len=0, tmp=str; *tmp; tmp++) {
        if (is_meta(*tmp))
            num_meta++;
        else
            len++;
    }

    ptr = (char *)malloc(len + num_meta*2 + 1);
    if (ptr == NULL)
        return ptr;

    for(tmp=ptr; *str; str++) {
        if (is_meta(*str)) {
            *tmp++ = '\\';
            *tmp++ = *str;
        } else {
            *tmp++ = *str;
        }
    }

    *tmp = '\0';     /* null terminate */
    
    return ptr;
}

//--------------------------------------------------------------------

bool
GetFileType(char *filepath,char *filetype)
{
	BNode		node(filepath);
	BNodeInfo	node_info(&node);

	filetype[0] = 0;
	if (node_info.GetType(filetype) == B_NO_ERROR) 
		return true;

	update_mime_info(filepath, 0, 1, 0);
	if (node_info.GetType(filetype) == B_NO_ERROR) 
		return true;

	return false;
}

void
GetFileTypeDescription(char *longmimetype,char *shortmimetype)
{
	BMimeType	mtype(longmimetype);

	if (mtype.GetShortDescription(shortmimetype) != 0)
		shortmimetype[0] = 0;
}

static bool
CheckForSpecialDirs(char *path)
{
	bool	retval = false;
	char	*suffix;

	if (path == NULL)
		return false;
		
	//
	//	here's a bit of a hack
	//	hmmm, what other special places should be here
	//
	suffix = strrchr(path, '/');
	if (suffix == NULL)
		retval = false;
	else if (strcmp(suffix,"/") == 0)
		retval = false;
	else if (strcmp(suffix,"/dev") == 0)
		retval = false;
	else if (strcmp(suffix,"/pipe") == 0)
		retval = false;
	else if (strcmp(suffix,"/Trash") == 0)
		retval = false;
	else
		retval = true;
	//
	//	check and see if its a read only volume
	//
	if (retval) {
		struct stat st;
		
		stat(path,&st);
		
		BVolume v(st.st_dev);
		retval = (v.IsReadOnly() == false);
	}
	
	return retval;
}

bool
CheckCanExpand(char *path)
{
	expand_action *ea;

	ea = find_action(path);
	if (ea)
		return true;
	else
		return false;
}

bool
CheckType(char *path,char *filetype)
{
	bool	retval=false;
	
	GetFileType(path,filetype);

	if (strcmp("application/x-vnd.Be-directory",filetype) == 0)
		retval = CheckForSpecialDirs(path);
	else
		retval = false;
	
	return retval;
}

void
ShowDirError(char *path)
{
	char msg[B_PATH_NAME_LENGTH + 256];
	
	sprintf(msg,"The directory %s was either moved, renamed, or is not supported.",path);
	What(msg);
}

bool
CanHandleDir(const entry_ref *ref)
{
	char path[B_PATH_NAME_LENGTH];
	
	entry_ref traversedRef(*ref);
	TraverseSymLink(&traversedRef);
	
	path_from_entry_ref(&traversedRef,path);

	BEntry entry(&traversedRef);
	if (entry.IsDirectory()) {
		if (CheckForSpecialDirs(path))
			return true;
	}
	
	return false;
}

void
ShowFileError(char *path)
{
	char msg[B_PATH_NAME_LENGTH + 256];
	char type[256];

	GetFileType(path,type);
	if (type == NULL)
		sprintf(msg,"The file: %s, is not supported.",path);
	else {
		char infostr[256];
		
		GetFileTypeDescription(type,infostr);
		
		if (infostr == NULL || strlen(infostr) < 1)
			sprintf(msg,"The file: %s, is not supported.",path);
		else
			sprintf(msg,"The file: %s, is not supported (%s).",path,infostr);
	}
					
	What(msg);
}

bool
CanHandleFile(const entry_ref *ref)
{
	char path[B_PATH_NAME_LENGTH];
	char type[256];

	entry_ref traversedRef(*ref);
	TraverseSymLink(&traversedRef);
	
	path_from_entry_ref(&traversedRef,path);

	if (!CheckType(path,type)) {
		if (!CheckCanExpand(path))
			return false;
	}
	
	return true;
}

bool
HandleTypeForRef(entry_ref *ref)
{
	TraverseSymLink(ref);
	//
	//	see if its a supported directory
	//	else check to see if its a supported file
	//
	if (CanHandleDir(ref))
		return true;
	else
		return CanHandleFile(ref);
}

bool
HandleTypeForPath(char *path)
{
	entry_ref ref;

	get_ref_for_path(path,&ref);
	
	return HandleTypeForRef(&ref);	
}



//--------------------------------------------------------------------

#define DOUBLE_QUOTE  '"'
#define SINGLE_QUOTE  '\''
#define BACK_SLASH    '\\'

char **
build_argv(char *str, int *argc)
{
	int table_size = 16, _argc;
	char **argv;

	if (argc == NULL)
		argc = &_argc;

	*argc = 0;
	argv  = (char **)calloc(table_size, sizeof(char *));

	if (argv == NULL)
		return NULL;
	
	while(*str) {
		/* skip intervening white space */
		while(*str != '\0' && (*str == ' ' || *str == '\t' || *str == '\n'))
			str++;
		
		if (*str == '\0')
			break;
		
		if (*str == DOUBLE_QUOTE) {
			argv[*argc] = ++str;
			while(*str && *str != DOUBLE_QUOTE) {
				if (*str == BACK_SLASH)
					strcpy(str, str+1);  /* copy everything down */
				str++;
			}
		} else if (*str == SINGLE_QUOTE) {
			argv[*argc] = ++str;
			while(*str && *str != SINGLE_QUOTE) {
				if (*str == BACK_SLASH)
					strcpy(str, str+1);  /* copy everything down */
				str++;
			}
		} else {
			argv[*argc] = str;
			while(*str && *str != ' ' && *str != '\t' && *str != '\n') {
				if (*str == BACK_SLASH)
					strcpy(str, str+1);  /* copy everything down */
				str++;
			}
		}
		
		if (*str != '\0')
			*str++ = '\0';   /* chop the string */
		
		*argc = *argc + 1;
		if (*argc >= table_size-1) {
			char **nargv;
			
			table_size = table_size * 2;
			nargv = (char **)calloc(table_size, sizeof(char *));
			
			if (nargv == NULL) {   /* drats! failure. */
				free(argv);
				return NULL;
			}
			
			memcpy(nargv, argv, (*argc) * sizeof(char *));
			free(argv);
			argv = nargv;
		}
	}
	
	return argv;
}



static expand_action *ea_table = NULL;
static int            action_count = 0;



void
real_free_action_table(expand_action *ea, int action_count)
{
	int i;

	if (ea == NULL)
		return;

	for(i=0; i < action_count; i++) {
		free(ea[i].extension);
		free(ea[i].list_cmd);
		free(ea[i].extract_cmd);
	}

	free(ea);
}

void
free_action_table(void)
{
	real_free_action_table(ea_table, action_count);

	ea_table     = NULL;
	action_count = 0;
}


void
load_action_table(const char *filename)
{
	int   argc, i, line_count = 0;
	FILE *fp;
	char  buff[512], **args;
	int cur_action = 0, num_actions = 10;
	expand_action *ea = NULL;

	action_count = 0;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("can't open extension action file %s\n", filename);
		return;
	}

	ea = (expand_action *)calloc(num_actions, sizeof(expand_action));
	if (ea == NULL) {
		fclose(fp);
		return;
	}


	while(fgets(buff, sizeof(buff), fp) != NULL) {
		line_count++;
		
		if (buff[0] == '#' || buff[0] == '\n' || buff[0] == '\r')
			continue;

		for(i=0; isspace(buff[i]) && buff[i]; i++)
			;

		if (buff[i] == '\0')
			continue;

		argc = 0;
		args = build_argv(buff, &argc);
		if (args == NULL)
			continue;

		if (argc < 4) {
			printf("line %d: bad action (need 4 fields, got %d): %s\n",
				   line_count, argc, buff);
			continue;
		}

		if (cur_action >= num_actions) {
			expand_action *ea2;
			
			num_actions += 10;
			ea2 = (expand_action*)realloc(ea,num_actions*sizeof(expand_action));
			if (ea2 == NULL) {
				real_free_action_table(ea, cur_action);
				return;
			}
			
			ea = ea2;
		}

		ea[cur_action].mime_type   = strdup(args[0]);
		ea[cur_action].extension   = strdup(args[1]);
		ea[cur_action].list_cmd    = strdup(args[2]);
		ea[cur_action].extract_cmd = strdup(args[3]);
		
		cur_action++;
		
		free(args);
	}

	ea_table     = ea;
	action_count = cur_action;
	
	fclose(fp);
	return;
}


expand_action defaults[] = {
	/* put longer extension first so they match before shorter sub-strings */
	{ 0,					".tar.gz",        "zcat %s | tar -tvf -",        "zcat %s | tar -xvf -" },
	{ 0,					".tar.Z",         "zcat %s | tar -tvf -",        "zcat %s | tar -xvf -" },
	{ 0,					".tgz",           "zcat %s | tar -tvf -",        "zcat %s | tar -xvf -" },
	{ "application/x-tar",	".tar",           "tar -tvf %s",                 "tar -xf %s" },
	{ "application/x-gzip",	".gz",            "echo %s | sed 's/.gz$//g'",   "gunzip %s" },
	{ "application/x-gunzip",".gz",            "echo %s | sed 's/.gz$//g'",   "gunzip %s" },
	{ "application/zip",	".zip",           "unzip -l %s",                 "unzip -o %s" },
	{ "application/x-zip-compressed",	".zip",           "unzip -l %s",                 "unzip -o %s" }
};


static expand_action *
real_find_action(char *filename, expand_action *ea_table, int action_count,
	bool checkForCompositeArchives)
{
	expand_action *table = ea_table;
	if (!checkForCompositeArchives) {
		char buffer[256];
		if (GetFileType(filename, buffer)) {
			ASSERT(buffer[0] <= 255);
			ASSERT(strlen(buffer));
			for(int i=0; i < action_count; i++, table++)
				if (table->mime_type && table->mime_type[0]
					&& strcmp(buffer, table->mime_type) == 0)
					return table;
		}
	}
	
	ASSERT(filename);
	uint32 pathlen = strlen(filename);
	
	table = ea_table;
	for(int i=0; i < action_count; i++, table++) {
		char *ptr;
		
		// we are only looking for composite extraction rules which have a
		// null for mime_type
		if (checkForCompositeArchives && table->mime_type && table->mime_type[0])
			continue;
	
		if (pathlen < strlen(table->extension))
			continue;

		ptr = filename + pathlen - strlen(table->extension);
		if (strcmp(ptr, table->extension) == 0) {
			return table;
		}
	}

	return NULL;
}


expand_action *
find_action(char *filename)
{
	expand_action *expander = NULL;

	// first try to find special rules for composite extractors
	if (ea_table) 
		expander = real_find_action(filename, ea_table, action_count, true);
	if (expander)
		return expander;

	expander = real_find_action(filename, defaults,
			sizeof(defaults) / sizeof(expand_action), true);
	if (expander)
		return expander;


	if (ea_table)
		expander = real_find_action(filename, ea_table, action_count, false);
	if (expander)
		return expander;

	return real_find_action(filename, defaults,
		sizeof(defaults) / sizeof(expand_action), false);
}



//--------------------------------------------------------------------


TXpandoMatic::TXpandoMatic()
{
	fFilePath = NULL;
	fFileType = NULL;
	fDirPath = NULL;
	fW = NULL;
#if _BUILD31_
	fProcessedKilled = false;
	fThread_ID = -1;
	SetProcess(kNoProcess, kNoAction);
#endif
}

TXpandoMatic::TXpandoMatic(	char *filepath,char *type,
							char *dirpath,
							TXpandoWindow *w)
{
	fFilePath = filepath;
	fFileType = type;
	fDirPath = dirpath;
	fW = w;
#if _BUILD31_
	fProcessedKilled = false;
	fThread_ID = -1;
	SetProcess(kNoProcess, kNoAction);
#endif
}

TXpandoMatic::~TXpandoMatic()
{
}

void TXpandoMatic::SetSrc(char *filepath)
{
	if (fFilePath)
		free(fFilePath);

	fFilePath = strdup(filepath);
}

void TXpandoMatic::SetDest(char *destpath)
{
	if (fDirPath)
		free(fDirPath);
	
	fDirPath = strdup(destpath);
}

void TXpandoMatic::SetFileType(char *filetype)
{
	if (fFileType)
		free(fFileType);
	
	fFileType = strdup(filetype);
}

void TXpandoMatic::SetParent(TXpandoWindow *w)
{
	fW = w;
}

bool TXpandoMatic::DoSanityCheck(void)
{
	return ( 	(fFilePath != NULL) &&
				(fDirPath != NULL) &&
				(fFileType != NULL) &&
				(fW != NULL));
}
	
static long
XPandoMaticExpander(TXpandoMatic *x)
{
  return x->ExpandFile();
}

long
TXpandoMatic::Expand()
{
	if (DoSanityCheck()) {
	  	char threadName[32];
	
	  	sprintf(threadName,"expand-o-matic");

#if _BUILD31_
		fProcessedKilled = false;
		SetProcess(kProcessing, kExpandingAction);
#endif

	  	long tid = spawn_thread((thread_entry)XPandoMaticExpander,threadName,
			B_NORMAL_PRIORITY,this);
		resume_thread(tid);
		
		return tid;
	} else {
		What("Xpand-O-Matic is not initialized");
		return -1;
	}
}

#if _BUILD31_
static int
mysystem(const char *cmd, long *thread_id)
{
	const char		*argv[4];
	char		*shell = "/bin/sh";

	*thread_id = -1;
	
	if (!cmd || *cmd == '\0')
		return 0;

	argv[0] = shell;
	argv[1] = "-c";
	argv[2] = (char *)cmd;
	argv[3] = NULL;

    if ((*thread_id = fork()) == 0) {			// child
		setpgid(0, 0);
		execve(argv[0], (char *const *) argv, (char *const *) environ);
		return 0;
	} else {									// parent
		while (wait_for_thread (*thread_id, thread_id) == B_INTERRUPTED)
			;
		return *thread_id;
	}
}
#endif

long TXpandoMatic::ExpandFile()
{
	expand_action *expander = NULL;
	status_t	   result=0;
	char 		   msg[512], tmp[512];	
	char		  *escDirPath, *clean_path;
	
	clean_path = escape_meta_chars(fFilePath);	
	escDirPath = escape_meta_chars(fDirPath);

	expander = find_action(fFilePath);
	if (expander == NULL) {
		free(clean_path);
		free(escDirPath);
		
		sprintf(msg, "Archive File %s is not understood.", fFilePath);
		((TXpandoWindow*)fW)->AddToList(msg);

		return -1;
	}
	
	strcpy(tmp, "cd %s; ");
	strcat(tmp, expander->extract_cmd);

	/* we pass extra copies of clean_path in case there are extra %s's
	   in the user's command */
	sprintf(msg, tmp, escDirPath, clean_path, clean_path, clean_path,
			clean_path, clean_path);

#if _BUILD31_
	result = mysystem(msg, &fThread_ID);
#else
	result = system(msg);
#endif

#if _BUILD31_
	if (Processing()) {	// expansion performed normally
#endif
		free(escDirPath);
		free(clean_path);
	
#if _BUILD31_
		//	set the current state before sending the message
		//	so that the controls will be reset correctly
		fThread_ID = -1;
		SetProcess(kNoProcess, kNoAction);
#endif
		if (!fProcessedKilled) {
			BMessage message(msg_expand_complete);
			message.AddInt32("result", result);
		
			fW->PostMessage(&message);
		}
#if _BUILD31_
	}
#endif
	
	return result;
}

static long XPandoMaticLister(TXpandoMatic *x)
{
  return x->ListFile();
}

long TXpandoMatic::List()
{
	if (DoSanityCheck()) {
	  	char threadName[32];
	
	  	sprintf(threadName,"list-o-matic");

#if _BUILD31_
		fProcessedKilled = false;
		SetProcess(kProcessing, kListingAction);
#endif

	  	long tid = spawn_thread((thread_entry)XPandoMaticLister,threadName,B_NORMAL_PRIORITY,this);
		resume_thread(tid);
	  	return tid;
	} else {
		What("Xpand-O-Matic is not initialized");
		return -1;
	}
}

#if _BUILD31_
static FILE *
my_popen(const char *cmd, const char *type, long* thread_id)
{
	int p[2];
	FILE *fp;
	char *args[4];
	
	if (*type != 'r' && *type != 'w')
		return NULL;

	if (pipe(p) < 0)
		return NULL;
     
	if ((*thread_id = fork()) > 0) {    /* then we are the parent */
		if (*type == 'r') {
			close(p[1]);
			fp = fdopen(p[0], type);
		} else {
			close(p[0]);
			fp = fdopen(p[1], type);
		}
        
		return fp;
	} else if (*thread_id == 0) {       /* we're the child */
		if (*type == 'r') {
			fflush(stdout);
			close(1);

			if (dup(p[1]) < 0)
				perror("dup of write side of pipe failed");
		} else {
			close(0);

			if (dup(p[0]) < 0)
				perror("dup of read side of pipe failed");
        }
		
		close(p[0]);            /* close since we dup()'ed what we needed */
		close(p[1]);

		args[0] = "/bin/sh";
		args[1] = "-c";
		args[2] = (char *)cmd;
		args[3] = NULL;

		setpgid(0, 0);
		execve(args[0], (char* const*) args, (char* const*) environ);
	} else {                    /* we're having major problems.. */
		close(p[0]);
		close(p[1]);
	}

	return NULL;
}
#endif

long TXpandoMatic::ListFile()
{
	status_t	result=0;
	char buffer[1024];	
	char msg[512];
	char *clean_path;
	FILE *fp;
	expand_action *expander = NULL;

	clean_path = escape_meta_chars(fFilePath);

	expander = find_action(fFilePath);
	if (expander == NULL) {
		free(clean_path);

		sprintf(msg, "Could not list %s.", fFilePath);
		((TXpandoWindow*)fW)->AddToList(msg);
		
		fW->PostMessage(msg_list_complete);
		return -1;
	}

	/* we pass extra copies of clean_path in case there are extra %s's
	   in the user's command */
	sprintf(msg, expander->list_cmd, clean_path, clean_path, clean_path,
			clean_path, clean_path, clean_path);

	free(clean_path);

#if _BUILD31_
	fp = my_popen(msg, "r", &fThread_ID);
#else
	fp = popen(msg, "r");
#endif

#if _BUILD31_
	if (Processing()) {	
#endif
		if (fp == NULL) {
			sprintf(msg, "Could not list %s.", fFilePath);
			((TXpandoWindow*)fW)->AddToList(msg);
			
			fW->PostMessage(msg_list_complete);
			return -2;
		}
	
		while (fgets(buffer, sizeof(buffer), fp) != NULL) {
			((TXpandoWindow*)fW)->AddToList(buffer);
		}
	
		pclose(fp);
		
#if _BUILD31_
		//	set the current state before sending the message
		//	so that the controls will be reset correctly
		fThread_ID = -1;
		SetProcess(kNoProcess, kNoAction);
#endif
		if (!fProcessedKilled)
			fW->PostMessage(msg_list_complete);
		
#if _BUILD31_
	}
#endif
	return result;
}

#if _BUILD31_
void
TXpandoMatic::SetProcess(process_state state, process_action action)
{
	fProcessState = state;
	fProcessAction = action;
}

process_state
TXpandoMatic::ProcessState() const
{
	return fProcessState;
}

process_action
TXpandoMatic::ProcessAction() const
{
	return fProcessAction;
}

bool
TXpandoMatic::Processing() const
{
	return ( ProcessState() != kNoProcess && ProcessAction() != kNoAction);
}

bool
TXpandoMatic::ProcessingPaused() const
{
	return ( ProcessAction() != kNoAction && ProcessState() == kPausedProcess);
}

static void
DumpThreadInfo(thread_info t)
{
	printf("**  Thread Info  **\n");
	printf("thread id - %li\n", t.thread);
	printf("team id - %li\n", t.team);
	printf("name - %s\n", t.name);
	printf("state - %i\n", t.state);
	printf("priority - %li\n", t.priority);
	printf("sem id - %li\n", t.sem);
}

static void
DumpTeamInfo( team_info t)
{
	printf("** Team Info **\n");
	printf("team id - %li\n",t.team);
	printf("thread count - %li\n", t.thread_count);
	printf("uid - %i, gid - %i\n", t.uid, t.gid);
}

void
TXpandoMatic::PauseProcessing()
{
	if (Processing()) {
//printf("pausing process\n");
		send_signal( -fThread_ID, SIGSTOP);
		fProcessState = kPausedProcess;		
	}
}

void
TXpandoMatic::ResumeProcessing()
{
	if (ProcessingPaused()) {
//printf("resuming process\n");
		send_signal( -fThread_ID, SIGCONT);
		fProcessState = kProcessing;
	}
}

void
TXpandoMatic::StopProcessing()
{
	if (Processing()) {
//printf("stopping process\n");
		send_signal( -fThread_ID, SIGINT);		
		fThread_ID = -1;
		SetProcess(kNoProcess, kNoAction);
		fProcessedKilled = true;
	}
}
#endif

