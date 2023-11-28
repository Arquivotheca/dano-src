

/*
 * TIFF Library BE-specific Routines.
 */
#include "tiffiop.h"
#include "sys/stat.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <DataIO.h>

//=======================================================

static tsize_t
tiffIOReadProc(thandle_t aStream, tdata_t buf, tsize_t size)
{	
	BPositionIO *posIO = static_cast<BPositionIO *>(aStream);
	return posIO->Read(buf, (size_t)size);
}

static tsize_t
tiffIOWriteProc(thandle_t aStream, tdata_t buf, tsize_t size)
{
	BPositionIO *posIO = static_cast<BPositionIO *>(aStream);
	return posIO->Write(buf, (size_t)size);
}

static toff_t
tiffIOSeekProc(thandle_t aStream, toff_t off, int whence)
{
	BPositionIO *posIO = static_cast<BPositionIO *>(aStream);
	return posIO->Seek(off, (uint32)whence);
}

static int
tiffIOCloseProc(thandle_t aStream)
{
	BPositionIO *posIO = static_cast<BPositionIO *>(aStream);
	// Nothing to be done for closing a positonIO stream
	return 0;
}

static toff_t
tiffIOSizeProc(thandle_t aStream)
{
	BPositionIO *posIO = static_cast<BPositionIO *>(aStream);

	// get current position for safe keeping
	off_t	oldPos = posIO->Position();
	
	// seek to end 
	posIO->Seek(0, SEEK_END);
	
	// get position again
	off_t ending = posIO->Position();
	
	// restore old position
	posIO->Seek(oldPos, SEEK_SET);
	
	// return this as the size
	return ending;
}


//=======================================================

static tsize_t
_tiffReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
//	return (fd_read((int) fd, buf, (size_t) size));
	return (read((int) fd, buf, (size_t) size));
}

static tsize_t
_tiffWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
//	return (fd_write((int) fd, buf, (size_t) size));
	return (write((int) fd, buf, (size_t) size));
}

static toff_t
//_tiffSeekProc(thandle_t fd, off_t off, int whence)
_tiffSeekProc(thandle_t fd, toff_t off, int whence)
{
//	return ((toff_t) fd_seek((int) fd, (off_t) off, whence));
	return ((toff_t) lseek((int) fd, (off_t) off, whence));
}

static int
_tiffCloseProc(thandle_t fd)
{
//	return (fd_close((int) fd));
	return (close((int) fd));
}

static toff_t
_tiffSizeProc(thandle_t fd)
{
//	return (fd_size((int) fd));
	struct stat st;
	fstat((int)fd,&st);
	return st.st_size;
}

#ifdef MMAP_SUPPORT
#include <sys/mman.h>

static int
_tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	toff_t size = _tiffSizeProc(fd);
	if (size != (toff_t) -1) {
		*pbase = (tdata_t)
		    mmap(0, size, PROT_READ, MAP_SHARED, (int) fd, 0);
		if (*pbase != (tdata_t) -1) {
			*psize = size;
			return (1);
		}
	}
	return (0);
}

static void
_tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
	(void) munmap(base, (off_t) size);
}
#else /* !MMAP_SUPPORT */
static int
_tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	return (0);
}

static void
_tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
}
#endif /* !MMAP_SUPPORT */

/*
 * Open a PositionIO for read/writing.
 */
 
TIFF*
TIFFStreamOpen(void *aStream, const char *name, const char *mode)
{
	
	BPositionIO *posIO = static_cast<BPositionIO *>(aStream);
	

	TIFF* tif;

	tif = TIFFClientOpen(name, mode,
	    (thandle_t) posIO,
	    tiffIOReadProc, tiffIOWriteProc,
	    tiffIOSeekProc, tiffIOCloseProc, tiffIOSizeProc,
	    _tiffMapProc, _tiffUnmapProc);
	
	return (tif);
}

/*
 * Open a TIFF file descriptor for read/writing.
 */
TIFF*
TIFFFdOpen(int fd, const char* name, const char* mode)
{
	TIFF* tif;

	tif = TIFFClientOpen(name, mode,
	    (thandle_t) fd,
	    _tiffReadProc, _tiffWriteProc,
	    _tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
	    _tiffMapProc, _tiffUnmapProc);
	if (tif)
		tif->tif_fd = fd;
	return (tif);
}

/*
 * Open a TIFF file for read/writing.
 */
TIFF*
TIFFOpen(const char* name, const char* mode)
{
	static const char module[] = "TIFFOpen";
	int m, fd;

	m = _TIFFgetMode(mode, module);
	if (m == -1)
		return ((TIFF*)0);
	fd = open(name, m);
	if (fd < 0) {
		TIFFError(module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}
	return (TIFFFdOpen(fd, name, mode));
}

void*
_TIFFmalloc(size_t s)
{
	return (malloc(s));
}

void
_TIFFfree(void* p)
{
	free(p);
}

void*
_TIFFrealloc(void* p, size_t s)
{
	return (realloc(p, s));
}
