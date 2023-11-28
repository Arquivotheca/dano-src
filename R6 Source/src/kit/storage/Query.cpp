/* ++++++++++
	FILE:	Query.cpp
	REVS:	$Revision: 1.13 $
	NAME:	$Author: lbj $
	DATE:	$Date: 1997/06/13 22:08:54 $
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

// ToDo:
// GetPredicate should use non-destructive version of commit_stack that
// uses lazy committing, uses a needCommit flag that gets set by
// every push but still has a fully usable stack after the
// first call to GetPredicate
//
// Should have a feature for reconstructing a query stack from a predicate
// to for instance allow switching from By Formula to By Attribute mode in
// the find panel
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <OS.h>
#include <fs_query.h>

#include <Directory.h>
#include <Entry.h>
#include <Handler.h>
#include <Looper.h>
#include <Node.h>
#include <Query.h>
#include <String.h>
#include <Volume.h>
#include <parsedate.h>

#include "QueryStack.h"
#include "priv_syscalls.h"
#include "token.h"
#include "message_util.h"

#define     PREFERRED_TOKEN (0xFFFFFFFE)

BQuery::BQuery()
{
	fPredicate = NULL;
	fLive = false;
	fDev = -1;
	fPort = 0;
	fToken = 0;
	fQueryFd = -1;
	fStack = new BQueryStack;
}

BQuery::~BQuery()
{
	Clear();
	delete fStack;
}

status_t
BQuery::Clear()
{
	free(fPredicate);
	fPredicate = NULL;
	fLive = false;
	fDev = -1;
	fPort = 0;
	fToken = 0;
	fStack->clear();
	
	if (fQueryFd >= 0) {
		_kclose_query_(fQueryFd);
		fQueryFd = -1;
	}
	return 0;
}

status_t
BQuery::SetVolume(const BVolume *v)
{
	if (fQueryFd >= 0)
		return EPERM;
	fDev = v->Device();
	return 0;
}

status_t
BQuery::SetPredicate(const char *expr)
{
	size_t		l;
	char		*p;

	if (fQueryFd >= 0)
		return EPERM;
	l = strlen(expr) + 1;
	p = (char *) realloc(fPredicate, l);
	if (!p)
		return ENOMEM;
	memcpy(p, expr, l);
	fPredicate = p;
	return 0;
}


void
BQuery::commit_stack()
{
	if (!fStack->is_empty()) {
		if (fPredicate)
			free(fPredicate);
		fPredicate = fStack->convertStackToString();
	}
}


size_t
BQuery::PredicateLength()
{
	commit_stack();

	if (fPredicate == NULL)
		return 0;
	else
		return strlen(fPredicate) + 1;
}

status_t
BQuery::GetPredicate(char *buf, size_t length)
{
	commit_stack();

	if (fPredicate == NULL) {
		return B_NO_INIT;
	}

	if (length <= strlen(fPredicate))
		return EINVAL;

	strcpy(buf, fPredicate);
	return 0;
}

status_t
BQuery::GetPredicate(BString *result)
{
	// ToDo:
	// use non-destructive version of commit_stack that
	// uses lazy committing, uses a needCommit flag that gets set by
	// every push but still has a fully usable stack after the
	// first call to GetPredicate

	// Also, should return more descriptive error messages here for
	// malformed queries

	commit_stack();

	if (!fPredicate) 
		return B_NO_INIT;

	*result = fPredicate;
	return B_OK;
}


status_t
BQuery::SetTarget(BMessenger msngr)
{
	BHandler	*h;
	BLooper		*l;

	if (fQueryFd >= 0)
		return EPERM;

	h = msngr.Target(&l);

	if (!h && !l)
		return EINVAL;

	fPort = _get_looper_port_(l);
	if (h)
		fToken = _get_object_token_(h);
	else
		fToken = PREFERRED_TOKEN;

	fLive = TRUE;
	return B_NO_ERROR;
}

bool
BQuery::IsLive(void) const
{
	return fLive;
}

dev_t 
BQuery::TargetDevice() const
{
	return fDev;
}


status_t
BQuery::Fetch()
{
	int			fd;
	uint32		flags;

	if (fDev <= 0)
		return B_NO_INIT;

	if (fQueryFd >= 0)
		return EPERM;

	commit_stack();

	if (!fPredicate)
		return B_NO_INIT;

	flags = 0;
	if (fLive)
		flags |= B_LIVE_QUERY;

	const char *predicate;
	BString expandedPredicate;
	if (strchr(fPredicate, '%') == NULL)
		// no dates in this predicate, just pass it directly to avoid a copy
		predicate = fPredicate;
	else {
		// pick up any date strings in the form: %today%
		// replace them with a decimal time value
		// everything else goes untouched

		// avoid quoted strings 
		const char *dateString = NULL;
		const char *lastNonDateRun = fPredicate;
		bool sawBackslash = false;
		bool eatingQuotes = false;
		for (const char *scanner = fPredicate; *scanner; scanner++) {
			switch (*scanner) {
				case '%':
					if (!eatingQuotes) {
						if (!dateString) {
							// seeing the start of a date string
							dateString = scanner + 1;
							// tack on the string data we got sofar, from now on we are
							// parsing the date string
							expandedPredicate.Append(lastNonDateRun, scanner - lastNonDateRun);
						} else {
							// done parsing the date string, convert it using parsedate
							lastNonDateRun = scanner + 1;
							
							// grab the string we identified as the date
							BString tmp;
							tmp.Append(dateString, scanner - dateString);
							
							dateString = NULL;
							
							tmp.CharacterDeescape('\\');
							// append to predicate as a decimal number
							expandedPredicate << (int32)parsedate(tmp.String(), time(NULL));
						}
						
					}
					sawBackslash = false;					
					break;

				case '\\':
					sawBackslash = (eatingQuotes && !sawBackslash);
					break;

				case '\"':
					if (eatingQuotes) {
						if (!sawBackslash)
							eatingQuotes = false;
					} else if (!sawBackslash)
						eatingQuotes = true;

					sawBackslash = false;					
					break;

				default:
					sawBackslash = false;
					break;
			}
		}
		expandedPredicate += lastNonDateRun;
		predicate = expandedPredicate.String();
	}

	fd = _kopen_query_(fDev, predicate, flags, fPort, fToken, true);

	if (fd < 0)
		return fd;

	fQueryFd = fd;
	return 0;
}

status_t
BQuery::GetNextEntry(BEntry *entry, bool traverse)
{
	long			count;
	char			tmp[sizeof(struct dirent) + 256];
	struct dirent	*d = (struct dirent *) &tmp;
	node_ref		node;
	BDirectory		dir;
	
	count = _kread_query_(fQueryFd, d, sizeof(tmp), 1);
	if (count < 0) {
		entry->Unset();
		return count;
	}

	if (count == 0) {
		entry->Unset();
		return ENOENT;
	}

	node.device = d->d_pdev;
	node.node = d->d_pino;
	dir.SetTo(&node);
	return entry->SetTo(&dir, d->d_name, traverse);
}

status_t
BQuery::GetNextRef(entry_ref *ref)
{
	long			count;
	char			tmp[sizeof(struct dirent) + 256];
	struct dirent	*d = (struct dirent *) &tmp;
	BDirectory		dir;
	status_t		err;
	
	count = _kread_query_(fQueryFd, d, sizeof(tmp), 1);
	if (count < 0) {
		err = count;
		goto clear;
	}

	if (count == 0) {
		err = ENOENT;
		goto clear;
	}

	err = ref->set_name(d->d_name);
	if (err)
		goto clear;
	ref->device = d->d_pdev;
	ref->directory = d->d_pino;
	return 0;

clear:
	ref->set_name(NULL);
	ref->device = -1;
	ref->directory = -1;
	return err;
}

int32
BQuery::GetNextDirents(struct dirent *buf, size_t length, 
						int32 num)
{
	return _kread_query_(fQueryFd, buf, length, num);
}

void
BQuery::PushAttr(const char *attr)
{
	BQueryElem *elem;
	
	elem = new BQueryElem;
	elem->dataType = ATTR;

	elem->data.attr = strdup(attr);
	fStack->push(elem);
}

static char *make_case_insensitive(const char *str);

void
BQuery::PushOp(query_op op)
{
	BQueryElem *elem;
	BQueryElem *stackTopElem;

	elem = new BQueryElem;
	elem->dataType = QUERY_OP;
	elem->data.op = op;

	/*
	 * Hack to handle case insensitive compares
	 */
	switch (op) {
	case B_CONTAINS:
		stackTopElem = fStack->top();
		if (stackTopElem->dataType == STRING) {
			char *oldstr = stackTopElem->data.s;
			char *newstr = (char *) malloc(strlen(oldstr) + 3);
			sprintf(newstr, "*%s*", oldstr);
			stackTopElem->data.s = newstr;
			free(oldstr);
		}
		break;

	case B_BEGINS_WITH:
		stackTopElem = fStack->top();
		if (stackTopElem->dataType == STRING) {
			char *oldstr = stackTopElem->data.s;
			char *newstr = (char *) malloc(strlen(oldstr) + 2);
			sprintf(newstr, "%s*", oldstr);
			stackTopElem->data.s = newstr;
			free(oldstr);
		}
		break;

	case B_ENDS_WITH:
		stackTopElem = fStack->top();
		if (stackTopElem->dataType == STRING) {
			char *oldstr = stackTopElem->data.s;
			char *newstr = (char *) malloc(strlen(oldstr) + 2);
			sprintf(newstr, "*%s", oldstr);
			stackTopElem->data.s = newstr;
			free(oldstr);
		}
		break;

	default:
		break;
	}

	fStack->push(elem);
}

