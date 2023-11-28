#include <sys/types.h>
#include <SupportDefs.h>
#include <Storage.h>
#include <Statable.h>
#include <Node.h>
#include <Volume.h>

status_t BStatable::GetNodeRef(node_ref *ref) const
{
	int			err;
	struct stat	st;

	err = GetStat(&st);

	if (err)
		return err;

	ref->device = st.st_dev;
	ref->node = st.st_ino;
	return B_OK;
}

bool	BStatable::IsFile() const
{
	int			err;
	struct stat	st;

	err = GetStat(&st);
	if (err)
		return FALSE;
	return S_ISREG(st.st_mode);
}

bool	BStatable::IsDirectory() const
{
	int			err;
	struct stat	st;

	err = GetStat(&st);
	if (err)
		return FALSE;
	return S_ISDIR(st.st_mode);
}

bool	BStatable::IsSymLink() const
{
	int			err;
	struct stat	st;

	err = GetStat(&st);
	if (err)
		return FALSE;
	return S_ISLNK(st.st_mode);
}

status_t 	BStatable::GetOwner(uid_t *owner) const
{
	struct stat		st;
	int				err;

	err = GetStat(&st);
	if (err)
		return err;
	*owner = st.st_uid;
	return 0;
}

status_t 	BStatable::SetOwner(uid_t owner)
{
	struct stat		st;

	st.st_uid = owner;
	return set_stat(st, WSTAT_UID);
}

status_t 	BStatable::GetGroup(gid_t *group) const
{
	struct stat		st;
	int				err;

	err = GetStat(&st);
	if (err)
		return err;
	*group = st.st_gid;
	return 0;
}

status_t 	BStatable::SetGroup(gid_t group)
{
	struct stat		st;

	st.st_gid = group;
	return set_stat(st, WSTAT_GID);
}

status_t 	BStatable::GetPermissions(mode_t *perms) const
{
	struct stat		st;
	int				err;

	err = GetStat(&st);
	if (err)
		return err;
	*perms = st.st_mode;
	return 0;
}

status_t 	BStatable::SetPermissions(mode_t perms)
{
	struct stat		st;

	st.st_mode = perms;
	return set_stat(st, WSTAT_MODE);
}

status_t	BStatable::GetSize(off_t *size) const
{
	struct stat		st;
	int				err;

	err = GetStat(&st);
	if (err)
		return err;
	*size = st.st_size;
	return 0;
}

#if 0
status_t 	BStatable::SetSize(off_t size)
{
	struct stat		st;

	st.st_size = size;
	return set_stat(st, WSTAT_SIZE);
}
#endif

status_t	BStatable::GetModificationTime(time_t *mtime) const
{
	struct stat		st;
	int				err;

	err = GetStat(&st);
	if (err)
		return err;
	*mtime = st.st_mtime;
	return 0;
}

status_t 	BStatable::SetModificationTime(time_t mtime)
{
	struct stat		st;

	st.st_mtime = mtime;
	return set_stat(st, WSTAT_MTIME);
}

status_t	BStatable::GetCreationTime(time_t *crtime) const
{
	struct stat		st;
	int				err;

	err = GetStat(&st);
	if (err)
		return err;
	*crtime = st.st_crtime;
	return 0;
}

status_t 	BStatable::SetCreationTime(time_t crtime)
{
	struct stat		st;

	st.st_crtime = crtime;
	return set_stat(st, WSTAT_CRTIME);
}


status_t	BStatable::GetAccessTime(time_t *atime) const
{
	struct stat		st;
	int				err;

	err = GetStat(&st);
	if (err)
		return err;
	*atime = st.st_atime;
	return 0;
}

status_t 	BStatable::SetAccessTime(time_t atime)
{
	struct stat		st;

	st.st_atime = atime;
	return set_stat(st, WSTAT_ATIME);
}

status_t BStatable::GetVolume(BVolume *vol) const
{
  struct stat st;
  status_t  err;

  err = GetStat(&st);

  if (err) {
	vol->SetTo(-1);
	return err;
  }

  if ((err=vol->SetTo(st.st_dev)) < B_NO_ERROR)
	return err;
  return B_NO_ERROR;
}

	void		BStatable::_OhSoStatable1() {}
	void		BStatable::_OhSoStatable2() {}
	void		BStatable::_OhSoStatable3() {}
