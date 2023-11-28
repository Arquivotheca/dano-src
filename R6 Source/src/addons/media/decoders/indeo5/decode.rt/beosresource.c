#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>

#include "datatype.h"
#include "../images/resource.h"

extern void *getbitmap(const char *filename);

void *hDriverModule = NULL;

typedef struct {
	int id;
	char *filename;
	uint8 *data;
} resource_t;

static resource_t resources[] = {
//	{ IDB_INDEO_TOO_OLD_REJECT_MSG, "../images/ivi_beta.bmp", NULL },
//	{ IDB_INDEO_TOO_NEW_REJECT_MSG, "../images/ivi_new.bmp", NULL }
	{ IDB_INDEO_TOO_OLD_REJECT_MSG, "ivi_beta.bmp", NULL },
	{ IDB_INDEO_TOO_NEW_REJECT_MSG, "ivi_new.bmp", NULL }
};

static void *LoadResource (void *handle, resource_t *r) __attribute__ ((stdcall));
static void *FindResourceA(void *handle, int id, int arg3) __attribute__ ((stdcall));
static void *LockResource(void *ptr) __attribute__ ((stdcall));
static void *FreeResource(void *ptr) __attribute__ ((stdcall));

void *(*_imp__LoadResource)(void *, resource_t *) = LoadResource;
void *(*_imp__FindResourceA)(void *, int, int) = FindResourceA;
void *(*_imp__LockResource)(void *) = LockResource;
void *(*_imp__FreeResource)(void *) = FreeResource;

static void * __attribute__ ((stdcall))
LoadResource(void *handle, resource_t *r)
{
//	printf("_imp__LoadResource 0x%08x 0x%08x\n", arg1, arg2);
	if(r == NULL) {
		printf("LoadResource 0x%08x 0x%08x, not found\n", handle, r);
		return NULL;
	}
	else {
		if(r->data == NULL) {
			r->data = getbitmap(r->filename);
#if 0
			int fd = open(r->filename,  O_RDONLY);
			if(fd < 0) {
				printf("LoadResource: could not open %s, %s\n", r->filename,
				       strerror(errno));
			}
			else {
				struct stat st;
				if(fstat(fd, &st) < 0) {
					printf("LoadResource: stat failed, %s\n", strerror(errno));
				}
				else {
					r->data = malloc(st.st_size+sizeof(BITMAPINFOHEADER));
					if(r->data == NULL) {
						printf("LoadResource: malloc(%d) failed\n", st.st_size);
					}
					else {
						LPBITMAPINFOHEADER bh = r->data;
						bh->biSize = sizeof(BITMAPINFOHEADER);
						bh->biWidth = 10;
						bh->biHeight = 10;
						bh->biPlanes = 1;
						bh->biBitCount = 8;
						bh->biCompression = 0;
						bh->biSizeImage = 100;
						bh->biXPelsPerMeter = 1;
						bh->biYPelsPerMeter = 1;
						bh->biClrUsed = 0;
						bh->biClrImportant = 0;
						if(read(fd, r->data+sizeof(BITMAPINFOHEADER), st.st_size) < st.st_size) {
							printf("LoadResource: short read\n");
						}
					}
					close(fd);
				}
			}
#endif
		}
		return r->data;
	}
}

static void * __attribute__ ((stdcall))
FindResourceA(void *handle, int id, int arg3)
{
	int i;
	for(i=0; i<2; i++) {
		if(resources[i].id == id)
			return &resources[i];
	}
	printf("_imp__FindResourceA 0x%08x 0x%08x 0x%08x not found\n", handle, id, arg3);
	return NULL;
}


static void * __attribute__ ((stdcall))
LockResource(void *ptr)
{
//	printf("LockResource 0x%08x\n", ptr);
	return ptr;
}

static void * __attribute__ ((stdcall))
FreeResource(void *ptr)
{
	//printf("FreeResource 0x%08x\n", ptr);
	return NULL;
}