void
BQuery::PushUInt32(uint32 c)
{
	BQueryElem *elem;

	elem = new BQueryElem;
	elem->dataType = UINT32;
	elem->data.unsigned_int32 = c;

	fStack->push(elem);
}


void
BQuery::PushInt32(int32 c)
{
	BQueryElem *elem;

	elem = new BQueryElem;
	elem->dataType = INT32;
	elem->data.signed_int32 = c;

	fStack->push(elem);
}


void
BQuery::PushUInt64(uint64 c)
{
	BQueryElem *elem;

	elem = new BQueryElem;
	elem->dataType = UINT64;
	elem->data.unsigned_int64 = c;

	fStack->push(elem);
}


void
BQuery::PushInt64(int64 c)
{
	BQueryElem *elem;

	elem = new BQueryElem;
	elem->dataType = INT64;
	elem->data.signed_int64 = c;

	fStack->push(elem);
}


void
BQuery::PushFloat(float f)
{
	BQueryElem *elem;

	elem = new BQueryElem;
	elem->dataType = FLOAT;
	elem->data.f = f;

	fStack->push(elem);
}

void
BQuery::PushDouble(double d)
{
	BQueryElem *elem;

	elem = new BQueryElem;
	elem->dataType = DOUBLE;
	elem->data.d = d;

	fStack->push(elem);
}


