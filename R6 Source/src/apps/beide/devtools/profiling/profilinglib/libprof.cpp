/*	libprof.cpp	*/
/*	Copyright © 1997 Metrowerks, Inc */
  
#pragma profile off

#include "libprof.h"
#include "MWUnmangle.h"

#include <stdio.h>
#include <OS.h>
#include <stdlib.h>
#include <string.h>
#include <Debug.h>

// to change the file type
#include <Node.h>
#include <NodeInfo.h>

#pragma profile off

#define KEY '_mwP'
#define DEBUG 0
#if defined(__cplusplus)
extern "C" {
#endif

/*	the magic, undocumented thread-local storage API	*/
/*	char * _thread_data_init(ulong key, size_t size, void * init);	*/
char * _thread_data(ulong key, size_t size);

/*	the functions we export	*/
void __PROFILE_ENTRY(char * func);
long long __PROFILE_EXIT(int long long);

#if defined(__cplusplus)
}
#endif


typedef struct thr_func
{
	struct thr_func *	next;
	char *				funcname;
	bigtime_t			kernel;
	bigtime_t			user;
	bigtime_t			clock;
	int64				called;
}
thr_func;

typedef struct thr_func thr_result;

typedef struct thr_prof
{
	int32			depth;
	thr_func *		funcs;
	int				hashsize;
	thr_result *	results;
}
thr_prof;


static	int				_profmaxdepth;
static	sem_id			_profsem;
static	int				_profhashsize;
static	thr_result *	_profresults;
static 	char *			_profdumpname;
static 	int				_profmaxdeptherror;
static	int32			_profmaxuserdepth;


static void 
change_file_type(const char* const inName, const char* const inTargetType)
{	
	BNode classNode(inName);
	BNodeInfo classNodeInfo(&classNode);
	classNodeInfo.SetType(inTargetType);
}



static void
remember_result(thr_prof *data, thr_result *result)
{
	int ix = ((int)result->funcname>>2)&(data->hashsize-1);
	thr_result *save = &data->results[ix];
	/*	using pointer-to-pointer would have been much easier	*/
	/*	but that would require allocating a lot of single struct rather	*/
	/*	than a big array for the (common case) top level	*/
again:
	if (!save->funcname || (save->funcname == result->funcname))
	{	/*	save here	*/
		save->funcname = result->funcname;
		save->user += result->user;
		save->kernel += result->kernel;
		save->clock += result->clock;
		save->called += result->called;
		return;
	}
	if (!save->next)
	{
		/*	create new and save there	*/
		save->next = (thr_func *)calloc(sizeof(thr_func), 1);

		ASSERT(save->next->funcname == NULL);
	}
	save = save->next;
	goto again;
}


static void
global_insert(thr_result *res)
{
	int ix = ((int)res->funcname>>2)&(_profhashsize-1);
	thr_result * hash = &_profresults[ix];
	/*	again; to avoid lots of small allocations, _profresults is an array of structs, so	*/
	/*	we can't do the simpler pointer-to-pointer traversal.		*/
//	printf("!%s || (%s == %s)\n", hash->funcname, hash->funcname, res->funcname);
again:
	if (!hash->funcname || (hash->funcname == res->funcname))
	{
		/*	save here	*/
		hash->called += res->called;
		hash->user += res->user;
		hash->kernel += res->kernel;
		hash->clock += res->clock;
		hash->funcname = res->funcname;
#if DEBUG
		printf("global insert for %s\n", res->funcname);
#endif
		return;
	}
	if (!hash->next)
	{
		/*	create new and save there	*/
		hash->next = (thr_func *)calloc(sizeof(thr_func), 1);

		ASSERT(hash->next->funcname == NULL);
	}
	hash = hash->next;
	goto again;
}


static void
global_merge_results(thr_prof *data)
{
	int ix;
	acquire_sem(_profsem);
	ASSERT (_profresults != NULL);

	for (ix=0; ix<data->hashsize; ix++)
	{
		thr_result *res = &data->results[ix];
		while (res != NULL)
		{
			if (res->funcname != NULL)
			{
				global_insert(res);
			}
			res = res->next;
		}
	}
	release_sem(_profsem);
}


static asm double
get_fpret()
{
	blr
}

static asm double
put_fpret(register double d)
{
	blr
}


void
__PROFILE_ENTRY(char *funcname)
{
	if (_profmaxdepth) // if profiling is turned on
	{
		thread_info info;
		thr_func *func;
		int depth;
		thr_prof * ptr;
		
		ptr = (thr_prof *)_thread_data(KEY, sizeof(thr_prof));
		depth = ptr->depth++;
		if (_profmaxuserdepth < depth)
		{
			atomic_add(&_profmaxuserdepth, 1);
		}
		//	printf("profile entry: %s, depth = %d\n", funcname, depth);
		if (depth == 0)				/*	first call for this thread	*/
		{
#if DEBUG
			printf("allocating new chain at %s\n", funcname);
#endif
			ptr->hashsize = 256;
			ptr->results = (thr_result *)calloc(sizeof(thr_result), 256);
			ptr->funcs = (thr_func *)malloc(sizeof(thr_func) * _profmaxdepth);
		}
		else if (depth >= _profmaxdepth)
		{
			if (depth >= _profmaxdeptherror)
			{
				// there is a possibility that this may become too large
				// but that's ok, since it won't cause any problems
				_profmaxdeptherror = depth + 1;
			}
			return;		/*	will automatically take care of case where profiling turned off	*/
		}
		func = &ptr->funcs[depth];
		memset(func, 0, sizeof(thr_func));
		func->funcname = funcname;
		get_thread_info(find_thread(NULL), &info);
		func->kernel = -info.kernel_time;
		func->user = -info.user_time;
		func->clock = -system_time();
		func->called = 0;
	}
}


