#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <Entry.h>
#include <Path.h>
#include <SymLink.h>
#include <Directory.h>

#include "priv_syscalls.h"

BSymLink::BSymLink()
{
}

BSymLink::BSymLink(const entry_ref *ref) : BNode(ref)
{
}

BSymLink::BSymLink(const BEntry *entry) : BNode(entry)
{
}

BSymLink::BSymLink(const char *path) : BNode(path)
{
}

BSymLink::BSymLink(const BDirectory *dir, const char *path) : BNode(dir, path)
{
}

BSymLink::BSymLink(const BSymLink &link) : BNode(link)
{
}

BSymLink::~BSymLink()
{
	set_fd(-1);
}

ssize_t	BSymLink::ReadLink(char *buf, size_t len)
{
	ssize_t		sz;

	sz =  _kreadlink_(fFd, NULL, buf, len);
	if (sz < 0)
		return sz;
	buf[sz] = '\0';
	return sz;
}

ssize_t	BSymLink::MakeLinkedPath(const BDirectory *dir, BPath *path)
{
  ssize_t sz;
  status_t err;
  BEntry entry;

  char buf[B_PATH_NAME_LENGTH+1];

  if (!dir || !path) {
	err = B_BAD_VALUE;
	goto error;
  }

  if ((sz=ReadLink(buf, B_PATH_NAME_LENGTH)) < B_NO_ERROR) {
	err = sz;
	goto error;
  }

  if (buf[0] == '/') {
	path->SetTo(buf);
	return (strlen(path->Path()));
	}

  if ((err=dir->GetEntry(&entry)) != B_NO_ERROR) 
	goto error;

  if ((err=entry.GetPath(path)) != B_NO_ERROR)
	goto error;

  if ((err=path->Append(buf)) != B_NO_ERROR) 
	goto error;

  return strlen(path->Path());

error:
  path->Unset();
  return err;
}

  
status_t	BSymLink::MakeLinkedPath(const char *dpath, BPath *path)
{
  BDirectory dir;
  status_t err;

  if (!dpath || !path) {
	err = B_BAD_VALUE;
	goto error;
  }
  
  if ((err=dir.SetTo(dpath)) != B_NO_ERROR) 
	goto error;

  return (MakeLinkedPath(&dir, path));

error:
  path->Unset();
  return err;
}

bool BSymLink::IsAbsolute(void)
{
  char buf[B_PATH_NAME_LENGTH+1];
  buf[0] = '\0';

  if (ReadLink(buf, B_PATH_NAME_LENGTH) < B_NO_ERROR)
	return false;
  
  return (buf[0] == '/');
}

void		BSymLink::_MissingSymLink1() {}
void		BSymLink::_MissingSymLink2() {}
void		BSymLink::_MissingSymLink3() {}
void		BSymLink::_MissingSymLink4() {}
void		BSymLink::_MissingSymLink5() {}
void		BSymLink::_MissingSymLink6() {}
