#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <OS.h>
#include <Debug.h>

#include <Resources.h>
#include <Entry.h>

#include <private/storage/write_res.h>


BResources::BResources()
{
	m_map = NULL;
	new_resource_map(&m_map, NULL);
	fDirty = false;
	fReadOnly = false;
}

/*------------------------------------------------------------------*/

BResources::BResources(const BFile *file, bool truncate)
{
	m_map = NULL;
	fDirty = true;
	fReadOnly = true;
	SetTo(file, truncate);
}

/*------------------------------------------------------------------*/

BResources::~BResources()
{
	SetTo(NULL);
	dispose_resource_map(m_map);
	m_map = NULL;
}


/*------------------------------------------------------------------*/

const BFile &
BResources::File() const
{
	return fFile;
}

/*------------------------------------------------------------------*/

status_t
BResources::SetTo(const BFile *file, bool truncate)
{	
	status_t	err = B_OK;
	if (fFile.InitCheck() == B_NO_ERROR) {
		if (!fReadOnly && fDirty) {
			int fd = fFile.Dup();
			int endian = 0;
			err = fd < 0 ? fd : 0;
			if (err == B_OK) {
				err = position_at_map(fd, true, &endian);
			}
			if (err >= B_OK) {
				err = write_resource_file(m_map, fd, endian, NULL);
			}
			if (fd >= 0) close(fd);
		}
	}
	fFile.Unset();
	dispose_resource_map(m_map);
	m_map = NULL;
	err = new_resource_map(&m_map, NULL);

	if (!file) {
		fDirty = false;
		fReadOnly = false;	/* OK to add to item */
		return err;
	}
	if (file->InitCheck() != B_OK) {
		return file->InitCheck();
	}
	fFile = *file;

	fDirty = truncate;
	fReadOnly = !fFile.IsWritable();

	if (!truncate) {
		int fd = fFile.Dup();
		int endian = 0;
		if (fd < 0) {
			err = fd;
		}
		else {
			err = position_at_map(fd, false, &endian);
		}
		if (err >= B_OK) {
			status_t err2 = read_resource_file(&m_map, fd, endian, NULL);
			if (err2 < 0 && err != 0) err = err2;
		}
		if (fd >= 0) close(fd);
	}
	return err > 0 ? 0 : err;
}

/*------------------------------------------------------------------*/

const void *	
BResources::LoadResource(
	type_code type,
	int32 id,
	size_t * out_size)
{
	int size;
	const char * name;
	const void * data;

	if (!m_map) {
		return NULL;
	}
	data = find_resource_by_id(m_map, type, id, &size, &name);
	if (data && out_size) *out_size = size;
	return data;
}


/*------------------------------------------------------------------*/

const void * 
BResources::LoadResource(
	type_code type,
	const char * name,
	size_t * out_size)
{
	int size;
	int id;
	const void * data;

	if (!m_map) {
		return NULL;
	}
	data = find_resource_by_name(m_map, type, name, &size, &id);
	if (data && out_size) *out_size = size;
	return data;
}


/*------------------------------------------------------------------*/

status_t 
BResources::PreloadResourceType(
	type_code type)
{
	if (!m_map) {
		return EBADF;
	}
	return load_resource_type(m_map, type);
}

/*------------------------------------------------------------------*/

status_t 
BResources::Sync()
{
	int ret;
	int fd;
	int endian;

	if (fFile.InitCheck() != B_OK) {
		return fFile.InitCheck();
	}
	if (fReadOnly) {
		return EPERM;
	}
	if (!fDirty) {
		return B_OK;
	}
	fd = fFile.Dup();
	if (fd < 0) return B_FILE_ERROR;
	ret = position_at_map(fd, true, &endian);
	if (ret < 0) {
		close(fd);
		return B_IO_ERROR;
	}
	ret = write_resource_file(m_map, fd, endian, NULL);
	close(fd);
	return (ret < 0) ? B_IO_ERROR : B_OK;
}

/*------------------------------------------------------------------*/

status_t 
BResources::MergeFrom(						/* adds resources in from_file to this file */
	BFile * from_file)
{
	int fd;
	int ret;
	int endian;

	if (!m_map) {
		return B_FILE_ERROR;
	}
	if (from_file->InitCheck()) {
		return from_file->InitCheck();
	}
	fd = from_file->Dup();
	if (fd < 0) {
		return B_FILE_ERROR;
	}
	if (!fReadOnly) {		/* It's OK to "merge" files logically at run-time. */
		fDirty = true;		/* If the file is read-only, it just won't receive changes. */
	}
	ret = position_at_map(fd, false, &endian);
	if (ret < 0) {
		close(fd);
		return B_IO_ERROR;
	}
	ret = read_resource_file(&m_map, fd, endian, NULL);
	close(fd);
	if (ret < 0) {
		return B_ERROR;		/* OOPS! */
	}
	return B_OK;
}

