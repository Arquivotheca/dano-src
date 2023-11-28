#include <BeBuild.h>
#include <stdlib.h>
#include <OS.h>
#include <image.h>
#include <priv_syscalls.h>
#include <priv_runtime.h>


typedef struct image_desc	image_desc;
typedef struct list_link	list_link;

struct image_desc {
	image_id	id;
	int32		seq;
	int32		order;
	void		(*routine)();
	char		*base;
	uint32		size;
};
 
struct list_link  {
	list_link	*next;
	image_desc	*list;
};


static int32		lck_count;
static int32		lck_owner;
static int32		lck_ownercount;
static sem_id		lck_sem;
static list_link	*glinkp;

static void
create_lock()
{
	lck_count = 0;
	lck_owner = -1;
	lck_ownercount = 0;
	lck_sem = create_sem(0, "addon lock");
}

void
__fork_addon_()
{
	create_lock();
}

static void
delete_lock()
{
	delete_sem(lck_sem);
}

static void
acquire_lock()
{
	status_t	err;
	int32		old;
	thread_id	owner;

	owner = find_thread(NULL);

	if (owner == lck_owner) {
		lck_ownercount++;
		return;
	}

	old = atomic_add(&lck_count, 1);
	if (old >= 1) {
		while ((err = acquire_sem(lck_sem)) == B_INTERRUPTED)
			;
		/* ### what should we do for other errors? */
	}

	lck_owner = owner;
	lck_ownercount = 1;
}

static void
release_lock()
{
	int32	old;

	lck_ownercount--;

	if (lck_ownercount == 0) {
		lck_owner = -1;
		old = atomic_add(&lck_count, -1);
		if (old > 1)
			release_sem(lck_sem);
	}
}

static int
compare_image_desc(image_desc *a, image_desc *b)
{
	if (a->seq != b->seq)
		return (a->seq - b->seq);
	return (a->order - b->order);
}

static void
call_routine_in_order(int seq, bool init)
{
	int32		cookie;
	int32		cnt;
	int32		i;
	image_desc	images[256];
	image_info			info;

	/* seq can be either -1 or a valid seq #.
	. -1: all images.
	. valid seq #: all images with that seq #.
	*/

	cnt = 0;
	cookie = 0;
	while (get_next_image_info(0, &cookie, &info) == B_NO_ERROR) {
		if ((seq != -1) && (seq != info.sequence))
			continue;
		images[cnt].id = info.id;
		images[cnt].order = info.init_order;
		images[cnt].seq = info.sequence;
		if (init)
			images[cnt].routine = info.init_routine;
		else
			images[cnt].routine = info.term_routine;
		cnt++;
	}
	
	qsort(images, cnt, sizeof(image_desc),
		(int (*)(const void *, const void *)) compare_image_desc);

	if (init) {
		for(i=0; i<cnt; i++)
			if (images[i].routine)
				(*images[i].routine)(images[i].id);
	} else {
		for(i=cnt-1; i>=0; i--)
			if (images[i].routine)
				(*images[i].routine)(images[i].id);
	}
}

int
_call_init_routines_()
{
	int32				cookie;
	int32				seq;
	image_info			info;

	create_lock();

	/* find the application seq # */
	seq = -1;
	cookie = 0;
	while (get_next_image_info(0, &cookie, &info) == B_NO_ERROR)
		if (info.type == B_APP_IMAGE) {
			seq = info.sequence;
			break;
		}

	/* call the init routines of the application clique */
	call_routine_in_order(seq, TRUE);

	return 0;
}

int
_call_term_routines_()
{
	/* call the term routines of all the images */
	call_routine_in_order(-1, FALSE);
	delete_lock();
	return 0;
}


thread_id
load_image(int32 argc, const char **argv, const char **envp)
{
	int	envc= 0;

	if(envp) {
		while(envp[envc]) {
			envc++;
		}
	}

	return _kload_image_(argc, (char**)argv, envc, (char**)envp, NULL, 0);
}

thread_id
_kload_image_etc_(int32 argc, const char **argv, const char **envp, char *buf, size_t s)
{
	int	envc= 0;

	if(envp) {
		while(envp[envc]) {
			envc++;
		}
	}

	return _kload_image_(argc, (char**)argv, envc, (char**)envp, buf, s);
}

image_id
load_add_on(const char *path)
{
	int			err;
	image_info	info;
	image_id	imid;
	const char	*lpath, *apath;

	acquire_lock();
	
	lpath = getenv("LIBRARY_PATH");
	apath = getenv("ADDON_PATH");

	imid = _kload_add_on_(path, apath, lpath);
	if (imid < 0) {
		err = imid;
		goto error1;
	}
	err = get_image_info(imid, &info);
	if (err)
		goto error1;
	call_routine_in_order(info.sequence, TRUE);
	release_lock();
	return imid;

error1:
	release_lock();
	return err;
}

