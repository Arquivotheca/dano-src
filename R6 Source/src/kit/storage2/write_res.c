/*	write_resources.c	*/

/*	This file is organically grown from recycled bytes.		*/
/*	Some day, someone is going to come and clean up this file.	*/
/*	It shouldn't be too hard. I just don't have time to do it	*/
/*	because I have other deadlines for more important stuff. 	*/


#if !defined(WRITE_RES_H)
#include "write_res.h"
#endif	/* WRITE_RES_H */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>

/* assumptions: int is 4 bytes, long is 4 bytes, short is 2 bytes, char is 1 byte */




#if defined(__POWERPC__)	/* not entirely kosher... */
static const int my_endian = 0;
#else
static const int my_endian = 1;
#endif


static int debug_level = 0;


typedef struct _res_link res_link;

struct _res_link {
	struct _res_link * next;
	unsigned int type;
	int id;
	char * name;	/* in the same block as _res_link */
	void * data;	/* malloc() */
	int size;
	off_t source_off;
	int source;
	int endian;
};

#define MAX_FILES 128

typedef struct _res_source {
	int		fd;
	int		ref_count;
} res_source;

struct _res_map {
	struct _res_link * list;
	int (*swapper)(unsigned int type, void * data, unsigned int size);
	int count;
	res_source sources[MAX_FILES];
};



static short _swap_short(unsigned char * data)
{
	unsigned char ch = data[0];
	data[0] = data[1];
	data[1] = ch;
	return *(short*)data;
}
#define SWAP_SHORT(x) _swap_short((unsigned char *)&x)

static long _swap_long(unsigned char * data)
{
	unsigned char ch = data[0];
	data[0] = data[3];
	data[3] = ch;
	ch = data[1];
	data[1] = data[2];
	data[2] = ch;
	return *(long*)data;
}
#define SWAP_LONG(x) (void)_swap_long((unsigned char *)&x)

static long long _swap_quad(unsigned char * data)
{
	unsigned char ch = data[0];
	data[0] = data[7];
	data[7] = ch;
	ch = data[1];
	data[1] = data[6];
	data[6] = ch;
	ch = data[2];
	data[2] = data[5];
	data[5] = ch;
	ch = data[3];
	data[3] = data[4];
	data[4] = ch;
	return *(long long *)data;
}


enum {
	swap_messenger = 1,
	swap_ref,
	swap_appv
};

static int get_special_type(unsigned int type, unsigned int * size)
{
	switch (type) {
	case 'MSNG':
		*size = 24;
		return swap_messenger;
	case 'APPV':
		*size = 340;
		return swap_appv;
	case 'RREF':	/* this is a real problem, as it doesn't have a fixed size */
		*size = 12+256;
		return swap_ref;
	}
	return 0;
}

