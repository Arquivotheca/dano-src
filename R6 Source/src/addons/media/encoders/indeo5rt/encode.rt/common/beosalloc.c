#include <errno.h>
#include <malloc.h>
#include <OS.h>

static void *Alloc(void *, size_t) __attribute__ ((stdcall)); 
static void Free(void *) __attribute__ ((stdcall));

void *(*_imp__GlobalAlloc)(void *, size_t) = Alloc;
void (*_imp__GlobalFree)(void *) = Free;
void *(*_imp__LocalAlloc)(void *, size_t) = Alloc;
void (*_imp__LocalFree)(void *) = Free;


typedef struct lock_s {
	sem_id	s;
	int32	c;
} lock_t;

#define LOCK(l)		{if(atomic_add(&l.c, -1) <= 0) acquire_sem(l.s);}
#define UNLOCK(l)	{if(atomic_add(&l.c, 1) < 0) release_sem(l.s);}

static lock_t alloc_lock = {-1, 0};
static void **allocations = NULL;
static int alloc_index = 0;
static int list_len = 0;

#define GROW_INCR	16

__attribute__ ((stdcall)) static void *Alloc(void *dummy, size_t size)
{
	void *ptr;

	LOCK(alloc_lock);

	if(alloc_index == list_len) {
		void **np;
		np = realloc(allocations, sizeof(void*) * (list_len+GROW_INCR));
		if(np == NULL) {
			UNLOCK(alloc_lock);
			printf("Indeo5rt: alloc table realloc failed!\n");
			return NULL;
		}
		allocations = np;
		list_len += GROW_INCR;
	}

	ptr = malloc(size);
	if(ptr != NULL) {
		allocations[alloc_index++] = ptr;
	}
	//printf("Indeo Alloc %p , dummy %p, size %d\n", ptr, dummy, size);

	UNLOCK(alloc_lock);

	return ptr;
}

__attribute__ ((stdcall)) static void Free(void *ptr)
{
	int i;

	LOCK(alloc_lock);

	//printf("Indeo Free %p\n", ptr);
	for(i=0; i < alloc_index; i++) {
		if(allocations[i] == ptr) {
			--alloc_index;
			if(alloc_index != i) {
				allocations[i] = allocations[alloc_index];
			}
			allocations[alloc_index] = (void*)0xdefaced1;

			UNLOCK(alloc_lock);

			free(ptr);
			return;
		}
	}
	printf("Indeo Free %p, not in allocation list\n", ptr);

	UNLOCK(alloc_lock);

	return;
}

void
InitIndeoAlloc()
{
	//printf("InitIndeoAlloc\n");
	alloc_lock.s = create_sem(1, "Indeo5rt alloc sem");
	alloc_lock.c = 1;
}

void
UninitIndeoAlloc()
{
	int i;

	//printf("UninitIndeoAlloc\n");
	for(i=0; i < alloc_index; i++) {
		if(allocations[i] != NULL) {
			printf("Indeo Allocation %p, not freed\n", allocations[i]);
			free(allocations[i]);
			allocations[i] = NULL;
		}
	}

	alloc_index = 0;
	list_len = 0;
	free(allocations);
	allocations = NULL;

	if(alloc_lock.s > 0) {
		delete_sem(alloc_lock.s);
		alloc_lock.s = -1;
		alloc_lock.c = 0;
	}
}
