
#include <Drivers.h>
#include <Entry.h>
#include <fcntl.h>
#include <File.h>
#include <SoundFile.h>
#include <stdio.h>
#include <String.h>
#include <string.h>
#include <unistd.h>
#include "CDDataSource.h"


// -- CDFillDataSource --------------------------------------------------------
 
CDFillDataSource::CDFillDataSource(uchar _fill, size_t _len)
{
	fill = _fill;
	len = _len;
}

CDFillDataSource::CDFillDataSource(BMessage *archive)
{
	archive->FindInt8("fill", (int8 *)&fill);
	archive->FindInt32("length", (int32 *)&len);
}

BArchivable *CDFillDataSource::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "CDFillDataSource")) {
		return new CDFillDataSource(archive);
	} else {
		return NULL;
	}	
}

status_t CDFillDataSource::Archive(BMessage *archive, bool /*deep*/) const
{
	archive->AddString("class", "CDFillDataSource");
	archive->AddInt8("fill", (int8)fill);
	archive->AddInt32("length", (int32)len);
	return B_OK;
}


status_t 
CDFillDataSource::InitCheck(void)
{
	return B_OK;
}

status_t 
CDFillDataSource::Read(void *data, size_t len, off_t /*posn*/)
{
	memset(data, fill, len);
	return B_OK;
}

size_t 
CDFillDataSource::Length(void)
{
	return len;
}

char *
CDFillDataSource::Description()
{
	BString str("A uniform stream of '");
	str << (char)fill << "' bytes";
	return strdup(str.String());
}

bool 
CDFillDataSource::IsAudio()
{
	return false;
}


// -- CDAudioFileDataSource ---------------------------------------------------

CDAudioFileDataSource::CDAudioFileDataSource(BEntry *entry)
{
	Init(entry);
}


CDAudioFileDataSource::CDAudioFileDataSource(BMessage *archive)
{
	BEntry entry;
	const char *archivePath = archive->FindString("path");
	if (archivePath) {
		entry.SetTo(archivePath, true);
		Init(&entry);
	} else {
		sf = NULL;
	} 
}

void CDAudioFileDataSource::Init(BEntry *entry)
{
	entry_ref ref;
	entry->GetRef(&ref);
	entry->GetPath(&path);
	sf = new BSoundFile(&ref,B_READ_ONLY);
	if(sf->InitCheck()) goto err;
	if((sf->FileFormat() != B_AIFF_FILE) && (sf->FileFormat() != B_WAVE_FILE)) goto err;
	if(sf->CountChannels() != 2) goto err;
	if(sf->FrameSize() != 4) goto err;
	if(sf->SamplingRate() != 44100) goto err;
	if(sf->ByteOrder() == B_LITTLE_ENDIAN){
		// Little Endian is cool
		byteswap = 0; 
	} else {
		// Big Endian needs swappage 
		byteswap = 1;
	}
	return;
err:
	
	delete sf;
	sf = NULL;
}


BArchivable *CDAudioFileDataSource::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "CDAudioFileDataSource")) {
		return new CDAudioFileDataSource(archive);
	} else {
		return NULL;
	}
}

status_t CDAudioFileDataSource::Archive(BMessage *archive, bool /*deep*/) const
{
	archive->AddString("class", "CDAudioFileDataSource");
	archive->AddString("path", path.Path());
	return B_OK;
}


status_t 
CDAudioFileDataSource::InitCheck(void)
{
	if(sf){
		return B_OK;
	} else {
		return B_ERROR;
	}
}


CDAudioFileDataSource::~CDAudioFileDataSource()
{
	if(sf){
		delete sf;
	}
}

status_t 
CDAudioFileDataSource::Read(void *data, size_t len, off_t posn)
{
	if(sf){
		status_t ret;
		if(sf->SeekToFrame(posn/4) != (posn/4)) return B_ERROR;
		ret = sf->ReadFrames((char *) data, len / 4);
		
		if(byteswap){
			uint16 *x = (uint16*) data;
			len /= 4;
			while(len){
				*x = B_SWAP_INT16(*x);
				x++;
				*x = B_SWAP_INT16(*x);
				x++;
				len--;				
			}
		}
	}
	return B_ERROR;
}

size_t 
CDAudioFileDataSource::Length(void)
{
	if(sf){
		return sf->CountFrames() * 4;
	} else {
		return 0;
	}
}

char *
CDAudioFileDataSource::Description()
{
	BString desc;
	if (sf != NULL) {
		desc << path.Leaf() << " (" << sf->SamplingRate() << " Hz, ";
		switch (sf->FileFormat()) {
		case B_AIFF_FILE:
			desc << "AIFF";
			break;
		case B_WAVE_FILE:
			desc << "WAV";
			break;
		case B_UNIX_FILE:
			desc << "UNIX";
			break; 
		default:
			desc << "UNKNOWN";
			break;
		}
		desc << " format)";
	}
	return strdup(desc.String());
}

bool 
CDAudioFileDataSource::IsAudio()
{
	return true;
}



// -- CDDataFileDataSource ----------------------------------------------------

CDDataFileDataSource::CDDataFileDataSource(BEntry *entry)
{
	Init(entry);
}

void CDDataFileDataSource::Init(BEntry *entry)
{
	entry->GetPath(&path);
	f = new BFile(entry,B_READ_ONLY);
	if(f->InitCheck()){
		delete f;
		f = NULL;
	}
}