int standard_swap(unsigned int type, void * data, unsigned int size)
{
	int native = 0;	/* size of uniform type elements (2, 4, 8) */
	int special = 0;	/* for non-uniform types (see above enum) */

	if (debug_level > 3) {
		fprintf(stderr, "DEBUG: standard_swap('%c%c%c%c', %d)\n", 
			type>>24, type>>16, type>>8, type, size);
	}
	switch (type) {
	/* special types go here */
	case 'APPF':
		native = 4;
		break;
	case 'APPV':
		special = swap_appv;
		break;
	case 'DBLE':	/* B_DOUBLE_TYPE */
		native = 8;
		break;
	case 'FLOT':	/* B_FLOAT_TYPE */
		native = 4;
		break;
	case 'LLNG':	/* B_INT64_TYPE */
		native = 8;
		break;
	case 'LONG':	/* B_INT32_TYPE */
		native = 4;
		break;
	case 'SHRT':	/* B_INT16_TYPE */
		native = 2;
		break;
	case 'MSNG':	/* B_MESSENGER_TYPE */
		special = swap_messenger;
		break;
	case 'OFFT':	/* B_OFF_T_TYPE */
		native = 8;
		break;
	case 'BPNT':	/* B_POINT_TYPE */
		native = 4;
		break;
	case 'RECT':	/* B_RECT_TYPE */
		native = 4;
		break;
	case 'RREF':	/* B_REF_TYPE */
		special = swap_ref;
		break;
	case 'SIZT':	/* B_SIZE_T_TYPE */
		native = 4;
		break;
	case 'SSZT':	/* B_SSIZE_T_TYPE */
		native = 4;
		break;
	case 'TIME':	/* B_TIME_TYPE */
		native = 4;
		break;
	case 'ULLG':	/* B_UINT64_TYPE */
		native = 8;
		break;
	case 'ULNG':	/* B_UINT32_TYPE */
		native = 4;
		break;
	case 'USHT':	/* B_UINT16_TYPE */
		native = 2;
		break;
	}
	if (special) {
		unsigned char * ptr = (unsigned char *)data;
		unsigned char * end = (unsigned char *)ptr+size;
		if (special == swap_messenger) {
			while (ptr < end) {
				_swap_long(ptr);
				_swap_long(ptr+4);
				_swap_long(ptr+8);
				_swap_long(ptr+12);
				_swap_long(ptr+16);
				/* don't swap last longword */
				ptr += 24;
			}
		}
		else if (special == swap_ref) {
			while (ptr < end) {
				_swap_long(ptr);
				_swap_quad(ptr+4);
				ptr = ptr+strlen((char *)ptr+12)+1;
			}
		}
		else if (special == swap_appv) {
			while (ptr < end) {
				_swap_long(ptr);
				_swap_long(ptr+4);
				_swap_long(ptr+8);
				_swap_long(ptr+12);
				_swap_long(ptr+16);
				ptr += 20+64+256;
			}
		}
		else {
			return 0;
		}
	}
	else if (native) {
		unsigned char * ptr = (unsigned char *)data;
		unsigned char * end = (unsigned char *)ptr+size;
		if (native == 2) {
			while (ptr < end) {
				_swap_short(ptr);
				ptr += 2;
			}
		}
		else if (native == 4) {
			while (ptr < end) {
				_swap_long(ptr);
				ptr += 4;
			}
		}
		else if (native == 8) {
			while (ptr < end) {
				_swap_quad(ptr);
				ptr += 8;
			}
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}
	return 1;
}


static int verify_resources(res_map * rmap)
{
	struct stat stbuf;
	int ix;
	res_link * list;
	int count;
	int corr = 0;

	fsync(fileno(stderr));
	sync();
	if (rmap == NULL) {
		if (debug_level > 0) fprintf(stderr, "verify_resources(): NULL resource map\n");
		return -1;
	}
	if (rmap->list == NULL) {
		if (debug_level > 0) fprintf(stderr, "verify_resources(): NULL resource list\n");
		/* but that's OK if there are no resources in it */
	}
	for (ix=0; ix<MAX_FILES; ix++) {
		if (rmap->sources[ix].fd >= 0) {
			if (rmap->sources[ix].ref_count <= 0) {
				if (debug_level > 0) fprintf(stderr, "verify_resources(): corrupt source ix %d fd %d ref_count %d\n", 
					ix, rmap->sources[ix].fd, rmap->sources[ix].ref_count);
				corr = -1;
			}
			if (fstat(rmap->sources[ix].fd, &stbuf) < 0) {
				if (debug_level > 0) fprintf(stderr, "verify_resources(): cannot stat source ix %d fd %d ref_count %d\n", 
					ix, rmap->sources[ix].fd, rmap->sources[ix].ref_count);
				corr = -1;
			}
		}
		else if (rmap->sources[ix].ref_count > 0) {
			if (debug_level > 0) fprintf(stderr, "verify_sources(): ref_count is %d with fd %d (ix %d)\n", 
				rmap->sources[ix].ref_count, rmap->sources[ix].fd, ix);
			corr = -1;
		}
	}
	list = rmap->list;
	count = 0;
	if (debug_level > 8) {
		fprintf(stderr, "entry     type          id     name    size     data  e src   offset\n");
		fprintf(stderr, "--------  ---- ----------- --------  ------ --------  - --- --------\n");
	}
	while (list != NULL) {
		list = list->next;
		count++;
		if (count > 2*rmap->count+1) {
			if (debug_level > 0) fprintf(stderr, 
				"verify_resources(): map says there should be %d; found %d so far. I'd better stop now.\n", 
				rmap->count, count);
			corr = -1;
			break;
		}
	}
	if (debug_level > 6) fprintf(stderr, "verify_resources(): found %d resources\n", count);
	return corr;
}


static int increase_ref(int fd, res_map * rmap)
{
	int ix;
	int ff = -1;
	struct stat a, b;

	if (fstat(fd, &a) < 0) {
		if (debug_level > 0) fprintf(stderr, "increase_ref(): fstat(%d) failed\n", fd);
		return -1;
	}
	for (ix=0; ix<MAX_FILES; ix++) {
		if (rmap->sources[ix].fd < 0) {
			if (ff == -1) {
				ff = ix;
			}
		}
		else {
			if (fstat(rmap->sources[ix].fd, &b) < 0) {
				if (debug_level > 0) fprintf(stderr, "increase_ref(): fstat() on existing %d failed\n", rmap->sources[ix].fd);
				return -1;
			}
			if ((a.st_dev == b.st_dev) && (a.st_ino == b.st_ino)) {
				rmap->sources[ix].ref_count++;
				return ix;	/* wee, a match! */
			}
		}
	}
	if (ff == -1) {	/* boo, no free space */
		if (debug_level > 0) fprintf(stderr, "increase_ref(): only %d files allowed\n", MAX_FILES);
		return -1;
	}
	rmap->sources[ff].fd = dup(fd);
	if (rmap->sources[ff].fd < 0) {
		if (debug_level > 0) fprintf(stderr, "increase_ref(): dup(%d) failed\n", fd);
		return -1;
	}
	rmap->sources[ff].ref_count = 1;
	if (debug_level > 5) verify_resources(rmap);
	return ff;
}

static int decrease_ref(res_link * list, res_map * rmap)
{
	int err = 0;
	if (!list || !rmap) {
		if (debug_level > 0) fprintf(stderr, "decrease_ref(): NULL argument\n");
		return -1;
	}
	if (list->source < 0) {
		if (debug_level > 0) fprintf(stderr, "decrease_ref(): resource entry doesn't reference anything\n");
		return -1;
	}
	if (--(rmap->sources[list->source].ref_count) == 0) {
		if (rmap->sources[list->source].fd >= 0) {
			if (debug_level > 3) fprintf(stderr, "TRACE: decrease_ref() closes fd %d (source %d)\n", 
				rmap->sources[list->source].fd, list->source);
			close(rmap->sources[list->source].fd);
			rmap->sources[list->source].fd = -1;
		}
		else {
			if (debug_level > 0) fprintf(stderr, "decrease_ref(): referenced file descriptor is already closec\n");
			err = -1;
		}
	}
	list->source = -1;
	if (debug_level > 5) verify_resources(rmap);
	return err;
}


static int load_resource(res_link * list, res_map * rmap)
{
	off_t w;
	struct stat stbuf;
	int sfd;
	int ret;

	if (list->data) {
		return 0;		/* already loaded */
	}
	if (list->source < 0) {
		if (debug_level > 0) fprintf(stderr, "load_resource(): corrupt resource entry (source)\n");
		return -1;	/* there is no source for the resource */
	}
	if (list->source_off <= 0) {
		if (debug_level > 0) fprintf(stderr, "load_resource(): corrupt resource entry (offset)\n");
		return -1;
	}
	sfd = rmap->sources[list->source].fd;
	if (fstat(sfd, &stbuf) < 0) {
		if (debug_level > 0) fprintf(stderr, "load_resource(): cannot fstat(%d)\n", sfd);
		return -1;
	}
	list->data = malloc(list->size);
	if (!list->data) {
		if (debug_level > 0) fprintf(stderr, "load_resource(): malloc(%d) failed\n", list->size);
		return -1;	/* out of memory */
	}
	w = lseek(sfd, list->source_off, SEEK_SET);
	if (w != list->source_off) {
		if (debug_level > 0) fprintf(stderr, "load_resource(): cannot lseek()\n");
		return -1;
	}
	w = read(sfd, list->data, list->size);
	if (w != list->size) {
		free(list->data);
		list->data = NULL;
		return -1;
	}
	if (list->endian != my_endian) {
		(*rmap->swapper)(list->type, list->data, list->size);
	}
	ret = decrease_ref(list, rmap);
	if (debug_level > 5) verify_resources(rmap);
	return ret;
}


int add_resource(res_map ** rmap, unsigned int type, int id, const void * data, int size, const char * name)
{
	unsigned int prev = type+1;	/* so we know it's different */
	res_link * n;
	int slotsize = sizeof(res_link)+(name ? strlen(name) : 0)+1;
	res_link ** list;
	if (!*rmap) {
		if (new_resource_map(rmap, standard_swap) < 0) {
			return -1;
		}
	}
	list = &(*rmap)->list;
	n = (res_link *)malloc(slotsize);
	if (!n) {
		if (debug_level > 0) fprintf(stderr, "add_resource malloc(%d) failed\n", slotsize);
		return -1;
	}
	n->data = malloc(size);
	if (!n->data) {
		free(n);
		if (debug_level > 0) fprintf(stderr, "add_resource malloc(%d) failed\n", size);
		return -1;
	}
	while (*list != NULL) {
		if ((prev == type) && ((*list)->type != type)) {
			break;	/* insert at end of type's entries */
		}
		prev = (*list)->type;
		if ((prev == type) && ((*list)->id == id)) {	/* replace same ID */
			res_link * d = *list;
			*list = d->next;
			if (d->source >= 0) {
				decrease_ref(d, *rmap);
			}
			free(d->data);
			free(d);
			break;
		}
		list = &(*list)->next;
	}
	n->next = *list;
	n->type = type;
	n->id = id;
	n->name = (char *)&n[1];
	if (!name) {
		n->name[0] = 0;
	}
	else {
		strcpy(n->name, name);
	}
	if (data) {
		memcpy(n->data, data, size);
	}
	n->size = size;
	n->source = -1;
	n->source_off = -1;
	n->endian = my_endian;
	*list = n;
	(*rmap)->count++;

	if (debug_level > 5)
		verify_resources(*rmap);
	return 0;
}


static int add_unloaded_resource(res_map * rmap, int fd, off_t offset, int endian, unsigned int type, int id, int size, const char * name)
{
	unsigned int prev = type+1;	/* so we know it's different */
	res_link * n;
	int slotsize = sizeof(res_link)+(name ? strlen(name) : 0)+1;
	res_link ** list;

	list = &rmap->list;
	n = (res_link *)malloc(slotsize);
	if (!n) {
		if (debug_level > 0) fprintf(stderr, "add_unloaded_resource malloc(%d) failed\n", slotsize);
		return -1;
	}
	n->data = NULL;
	while (*list != NULL) {
		if ((prev == type) && ((*list)->type != type)) {
			break;	/* insert at end of type's entries */
		}
		prev = (*list)->type;
		if ((prev == type) && ((*list)->id == id)) {	/* replace same ID */
			res_link * d = *list;
			*list = d->next;
			if (d->source >= 0) {
				decrease_ref(d, rmap);
			}
			free(d->data);
			free(d);
			break;
		}
		list = &(*list)->next;
	}
	n->next = *list;
	n->type = type;
	n->id = id;
	n->name = (char *)&n[1];
	strcpy(n->name, name);
	n->size = size;
	n->source = increase_ref(fd, rmap);
	n->source_off = offset;
	n->endian = endian;
	if (n->source < 0) {
		free(n);
		return -1;
	}
	*list = n;	/* insert */
	rmap->count++;
	if (debug_level > 5)
		verify_resources(rmap);
	return 0;
}


const void * find_resource_by_id(res_map * rmap, unsigned int type, int id, int * out_size, const char ** out_name)
{
	res_link * list;

	if (!rmap) return NULL;
	list = rmap->list;

	while (list != NULL) {
		if (debug_level > 2) fprintf(stderr, "DEBUG: considering %c%c%c%c %d\n", 
			list->type>>24, list->type>>16, list->type>>8, list->type, list->id);
		if ((list->type == type) && (list->id == id)) {
			if (out_size) {
				*out_size = list->size;
			}
			if (out_name) {
				*out_name = list->name;
			}
			if (!list->data) {
				(void)load_resource(list, rmap);
			}
			if (debug_level > 5) verify_resources(rmap);
			return list->data;
		}
		list = list->next;
	}
	if (debug_level > 5) verify_resources(rmap);
	return NULL;
}

const void * find_resource_by_name(res_map * rmap, unsigned int type, const char * name, int * out_size, int * out_id)
{
	res_link * list;

	if (!rmap) return NULL;
	list = rmap->list;

	while (list != NULL) {
		if (debug_level > 2) fprintf(stderr, "DEBUG: considering %c%c%c%c %s\n", 
			list->type>>24, list->type>>16, list->type>>8, list->type, list->name);
		if ((list->type == type) && name && list->name && !strcmp(list->name, name)) {
			if (out_size) {
				*out_size = list->size;
			}
			if (out_id) {
				*out_id = list->id;
			}
			if (!list->data) {
				(void)load_resource(list, rmap);
			}
			if (debug_level > 5) verify_resources(rmap);
			return list->data;
		}
		list = list->next;
	}
	if (debug_level > 5) verify_resources(rmap);
	return NULL;
}

int count_resources(res_map * rmap)
{
	if (!rmap) {
		return -1;
	}
	return rmap->count;
}

const void * find_resource_by_index(res_map * rmap, int index, unsigned int * type, int * id, int * out_size, const char ** out_name)
{
	res_link * list;
	int cnt = 0;

	if (rmap == NULL) return NULL;
	list = rmap->list;

	if (index < 0) {
		return NULL;
	}
	while (list != NULL) {
		if (cnt == index) {
			if (type) {
				*type = list->type;
			}
			if (id) {
				*id = list->id;
			}
			if (out_size) {
				*out_size = list->size;
			}
			if (out_name) {
				*out_name = list->name;
			}
			if (debug_level > 5) verify_resources(rmap);
			return list->data;
		}
		cnt++;
		list = list->next;
	}
	if (debug_level > 5) verify_resources(rmap);
	return NULL;
}

int remove_resource(res_map * rmap, const void * resource)
{
	res_link ** list;

	if (!rmap) return -1;
	if (!resource) {
		if (debug_level > 0) fprintf(stderr, "remove_resource(): NULL resource\n");
		return -1;
	}
	list = &rmap->list;
	while (*list != NULL) {
		if ((*list)->data == resource) {
			res_link * d = *list;
			*list = (*list)->next;
			/* since it has data, there is no ref to decrease */
			free(d->data);
			free(d);
			rmap->count--;
			if (debug_level > 5) verify_resources(rmap);
			return 0;
		}
		list = &(*list)->next;
	}
	if (debug_level > 0) fprintf(stderr, "remove_resource(): not found\n");
	if (debug_level > 5) verify_resources(rmap);
	return -1;
}

int remove_resource_id(res_map * rmap, unsigned int type, int id)
{
	res_link **list;

	if (!rmap) return -1;
	list = &rmap->list;
	while (*list != NULL) {
		if (((*list)->type == type) && ((*list)->id == id)) {
			res_link * d = *list;
			*list = (*list)->next;
			if (d->source >= 0) {
				decrease_ref(d, rmap);
			}
			free(d->data);
			free(d);
			rmap->count--;
			if (debug_level > 5) verify_resources(rmap);
			return 0;
		}
		list = &(*list)->next;
	}
	if (debug_level > 0) fprintf(stderr, "remove_resource(): not found\n");
	if (debug_level > 5) verify_resources(rmap);
	return -1;
}

int replace_resource_data(res_map * rmap, const void * resource, void * new_resource, int size)
{
	res_link ** list;

	if (!rmap) return -1;
	if (!resource || !new_resource) {
		if (debug_level > 0) fprintf(stderr, "replace_resource_data(): NULL resource\n");
		return -1;
	}
	list = &rmap->list;
	while (*list != NULL) {
		if ((*list)->data == resource) {
			free((*list)->data);
			(*list)->data = new_resource;
			(*list)->size = size;
			if (debug_level > 5) verify_resources(rmap);
			return 0;
		}
		list = &(*list)->next;
	}
	if (debug_level > 0) fprintf(stderr, "remove_resource(): not found\n");
	if (debug_level > 5) verify_resources(rmap);
	return -1;
}




/* file I/O */


typedef struct {
	int offset;
	int size;
	int type;
} df_entry;

typedef struct  {
	int	signature;
	int	record_count;
	int    offset_table_ptr;
	int    hole_table_ptr;
	int    free_entry_hint;
	int	dirty_count;
	int	unique;
	int    reserved[10];
}       df_header;


typedef struct pef_cheader {
	short	magic1;
	short	magic2;
	int		cont_id;
	int		arch_id;
	int		version;
	int		mac_time_stamp;
	int		od_version;
	int		oi_version;
	int		c_version;
	short	snum;
	short	slnum;
	char	*addr;
} pef_cheader;

typedef struct pef_sheader {
	int		name;
	char	*addr;
	int		exec_size;
	int		init_size;
	int		raw_size;
	int		offset;
	char	kind;
	char	alignment;
	char	sharing;
	char	reserved;
} pef_sheader;

#define	PEF_MAGIC1	0x4a6f
#define	PEF_MAGIC2	0x7921
#define	RES_CONTID	'resf'
#define	PEF_CONTID	0x70656666		/* 'peff' */


#if defined(__ELF__)

char file_header[4] = "RS";

#elif defined(__POWERPC__)

struct pef_cheader file_header = {
	PEF_MAGIC1, PEF_MAGIC2,
	RES_CONTID
};

#else
	#error port it already!
#endif

#define INITIAL_DIR_SIZE	128		/* must be a mult of DIR_GROW  */
#define INITIAL_HOLE_SIZE	30		/* must be a mult of HOLE_GROW */
#define SYSTEM_ENTRY	1001
#define RESOURCE_ENTRY	0
#define RESERVED_BLOCKS	10
#define SIGNATURE		0x444f
#define VERSION		0x1000

static int
checksum(const char *buffer, int size)	/* I didn't write this, don't blame me! -- hplus */
{
	const unsigned char	*buf = (const unsigned char *)buffer;
	unsigned int temp;
	unsigned int sum = 0;

	while (size) {
		temp = 0;
		if (size > 3) {
			temp = *buf++;
			temp = (temp << 8) + *buf++;
			temp = (temp << 8) + *buf++;
			temp = (temp << 8) + *buf++;
			size -= 4;
		} else {
			while (size) {
				temp = (temp << 8) + *buf++;
				size -= 1;
			}
		}
		sum += temp;
	}
	return sum;
}


static void
mappend(
	char ** ptr,
	int * size,
	int * phys,
	const void * data,
	int dsize)
{
	if (dsize+*size > *phys) {
		int nphys = *phys + dsize + 1000;
		char * nptr = (char *)realloc(*ptr, nphys);
		if (!nptr) {
			if (debug_level > 0) fprintf(stderr, "out of memory in mappend()\n");
			exit(1);
		}
		*ptr = nptr;
		*phys = nphys;
	}
	memcpy((*ptr)+*size, data, dsize);
	*size += dsize;
}

static int
calc_nlen(
	const char * str)
{
	if (str == NULL) return 0;
	if (*str == 0) return 0;
	return strlen(str)+1;
}

static int
copy_file_data(
	res_link * list,
	res_map * map,
	int fd, 
	int do_swap,
	unsigned int type,
	int (*swapper)(unsigned int type, void * data, unsigned int size))
{
	char temp[512];
	char * ptr = temp;
	int ptrsize = 512;
	int rd;
	int wr;
	int togo;
	off_t cur;
	unsigned int special_size;

	if ((list->source < 0) || (map->sources[list->source].fd < 0)) {
		if (debug_level > 0) fprintf(stderr, "copy_file_data(): bad resource file reference\n");
		return -1;
	}
	if (list->size > ptrsize) {
		ptrsize = list->size;
		if (list->size > 128*1024L) {
			ptrsize = 128*1024L;
		}
		ptr = malloc(ptrsize);
		if (!ptr) {
			ptrsize = 512;
			ptr = temp;
		}
	}
	if (get_special_type(type, &special_size)) {
		ptrsize = ptrsize - (ptrsize % special_size);
	}
	else { /* align on 8 bytes */
		ptrsize = ptrsize&~7;
	}
	assert(ptrsize > 0);
	if (ptrsize <= 0) {
		if (debug_level > 0) fprintf(stderr, "copy_file_data(): ptrsize is %d!!!\n", ptrsize);
		if (ptr != temp) free(ptr);
		return -1;
	}
	cur = lseek(fd, 0, SEEK_CUR);
	if (cur < 0) {
		if (debug_level > 0) fprintf(stderr, "copy_file_data(): cannot get file size\n");
		if (ptr != temp) free(ptr);
		return cur;
	}
	togo = list->size;
	while (togo > 0) {
		if (lseek(map->sources[list->source].fd, list->source_off+list->size-togo, SEEK_SET) < 0) {
			if (debug_level > 0) fprintf(stderr, "copy_file_data(): cannot lseek() source file\n");
			if (ptr != temp) free(ptr);
			return list->size-togo;
		}
		if (togo < ptrsize) {
			ptrsize = togo;
		}
		rd = read(map->sources[list->source].fd, ptr, ptrsize);
		if (rd != ptrsize) {
			if (debug_level > 0) fprintf(stderr, "copy_file_data(): cannot read() source file\n");
			if (ptr != temp) free(ptr);
			return list->size-togo;
		}
		if (do_swap) {
			(*swapper)(list->type, ptr, rd);
		}
		if (lseek(fd, cur+list->size-togo, SEEK_SET) < 0) {
			if (debug_level > 0) fprintf(stderr, "copy_file_data(): cannot lseek() destination file\n");
			if (ptr != temp) free(ptr);
			return list->size-togo;
		}
		wr = write(fd, ptr, rd);
		if (wr != rd) {
			if (debug_level > 0) fprintf(stderr, "copy_file_data(): cannot write() destination file\n");
			if (ptr != temp) free(ptr);
			return list->size-togo+(wr > 0 ? wr : 0);
		}
		togo -= wr;
	}
	if (ptr != temp) {
		free(ptr);
	}
	return list->size;
}

static int
write_data_make_map(
	int fd,
	res_map * rmap,
	long offset,
	df_entry * ptr,
	int endian,
	int (*swapper)(unsigned int type, void * data, unsigned int size),
	char ** map,
	int * size)
{
	int phys;
	unsigned int old_type;
	int curblock;
	int csum;
	int zero;
	res_link * link = rmap->list;

	phys = 4096;
	*map = (char *)calloc(4096, 1);
	if (!*map) {
		if (debug_level > 0) fprintf(stderr, "write_data_make_map() calloc() failed\n");
		return -1;
	}
	*size = 0;
	old_type = link ? link->type+1 : -1; /* so it's different */
	curblock = RESERVED_BLOCKS+1;
	while (link != NULL) {
		long pos = lseek(fd, 0, SEEK_CUR);
		int w;
		int userblock;
		short nlen;
		int swapped = 0;
		int temp;

		if (link->data) {
			/* swap user data when going out */
			if (endian != my_endian) {
				swapped = (*swapper)(link->type, link->data, link->size);
			}
	 		w = write(fd, link->data, link->size);
	 		if (swapped) {	/* but preserve in memory */
	 			(*swapper)(link->type, link->data, link->size);
	 		}
			if (w != link->size) {
				if (debug_level > 0) fprintf(stderr, "write_data_make_map() data write() failed: %d\n", w);
			}
	 	}
	 	else {
			w = copy_file_data(link, rmap, fd, endian != link->endian, link->type, swapper);
	 	}
		if (w != link->size) {
			return -1;
		}
		if (link->type != old_type) {
			int id = -1;
			if (*size > 0) {
				if (endian != my_endian) {
					SWAP_LONG(id);
				}
				mappend(map, size, &phys, &id, 4);	/* terminate this type */
				mappend(map, size, &phys, &id, 4);
			}
			old_type = link->type;
			id = old_type;
			if (endian != my_endian) {
				if (debug_level > 2) fprintf(stderr, "DEBUG: swapping type from %c%c%c%c\n", id>>24, id>>16, id>>8, id);
				SWAP_LONG(id);
			}
			mappend(map, size, &phys, &id, 4);
		}
		temp = link->id;
		if (endian != my_endian) {
			SWAP_LONG(temp);
		}
		mappend(map, size, &phys, &temp, 4);
		userblock = curblock-RESERVED_BLOCKS;
		if (endian != my_endian) {
			SWAP_LONG(userblock);
		}
		mappend(map, size, &phys, &userblock, 4);
		ptr[curblock].offset = pos-offset;
		ptr[curblock].size = link->size;
		ptr[curblock].type = RESOURCE_ENTRY;
		curblock++;
		nlen = calc_nlen(link->name);
		if (endian != my_endian) {
			SWAP_SHORT(nlen);
		}
		mappend(map, size, &phys, &nlen, 2);
		if (endian != my_endian) {
			SWAP_SHORT(nlen); /* for use below */
		}
		if (nlen > 0) {
			mappend(map, size, &phys, link->name, nlen);
		}
		link = link->next;
	}
	{
		int id = -1;
		if (endian != my_endian) {
			SWAP_LONG(id);
		}
		mappend(map, size, &phys, &id, 4);
		mappend(map, size, &phys, &id, 4);
	}
	csum = checksum(*map, *size);
	if (endian != my_endian) {
		SWAP_LONG(csum);
	}
	mappend(map, size, &phys, &csum, 4);
	zero = 0;
	SWAP_LONG(zero);
	mappend(map, size, &phys, &zero, 4);
	return 0;
}


static int
preload_resources(
	res_map * map,
	int fd)
{
	struct stat a, b;
	int ix;

	if (fstat(fd, &a) < 0) {
		if (debug_level > 0) fprintf(stderr, "preload_resources(): fstat(%d) failed\n", fd);
		return -1;
	}
	/* look for resources that are latent in a file that is the same as the output file */
	for (ix=0; ix<MAX_FILES; ix++) {
		if (map->sources[ix].fd >= 0) {
			if (fstat(map->sources[ix].fd, &b) < 0) {
				if (debug_level > 0) fprintf(stderr, "preload_resources(): fstat() esiting %d filed\n", 
					map->sources[ix].fd);
			}
			else if ((a.st_dev == b.st_dev) && (a.st_ino == b.st_ino)) {
				res_link * list = map->list;
				while (list != NULL) {
					if (list->source == ix) {
						if (load_resource(list, map) < 0) {
							return -1;
						}
					}
					list = list->next;
				}
				break;	/* so we found the fd that was identical */
			}
		}
	}
	if (debug_level > 5) verify_resources(map);
	return 0;
}


int
write_resource_file(
	res_map * rmap, 
	int fd,
	int endian,
	int (*swapper)(unsigned int type, void * data, unsigned int size))
{
	off_t hdrpos = 0;
	int dir_size = INITIAL_DIR_SIZE;
	int w;
	int count;
	df_header header;
	df_entry * ptr;
	int ix;

	if (debug_level > 5) verify_resources(rmap);
	if (endian != my_endian) {
		if (debug_level > 2) fprintf(stderr, "DEBUG: swapping in effect for writing\n");
	}
	if (swapper == NULL) {
		swapper = rmap->swapper;
	}
	if (rmap == NULL) {
		if (debug_level > 0) fprintf(stderr, "write_resource_file() resource map is NULL\n");
		return -1;
	}
#if 0
	if (rmap->list == NULL) {
		if (debug_level > 0) fprintf(stderr, "write_resource_file() resources is NULL\n");
		return -1;
	}
#endif
	hdrpos = lseek(fd, 0, SEEK_CUR);
	if (debug_level > 2) fprintf(stderr, "header found at pos %Ld\n", hdrpos);

	if (preload_resources(rmap, fd) < 0) {
		return -1;
	}

	count = count_resources(rmap);
	while (dir_size < count+RESERVED_BLOCKS+1) {
		dir_size += INITIAL_DIR_SIZE;
	}

	memset(&header, 0, sizeof(header));
	header.signature = (SIGNATURE << 16) | VERSION;
	header.record_count = count;
	header.offset_table_ptr = sizeof(df_header);
	header.hole_table_ptr = sizeof(df_header)+sizeof(df_entry)*dir_size;
	header.free_entry_hint = 0;
	header.dirty_count = 0;
	header.unique = 0;

	if (endian != my_endian) {
		if (debug_level > 4) fprintf(stderr, "DEBUG: swapping resource header (output)\n");
		standard_swap('LONG', &header, sizeof(header));
	}

	lseek(fd, hdrpos, SEEK_SET);	/* seek back after loading resources */
	if ((w = write(fd, &header, sizeof(header))) != sizeof(header)) {
		if (debug_level > 0) fprintf(stderr, "write_resource_file() resource header write() failed: %d\n", w);
		return -1;
	}

	if (endian != my_endian) {	/* swap back for use below */
		standard_swap('LONG', &header, sizeof(header));
	}

	ptr = (df_entry *)calloc(sizeof(df_entry), dir_size);
	for (ix=0; ix<dir_size; ix++) {
		ptr[ix].offset = 0;
		ptr[ix].size = -1;
		ptr[ix].type = SYSTEM_ENTRY;
	}
	ptr[0].offset = header.offset_table_ptr;
	ptr[0].size = sizeof(df_entry)*dir_size;
	ptr[1].offset = header.hole_table_ptr;
	ptr[1].size = sizeof(df_entry)*INITIAL_HOLE_SIZE;
	if (endian != my_endian) {
		standard_swap('LONG', ptr, dir_size*sizeof(df_entry));
	}
	if ((w = write(fd, ptr, sizeof(df_entry)*dir_size)) != (ssize_t)sizeof(df_entry)*dir_size) {
		if (debug_level > 0) fprintf(stderr, "write_resource_file() chunk rmap write() failed: %d\n", w);
		free(ptr);
		return -1;
	}
	if ((w = write(fd, &ptr[2], INITIAL_HOLE_SIZE*sizeof(df_entry))) != (ssize_t)sizeof(df_entry)*INITIAL_HOLE_SIZE) {
		if (debug_level > 0) fprintf(stderr, "write_resource_file() hole rmap write() failed: %d\n", w);
		free(ptr);
		return -1;
	}
	if (endian != my_endian) {
		/* swap back because we use this array later down */
		standard_swap('LONG', ptr, dir_size*sizeof(df_entry));
	}

	if (count >= 0) {
		char * map;
		int size;
		off_t file_end = 0;
		if (write_data_make_map(fd, rmap, hdrpos, ptr, endian, swapper, &map, &size) < 0) {
			free(ptr);
			return -1;
		}
		/* write map block */
		ptr[RESERVED_BLOCKS].size = size;
		ptr[RESERVED_BLOCKS].offset = lseek(fd, 0, SEEK_CUR)-hdrpos;
		/* map is swapped when made */
		if ((w = write(fd, map, size)) != size) {
			if (debug_level > 0) fprintf(stderr, "write_resource_file() resource map write() failed: %d\n", w);
			free(map);
			free(ptr);
			return -1;
		}
		free(map);
		file_end = lseek(fd, 0, SEEK_CUR);
		if (file_end > 0) {
			ftruncate(fd, file_end);	/* no more growing resource files */
		}
		lseek(fd, hdrpos+sizeof(df_header), SEEK_SET);
		if (endian != my_endian) {
			if (debug_level > 2) fprintf(stderr, "DEBUG: first entry before write swap: off %x size %x type %x\n", 
				ptr[0].offset, ptr[0].size, ptr[0].type);
			standard_swap('LONG', ptr, dir_size*sizeof(df_entry));
			if (debug_level > 2) fprintf(stderr, "DEBUG: first entry after write swap: off %x size %x type %x\n", 
				ptr[0].offset, ptr[0].size, ptr[0].type);
		}
		if ((w = write(fd, ptr, sizeof(df_entry)*dir_size)) != (ssize_t)sizeof(df_entry)*dir_size) {
			if (debug_level > 0) fprintf(stderr, "write_resource_file() chunk map update write() failed: %d\n", w);
			free(ptr);
			return -1;
		}
		free(ptr);
	}
	else {
		if (debug_level > 0) fprintf(stderr, "resource count is < 0\n");
	}
	fsync(fd);
	if (debug_level > 5) verify_resources(rmap);
	return 0;
}


static char *
read_block(
	int fd,
	df_entry block,
	off_t file_delta)
{
	char * data;
	int w;

	if ((block.offset < (ssize_t)sizeof(df_header)) || (block.size < 0)) {
		if (debug_level > 0) fprintf(stderr, "read_block(): bogus block info\n");
		return NULL;
	}
	data = (char *)malloc(block.size ? block.size : 1);	/* malloc(0) may return NULL */
	if (!data) {
		if (debug_level > 0) fprintf(stderr, "read_block(): malloc(%d) failed\n", block.size);
		return NULL;
	}
	if (lseek(fd, block.offset+file_delta, SEEK_SET) < 0) {
		if (debug_level > 0) fprintf(stderr, "read_block(): lseek() failed\n");
		free(data);
		return NULL;
	}
	if ((w = read(fd, data, block.size)) != block.size) {
		if (debug_level > 0) fprintf(stderr, "read_block(): read() failed\n");
		free(data);
		return NULL;
	}
	return data;
}


static int
parse_resource_map(
	int fd,
	df_entry * blocks,
	int num_blocks,
	int endian,
	char * map,
	int map_size,
	off_t file_delta, 
	res_link ** list,
	res_map * rmap)
{
	int check;
	int flag;
	unsigned int type;
	int id;
	int block;
	short name_size;
	char * end;

	if (!blocks || !map || (map_size < 12) || !list) {
		if (debug_level > 0) fprintf(stderr, "parse_resource_map() bad arguments\n");
		return -1;
	}
	memcpy(&flag, map+map_size-4, 4);
	if (endian != my_endian) {
		SWAP_LONG(flag);
	}
	switch (flag) {
	case 0:
		map_size -= 8;
		memcpy(&check, map+map_size, 4);
		if (endian != my_endian) {
			SWAP_LONG(check);
		}
		if (check != checksum(map, map_size)) {
			if (debug_level > 0) fprintf(stderr, "parse_resource_map(): map checksum is bad\n");
			return -1;
		}
		break;
	case -1:
		/* old format, no checksum */
		break;
	default:
		if (debug_level > 0) fprintf(stderr, "parse_resource_map(): map terminated with %d ???\n", flag);
		return -1;
	}
	end = map + map_size;
	if (map_size > 8) while (map < end) {
		if (map+4 > end) goto corrupt;
		memcpy(&type, map, 4);
		if (endian != my_endian) {
			SWAP_LONG(type);
		}
		map += 4;
		while (map < end) {
			if (map+4 > end) goto corrupt;
			memcpy(&id, map, 4);
			if (endian != my_endian) {
				SWAP_LONG(id);
			}
			map += 4;
			if (map+4 > end) goto corrupt;
			memcpy(&block, map, 4);
			if (endian != my_endian) {
				SWAP_LONG(block);
			}
			map += 4;
			if (block == -1) {
				break;
			}
			if ((block < 0) || (block >= num_blocks-RESERVED_BLOCKS)) {
				goto corrupt;
			}
			if (map+2 > end) goto corrupt;
			memcpy(&name_size, map, 2);
			if (endian != my_endian) {
				SWAP_SHORT(name_size);
			}
			map += 2;
			if (map+name_size > end) goto corrupt;
#if LOAD_RESOURCES
			data = read_block(fd, blocks[RESERVED_BLOCKS+block], file_delta);
			if (data) {
				if (endian != my_endian) {
					if (debug_level > 2) fprintf(stderr, "DEBUG: calling swapper for %c%c%c%c %d\n", 
						type>>24, type>>16, type>>8, type, id);
					(*swapper)(type, data, blocks[RESERVED_BLOCKS+block].size);
				}
				add_resource(list, type, id, data, blocks[RESERVED_BLOCKS+block].size, name_size ? map : "");
				free(data);
			}
			else {
				if (debug_level > 0) fprintf(stderr, "parse_resource_map(): skipping resource '%c%c%c%c' %d\n", 
					type>>24, type>>16, type>>8, type, id);
			}
#else
			add_unloaded_resource(rmap, fd, blocks[RESERVED_BLOCKS+block].offset+file_delta, endian, type, id, blocks[RESERVED_BLOCKS+block].size, name_size ? map : "");
#endif
			map += name_size;
		}
	}
	if (debug_level > 5) verify_resources(rmap);
	return 0;
corrupt:
	if (debug_level > 0) fprintf(stderr, "parse_resource_map(): corrupt resource map detected!\n");
	return -1;
}


static void
check_debug()
{
	if (!debug_level) {
		const char * ptr = getenv("RESOURCE_DEBUG");
		if (ptr) {
			if (!strcmp(ptr, "trace")) {
				debug_level = 2;
			}
			else if (!strcmp(ptr, "debug")) {
				debug_level = 3;
			}
			else if (!strcmp(ptr, "errors")) {
				debug_level = 1;
			}
			else if (!strcmp(ptr, "full")) {
				debug_level = 9;
			}
			else {
				debug_level = atoi(ptr);
			}
			if (!debug_level) {
				debug_level = 1;
			}
		}
		else {
			debug_level = -1;
		}
	}
}


int
new_resource_map(
	res_map ** rmap,
	int (*swapper)(unsigned int type, void * data, unsigned int size))
{
	int ix;

	check_debug();
	*rmap = (res_map *)malloc(sizeof(res_map));
	if (!*rmap) 
		return -1;

	(*rmap)->list = NULL;
	(*rmap)->count = 0;
	if (!swapper) {
		swapper = standard_swap;
	}
	(*rmap)->swapper = swapper;
	for (ix=0; ix<MAX_FILES; ix++) {
		(*rmap)->sources[ix].fd = -1;
		(*rmap)->sources[ix].ref_count = 0;
	}
	if (debug_level > 5) verify_resources(*rmap);
	return 0;
}


int 
read_resource_file(
	res_map ** rmap, 
	int fd,
	int endian,
	int (*swapper)(unsigned int type, void * data, unsigned int size))
{
	int w;
	df_header header;
	df_entry * blocks;
	df_entry one_ent;
	off_t file_delta;
	char * resmap;
	res_link ** list;
	int num_blocks;

	if (!rmap) {
		if (debug_level > 0) fprintf(stderr, "read_resource_file(): bad arguments\n");
		return -1;
	}
	if (debug_level > 5) verify_resources(*rmap);
	if ((swapper == NULL) && (*rmap != NULL)) {
		swapper = (*rmap)->swapper;
	}
	if (swapper == NULL) {
		swapper = standard_swap;
	}
	if (!*rmap) {
		if (new_resource_map(rmap, swapper)) {
			return -1;
		}
	}
	list = &(*rmap)->list;
	if (endian != my_endian) {
		if (debug_level > 2) fprintf(stderr, "DEBUG: swapping in effect for reading\n");
	}
	file_delta = lseek(fd, 0, SEEK_CUR);
	if ((w = read(fd, &header, sizeof(header))) != sizeof(header)) {
		if (debug_level > 0) fprintf(stderr, "read_resource_file() read() resource header failed\n");
		return -1;
	}
	if (endian != my_endian) {
		if (debug_level > 4) fprintf(stderr, "DEBUG: swapping resource header (input)\n");
		standard_swap('LONG', &header, sizeof(header));
	}
	if ((header.signature>>16) != SIGNATURE) {
		if (debug_level > 0) fprintf(stderr, "read_resource_file() resource header version is wrong\n");
		return -1;
	}
	if (lseek(fd, file_delta+header.offset_table_ptr, SEEK_SET) < 0) {
		if (debug_level > 0) fprintf(stderr, "read_resource_file() offset table lseek() failed\n");
		return -1;
	}
	if ((w = read(fd, &one_ent, sizeof(one_ent))) != sizeof(one_ent)) {
		if (debug_level > 0) fprintf(stderr, "read_resource_file() read() offset table first slot failed\n");
		return -1;
	}
	if (endian != my_endian) {
		if (debug_level > 2) fprintf(stderr, "DEBUG: one_ent before swapping is off %x  size %x  type %x\n", one_ent.offset, 
			one_ent.size, one_ent.type);
		standard_swap('LONG', &one_ent, sizeof(one_ent));
		if (debug_level > 2) fprintf(stderr, "DEBUG: one_ent after swapping is off %x  size %x  type %x\n", one_ent.offset, 
			one_ent.size, one_ent.type);
	}
	blocks = (df_entry *)malloc(one_ent.size);
	num_blocks = one_ent.size/sizeof(one_ent);
	if (!blocks) {
		if (debug_level > 0) fprintf(stderr, "read_resource_file() malloc(%d) failed\n", one_ent.size);
		return -1;
	}
	*blocks = one_ent;
	if ((w = read(fd, &blocks[1], one_ent.size-sizeof(one_ent))) != (ssize_t)(one_ent.size-sizeof(one_ent))) {
		if (debug_level > 0) fprintf(stderr, "read_resource_file() read() offset table failed\n");
		free(blocks);
		return -1;
	}
	if (endian != my_endian) {
		standard_swap('LONG', &blocks[1], one_ent.size-sizeof(one_ent));
	}
	if ((blocks[RESERVED_BLOCKS].type != SYSTEM_ENTRY) || (blocks[RESERVED_BLOCKS].size < 12) || 
		(blocks[RESERVED_BLOCKS].offset < (ssize_t)sizeof(df_header))) {
		if (debug_level > 0) fprintf(stderr, "read_resource_file() rmap block is not there\n");
		free(blocks);
		return -1;
	}
	resmap = read_block(fd, blocks[RESERVED_BLOCKS], file_delta);
	if (!resmap) {
		free(blocks);
		return -1;
	}
	w = parse_resource_map(fd, blocks, num_blocks, endian, resmap, blocks[RESERVED_BLOCKS].size, file_delta, list, *rmap);
	one_ent = *blocks;
	if (debug_level > 2) fprintf(stderr, "DEBUG: first entry after parsing is off %x  size %x  type %x\n", one_ent.offset, 
		one_ent.size, one_ent.type);
	free(blocks);
	free(resmap);
	if (debug_level > 5) verify_resources(*rmap);
	return w;
}


int iterate_resources(res_map * rmap, void ** cookie, unsigned int * type, int * id, const void ** data, int * size, const char ** name)
{
	res_link * list;

	if (type) *type = 0;
	if (id) *id = 0;
	if (data) *data = NULL;
	if (size) *size = 0;
	if (name) *name = NULL;

	if (!rmap) {
		return -1;
	}
	if (!cookie) {
		return -1;
	}
	if (!*cookie) {
		*cookie = rmap->list;
		if (!*cookie) {
			*cookie = (void *)-1;
			if (debug_level > 3) fprintf(stderr, "DEBUG: no resources to iterate\n");
			return 1;
		}
	}
	else if (*cookie == (void *)-1) {
		if (debug_level > 3) fprintf(stderr, "DEBUG: iterate_resources was done\n");
		return 1;
	}

	list = (res_link *)*cookie;
	if (type) *type = list->type;
	if (id) *id = list->id;
	if (data) *data = list->data;
	if (size) *size = list->size;
	if (name) *name = list->name;
	*cookie = list->next;
	if (!*cookie) {
		*cookie = (void *)-1;
	}

	if (debug_level > 5) verify_resources(rmap);
	return 0;
}


int load_resource_type(res_map * rmap, unsigned int type)
{
	int errs = 0;
	res_link * list;

	if (!rmap) return 0;
	list = rmap->list;

	while (list != NULL) {
		if ((type == 0 || (list->type == type)) && load_resource(list, rmap)) {
			errs++;
		}
		list = list->next;
	}
	if (debug_level > 5) verify_resources(rmap);
	return -errs;
}


void dispose_resource_map(res_map * rmap)
{
	res_link * list;
	int ix;

	if (!rmap) return;
	if (debug_level > 1) verify_resources(rmap);	/* verify resources if debugging */
	list = rmap->list;
	/* dispose memory */
	while (list != NULL) {
		res_link * d = list;
		list = list->next;
		free(d->data);
		free(d);
	}
	/* dispose other things (files) */
	for (ix=0; ix<MAX_FILES; ix++) {
		if (rmap->sources[ix].fd >= 0) {
			close(rmap->sources[ix].fd);
		}
	}
	free(rmap);
}





#define PE_SIGNATURE_OFFSET  0x3c
#define SIZE_OF_PE_SIGNATURE  4
#define RESRC_TAB         2

typedef struct coff_hdr {
	unsigned short 	mchn_typ;  /*Number identifying target machine*/
	short	num_sectns;   /*Number of sections*/	
	ulong	tim_date;     /*Time and day stamp*/
	char	*symtab_ptr;  /*Pointer to symbol table */
	ulong	nsyms;	      /*Number of symbol table enteries*/
	ushort	opthdr_sz;    /*Size of optional header*/
	ushort	flags;        /*Flags indicating characteristics of file*/
} coff_header;

typedef struct standard_flds{
	ushort	magic;   /*Identifying the state of image file.*/
	char	lmajor;	 /*Major revision number of linker*/
	char	lminor;  /*Minor revision number of linker*/
	ulong	csize;   /*Code size or sum of all text sections*/
	ulong	dsize;   /*Intlzd data size or sum of all data sections*/
	ulong	bsize;   /*Unintlzd data size or sum of all BSS sections*/
	/*Following three pointers are relative to image base.*/
	ulong	entry;  /*Address of entry point */
	ulong	code_base;/*Address of begining of code section*/
	ulong	data_base;/*Address of begining of data section*/
} standard_flds;

typedef struct nt_specific {
	char	*prefer_base;
	int	sctn_algnmnt;
	int	file_algnmnt;
	ushort	os_major;
	ushort	os_minor;
	ushort	img_major;
	ushort	img_minor;
	ushort	subsys_major;
	ushort	subsys_minor;
	char	resrvd[4];
	ulong	img_size;
	ulong	hdr_size;
	ulong	file_chksum;
	ushort	subsys;
	ushort	dll;
	ulong	stk_rsrv_size;
	ulong	stk_commit_size;
	ulong	heap_rsrv_size;
	ulong	heap_commit_size;
	ulong	loader_flgs;
	int	num_data_sctns;
} nt_specific;

typedef struct data_directories {
	ulong	rva;
	ulong	size;
} data_directories;

#if defined(__POWERPC__)
#define SWAP_LTL(x) _swap_long((unsigned char *)&x)
#define SWAP_BIG(x) 
#define SWAP_BIG2(x)
#else
#define SWAP_LTL(x)
#define SWAP_BIG(x) _swap_long((unsigned char *)&x)
#define SWAP_BIG2(x) _swap_short((unsigned char *)&x)
#endif

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
static void _swap_cheader(pef_cheader * cheader)
{
	SWAP_BIG2(cheader->magic1);
	SWAP_BIG2(cheader->magic2);
	SWAP_BIG(cheader->cont_id);
	SWAP_BIG(cheader->arch_id);
	SWAP_BIG(cheader->version);
	SWAP_BIG(cheader->mac_time_stamp);
	SWAP_BIG(cheader->od_version);
	SWAP_BIG(cheader->oi_version);
	SWAP_BIG(cheader->c_version);
	SWAP_BIG2(cheader->snum);
	SWAP_BIG2(cheader->slnum);
	SWAP_BIG(cheader->addr);
}
static void _swap_sheaders(pef_sheader * sheaders, int count)
{
	int ix;
	for (ix=0; ix<count; ix++) {
		SWAP_BIG(sheaders->name);
		SWAP_BIG(sheaders->addr);
		SWAP_BIG(sheaders->exec_size);
		SWAP_BIG(sheaders->init_size);
		SWAP_BIG(sheaders->raw_size);
		SWAP_BIG(sheaders->offset);
		sheaders++;
	}
}
#else
static void _swap_cheader(pef_cheader *)
{
	/* do nothing */
}
static void _swap_sheaders(pef_sheader *, int )
{
	/* do nothing */
}
#endif


typedef char			*Elf32_Addr;
typedef unsigned short	Elf32_Half;
typedef unsigned long	Elf32_Off;
typedef long			Elf32_Sword;
typedef unsigned long	Elf32_Word;
#define EI_NIDENT	16		/* size of e_ident[] */

typedef struct {
	unsigned char	e_ident[EI_NIDENT];	/* file identifiers */
	Elf32_Half		e_type;				/* object file type */
	Elf32_Half		e_machine;			/* CPU architecture */
	Elf32_Word		e_version;			/* object file version */
	Elf32_Addr		e_entry;			/* Va of entry point */
	Elf32_Off		e_phoff;			/* pheader file offset */
	Elf32_Off		e_shoff;			/* section header file offset */
	Elf32_Word		e_flags;			/* machine specific flags */
	Elf32_Half		e_ehsize;			/* elf header size */
	Elf32_Half		e_phentsize;		/* size of a pheader entry */
	Elf32_Half		e_phnum;			/* # of program header entries */
	Elf32_Half		e_shentsize;		/* size of section hdr entry */
	Elf32_Half		e_shnum;			/* # of section header entries */
	Elf32_Half		e_shstrndx;			/* section header table index */
} Elf32_Ehdr;

typedef struct {
	Elf32_Word	p_type;		/* segment type */
	Elf32_Off	p_offset;	/* file offset of segment */
	Elf32_Addr	p_vaddr;	/* virtual address of segment */
	Elf32_Addr	p_paddr;	/* physical address of segment */
	Elf32_Word	p_filesz;	/* # file bytes of segment */
	Elf32_Word	p_memsz;	/* # memory bytes of segment */
	Elf32_Word	p_flags;	/* segment flags */
	Elf32_Word	p_align;	/* alignment value */
} Elf32_Phdr;

typedef struct {
	Elf32_Word	sh_name;		/* section name */
	Elf32_Word	sh_type;		/* section type */
 	Elf32_Word	sh_flags;		/* misc attribute flags */
	Elf32_Addr	sh_addr;		/* address in memory image */
	Elf32_Off	sh_offset;		/* file offset of section */
	Elf32_Word	sh_size;		/* size of section */
	Elf32_Word	sh_link;		/* type-dependent link to other section */
	Elf32_Word	sh_info;		/* type-dependent extra info */
	Elf32_Word	sh_addralign;	/* address alignment modulus */
	Elf32_Word	sh_entsize;		/* entry size, for tables */
} Elf32_Shdr;



#define SHORT_SWAP(s) ((((unsigned short)s & 0xff00)>>8)|(((unsigned short)s & 0xff)<<8))
#define INT_SWAP(i) (((i>>24)&0xff)|((i>>8)&0xff00)|((i&0xff00)<<8)|((i&0xff)<<24))


/*
   returns:
  -1 error
   1 seeked to end of ELF, more data there
   0 created new header, or EOF is <= end of ELF
 */

static int
parse_elf(
	int fd,
	int for_writing,
	int * endian)
{
	unsigned int ix;
	int w;
	Elf32_Ehdr file_hdr;
	Elf32_Phdr * p_hdrs = 0;
	Elf32_Shdr * s_hdrs = 0;
	unsigned int offset;
	unsigned int count;
	unsigned int size;
	unsigned int max_size = sizeof(Elf32_Ehdr);
	int swap = 0;
	char detect[4];
	unsigned int align = 0;
	off_t filesize;

	offset = 1;
	memcpy(detect, &offset, 4);
	if ((filesize = lseek(fd, 0, SEEK_END)) < 0) {
		if (debug_level > 0) fprintf(stderr, "parse_elf(): cannot get file size\n");
		return -1;
	}
	lseek(fd, 0, SEEK_SET);
	if ((w = read(fd, &file_hdr, sizeof(Elf32_Ehdr))) != sizeof(Elf32_Ehdr)) {
		if (for_writing && (w == 0)) {
			if (debug_level > 2) fprintf(stderr, "DEBUG: creating ELF header\n");
			memset(&file_hdr, 0, sizeof(file_hdr));
			file_hdr.e_ident[0] = 0x7f; file_hdr.e_ident[1] = 'E'; file_hdr.e_ident[2] = 'L';
			file_hdr.e_ident[3] = 'F'; file_hdr.e_ident[4] = 1; file_hdr.e_ident[5] = (detect[0] == 1) ? 1 : 2;
			file_hdr.e_ident[6] = 1;
			*endian = (detect[0] == 1);
			if ((w = write(fd, &file_hdr, sizeof(file_hdr))) != sizeof(file_hdr)) {
				if (debug_level > 0) fprintf(stderr, "parse_elf(): cannot write new file header\n");
				return -1;
			}
			if ((w = lseek(fd, (sizeof(file_hdr)+31)&~31, SEEK_SET)) < 0) {
				if (debug_level > 0) fprintf(stderr, "parse_elf(): cannot seek to resource data position after new header\n");
				return -1;
			}
			return 1; /* we created header */
		}
		if (debug_level > 0) fprintf(stderr, "parse_elf(): can't read file header\n");
		return -1;
	}
	if ((file_hdr.e_ident[0] != 0x7f) || (file_hdr.e_ident[1] != 'E') ||
		(file_hdr.e_ident[2] != 'L') || (file_hdr.e_ident[3] != 'F') ||
		(file_hdr.e_ident[4] != 1) || (file_hdr.e_ident[6] != 1)) {
		if (debug_level > 0) fprintf(stderr, "parse_elf(): not an ELF file we know\n");
		return -1;
	}
	if (detect[0] == 1) {	/* little_endian machine */
		swap = (file_hdr.e_ident[5] != 1);
		*endian = swap ? 0 : 1;
	}
	else {
		swap = (file_hdr.e_ident[5] != 2);
		*endian = swap ? 1 : 0;
	}

	if (swap) {
		offset = INT_SWAP(file_hdr.e_phoff);
		size = SHORT_SWAP(file_hdr.e_phentsize);
		count = SHORT_SWAP(file_hdr.e_phnum);
	}
	else {
		offset = file_hdr.e_phoff;
		size = file_hdr.e_phentsize;
		count = file_hdr.e_phnum;
	}
	if (debug_level > 2) fprintf(stderr, "DEBUG: there are %d program headers of size %d\n", count, size);
	if (count > 0) {
		p_hdrs = (Elf32_Phdr *)calloc(sizeof(Elf32_Phdr), count);
		if (!p_hdrs) {
			if (debug_level > 0) fprintf(stderr, "parse_elf(): out of memory allocating program headers\n");
			return -1;
		}
		if (max_size < offset+count*size) {
			max_size = offset+count*size;
		}
		for (ix=0; ix<count; ix++) {
			if ((w = lseek(fd, offset+ix*size, SEEK_SET)) < 0) {
				if (debug_level > 0) fprintf(stderr, "parse_elf(): cannot seek to program header\n");
				free(p_hdrs);
				return -1;
			}
			if ((w = read(fd, &p_hdrs[ix], sizeof(Elf32_Phdr))) != sizeof(Elf32_Phdr)) {
				if (debug_level > 0) fprintf(stderr, "parse_elf(): read() of program header failed\n");
				free(p_hdrs);
				return -1;
			}
			if (swap) {
				unsigned int cur = INT_SWAP(p_hdrs[ix].p_offset)+INT_SWAP(p_hdrs[ix].p_filesz);
				if (cur > max_size) {
					max_size = cur;
				}
				if (align < INT_SWAP(p_hdrs[ix].p_align)) {
					align = INT_SWAP(p_hdrs[ix].p_align);
				}
			}
			else {
				unsigned int cur = p_hdrs[ix].p_offset+p_hdrs[ix].p_filesz;
				if (cur > max_size) {
					max_size = cur;
				}
				if (align < p_hdrs[ix].p_align) {
					align = p_hdrs[ix].p_align;
				}
			}
		}
		free(p_hdrs);
	}
	if (debug_level > 2) fprintf(stderr, "DEBUG: after pheaders, align=%d, size=%d\n", align, max_size);

	if (swap) {
		offset = INT_SWAP(file_hdr.e_shoff);
		size = SHORT_SWAP(file_hdr.e_shentsize);
		count = SHORT_SWAP(file_hdr.e_shnum);
	}
	else {
		offset = file_hdr.e_shoff;
		size = file_hdr.e_shentsize;
		count = file_hdr.e_shnum;
	}
	if (debug_level > 2) fprintf(stderr, "DEBUG: there are %d section headers of size %d\n", count, size);
	if (count > 0) {
		s_hdrs = (Elf32_Shdr *)calloc(sizeof(Elf32_Shdr), count);
		if (!s_hdrs) {
			if (debug_level > 0) fprintf(stderr, "parse_elf(): out of memory allocating section headers\n");
			return -1;
		}
		if (max_size < offset+count*size) {
			max_size = offset+count*size;
		}
		for (ix=0; ix<count; ix++) {
			if ((w = lseek(fd, offset+ix*size, SEEK_SET)) < 0) {
				if (debug_level > 0) fprintf(stderr, "parse_elf(): cannot seek to program header\n");
				free(s_hdrs);
				return -1;
			}
			if ((w = read(fd, &s_hdrs[ix], sizeof(Elf32_Shdr))) != sizeof(Elf32_Shdr)) {
				if (debug_level > 0) fprintf(stderr, "parse_elf(): read() of section header failed\n");
				free(s_hdrs);
				return -1;
			}
			if (swap) {
				unsigned int cur = INT_SWAP(s_hdrs[ix].sh_offset);
				if (INT_SWAP(s_hdrs[ix].sh_type) != 8) {	/* SHT_NOBITS */
					cur += INT_SWAP(s_hdrs[ix].sh_size);
				}
				if (cur > max_size) {
					max_size = cur;
				}
			}
			else {
				unsigned int cur = s_hdrs[ix].sh_offset;
				if (s_hdrs[ix].sh_type != 8) {	/* SHT_NOBITS */
					cur += s_hdrs[ix].sh_size;
				}
				if (cur > max_size) {
					max_size = cur;
				}
			}
			/* there's no file alignment requirement in sections */
		}
		free(s_hdrs);
	}
	if (debug_level > 2) fprintf(stderr, "DEBUG: after sheaders, align=%d, size=%d\n", align, max_size);
	if (align < 32) {
		align = 32;	/* shouldn't really happen... */
	}
	else {
		unsigned int i = 1;
		while (i <= align) {
			if (i == align) {
				goto align_ok;
			}
			i = i<<1;
		}
		if (debug_level > 0) fprintf(stderr, "parse_elf(): alignment is not a power of two!\n");
		align = 32;
align_ok:
		;
	}
	if ((w = lseek(fd, (max_size+align-1)&~(align-1), SEEK_SET)) < 0) {
		if (debug_level > 0) fprintf(stderr, "parse_elf(): cannot seek to resource data start\n");
		return -1;
	}
	if (filesize <= ((max_size+align-1) & ~(align-1))) {
		return 0;	/* no data there */
	}
	return 1;
}


int
position_at_map(
	int fd,
	int for_write,
	int * endian)
{
	int w;
	off_t pos;

	check_debug();

	*endian = 0;
	if (lseek(fd, 0, SEEK_SET) < 0) {
		if (debug_level > 0) fprintf(stderr, "position_at_map(): lseek() to beginning failed\n");
		return -1;
	}
	{
		int offset;
		char buf[4];
		off_t filesize;

		*endian = 1;	/* this is a little-endian file */
		if ((w = read(fd, buf, 4)) != 4) {
			if (w == 0) {	/* file empty */
#if defined(__ELF__)
				if (for_write) {
					if (debug_level > 2) fprintf(stderr, "DEBUG: creating ELF header\n");
					/* file header only written on little-endian systems, don't swap */
					if ((w = write(fd, &file_header, sizeof(file_header))) < 0) {
						if (debug_level > 0) fprintf(stderr, "position_at_map(): write() file_header failed\n");
						return -1;
					}
				}
				return 0;	/* we just created it */
#else
				goto write_pef;
#endif
			}
			if (debug_level > 0) fprintf(stderr, "position_at_map(): read() signature failed\n");
			return -1;
		}
		if ((buf[0] == 'R') && (buf[1] == 'S')) {
			return 1;
		}
		if ((buf[0] != 'M') || (buf[1] != 'Z')) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): expected MZ header\n");
			lseek(fd, 0, SEEK_SET);
			goto try_pef;
		}
		*endian = 1;	/* this is a little-endian file */
		if (lseek(fd, PE_SIGNATURE_OFFSET, SEEK_SET) < 0) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): cannot seek to PE offset location\n");
			return -1;
		}
		if ((w = read(fd, &offset, 4)) != 4) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): read() PE offset failed\n");
			return -1;
		}
		SWAP_LTL(offset);
		if (lseek(fd, offset, SEEK_SET) < 0) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): lseek() to PE failed\n");
			return -1;
		}
		if ((w = read(fd, buf, 2)) != 2) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): read() PE signature failed\n");
			return -1;
		}
		if ((buf[0] != 'P') || (buf[1] != 'E')) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): expected PE signature\n");
			return -1;
		}
		offset += SIZE_OF_PE_SIGNATURE + sizeof(coff_header) +
			sizeof(standard_flds) +	sizeof(nt_specific) +
			sizeof(data_directories) * RESRC_TAB;
		if ((pos = lseek(fd, offset, SEEK_SET)) < 0) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): cannot seek to resource section offset\n");
			return -1;
		}
		if ((w = read(fd, &offset, 4)) != 4) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): cannot read resource section offset\n");
			return -1;
		}
		SWAP_LTL(offset);
		/* if non-0, seek */
		if (offset > 0) {	
			off_t filesize = lseek(fd, 0, SEEK_END);
			if (filesize < 0) {
				if (debug_level > 0) fprintf(stderr, "position_at_map(): cannot get EOF\n");
				return -1;
			}
			if (lseek(fd, offset, SEEK_SET) < 0) {
				if (debug_level > 0) fprintf(stderr, "position_at_map(): final lseek() failed\n");
				return -1;
			}
			if (offset >= filesize) {
				return 0;	/* there is no data */
			}
			return 1;	/* there is a section, we assume */
		}
		/* if 0, or points to bad section, create section if for writing */
		if (!for_write) {
			return 0;	/* no resource section */
		}
		filesize = lseek(fd, 0, SEEK_END);
		if (filesize < 0) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): cannot get end of file\n");
			return -1;
		}
		filesize = (filesize + 15)&~15;
		if (lseek(fd, pos, SEEK_SET) < 0) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): cannot re-seek to resource section offset\n");
			return -1;
		}
		offset = filesize;
		SWAP_LTL(offset);	/* to little */
		if ((w = write(fd, &offset, 4)) != 4) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): write of new resource section offset failed\n");
			return -1;
		}
		if (lseek(fd, filesize, SEEK_SET) < 1) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): final lseek with allocated rmap failed\n");
			return -1;
		}
		return 0;	/* we allocated space for it, so it's not there yet */
	}
