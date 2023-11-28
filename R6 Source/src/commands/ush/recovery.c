#include <stdio.h>
#include <string.h>
#include <fs_info.h>
#include <Drivers.h>
#include <priv_syscalls.h>
#include <nsmessages.h>
#include <TypeConstants.h>


/* addfirstpartition */

static char *get_raw_device(char *name);
static int add_primary_partition(char *rawdev);

status_t do_addfirstpartition(int argc, char **argv)
{
	char name[B_FILE_NAME_LENGTH + 1];

	if (get_raw_device(name) == NULL) {
		printf("Failed on get_raw_device\n");
		return 1;
	}
	
	if (add_primary_partition(name) != B_OK) {
		printf("Failed on add_primary_partition\n");
		return 1;
	}

	return 0;
}

// return /dev/disk/.../raw for boot
static char *
get_raw_device(char *name)
{
	char *cp;
	char buf[128];	
	struct fs_info fsinfo;

	if (_kstatfs_(-1, NULL, -1, "/boot", &fsinfo) != 0) {
		printf("Failed on kstatfs\n");
		return NULL;
	}
	
	strcpy(name,fsinfo.device_name);
	sprintf(buf, "BOOTPARTITION=%s", name);
	putenv(buf);
	
	
	cp = strrchr(name, '/');
	if (cp == NULL) {
		// Ugh?
		printf("Failed : %s is invalid dev name\n",name);
		return NULL;
	}
	cp++ ; *cp = '\0';

	strcat(name,"raw");
	return name;	
}

struct partition_entry {
	uchar	active;
	uchar	start_chs[3];
	uchar	id;
	uchar	end_chs[3];
	uint32	start_lba;
	uint32	len_lba;
};

static status_t make_first_partition(off_t offset, off_t size, int32 lbs, char *bdev);

static int
add_primary_partition(char *rawdev)
{
	int fd, block_size,rc;
	unsigned char sb[512];
	struct partition_entry *entry;
	device_geometry g;
	
	fd = open(rawdev, O_RDONLY);
	if (fd < 0) {
		printf("Failed on opening %s\n",rawdev);
		return fd;
	}
	
	if (ioctl(fd, B_GET_GEOMETRY, &g) < 0) {
		printf("Failed on B_GET_GEOMETRY ioctl\n");
		goto error0;
	}
	block_size = g.bytes_per_sector;
	
	if (read(fd, sb, 512) != 512) {
		printf("Failed on reading boot sector\n");
		goto error0;
	}
	
	entry = (struct partition_entry*)(sb + 0x1be);
	
	rc = make_first_partition((off_t)(entry->start_lba * block_size),
		(off_t)(entry->len_lba * block_size),
		(int32)block_size, rawdev);
	
	close(fd);
	return rc;
	
error0:
	close(fd);
	return B_ERROR;
}

static status_t make_first_partition(off_t offset, off_t size, int32 lbs, char *bdev)
{
	int fd;
	char bufa[B_FILE_NAME_LENGTH + 1];
	partition_info part;
	struct stat st;
	char *cp;
	char buf[128];
	
	strcpy(bufa, bdev);
	cp = strrchr(bufa, '/');
	cp++ ; *cp = '\0';
	strcat(bufa,"0_0");

	sprintf(buf,"OTHERPARTITION=%s",bufa);
	putenv(buf);
	
	unlink(bufa);
	fd = creat(bufa, 0666);
	if (fd < 0) {
		printf("Failed on creating %s\n",bufa);
		return(-1);
	}
	
	part.offset = offset;
	part.size = size;
	part.logical_block_size = lbs;
	part.session = 0;
	part.partition = 0;
	strcpy(part.device, bdev);

//	printf("%Lx %Lx %x %x %x %s\n",part.offset,
//	part.size, (unsigned int) part.logical_block_size,
//	(unsigned int) part.session, (unsigned int) part.partition,
//	part.device);

	if (ioctl(fd, B_SET_PARTITION, &part) < 0) {
		printf("Failed on B_SET_PARTITION ioctl\n");
		close(fd);
		return(-1);
	}

	close(fd);
	return B_OK;
}


/* get user settings */
/* a really, really bad idea */

typedef struct {
	const char *name;
	char *data;
	const char *exportname;
} param;


static void export_params(param *params);
static char *read_file(const char *name, char **pbuf, int *len);
static int get_params_from_bmessage(const char *ptr, int buflen, param *p);


status_t 
do_getdialupsettings(int argc, char **argv)
{
	char *buf = NULL;
	int len = 0;
	param params[] = {
		{ PPPSESS_phone_number, NULL, "R_PHONE"},
		{ PPPSESS_username, NULL, "R_USER"},
		{ PPPSESS_password, NULL, "R_PASSWD" },
		{ NULL, NULL, NULL }
	};	

	buf = read_file(argv[0],&buf,&len);
	if (buf == NULL) {
		return 1;
	}

	if (get_params_from_bmessage(buf,len,params) < B_OK) {
		return 1;
	}

	export_params(params);
	return 0;
}


static void export_params(param *params) {
	param *p;
	char buf[128];
	for (p = params; p->name; p++) {
		if (p->data) {
			sprintf(buf,"%s=%s",p->exportname,p->data);
			putenv(buf);
			free(p->data);
		}
	}
	return;
}

static char *
read_file(const char *name, char **pbuf, int *len)
{
	char *buf;
	int fd;
	int size;
	struct stat stbuf;
	
	fd = open(name,O_RDONLY);
	if (fd < 0) {
		printf("couldnt open %s\n",name);
		return NULL;
	}
	
	fstat(fd, &stbuf);
	size = stbuf.st_size;
	
	buf = (char *)malloc(size);
	if (! buf) {
		close(fd);
		return NULL;
	}
	
	if (read(fd,buf,size) < size) {
		close(fd);
		return NULL;
	}
	
	close(fd);
	
	*len = size;
	*pbuf = buf;
	return buf;
}