status_t
unload_add_on(image_id imid)
{
	int			err;
	image_info	info;
	bool		found;
	image_desc	*list, *ulist, *tmp;
	image_id	*rlist, *rtmp;
	int32		num, rnum, unum;
	int32		i, j, k;
	int32		first, last, nx;
	image_id	lib;
	int32		cookie;
	list_link	link;
	list_link	*alink;

	acquire_lock();

	/*
	call the atexit functions and term routine for the add-on
	and unload it.
	*/

	err = get_image_info(imid, &info);
	if (err)
		goto error1;
	exit_container((char *)info.text, ((char*)info.data + info.data_size) - (char *)info.text);
	if (info.term_routine)
		(*info.term_routine)(imid);
	_kunload_add_on_(imid);

	/*
	build a list of all the binaries in the team (list).
	build another list containing all the binaries of type B_APP_IMAGE and B_ADD_ON_IMAGE (rlist).
	this latter one is used to explore the graph of used libraries.
	*/

	list = (image_desc *) malloc(16*sizeof(image_desc));
	if (!list) {
		err = ENOMEM;
		goto error1;
	}
	rlist = (image_id *) malloc(16*sizeof(image_id));
	if (!rlist) {
		err = ENOMEM;
		goto error2;
	}
	cookie = 0;
	num = 0;
	rnum = 0;
	while (get_next_image_info(0, &cookie, &info) == B_NO_ERROR) {

		if (num % 16 == 15) {
			tmp = (image_desc *) realloc(list, (num+16)*sizeof(image_desc));
			if (!tmp) {
				err = ENOMEM;
				goto error3;
			}
			list = tmp;
		}
		list[num].id = info.id;
		list[num].seq = info.sequence;
		list[num].order = info.init_order;
		list[num].routine = info.term_routine;
		list[num].base = (char *) info.text;
		list[num].size = ((char*)info.data + info.data_size) - (char *)info.text;
		num++;

		if ((info.type == B_APP_IMAGE) || (info.type == B_ADD_ON_IMAGE)) {
			if (rnum % 16 == 15) {
				rtmp = (image_id *) realloc(rlist, (rnum+16)*sizeof(image_id));
				if (!rtmp) {
					err = ENOMEM;
					goto error3;
				}
				rlist = rtmp;
			}
			rlist[rnum] = info.id;
			rnum++;
		}
	}

	/*
	explore the graph of used binaries (bredth first exploration, could use depth first).
	*/

	first = 0;
	last = rnum;
	nx = last;
	while(first < last) {
		for(i=first; i<last; i++) {
			j = 0;
			while (TRUE) {
				lib = get_nth_image_dependence(rlist[i], j++);
				if (lib < 0)
					break;

				/* put it in the reached list if it does not already appear there */

				for(k=0; k<nx; k++)
					if (rlist[k] == lib)
						break;

				if (k == nx) {
					if (nx % 16 == 15) {
						rtmp = (image_id *) realloc(rlist, (nx+16)*sizeof(image_id));
						if (!rtmp) {
							err = ENOMEM;
							goto error3;
						}
						rlist = rtmp;
					}
					rlist[nx++] = lib;
				}
			}
		}
		first = last;
		last = nx;
	}
	rnum = last;

	/*
	build the list of unreached libraries
	in the case unload_add_on() is reentered (a term routine calling unload_add_on itself),
	we also eliminate the unreached libraries that are taken care of at a higher level.
	*/

	ulist = (image_desc *) malloc((num-rnum+1)*sizeof(image_desc));
	if (!ulist) {
		err = ENOMEM;
		goto error3;
	}
	unum = 0;
	for(i=0; i<num; i++) {
		found = FALSE;
		for(j=0; j<rnum; j++)
			if (list[i].id == rlist[j]) {
				found = TRUE;
				break;
			}
		for(alink=glinkp; alink && !found; alink=alink->next)
			for(j=0; alink->list[j].id > 0; j++)
				if (list[i].id == alink->list[j].id) {
					found = TRUE;
					break;
				}

		if (!found) {
			ulist[unum] = list[i];
			unum++;
		}
	}
	ulist[unum].id = 0;
	
	/*
	sort the unreached library by init order
	*/

	qsort(ulist, unum, sizeof(image_desc), (int (*)(const void *, const void *)) compare_image_desc);

	/*
	link our list of unreached containers to the global list.
	*/

	link.next = glinkp;
	link.list = ulist;
	glinkp = &link;

	/*
	for each unreached container: call the atexit functions and the term routine, then unload it.
	*/	

	
	for(i=unum-1; i>=0; i--) {
		exit_container(ulist[i].base, ulist[i].size);
		if (ulist[i].routine)
			(*ulist[i].routine)(ulist[i].id);
		_kunload_library_(ulist[i].id);
	}

	
	/*
	unlink our list of unreached containers from the global list.
	*/

	glinkp = link.next;

	release_lock();

	free(list);
	free(rlist);
	free(ulist);

	return 0;

error3:
	free(rlist);
error2:
	free(list);
error1:
	release_lock();
	return err;
}
