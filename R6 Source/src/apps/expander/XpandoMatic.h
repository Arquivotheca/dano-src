#ifndef XPANDO_MATIC
#define XPANDO_MATIC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Entry.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Volume.h>
#include <Window.h>

#include "utils.h"

#include "build_prefs.h"

class TXpandoWindow;

bool GetFileType(char *filepath,char *filetype);
void GetFileTypeDescription(char *longmimetype,char *shortmimetype);
bool CheckCanExpand(char *path);
bool CheckType(char *path,char *type);
void ShowDirError(char *path);
bool CanHandleDir(const entry_ref *ref);
void ShowFileError(char *path);
bool CanHandleFile(const entry_ref *ref);
bool HandleTypeForRef(entry_ref *ref);
bool HandleTypeForPath(char *path);

struct expand_action {
	char *mime_type;
	char *extension;
	char *list_cmd;
	char *extract_cmd;
};

expand_action *find_action(char *filename);
void           load_action_table(const char *filename);
void           free_action_table(void);

enum process_state {
	kNoProcess,
	kProcessing,
	kPausedProcess
};

enum process_action {
	kNoAction,
	kListingAction,
	kExpandingAction
};

class TXpandoMatic {
public:
	TXpandoMatic();
					TXpandoMatic(char *filepath, char *type,
						char *dirpath, TXpandoWindow *w);
					~TXpandoMatic();
	
	void 			SetSrc(char *filepath);
	void 			SetDest(char *destpath);
	void 			SetFileType(char *filetype);
	void 			SetParent(TXpandoWindow *w);
	
	bool 			DoSanityCheck(void);
	
	long 			Expand();
	long 			ExpandFile();

	long 			List();
	long 			ListFile();

#if _BUILD31_
	bool			Processing() const;
	bool			ProcessingPaused() const;
	void			SetProcess(process_state state, process_action action);
	process_state 	ProcessState() const;
	process_action	ProcessAction() const;
	void			PauseProcessing();
	void			ResumeProcessing();
	void 			StopProcessing();
#endif
	
private:
#if _BUILD31_
	bool			fProcessedKilled;
	process_state	fProcessState;
	process_action	fProcessAction;
	thread_id		fThread_ID;
#endif
	
	char 			*fFilePath;
	char 			*fFileType;
	char 			*fDirPath;

	TXpandoWindow 	*fW;
};

#endif