long long
__PROFILE_EXIT(long long retval)
{
	double fv = get_fpret();	/*	save floating point return value	*/

	if (_profmaxdepth)			/*	if profiling is turned on		*/
	{
		thread_info info;
		int depth;
		thr_func * func;	
		thr_prof * ptr = (thr_prof *)_thread_data(KEY, sizeof(thr_prof));

		depth = --ptr->depth;
		if (depth >= _profmaxdepth)
		{
			put_fpret(fv);
			return retval;
		}
	
		ASSERT(depth >= 0);
	
		get_thread_info(find_thread(NULL), &info);
		func = &ptr->funcs[depth];
		func->user += info.user_time;
		func->kernel += info.kernel_time;
		func->clock += system_time();
		func->called += 1;
		remember_result(ptr, func);

		if (depth == 0)
		{	/*	last frame of this thread	*/
			global_merge_results(ptr);
			free(ptr->funcs);
			free(ptr->results);
		}
	}
#if DEBUG
	else
		puts("profiling is turned off");
#endif

	put_fpret(fv);			/*	restore floating point return value	*/
	return retval;			/*	return with integer return value	*/
}

static void
PROFILE_DUMP_ATEXIT()
{
	PROFILE_DUMP(_profdumpname);
	free(_profdumpname);
}


void
PROFILE_INIT_SHARED_LIB(int	maxdepth, const char* const path)
{
	atexit(PROFILE_DUMP_ATEXIT);
	_profdumpname = strdup(path);
	PROFILE_INIT(maxdepth);
}


void
PROFILE_INIT(int maxdepth)
{
	if (!_profsem)
	{
		_profsem = create_sem(1, "profiling thread death");
		_profhashsize = 1024;
		_profresults = (thr_result *)calloc(sizeof(thr_result), _profhashsize);
		_profmaxdeptherror = 0;
		_profmaxuserdepth = 0;
	}
	_profmaxdepth = maxdepth;
	printf("Intializing profiler - profile entry: %d, sem: %p, %d, %p\n", _profmaxdepth, _profsem, _profhashsize, _profresults);
}



/*	call PROFILE_DUMP at the end of main() to dump data into specified file	*/
void
PROFILE_DUMP(const char* const path)
{
	FILE *f;

	thr_prof * ptr = (thr_prof *)_thread_data(KEY, sizeof(thr_prof));
	int depth = --ptr->depth;
		
	/* merge outstanding profiling table into the global profiling table */
	if (depth >= 0)
	{
		global_merge_results(ptr);
		free(ptr->funcs);
		free(ptr->results);
	}

	printf("saving profiling information to %s\n", path);

	_profmaxdepth = 0;	/*	turn off profiling	*/

	ASSERT(_profsem > 0);
	acquire_sem(_profsem);

	f = fopen(path, "w");


	if (f != NULL)
	{
		int ix;
		clock_t clock;
		time(&clock);

		if (_profmaxdeptherror)
		{
			fprintf(f, "WARNING: Could not save all profiling information.  \n");
			fprintf(f, "This program needs a depth of at least %d.  \n", _profmaxdeptherror);
			fprintf(f, "To get all the profiling information, you should change \n");
			fprintf(f, "the value in PROFILE_INIT to a value greater than %d.\n", _profmaxdeptherror);
		}
		
		fprintf(f, "Maximum depth used: %ld\n", _profmaxuserdepth + 1);
		fprintf(f, "Profile done at %s", ctime(&clock));
		fprintf(f, "# %-10s\t%-8s\t%s\t%s\t%s\t\n", "count", "usec/call", "user", "kernel", "wallclock");
//		fprintf(f, "# count\tname\tuser\tkernel\twallclock\n");

		char methodname[1024];
		for (ix = 0; ix < _profhashsize; ix++)
		{
			thr_result *res = &_profresults[ix];
			while (res != NULL)
			{
				if (res->funcname != NULL)
				{
					MWUnmangle(res->funcname, methodname, 1024);
					fprintf(f, "%9Ld\t%8.2f\t%s\t%Ld\t%Ld\t%Ld\n",
						
						res->called,
						((float)res->clock / (float)res->called),
						methodname,
						res->user,
						res->kernel,
						res->clock);
				}
				res = res->next;
			}
		}
		fclose(f);
	    change_file_type(path, "text/x-prof-code");
	}
	release_sem(_profsem);
}


