/* emacs: -*- C++ -*- */
/*
 * Copyright (c) 1996 Be, Inc.	All Rights Reserved
 */
#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

#include <string.h>
#include <stdlib.h>
#include <support/Locker.h>

#define BACKLOG_DEFAULT 32

class GetHits {
public:
	virtual void Hit(int hitcount) = 0;
};
	
class WebServer {
	int s;
	int hits;
	GetHits *callback;
	char *dirname;
	BLocker lock;
public:
	WebServer(void);
	~WebServer(void);
	int Start(int backlog = BACKLOG_DEFAULT);
	int IsRunning(void) { return (s >= 0); };
	void Stop(void);
	void SetCallback(GetHits *callback);
	void ClearHits(void);
	int GetSocket(void) { return (s); }
	void Hit(void);
	void SetIndexFileName(const char *file) {
		lock.Lock();
		if (this->index_file_name) {
			free(this->index_file_name);
		}
		this->index_file_name = strdup(file);
		lock.Unlock();
	}
	const char *GetIndexFileName(void) {
		return index_file_name;
	}
	const char *GetDirName(void) { return (dirname); }
	void SetDirName(const char *dirname) {
		lock.Lock();
		if (this->dirname) {
			free(this->dirname);
		}
		if (dirname) {
			this->dirname = strdup(dirname);
		}
		else {
			this->dirname = NULL;
		}
		lock.Unlock();
	}
	const bool GetDirListFlag(void) {
		return dir_list_flag;
	}
	void SetDirListFlag(bool f) {
		lock.Lock();
		dir_list_flag = f;
		lock.Unlock();
	}
	void Lock(void) { lock.Lock(); }
	void Unlock(void) { lock.Unlock(); }
	
private:
	char	*index_file_name;
	bool	dir_list_flag;
};

#endif
