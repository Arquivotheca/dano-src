//
// Web server log
//
// Copyright (c) 1997, Be Incorporated.  All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <Button.h>
#include <Alert.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Path.h>
#include <ScrollBar.h>
#include <TextView.h>
#include <FilePanel.h>
#include <File.h>
#include <StorageDefs.h>
#include <NodeInfo.h>
#include <InterfaceDefs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <Entry.h>
#include <errno.h>
#include <time.h>
#include "pmapp.h"
#include "pmwin.h"
#include "hardcoded.h"
#include "pmlog.h"

static char *get_date (const char *format);

PMLog::PMLog(WebServer *server, PMWindow *window) {
	const char	*path;

	// Create the add benaphore

	add_lock = create_sem(1, "www log locker");
	add_ben_val = 0;

	this->server = server;
	this->window = window;

	// No log file yet
	
	logPath = NULL;
	logf = NULL;

	if (window->settings->InitCheck() != B_OK) {
		// It appears that there is no settings file - don't bother
		return;
	}

	path = window->settings->FindString("log_path");
	if (path) {
		SetLogPath(path);
	}

	if (window->settings->FindBool("log_console_flag", &loggingConsole) != B_OK) {
		loggingConsole = true;	// Default is true
	}
	loggingFile = window->settings->FindBool("log_file_flag");
								// Default is false
}

PMLog::~PMLog() {
	if (logf) {
		fclose(logf);
	}
	SaveSettings();
	delete_sem(add_lock);
}

void PMLog::SaveSettings(void) {
	if (logPath) {
		window->settings->SetString("log_path", logPath);
	}
	else {
		window->settings->RemoveName("log_path");
	}
	
	window->settings->SetBool("log_console_flag", loggingConsole);
	window->settings->SetBool("log_file_flag", loggingFile);
}

// Call this periodically to flush to disk
void PMLog::DiskUpdate(void) {
	if (logf) {
		fflush(logf);
	}
}


status_t PMLog::Lock(void) {
	int32 previous;

	previous = atomic_add(&add_ben_val, 1);
	if (previous >= 1) {
		return acquire_sem(add_lock);
	}
	return B_OK;
}

status_t PMLog::Unlock(void) {
	int32 previous;

	previous = atomic_add(&add_ben_val, -1);
	if (previous > 1) {
		release_sem(add_lock);
	}
	return B_OK;
}

// Pass NULL to log date/time
void PMLog::AddToLog(const char *msg) {
	char *t;

	t = NULL;
	t = get_date(NULL);

	// Log to console

	if (loggingConsole) {
		window->Lock();
		window->log->Select(2000000000,2000000000);
		window->log->ScrollToSelection();
		if (msg) {
			window->log->Insert(msg);
		}
		else {
			window->log->Insert("[");
			window->log->Insert(t);
			window->log->Insert("]: ");
		}
		window->Unlock();
	}
	
	// Add to file
	
	if (loggingFile && logf) {
		if (msg) {
			fprintf(logf, msg);
		}
		else {
			fprintf(logf, "[%s]: ", t);
		}
	}
	DiskUpdate();
}

void PMLog::ClearConsole(void) {
	window->Lock();
	window->log->Delete(0, 2000000000);
	window->Unlock();
}

void PMLog::ClearFile(void) {
	if (logf) {
		fclose(logf);
	}
	if (logPath) {
		remove(logPath);
		logf = open_text_file(logPath, "a");
	}
}

void PMLog::ClearLog(void) {
	ClearConsole();
	ClearFile();
}

bool PMLog::IsLoggingToConsole(void) {
	return loggingConsole;
}

void PMLog::LogToConsole(bool flag) {
	loggingConsole = flag;
}

bool PMLog::IsLoggingToFile(void) {
	return loggingFile;
}

void PMLog::LogToFile(bool flag) {
	loggingFile = flag;
}


// Pass NULL to stop logging to file.
// This copies the string; you should
// delete the original when you don't
// need it anymore.
void PMLog::SetLogPath(const char *path) {

	// Close the current logfile if there is one

	if (logf) {
		fclose(logf);
		logf = NULL;
	}
	if (logPath) {
		free(logPath);
		logPath = NULL;
	}
	
	// If a new one was specified, open it up

	if (path) {
		logf = open_text_file(path, "a");
		if (logf) {
			logPath = strdup(path);
		}
	}
}

char *PMLog::LogPath(void) {
	return logPath;
}

static char *get_date (const char *format) {
	time_t caltime;
	struct tm *mytym;
	char *s;
	
	caltime = time(NULL);
	mytym = localtime(&caltime);
	s = asctime(mytym);
	s[strlen(s)-1] = '\0';
	return s;
}

FILE *open_text_file(const char *path, char *flags) {
	if (path) {
		BFile *file;
		file = new BFile(path, B_WRITE_ONLY|B_CREATE_FILE);
		if (file->InitCheck() == B_OK) {
			BNodeInfo nodeInfo(file);
			if (nodeInfo.InitCheck() == B_OK) {
				nodeInfo.SetType("text/plain");
			}
			delete file;
			return fopen(path, flags);
		}
	}
	return NULL;
}