// Taken from <message_util.h>
/* --------------------------------------------------------------------- */
// shared defines with the app_server. 

#define	FOB_WHAT			'FOB1'

//
// flags for the overall message (the bitfield is 1 byte)
//
#define FOB_BIG_ENDIAN		0x01
#define FOB_INCL_TARGET		0x02
#define FOB_INCL_REPLY		0x04
#define FOB_SCRIPT_MSG		0x08

//
// flags for each entry (the bitfield is 1 byte)
//
#define FOB_VALID			0x01
#define FOB_MINI_DATA		0x02
#define FOB_FIXED_SIZE		0x04
#define FOB_SINGLE_ITEM		0x08

#define END_OF_ENTRIES		0x0



static void copy_into_long(int32 *dst, const unsigned char *src, bool swap)
{
	if (swap)
		*dst = __swap_int32(*((uint32 *) src));
	else
		*dst = *((uint32 *) src);
}

static void copy_from_long(unsigned char *dst, int32 src)
{
	*((uint32 *) dst) = src;
}

static void
store_param(const char *name, const char *msgstr, int32 len, param *params)
{
	param *p;

	// The first char of msgstr is the string length.
	if ((*msgstr == 0) || (*msgstr == 1)) {
		return;
	}
	msgstr++; len--;

	while ((*msgstr == 0) && (len-- > 0)) msgstr++;
	if (len < 0) {
		return;
	}
		
	for (p = params; p->name; p++) {
		if (strcmp(name,p->name) == 0) {
			p->data = strdup(msgstr);
			return;
		}
	}
}

static int
get_params_from_bmessage(const char *ptr, int buflen, param *p)
{
	status_t error = B_OK;
	bool swap = false;
	uint32 size;
	uint32 sig;
	uint32 what;
	uchar mflags;
	const uint32 kMinHdrSize = 17;
	
	const unsigned char	*buf = (const unsigned char *) ptr;
	const unsigned char	*start = buf;
	const unsigned char	*expected_end;

	if (buflen < kMinHdrSize) {
		goto bad_data;
	}
	
	copy_into_long((int32*)&sig,buf,false); buf+=4;
	if (sig == '1BOF')
		swap = true;
	
	if (swap) sig = __swap_int32(sig);
	if (sig != FOB_WHAT) {
		goto bad_data;
	}

	
	// Skip checksum.
	buf += 4;
	copy_into_long((int32*)&size,buf,swap); buf+=4;
	copy_into_long((int32*)&what,buf,swap); buf+=4;
	mflags = *buf; buf += 1;

	if (size < kMinHdrSize) {
		goto bad_data;
	}
	
	expected_end = buf + size;
	
	while (1) {
		uchar flags;
		int32 type;
		bool	is_fixed_size;
		bool	single_item;
		int32	count;
		int32	chunk_size;
		int32	logical;
		uchar	name_length;
		
		char	entry_name[64];
		char	entry_data[64];


		if (buf+1 >= expected_end) {
			goto bad_data;
		}

		flags = *buf;										buf += 1;
		if (flags == END_OF_ENTRIES)
			break;

		if (buf+4 >= expected_end) {
			goto bad_data;
		}

		copy_into_long((long*) &type, buf, swap);			buf += 4;

		is_fixed_size = ((flags & FOB_FIXED_SIZE) != 0);
		single_item = ((flags & FOB_SINGLE_ITEM) != 0);

		if ((flags & FOB_MINI_DATA) != 0) {
			if (single_item) {
				count = 1;
			} else {
				if (buf+1 >= expected_end) {
					goto bad_data;
				}
				count = *buf;								buf += 1;
			}
			if (buf+1 >= expected_end) {
				goto bad_data;
			}
			logical = *buf;									buf += 1;
		} else {
			if (single_item) {
				count = 1;
			} else {
				if (buf+4 >= expected_end) {
					goto bad_data;
				}
				copy_into_long((long*) &count, buf, swap);	buf += 4;
			}
			if (buf+4 >= expected_end) {
				goto bad_data;
			}
			copy_into_long((long*) &logical, buf, swap);	buf += 4;
		}
		
		name_length = *buf;									buf += 1;

		if ((buf + logical + name_length >= expected_end) || (logical < 0)) {
			goto bad_data;
		}
		if ((count <= 0) || (logical < count)) {
			goto bad_data;
		}

		if (is_fixed_size)
			chunk_size = logical / count;
		else
			chunk_size = 0;


		//printf("nlen=%d, dlan=%d\n", name_length, logical);
		if (type == B_STRING_TYPE) {
			memset(entry_name,0,sizeof(entry_name));
			memset(entry_data,0,sizeof(entry_data));
			
			memcpy(entry_name, buf, name_length);				buf += name_length;
			entry_name[name_length] = '\0';
	
			memcpy(entry_data, buf, logical);		buf += logical;
			//printf("%s : %s\n",entry_name,entry_data);
			
			store_param(entry_name, entry_data, logical, p);
			
		} else {
			//printf("skipping non string\n");
			buf += (logical + name_length);
		}


	}

	if ((buf - start) != size) {
		//printf("U: expected size=%d, got=%d\n", size, buf-start);
		goto bad_data;
	}

	return B_NO_ERROR;

bad_data:
	error = B_BAD_VALUE;
	printf("bad data\n");
other_error:
	return error;
	
}


