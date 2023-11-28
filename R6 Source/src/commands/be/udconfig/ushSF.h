/*
 * Copyright 2000 Be, Incorporated.  All Rights Reserved.
 */

#ifndef _USHSETTINGS_H_
#define _USHSETTINGS_H_

#include "Settings.h"

struct iovec;

class ushSF : public SettingsFile {
public:
	
	ushSF(const char *name);
	~ushSF();

	virtual status_t Load(int32 flags = S_NONE);
	virtual status_t Save(int32 flags = S_NONE);


private:
	
	void Reset();


private:	// Load() helpers

	status_t read_more();
	status_t alloc_more();
	status_t find_setenv();
	status_t in_advance(size_t, int);
	status_t save_write(const char*, size_t);
	status_t save_write(char);
	status_t get_next_token();
	void tie_off_vec();


private:	// Save() helpers

	const char *quote(const char*);


private:

	iovec *fContents;
	size_t fVecCount;
	size_t fVecAlloc;

	BList	fShameList;


private:	// these are only used during the call to Load()

	int _fd;
	char *_readbuf;

	char *_in;
	int _in_left;

	char *_save;
	int _save_left;

	char *_token;
	int _token_alloc;
};


#endif	/* _USHSETTINGS_H_ */