void
BQuery::PushString(const char *str, bool case_insensitive)
{
	BQueryElem *elem;

	elem = new BQueryElem;
	elem->dataType = STRING;
	if (case_insensitive) {
		elem->data.s = make_case_insensitive(str);
	} else {
		elem->data.s = strdup(str);
	}

	fStack->push(elem);
}

status_t
BQuery::PushDate(const char *str)
{
	if (parsedate(str, time(NULL)) == -1)
		// reject malformed date strings that we cannot parse
		return B_BAD_VALUE;

	BQueryElem *elem;

	elem = new BQueryElem;
	elem->dataType = DATE;
	elem->data.s = strdup(str);

	fStack->push(elem);

	return B_OK;
}

static char *
make_case_insensitive(const char *str)
{
	int len, num_alphabetic = 0, num_meta = 0;
	char *tmp, *ptr;

	for(len=0, tmp=(char *)str; *tmp; tmp++) {
		if (isalpha(*tmp))
			num_alphabetic++;
		else
			len++;
	}

	ptr = (char *)malloc(len + num_meta*2 + num_alphabetic*4 + 1);
	if (ptr == NULL)
		return ptr;

	for(tmp=ptr; *str; str++) {
		if (isalpha(*str)) {
			*tmp++ = '[';
			*tmp++ = tolower(*str);
			*tmp++ = toupper(*str);
			*tmp++ = ']';
		}
		else {
			*tmp++ = *str;
		}
	}

	*tmp = '\0';     /* null terminate */
	
	return ptr;
}

	void		BQuery::_QwertyQuery1() {}
	void		BQuery::_QwertyQuery2() {}
	void		BQuery::_QwertyQuery3() {}
	void		BQuery::_QwertyQuery4() {}
	void		BQuery::_QwertyQuery5() {}
	void		BQuery::_QwertyQuery6() {}