/*------------------------------------------------------------------*/

status_t 
BResources::WriteTo(						/* like a "SetTo()" with truncate without flushing data followed by a "Sync()" */
	BFile * new_file)
{
	status_t err;

	fFile.Unset();
	err = new_file->InitCheck();
	if (err < B_OK) return err;
	if (!new_file->IsWritable()) return err;
	fFile = *new_file;
	fDirty = true;
	fReadOnly = false;
	return Sync();
}

/*------------------------------------------------------------------*/

bool 
BResources::GetResourceInfo(
	const void * resource,
	type_code * out_type,
	int32 * out_id,
	size_t * out_size,
	const char ** out_name)
{
	unsigned int type;
	int id;
	const void * data;
	int size;
	const char * name;

	void * cookie = NULL;

	if (!m_map || !resource) {
		return false;
	}

	while (iterate_resources(m_map, &cookie, &type, &id, &data, &size, &name) == 0) {
		if (resource == data) {
			if (out_type) *out_type = type;
			if (out_id) *out_id = id;
			if (out_name) *out_name = name;
			if (out_size) *out_size = size;
			return true;
		}
	}
	return false;
}


/*------------------------------------------------------------------*/

status_t 
BResources::RemoveResource(
	const void * resource)
{
	if (!m_map) {
		return B_FILE_ERROR;
	}
	if (!resource) {
		return B_BAD_VALUE;
	}
	if (remove_resource(m_map, resource) < 0) {
		return B_ERROR;
	}
	fDirty = true;
	return B_OK;
}


/*------------------------------------------------------------------*/

/*** DEPRECATED ***/
long	BResources::WriteResource(type_code type,
								  int32 id,
								  const void *in_data,
								  off_t offset,
								  size_t data_size)
{
	if (!in_data || offset < 0) {
		return B_BAD_VALUE;
	}
	if (fReadOnly) {
		return EPERM;
	}
	if (!m_map) {
		return B_FILE_ERROR;
	}
	size_t size = 0;
	const void * data = LoadResource(type, id, &size);
	if (!data) return B_BAD_VALUE;
	if (size < offset+data_size) {
		void * np = malloc(offset+data_size);
		if (!np) return B_NO_MEMORY;
		memcpy(np, data, size);
		memcpy(((char*)np)+offset, in_data, data_size);
		return (replace_resource_data(m_map, data, np, offset+data_size) < 0) ? B_ERROR : B_OK;
	}
	memcpy(((char *)data)+offset, in_data, data_size);	/*** WARNING: this is a skanky hack! ***/
	fDirty = true;
	return B_OK;
}

/*------------------------------------------------------------------*/

/*** DEPRECATED ***/
long	BResources::ReadResource(type_code type,
		     					   	   int32 id,
		     					   	   void *out_data,
									   off_t offset,
		     					   	   size_t data_size)
					
{
	if (!out_data || offset < 0) {
		return B_BAD_VALUE;
	}
	if (!m_map) {
		return B_FILE_ERROR;
	}
	size_t size = 0;
	const void * data = LoadResource(type, id, &size);
	if (!data) return B_BAD_VALUE;
	if (size <= offset) {
		return B_OK;	/* no data to read */
	}
	if (size < offset+data_size) {
		data_size = size - offset;
	}
	memcpy(out_data, ((char *)data)+offset, data_size);
	return B_OK;
}

/*------------------------------------------------------------------*/

long	BResources::AddResource(type_code type,
		     					   int32 id,
		     					   const void *data,
		     					   size_t data_size,
								   const char *name)
{
	if (fReadOnly) {
		return EPERM;
	}
	if (!m_map) {
		return B_FILE_ERROR;
	}
	if (!data) {
		return B_BAD_VALUE;
	}
	if (add_resource(&m_map, type, id, data, data_size, name) < 0) {
		return B_NO_MEMORY;
	}
	fDirty = true;
	return B_OK;
}

/*------------------------------------------------------------------*/

bool	BResources::HasResource(type_code in_type, int32 in_id)
{
	unsigned int type;
	int id;
	const void * data;
	int size;
	const char * name = NULL;

	void * cookie = NULL;

	while (iterate_resources(m_map, &cookie, &type, &id, &data, &size, &name) == 0) {
		if ((type == in_type) && (id == in_id)) {
			return true;
		}
	}
	return false;
}

/*------------------------------------------------------------------*/