CDDataFileDataSource::CDDataFileDataSource(BMessage *archive)
{
	BEntry entry;
	const char *archivePath = archive->FindString("path");
	if (archivePath) {
		entry.SetTo(archivePath, true);
		Init(&entry);
	}
}

BArchivable *CDDataFileDataSource::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "CDDataFileDataSource")) {
		return new CDDataFileDataSource(archive);
	} else {
		return NULL;
	}
}

status_t CDDataFileDataSource::Archive(BMessage *archive, bool /*deep*/) const
{
	archive->AddString("class", "CDDataFileDataSource");

	const char *pp = NULL;
	archive->FindString("project_path", &pp);
	
	BPath projectPath(pp);
	BPath parent;
	projectPath.GetParent(&parent);
		
	BString parentPath(parent.Path());
	BString filePath(path.Path());

	if(filePath.Compare(parentPath, parentPath.Length()) == 0) {
		// Store as relative
		filePath.RemoveFirst(parentPath);
		// Strip any leading slashes...
		if(filePath.ByteAt(0) == '/')
			filePath.RemoveFirst("/");

		archive->AddString("path", filePath.String());
	} else {
		// Store as absolute
		archive->AddString("path", path.Path());
	}
	return B_OK;
}


status_t 
CDDataFileDataSource::InitCheck(void)
{
	if(f) {
		return B_OK;
	} else {
		return B_ERROR;
	}
}

CDDataFileDataSource::~CDDataFileDataSource()
{
	if(f){
		delete f;
	}
}

status_t 
CDDataFileDataSource::Read(void *data, size_t len, off_t posn)
{
	if(f){
		if((size_t)f->ReadAt(posn, data, len) == len){
			return B_OK;
		} else {
			return B_ERROR;
		}
	} else {
		return B_ERROR;
	}
}

size_t
CDDataFileDataSource::Length(void)
{
	if(f){
		off_t sz;
		if(f->GetSize(&sz)){
			return 0;
		} else {
			return sz;
		}
	} else {
		return 0;
	}
}

char *
CDDataFileDataSource::Description()
{
	return strdup(path.Leaf());
}

bool 
CDDataFileDataSource::IsAudio()
{
	return false;
}


static int 
get_device_blocksize(int fd)
{
    device_geometry dg;
    if (ioctl(fd, B_GET_GEOMETRY, &dg, sizeof(dg)) < 0) {
		return -1;
    }
    return dg.bytes_per_sector;
}

static off_t
get_device_blocks__(int fd)
{
//    struct stat st;
    device_geometry dg;

    if (ioctl(fd, B_GET_GEOMETRY, &dg, sizeof(dg)) >= 0) {
        return (off_t)dg.cylinder_count *
               (off_t)dg.sectors_per_track *
               (off_t)dg.head_count;
    }
    return -1;
}


// -- CDDiskDeviceDataSource --------------------------------------------------

CDDiskDeviceDataSource::CDDiskDeviceDataSource(char *_path)
{
	Init(_path);
}


CDDiskDeviceDataSource::CDDiskDeviceDataSource(BMessage *archive)
{
	const char *archivePath = archive->FindString("path");	
	if (!archivePath) {
		archivePath = B_EMPTY_STRING;
	}
	Init(const_cast<char *>(archivePath));
}

void CDDiskDeviceDataSource::Init(char *_path)
{
	path.SetTo((const char *)_path);
	if(strncmp(_path,"/dev/disk",9)) {
		fd = -1;
	} else {
		fd = open(_path, O_RDONLY);
		if(fd < 0){
			fd = -1;
		} else {
			blocks = get_device_blocks__(fd);
			blocksize = get_device_blocksize(fd);
			if((blocks < 0) || (blocksize < 0)) {
				close(fd);
				fd = -1;
			}
		}
	}
}


BArchivable *CDDiskDeviceDataSource::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "CDDiskDeviceDataSource")) {
		return new CDDiskDeviceDataSource(archive);
	} else {
		return NULL;
	}
}

status_t CDDiskDeviceDataSource::Archive(BMessage *archive, bool /*deep*/) const
{
	archive->AddString("class", "CDDiskDeviceDataSource");
	archive->AddString("path", path.Path());
	return B_OK;
}


status_t 
CDDiskDeviceDataSource::InitCheck(void)
{
	if(fd >= 0) {
		return B_OK;
	} else {
		return B_ERROR;
	}
}

CDDiskDeviceDataSource::~CDDiskDeviceDataSource()
{
	if(fd >= 0){
		close(fd);
	}
}

status_t 
CDDiskDeviceDataSource::Read(void *data, size_t len, off_t posn)
{
	if(lseek(fd, posn, SEEK_SET) != posn) {
		return B_ERROR;
	}
	if((size_t)read(fd, data, len) != len) {
		return B_ERROR;
	}
	return B_OK;
}

size_t 
CDDiskDeviceDataSource::Length(void)
{
	return ((size_t)blocks) * ((size_t)blocksize);
}

char *
CDDiskDeviceDataSource::Description()
{
	BString desc("Raw disk device: ");
	desc << path.Path();
	return strdup(desc.String());
}

bool 
CDDiskDeviceDataSource::IsAudio()
{
	return false;
}

