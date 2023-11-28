/*
 * Copyright 2000 Be, Incorporated.  All Rights Reserved.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/uio.h>

#include "ushSF.h"

#define BUFLEN	(100)

#define MARK_NAME	'\001'
#define MARK_VALUE	'\002'
#define IS_MARK(c)	(c == MARK_NAME || c == MARK_VALUE)

ushSF::ushSF(const char *name) :
	SettingsFile(name),
	fContents(NULL),
	fVecCount(0),
	fVecAlloc(0),
	_token(NULL),
	_token_alloc(0)
{
}


ushSF::~ushSF()
{
	Reset();

	free(_token);
	free(fContents);
}


void
ushSF::Reset()
{
	size_t i;

	ClearSettingsList();

	if(fContents) {
		for(i = 0; i < fVecCount; i++) {
			free(fContents[i].iov_base);
		}
		fVecCount = 0;
	}
}


status_t
ushSF::read_more()
{
	int nbytes;

	memmove(_readbuf, _in, _in_left);
	_in = _readbuf;

	nbytes = read(_fd, _readbuf + _in_left, BUFLEN - _in_left);
	if(nbytes == 0) {
		return -1;
	}
	if(nbytes < 0) {
		return errno;
	}
	_in_left += nbytes;

	return B_OK;
}

void
ushSF::tie_off_vec()
{
	if(fVecCount > 0) {
		iovec *v = &fContents[fVecCount-1];
		v->iov_len = BUFLEN - _save_left;
		_save_left = 0;
		_save = NULL;
	}
}

status_t
ushSF::alloc_more()
{
	iovec *v;
	// tie off the current iovec, add a new one
	// and reset the pointers etc.
	//

	tie_off_vec();

	if(fVecCount == fVecAlloc) {
		int nalloc = fVecAlloc ? fVecAlloc * 2 : 2;
		v = (iovec*)realloc(fContents, nalloc * sizeof(iovec));
		if(v == NULL) {
			return B_NO_MEMORY;
		}
		fContents = v;
		fVecAlloc = nalloc;
	}

	v = &fContents[fVecCount];
	v->iov_base = malloc(BUFLEN);
	if(v->iov_base == NULL) {
		return B_NO_MEMORY;
	}
	v->iov_len = BUFLEN;
	fVecCount++;

	_save = (char*)v->iov_base;
	_save_left = BUFLEN;

	return B_OK;
}

status_t
ushSF::in_advance(size_t c, int save)
{
	status_t err;
	size_t n;
	
	while(c > 0) {
		if((size_t)_in_left < c) {
			err = read_more();
			if(err != B_OK) {
				return err;
			}
		}
		
		// _in_left is always positive at this point
		//
		n = MIN(c, (size_t)_in_left);

		if(save) {
			err = save_write(_in, n);
			if(err != B_OK) {
				return err;
			}
		}

		_in += n;
		_in_left -= n;

		c -= n;
	}

	return B_OK;
}

status_t
ushSF::save_write(const char *s, size_t c)
{
	status_t err;
	size_t n;

	while(c > 0) {
		if((size_t)_save_left < c) {
			err = alloc_more();
			if(err != B_OK) {
				return err;
			}
		}

		// _save_left is always positive at this point
		//
		n = MIN(c, (size_t)_save_left);

		memcpy(_save, s, n);
		_save += n;
		_save_left -= n;

		c -= n;
	}

	return B_OK;
}

status_t
ushSF::save_write(char c)
{
	return save_write(&c, 1);
}

#define ADVANCE_WHILE(condition)								\
	err = B_OK;													\
	while(err == B_OK) {										\
		while(_in_left > 0 && _save_left > 0 && (condition)) {	\
			*_save++ = *_in++;									\
			--_in_left;											\
			--_save_left;										\
		}														\
		if(_in_left == 0) {										\
			err = read_more();									\
			continue;											\
		}														\
		if(_save_left == 0) {									\
			err = alloc_more();									\
			continue;											\
		}														\
		break;													\
	}

#define EAT_WS()		ADVANCE_WHILE(isspace(*_in))
#define EAT_BLANKS()	ADVANCE_WHILE(*_in == ' ' || *_in == '\t')
#define EAT_LINE()		ADVANCE_WHILE(*_in != '\n' && *_in != '\r'); EAT_WS()

ssize_t
ushSF::get_next_token()
{
	char *end;
	int end_left;
	int tok_len;
	int n_tok_len;
	status_t err = B_OK;
	enum {
		UNQUOTED,
		QUOTED,
		ESCAPED
	} state, last;

	if(_token) {
		memset(_token, 0, _token_alloc);
	}

	EAT_BLANKS();
	if(err != B_OK) {
		return err;
	}
	if(*_in == '#') {
		// no more tokens on this line
		//
		return 0;
	}

	// find the length of the token
	// that begins at _in.
	//
	end = _in;
	end_left = _in_left;
	tok_len = 0;

	n_tok_len = 0;

	state = UNQUOTED;
	while(tok_len < _in_left) {
		char c;

		c = 0;
		switch(state) {
		case UNQUOTED:
			if(isspace(*end)) {
				return tok_len;
			}
			else if(*end == '"') {
				state = QUOTED;
			}
			else if(*end == '\\') {
				state = ESCAPED;
				last = UNQUOTED;
			}
			else {
				c = *end;
			}

			break;

		case QUOTED:
			if(*end == '\r' || *end == '\n') {
				// newline in string constant.
				// ush won't recognize this as a token
				//
				return 0;
			}
			else if(*end == '\\') {
				state = ESCAPED;
				last = QUOTED;
			}
			else if(*end == '"') {
				state = UNQUOTED;
			}
			else {
				c = *end;
			}

			break;
		
		case ESCAPED:
			if(*end == '\r' || *end == '\n') {
				// escaped newline
				// ush won't recognize this as a token
				//
				return 0;
			}
			switch(*end) {
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            default: c = *end;	// incl. "
			}
			state = last;
			break;
		}

		if(c != 0) {
			if(n_tok_len >= _token_alloc-1) {
				char *ntok;
				int nalloc = _token_alloc ? _token_alloc * 2 : 512;
				
				ntok = (char*)realloc(_token, nalloc);
				if(ntok == NULL) {
					break;
				}
				_token = ntok;
				_token_alloc = nalloc;

				memset(_token + n_tok_len, 0, _token_alloc - n_tok_len);
			}

			_token[n_tok_len++] = c;
		}

		end++;
		tok_len++;

		if(tok_len == _in_left && _in != _readbuf) {
			// push the token to the front of the buffer
			//
			err = read_more();
			if(err != B_OK) {
				// proly EOF
				break;
			}
			end = _in + tok_len;
		}
	}

	return tok_len;
}

//
// push the input pointer just after the next occurence of
// "setenv" as the first token on a line
//
status_t
ushSF::find_setenv()
{
	ssize_t tok_len;
	status_t err;

	err = B_OK;
	while(err == B_OK) {
		tok_len = get_next_token();
		if(tok_len < 0) {
			return (status_t)tok_len;
		}

		if(tok_len == sizeof("setenv")-1 && strncmp(_token, "setenv", sizeof("setenv")-1) == 0) {
			return in_advance(sizeof("setenv")-1, true);
		}

		// else "setenv" is not the first token on the line
		//
		EAT_LINE();
	}

	return err;
}

status_t 
ushSF::Load(int32 flags)
{
	Setting s;
	int32 next_id = 0;
	status_t err;
	BList *list;

	Reset();

	list = AcquireSettingsList();
	if(list == NULL) {
		return InitCheck();
	}

	_fd = open(GetName(), O_RDONLY);
	if(_fd < 0) {
		ReleaseSettingsList();
		return (status_t)_fd;
	}

	_readbuf = (char*)malloc(BUFLEN);
	if(_readbuf == NULL) {
		close(_fd);
		ReleaseSettingsList();
		return B_NO_MEMORY;
	}
	_in = _readbuf;
	_in_left = 0;

	_save = NULL;
	_save_left = 0;

	memset(&s, 0, sizeof(s));

	//
	// find lines with 'setenv' as the first token
	//

	while(find_setenv() == B_OK) {
		ssize_t tok_len;
		char markbuf[1 + sizeof(int32)];

		s.cookie = next_id;

		tok_len = get_next_token();
		if(tok_len < 0) {
			break;
		}
		else if(tok_len > 0) {
			// make sure the next 5 bytes are in
			// the same buffer
			//
			markbuf[0] = MARK_NAME;
			*(int32*)(&markbuf[1]) = s.cookie;
			err = save_write(markbuf, sizeof(markbuf));
			if(err != B_OK) {
				break;
			}

			s.name = strdup(_token);
			if(s.name == NULL) {
				err = B_NO_MEMORY;
				break;
			}
		}
		else {
			goto advance;
		}

		in_advance(tok_len, false);

		tok_len = get_next_token();
		if(tok_len < 0) {
			break;
		}
		else if(tok_len > 0) {
			// make sure the next 5 bytes are in
			// the same buffer
			//
			markbuf[0] = MARK_VALUE;
			*(int32*)(&markbuf[1]) = s.cookie;
			err = save_write(markbuf, sizeof(markbuf));
			if(err != B_OK) {
				break;
			}

			s.value = strdup(_token);
			if(s.value == NULL) {
				err = B_NO_MEMORY;
				break;
			}
		}
		else {
			goto advance;
		}

		in_advance(tok_len, false);

	advance:
		if(s.name != NULL) {
			// something was successful
			//
			Setting *sp = (Setting*)malloc(sizeof(Setting));
			if(sp == NULL) {
				err = B_NO_MEMORY;
				break;
			}
			memcpy(sp, &s, sizeof(Setting));

			if(sp->value == NULL) {
				// not enough params, but it's too late
				// to fix it in fContents;  save this Setting
				// for insertion on Save.
				//
				if(!fShameList.AddItem((void*)sp)) {
					err = B_ERROR;
					free(sp);
					break;
				}
			}
			else {
				// a well-formed setenv call
				//
				if(!list->AddItem((void*)sp)) {
					err = B_ERROR;
					free(sp);
					break;
				}
			}
			memset(&s, 0, sizeof(s));
			next_id++;
		}

		EAT_LINE();
		if(err != B_OK) {
			break;
		}
	}

	// if the Setting wasn't added to a list, these could be non-zero.
	//
	free(s.name);
	free(s.value);

	tie_off_vec();

	close(_fd);
	_fd = -1;

	free(_readbuf);
	_readbuf = NULL;

	ReleaseSettingsList();

	return B_OK;
}

int
count_chars(const char *s, int c)
{
	int count = 0;

	while((s = strchr(s, c)) != NULL) {
		count++;
	}

	return count;
}

const char *
ushSF::quote(const char *s)
{
	char *ss;
	int slen;
	int quote = false;

	// the most this string could expand is 2x + 2
	// escape every character, plus double quotes.
	//
	slen = strlen(s) * 2 + 2 + 1;

	if(_token_alloc < slen) {
		ss = (char*)realloc(_token, slen);
		if(ss == NULL) {
			return NULL;
		}
		_token = ss;
		_token_alloc = slen;
	}

	ss = _token;

	// if there are any spaces,
	// put the whole thing in
	// double-quotes.
	//
	if(strchr(s, ' ')) {
		quote = true;
		*ss++ = '"';
	}

	// if the first character is '#'
	// and we're not quoting, escape it.
	//
	if(!quote && *s == '#') {
		*ss++ = '\\';
	}

	// copy the data, escaping any
	// '"', '^M', '^J', '\', '$'
	//
	while(*s) {
		switch(*s) {
		case '"':
		case '\\':
		case '$':
			*ss++ = '\\';
		default:
			*ss++ = *s;
			break;

		case '\n':
			*ss++ = '\\';
			*ss++ = 'n';
			break;

		case '\r':
			*ss++ = '\\';
			*ss++ = 'r';
			break;
		}
		s++;
	}

	if(quote) {
		*ss++ = '"';
	}

	*ss = '\0';

	return _token;
}

status_t 
ushSF::Save(int32 flags)
{
	BList *list;
	FILE *fp;
	Setting *setting = NULL;
	status_t err;
	size_t vn;
	int ret;
	int listi;
	int shamei;

	if(fContents == NULL || fVecCount == 0) {
		return B_NO_INIT;
	}

	list = AcquireSettingsList();
	if(list == NULL) {
		return B_ERROR;
	}

//! may want to back up old version of the file..
//  would be better to do it elsewhere, though.
//	in any case, open with creat/truncate

	// make sure we truncate the old file
	// and create it if it's not there anymore
	// ("w" does this)
	//
	fp = fopen(GetName(), "w");
	if(fp == NULL) {
		err = errno;
		goto exit;
	}

	// arve says I should do this for better
	// cfs compression.  Usually results in
	// the whole file being written at once.
	//
	setvbuf(fp, NULL, _IOFBF, 64*1024);

	vn = 0;
	_save_left = 0;
	listi = 0;
	shamei = 0;

	// walk through fContents, writing until
	// we find a MARK byte.  FILE writing
	// is buffered, so go ahead and write
	// a byte at a time.
	//
	while(1) {
		while(_save_left > 0 && !IS_MARK(*_save)) {
			ret = fputc(*_save, fp);
			if(ret == EOF) {
				goto exit;
			}
			_save++;
			--_save_left;
		}
		if(_save_left == 0) {
			if(vn < fVecCount) {
				_save = (char*)fContents[vn].iov_base;
				_save_left = fContents[vn].iov_len;
				vn++;
			}
			else {
				break;
			}
		}
		else {
			const char *q;
			int32 id;
			int mark;

			// found a MARK byte.
			// Load() makes sure that the 5 mark
			// bytes are always in a single buffer
			//
			if((size_t)_save_left < 1 + sizeof(int32)) {
				fprintf(stderr, "ushSF::Save: malformed fContents iovec, %d bytes left.\n", _save_left);
				exit_thread(B_ERROR);
			}
			mark = *_save++;
			--_save_left;

			id = *(int32*)_save;
			_save += sizeof(int32);
			_save_left -= sizeof(int32);

			if(setting == NULL || setting->cookie != id) {
				// this id should be the next Setting in
				// the SettingsList or the ShameList.
				//
				setting = (Setting*)list->ItemAt(listi);
				if(!setting || setting->cookie != id) {
					setting = (Setting*)fShameList.ItemAt(shamei);
					if(!setting || setting->cookie != id) {
						fprintf(stderr, "ushSF::Save: MARK id not at head of list\n");
						exit_thread(B_ERROR);
					}
					shamei++;
				}
				else {
					listi++;
				}
			}

			switch(mark) {
			case MARK_NAME:
				if(setting->name == NULL) {
					fprintf(stderr, "ushSF::Save: matching name is NULL\n");
					exit_thread(B_ERROR);
				}
				q = quote(setting->name);
				if(q == NULL) {
					err = B_NO_MEMORY;
					break;
				}
				fputs(q, fp);
				break;

			case MARK_VALUE:
				if(setting->name == NULL) {
					fprintf(stderr, "ushSF::Save: matching value is NULL\n");
					exit_thread(B_ERROR);
				}
				q = quote(setting->value);
				if(q == NULL) {
					err = B_NO_MEMORY;
					break;
				}
				fputs(q, fp);
				break;

			default:
				fprintf(stderr, "ushSF::Save: bad MARK byte 0x%x\n", mark);
				exit_thread(B_ERROR);
			}
		}
	}

	err = B_OK;
exit:
	if(fp) fclose(fp);
	ReleaseSettingsList();
	return err;
}
