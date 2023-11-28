//
// Web server log
//
// Copyright (c) 1997, Be Incorporated.  All rights reserved.
//

#ifndef _LOG_H_
#define _LOG_H_
#include <OS.h>
#include "pmwin.h"
#include "web_server.h"

class PMWindow;
class WebServer;

class PMLog : BHandler {
	public:
		PMLog(WebServer *server, PMWindow *window);
		~PMLog();
		void AddToLog(const char *msg);
		bool IsLoggingToFile(void);
		void LogToFile(bool logflag);
		bool IsLoggingToConsole(void);
		void LogToConsole(bool logflag);
		void SetLogPath(const char *path);
		char *LogPath(void);
		void SaveSettings(void);
		void ClearLog(void);
		void ClearConsole(void);
		void ClearFile(void);
		void DiskUpdate(void);
		status_t Lock(void);
		status_t Unlock(void);

	private:
		sem_id	add_lock;
		int32	add_ben_val;
		bool	loggingFile;
		bool	loggingConsole;
		WebServer *server;
		PMWindow *window;
		char	*logPath;
		FILE	*logf;
};

FILE *open_text_file(const char *path, char *flags);

#endif