try_pef:
	{
		pef_cheader cheader;
		pef_sheader * sheaders;
		int ix;
		off_t end;

		*endian = 0;	/* this might be a big-endian file */
		pos = sizeof(cheader);
		if ((w = read(fd, &cheader, sizeof(cheader))) != sizeof(cheader)) {
#if defined(__POWERPC__)
			if (w == 0) {
write_pef:
				*endian = 0;	/* this is a big-endian file */
				/* file header only written on big-endian systems, don't swap */
				if (for_write) if (debug_level > 2) fprintf(stderr, "DEBUG: creating PEF header\n");
				if (for_write && ((w = write(fd, &file_header, sizeof(file_header))) != sizeof(file_header))) {
					if (debug_level > 0) fprintf(stderr, "position_at_map(): cannot write() new file header\n");
					return -1;
				}
				return 0;	/* no data, we just created header */
			}
#endif
			if (w > 0) {
				goto try_elf;
			}
			if (debug_level > 0) fprintf(stderr, "position_at_map(): read() file header failed\n");
			return -1;
		}
		_swap_cheader(&cheader);
		if (cheader.cont_id == 'resf') {
			/* we're done! */
			return 1;
		}
		if (cheader.cont_id != 'peff') {
			goto try_elf;
		}
		sheaders = (pef_sheader *)calloc(sizeof(pef_sheader), cheader.snum);
		if (!sheaders) {
			return -1;
		}
		if ((w = read(fd, sheaders, sizeof(pef_sheader)*cheader.snum)) != 
			(ssize_t)sizeof(pef_sheader)*cheader.snum) {
			return -1;
		}
		_swap_sheaders(sheaders, cheader.snum);
		for (ix=0; ix<cheader.snum; ix++) {
			if (sheaders[ix].offset+sheaders[ix].raw_size > pos) {
				pos = sheaders[ix].offset + sheaders[ix].raw_size;
			}
		}
		end = lseek(fd, 0, SEEK_END);
		if (end < 0) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): can't find EOF\n");
			return -1;
		}
		if (lseek(fd, pos, SEEK_SET) < 0) {
			if (debug_level > 0) fprintf(stderr, "position_at_map(): lseek() to resource data failed\n");
		}
		if (end == pos) {
			return 0;	/* no actual data */
		}
		return 1;	/* there is data */
	}
try_elf:
	return parse_elf(fd, for_write, endian);
}


#if CREATE_FILE
int
main()
{
	int fd = open("output.rsrc", O_RDWR | O_CREAT, 0666);
	res_map * rmap = NULL;

	int data = 4;
	add_resource(&rmap, 'INT ', 1, &data, sizeof(data), "four");

	add_resource(&rmap, 'STR ', 1, "four", 5, "four");

	data = 2;
	add_resource(&rmap, 'INT ', 2, &data, sizeof(data), "two");

	add_resource(&rmap, 'STR ', 2, "two", 4, "two");

	data = 3;
	add_resource(&rmap, 'INT ', 3, &data, sizeof(data), "three");

	add_resource(&rmap, 'STR ', 3, "three", 6, "three");

	data = 1;
	add_resource(&rmap, 'INT ', 1, &data, sizeof(data), "one");

	add_resource(&rmap, 'STR ', 1, "one", 4, "one");

	if (position_at_map(fd, 1) < 0) {
		if (debug_level > 0) fprintf(stderr, "position_at_map() failed\n");
	}
	if (write_resource_file(rmap, fd) < 0) {
		if (debug_level > 0) fprintf(stderr, "file write failed\n");
	}
	close(fd);
	return 0;
}
#endif