bool	BResources::HasResource(type_code in_type, const char *in_name)
{
	unsigned int type;
	int id;
	const void * data;
	int size;
	const char * name;

	void * cookie = NULL;

	if (!in_name) {
		return false;
	}

	while (iterate_resources(m_map, &cookie, &type, &id, &data, &size, &name) == 0) {
		if ((type == in_type) && !strcmp(name, in_name)) {
			return true;
		}
	}
	return false;
}

/*------------------------------------------------------------------*/

/*** DEPRECATED ***/
void	*BResources::FindResource(type_code type, 
								  const char *name, 
								  size_t *data_size)
{
	int size = 0;
	int id;
	const void * data = find_resource_by_name(m_map, type, name, &size, &id);
	if (data_size) *data_size = size;
	if (!data) return NULL;
	void * ret = malloc(size);
	if (!ret) return NULL;
	memcpy(ret, data, size);
	return ret;
}

/*------------------------------------------------------------------*/

/*** DEPRECATED ***/
void	*BResources::FindResource(type_code type, 
								  int32 id, 
								  size_t *data_size)
{
	int size = 0;
	const char * name;
	const void * data = find_resource_by_id(m_map, type, id, &size, &name);
	if (data_size) *data_size = size;
	if (!data) return NULL;
	void * ret = malloc(size);
	if (!ret) return NULL;
	memcpy(ret, data, size);
	return ret;
}


/*------------------------------------------------------------------*/

int	BResources::RemoveResource(type_code type, int32 id)
{
	if (!m_map) {
		return B_FILE_ERROR;
	}
	if (fReadOnly) {
		return EPERM;
	}
	if (remove_resource_id(m_map, type, id) < 0) {
		return B_BAD_VALUE;
	}
	fDirty = true;
	return B_OK;
}

/*------------------------------------------------------------------*/

bool	BResources::GetResourceInfo( int32 resIndex,
										type_code* out_type,
										int32* out_id,
										const char **out_name,
										size_t* out_size)
{
	unsigned int type;
	int id;
	const void * data;
	int size;
	const char * name;

	void * cookie = NULL;

	if (!m_map) {
		return false;
	}

	while (iterate_resources(m_map, &cookie, &type, &id, &data, &size, &name) == 0) {
		if (resIndex-- == 0) {
			if (out_type) *out_type = type;
			if (out_id) *out_id = id;
			if (out_name) *out_name = name;
			if (out_size) *out_size = size;
			return true;
		}
	}
	return false;
}

/*------------------------------------------------------------------*/

bool	BResources::GetResourceInfo( type_code in_type,
									 int32 resIndex,
									 int32* out_id,
									 const char **out_name,
									 size_t* out_size)
{
	unsigned int type;
	int id;
	const void * data;
	int size;
	const char * name;

	void * cookie = NULL;

	if (!m_map) {
		return false;
	}

	while (iterate_resources(m_map, &cookie, &type, &id, &data, &size, &name) == 0) {
		if ((in_type == type) && (resIndex-- == 0)) {
			if (out_id) *out_id = id;
			if (out_name) *out_name = name;
			if (out_size) *out_size = size;
			return true;
		}
	}
	return false;
}

/*------------------------------------------------------------------*/

bool	BResources::GetResourceInfo( type_code in_type,
									 int32 in_id,
									 const char **out_name,
									 size_t* out_size)
{
	unsigned int type;
	int id;
	const void * data;
	int size;
	const char * name;

	void * cookie = NULL;

	if (!m_map) {
		return false;
	}

	while (iterate_resources(m_map, &cookie, &type, &id, &data, &size, &name) == 0) {
		if ((in_type == type) && (in_id == id)) {
			if (out_name) *out_name = name;
			if (out_size) *out_size = size;
			return true;
		}
	}
	return false;
}

/*------------------------------------------------------------------*/

bool	BResources::GetResourceInfo( type_code in_type,
										const char *in_name,
										int32* out_id,
										size_t* out_size)
{
	unsigned int type;
	int id;
	const void * data;
	int size;
	const char * name;

	void * cookie = NULL;

	if (!m_map || !in_name) {
		return false;
	}

	while (iterate_resources(m_map, &cookie, &type, &id, &data, &size, &name) == 0) {
		if ((in_type == type) && name && !strcmp(in_name, name)) {
			if (out_id) *out_id = id;
			if (out_size) *out_size = size;
			return true;
		}
	}
	return false;
}

#if !_PR3_COMPATIBLE_

void 
BResources::_ReservedResources1()
{
}

void 
BResources::_ReservedResources2()
{
}

void 
BResources::_ReservedResources3()
{
}

void 
BResources::_ReservedResources4()
{
}

void 
BResources::_ReservedResources5()
{
}

void 
BResources::_ReservedResources6()
{
}

void 
BResources::_ReservedResources7()
{
}

void 
BResources::_ReservedResources8()
{
}

#endif
