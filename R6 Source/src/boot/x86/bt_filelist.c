#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <drivers/KernelExport.h>

#include <boot.h>

/* number of levels to follow symlinks */
#define SYMLINK_THRESHOLD 5

/* vyt: write this for real */
static status_t
canon(char *str)
{
	char *p, *q;

	p = q = str;
	while (*p) {
		if (*p == '.') {
			if (*(p+1) == '/') {
				p += 2;
				continue;
			} else if ((*(p+1) == '.') && (*(p+2) == '/')) {
				p += 3;
				q-=2;
				while ((q >= str) && (*q != '/')) q--;
				if (q < str) return B_ERROR;
				q++;
				continue;
			}
		}
		while (*p && (*p != '/'))
			*(q++) = *(p++);
		*(q++) = *p;
		if (*p) p++;
	}
	return B_OK;
}

static status_t
get_canonical_name(char *path)
{
	status_t result = B_OK;
	int depth;

	for (depth=0;depth < SYMLINK_THRESHOLD;depth++) {
		char link[2 * B_FILE_NAME_LENGTH];

		sprintf(link, "%s/../", path);
		if (readlink(path, link + strlen(link), B_FILE_NAME_LENGTH) != B_OK) break;

		if ((result = canon(link)) != B_OK) break;

		strcpy(path, link);
	}

	if (depth == SYMLINK_THRESHOLD) {
		dprintf("Symbolic link chain too long (%s)\n", path);
		result = B_ERROR;
	}
	
	return result;
}

struct boot_filelist_list {
	struct boot_filelist item;
	struct boot_filelist_list *next;
};

static status_t
build_bootfilelist_list(const char *path,
		struct boot_filelist_list **list)
{
	DIR *dirp;
	struct dirent *de;
	status_t error = B_OK;
	struct boot_filelist item;
	struct boot_filelist_list *c;
	char *p;

	dirp = opendir(path);
	if (!dirp) return B_ERROR;

	c = *list;
	while (c && c->next) c = c->next;

	while ((de = readdir(dirp)) != NULL) {
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;

		strcpy(item.path, path);
		p = item.path + strlen(path);
		*(p++) = '/';
		strcpy(p, de->d_name);

		if (get_canonical_name(item.path) == B_OK) {
			struct stat st;
			stat(item.path, &st);
			if (S_ISDIR(st.st_mode)) {
				/* recurse into subdirectories */
				build_bootfilelist_list(item.path, list);
				c = *list;
				while (c && c->next) c = c->next;
			} else {
				struct boot_filelist_list *n;
				char *p1, *p2;
				/* check if the add-on is already in the list, treating
				 * system and user add-ons with the same name as equivalent */

				/* +1 to avoid problems with volumes named "add-ons" */
				p1 = strstr(item.path + 1, "/add-ons/");
				for (n=*list;n;n=n->next) {
					p2 = strstr(n->item.path + 1, "/add-ons/");
					if ((p1 && p2) && !strcmp(p1, p2)) {
						break;
					}
				}

				/* if the add-on is new, tack it to the end of the list */
				if (!n) {
					n = malloc(sizeof(*n));
					n->item = item;
					n->next = NULL;
					if (c) c->next = n; else *list = n;
					c = n;
				}
			}
		}
	}

	closedir(dirp);
	
	return error;
}

/* accepts path of form path1:path2:path3 */
status_t build_directory_list(const char *path, uint32 *num,
		struct boot_filelist **files)
{
	uint32 count = 0;
	struct boot_filelist *p;
	struct boot_filelist_list *list, *c, *n;
	const char *s;
	char *t, path2[B_FILE_NAME_LENGTH];

	list = NULL;
	s = path;
	while (s) {
		t = strchr(s, ':');
		if (t) {
			strncpy(path2, s, t - s);
			path2[(uint32)(t-s)] = 0;
			t++;
		} else {
			strcpy(path2, s);
		}
		build_bootfilelist_list(path2, &list);
		for (c=list,count=0;c;c=c->next,count++)
			;
		s = t;
	}
	
	*num = count;
	p = *files = (struct boot_filelist *)malloc(count * sizeof(**files));
	c = list;
	while (c) {
		n = c->next;
		*(p++) = c->item;
		free(c);
		c = n;
	}

	return B_OK;
}
