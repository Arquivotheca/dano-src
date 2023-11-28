#include "IRegion.h"

#include <kernel/OS.h>
#include <support/atomic.h>
#include <support/Autolock.h>
#include <support/Debug.h>
#include <support/StreamIO.h>
#include <support/StringIO.h>

#include <stdlib.h>
#include <stdio.h>

#include <new>

// We always want to compile in the region rotation function,
// even though the app_server may be built without it enabled.
#undef ROTATE_DISPLAY
#define ROTATE_DISPLAY 1

#include <app_server_p/display_rotation.h>

// Optimization options
#define OPT_REGION_ALLOC 1		// Cache region memory?
#define OPT_SIMPLE_REGIONS 1	// Check for simple "1 rect OP 1 rect -> 1 rect" cases?

// Debugging options
#define DB_INTEGRITY_CHECKS 0	// Make sure region is valid after operations?
#define DB_WATCH_ALLOC 0		// Track memory allocations for leak checking?
#define DB_GROWTH_CHECK 0		// Report needless memory growth?
#define DB_MALLOC_DEBUG 0		// Add checks for compatibility with -fcheck-memory-usage?
#define DB_NO_SHARING 0			// Don't include copy-on-write optimization?

// This is the smallest number of rectangles we will allocate for out-of-line
// region data.
#define MIN_AVAIL 8

// Macro for performing bounds check to not trip over -fcheck-memory-usage.
// This is to protected places where we are needlessly reading past the end
// of arrays, but not in a damaging way.
#if DB_MALLOC_DEBUG
#define BCHECK(x) x
#else
#define BCHECK(x)
#endif

// Optimization for gcc
#if __GNUC__
#define DECLARE_RETURN(x) return x
#else
#define DECLARE_RETURN(x)
#endif

#define	smax(x1, x2)	if (x1 < x2) x1 = x2
#define	smin(x1, x2)	if (x1 > x2) x1 = x2
#define	MAX_OUT			0x0FFFFFFF

static const clipping_rect invalRect = {MAX_OUT,MAX_OUT,-MAX_OUT,-MAX_OUT};

int32 max_region_mem = 0;
int32 cur_region_mem = 0;
int32 max_region_cnt = 0;
int32 cur_region_cnt = 0;

/*----------------------------------------------------------------*/

#if DB_WATCH_ALLOC
static void	add_region_memory(int32 amount)
{
	//printf("Adding %ld bytes...\n", amount);
	amount = atomic_add(&cur_region_mem, amount) + amount;
	int32 current;
	while ((current=max_region_mem) < amount) {
		compare_and_swap32(&max_region_mem, current, amount);
	}
	//printf("New total = %ld, new maximum = %ld\n", cur_region_mem, max_region_mem);
}

static void	rem_region_memory(int32 amount)
{
	//printf("Removing %ld bytes....\n", amount);
	atomic_add(&cur_region_mem, -amount);
	//printf("New total = %ld\n", cur_region_mem);
}

static void	add_region_count()
{
	const int32 amount = atomic_add(&cur_region_cnt, 1) + 1;
	int32 current;
	while ((current=max_region_cnt) < amount) {
		compare_and_swap32(&max_region_cnt, current, amount);
	}
}

static void	rem_region_count()
{
	atomic_add(&cur_region_cnt, -1);
}

#define ADD_REGION_MEMORY(a) add_region_memory(a)
#define REM_REGION_MEMORY(a) rem_region_memory(a)
#define ADD_REGION_COUNT() add_region_count()
#define REM_REGION_COUNT() rem_region_count()

#else

#define ADD_REGION_MEMORY(a) 
#define REM_REGION_MEMORY(a) 
#define ADD_REGION_COUNT() 
#define REM_REGION_COUNT() 

#endif

/*----------------------------------------------------------------*/

#if DB_INTEGRITY_CHECKS
#define CHECK_REGION(r) (r).CheckIntegrity();
#else
#define CHECK_REGION(r)
#endif

/*----------------------------------------------------------------*/

#if OPT_REGION_ALLOC

namespace BPrivate {

class IRegionCache
{
public:
	IRegionCache()
	{
		fFastSem = create_sem(0, "region alloc sem");
	}
	~IRegionCache()
	{
		acquire_fast_sem();
		IRegion::region* data = fDataList;
		while (data) {
			IRegion::region* next = (IRegion::region*)data->link;
			free(data);
			data = next;
		}
		fDataList = NULL;
		IRegion* reg = fRegionList;
		while (reg) {
			IRegion* next = (IRegion*)reg->fBounds.link;
			delete reg;
			reg = next;
		}
		fRegionList = NULL;
		release_fast_sem();
		delete_sem(fFastSem);
	}

	static void acquire_fast_sem()
	{
		if (atomic_add(&fFastLock, 1) >= 1)
			acquire_sem(fFastSem);	
	}
	
	static void release_fast_sem()
	{
		if (atomic_add(&fFastLock, -1) > 1)
			release_sem(fFastSem);
	}

	static IRegion::region* GetData(const int32 count);
	static void SaveData(IRegion::region* reg);

	static IRegion* GetRegion();
	static void SaveRegion(IRegion* reg);
	
private:
	static int32 fFastLock;
	static sem_id fFastSem;
	static int32 fDataCount;
	static IRegion::region* fDataList;
	static int32 fRegionCount;
	static IRegion* fRegionList;
};

inline IRegion::region* IRegionCache::GetData(const int32 count)
	DECLARE_RETURN(top)
{
	acquire_fast_sem();
	IRegion::region* top = fDataList;
	IRegion::region** prev = &fDataList;
	while (top) {
		if (top->avail >= count) {
			fDataCount--;
			#if DB_INTEGRITY_CHECKS
			if (*prev != top)
				debugger("Bad prev pointer in GetData()");
			#endif
			*prev = (IRegion::region*)top->link;
			release_fast_sem();
			top->link = NULL;
			return top;
		}
		prev = (IRegion::region**)&(top->link);
		top = (IRegion::region*)top->link;
	}
	release_fast_sem();
	if (count < 8) {
		top = (IRegion::region*)malloc(sizeof(IRegion::region) + 7*sizeof(clipping_rect));
		if (top) top->avail = 8;
	} else {
		top = (IRegion::region*)malloc(sizeof(IRegion::region) + (count-1)*sizeof(clipping_rect));
		if (top) top->avail = count;
	}
	return top;
}

inline void IRegionCache::SaveData(IRegion::region* reg)
{
	#if DB_INTEGRITY_CHECKS
	if (reg->avail < MIN_AVAIL)
		debugger("Saving bad avail");
	if (reg->refs != 0)
		debugger("Saving bad refs");
	#endif
	if (fDataCount < 32) {
		acquire_fast_sem();
		reg->link = fDataList;
		fDataList = reg;
		fDataCount++;
		release_fast_sem();
	} else {
		free(reg);
	}
}

int32 IRegionCache::fFastLock = 0;
sem_id IRegionCache::fFastSem = B_BAD_SEM_ID;
int32 IRegionCache::fDataCount = 0;
IRegion::region* IRegionCache::fDataList = NULL;
int32 IRegionCache::fRegionCount = 0;
IRegion* IRegionCache::fRegionList = NULL;

static IRegionCache cache;
}
using namespace BPrivate;
#endif

inline void IRegion::region::acquire() const
{
	atomic_add(&refs, 1);
}

inline void IRegion::region::release() const
{
	if (atomic_add(&refs, -1) == 1) {
		REM_REGION_MEMORY(sizeof(region) + sizeof(clipping_rect)*(avail-1));
		#if OPT_REGION_ALLOC
		IRegionCache::SaveData(const_cast<region*>(this));
		#else
		free(const_cast<region*>(this));
		#endif
	}
}

inline IRegion::region* IRegion::region::create(const int32 init_avail) DECLARE_RETURN(r)
{
	ADD_REGION_MEMORY(sizeof(region) + sizeof(clipping_rect)*(init_avail-1));
	#if OPT_REGION_ALLOC
	region* r =  IRegionCache::GetData(init_avail);
	#else
	region* r;
	if (init_avail < 8) {
		r = (IRegion::region*)malloc(sizeof(IRegion::region) + 7*sizeof(clipping_rect));
		if (r) r->avail = 8;
	} else {
		r = (IRegion::region*)malloc(sizeof(IRegion::region) + (init_avail-1)*sizeof(clipping_rect));
		if (r) r->avail = init_avail;
	}
	#endif
	if (r) {
		r->refs = 1;
		r->count = 0;
		#if DB_INTEGRITY_CHECKS
		if (r->avail < MIN_AVAIL)
			debugger("Created bad avail");
		#endif
	}
	return r;
}

inline IRegion::region* IRegion::region::edit(const int32 needed, const int32 coulduse) const DECLARE_RETURN(r)
{
	region* r;
	
	// Are we the sole owner of this region?
	if (refs <= 1) {
		if (needed <= avail && coulduse >= (avail/4)) {
			return const_cast<region*>(this);
		}
		
		r = const_cast<region*>(this);
		ADD_REGION_MEMORY(sizeof(clipping_rect)*(coulduse-avail));
		r = (region*)realloc(r, sizeof(region) + sizeof(clipping_rect)*(coulduse-1));
		if (r) {
			r->avail = coulduse;
			#if DB_INTEGRITY_CHECKS
			if (r->avail < MIN_AVAIL)
				debugger("Editing bad avail");
			#endif
		}
		return r;
	}
	
	// Need to copy the shared region.
	r = create(coulduse);
	if (r) {
		const int32 size = needed < count ? needed : count;
		memcpy(r->rects, rects, sizeof(clipping_rect)*(size));
		r->count = size;
		#if DB_INTEGRITY_CHECKS
		if (r->avail < MIN_AVAIL)
			debugger("Editing bad avail");
		#endif
	}
	release();
	return r;
}

inline IRegion::region* IRegion::region::reuse(const int32 needed, const int32 coulduse) const
{
	// Are we the sole owner of this region, and won't be resizing it?
	if (refs <= 1 && needed <= avail && coulduse >= (avail/4)) {
		return const_cast<region*>(this);
	}
	
	// Handle resizing and needing to copy a shared region in the same way.
	// (It isn't worthwhile to do a realloc() here because we aren't going
	// to need the existing data anyway.)
	release();
	return create(coulduse);
}

/*----------------------------------------------------------------*/
/*----------------------------------------------------------------*/
/*----------------------------------------------------------------*/

IRegion::IRegion()
{
	ADD_REGION_COUNT();
	fData = &fBounds;
	fBounds.refs = 10000;
	fBounds.count = 0;
	fBounds.avail = 1;
	Bounds() = invalRect;
	CHECK_REGION(*this);
}

IRegion::IRegion(const clipping_rect& r)
{
	ADD_REGION_COUNT();
	fData = &fBounds;
	fBounds.refs = 10000;
	fBounds.count = 1;
	fBounds.avail = 1;
	Bounds() = r;
	CHECK_REGION(*this);
}

IRegion::IRegion(const IRegion& o)
{
	CHECK_REGION(o);
	
	ADD_REGION_COUNT();
	fBounds = o.fBounds;
	if (o.fData == &o.fBounds || !o.fData) {
		fData = &fBounds;
		fBounds.refs = 10000;
	} else {
		#if DB_INTEGRITY_CHECKS
		if (o.fData->avail < MIN_AVAIL)
			debugger("Bad incoming avail");
		#endif
		#if DB_NO_SHARING
		fData = region::create(o.fData->count);
		if (fData) {
			memcpy(const_cast<region*>(fData)->rects, o.fData->rects, o.fData->count*sizeof(clipping_rect));
			const_cast<region*>(fData)->count = o.fData->count;
		}
		#else
		fData = o.fData;
		fData->acquire();
		#endif
	}
	CHECK_REGION(*this);
}

IRegion::~IRegion()
{
	CHECK_REGION(*this);
	REM_REGION_COUNT();
	if (fData) {
		fData->release();
	}
}

/*----------------------------------------------------------------*/

IRegion& IRegion::operator=(const IRegion& o)
{
	CHECK_REGION(o);
	
	if (this == &o) return *this;
	if (fData) {
		fData->release();
	}
	fBounds = o.fBounds;
	if (o.fData == &o.fBounds || !o.fData) {
		fData = &fBounds;
		fBounds.refs = 10000;
	} else {
		#if DB_INTEGRITY_CHECKS
		if (o.fData->avail < MIN_AVAIL)
			debugger("Bad incoming avail");
		#endif
		#if DB_NO_SHARING
		fData = region::create(o.fData->count);
		if (fData) {
			memcpy(const_cast<region*>(fData)->rects, o.fData->rects, o.fData->count*sizeof(clipping_rect));
			const_cast<region*>(fData)->count = o.fData->count;
		}
		#else
		fData = o.fData;
		fData->acquire();
		#endif
	}
	CHECK_REGION(*this);
	return *this;
}

bool IRegion::operator==(const IRegion& o) const
{
	if (!fData || !o.fData)
		return fData == o.fData;
	if (fData->count != o.fData->count)
		return false;
	if ((Bounds().top != o.Bounds().top) ||
		(Bounds().left != o.Bounds().left) ||
		(Bounds().right != o.Bounds().right) ||
		(Bounds().bottom != o.Bounds().bottom))
		return false;
	if (fData == o.fData)
		return true;
	if (memcmp(fData->rects, o.fData->rects, sizeof(clipping_rect)*fData->count) == 0)
		return true;
	return false;
}

/*----------------------------------------------------------------*/

inline IRegion::region* IRegion::Edit(const int32 needed, const int32 coulduse) const
{
	// Fast case: we own the region, and don't need to resize it.
	if ((fData == &fBounds || fData->refs <= 1) && needed <= fData->avail)
		return const_cast<region*>(fData);
	
	// All other cases.
	return EditSlow(needed, coulduse);
}

inline IRegion::region* IRegion::ReUse(const int32 needed, const int32 coulduse) const
{
	// Fast case: we own the region, and don't need to resize it.
	if ((fData == &fBounds || (fData && fData->refs <= 1)) && needed <= fData->avail)
		return const_cast<region*>(fData);
	
	// All other cases.
	return ReUseSlow(needed, coulduse);
}

/*----------------------------------------------------------------*/

clipping_rect* IRegion::EditRects(int32 needed, int32 coulduse)
{
	if (!fData) return NULL;
	region* reg = Edit(needed, coulduse);
	if (reg) return reg->rects;
	return NULL;
}

clipping_rect* IRegion::CreateRects(int32 needed, int32 coulduse)
{
	region* reg = ReUse(needed, coulduse);
	if (reg) return reg->rects;
	return NULL;
}

void IRegion::SetRectCount(int32 num)
{
	if (!fData) return;
	region* reg = Edit(num, num);
	if (reg) reg->count = num;
}

/*----------------------------------------------------------------*/

void IRegion::MakeEmpty()
{
	if (!fData) {
		// Recover from memory error.
		fData = &fBounds;
	} else if (fData->refs > 1) {
		fData->release();
		fData = &fBounds;
	}
	const_cast<region*>(fData)->count = 0;
	fBounds.refs = 10000;
	fBounds.avail = 1;
	Bounds() = invalRect;
}

void IRegion::Set(const clipping_rect& r)
{
	if (r.is_valid()) {
		region* reg = ReUse(1, 1);
		if (reg) {
			reg->count = 1;
			reg->rects[0] = Bounds() = r;
		}
		CHECK_REGION(*this);
	} else {
		MakeEmpty();
	}
}

/*----------------------------------------------------------------*/

void IRegion::AddRect(const clipping_rect& r)
{
	if (!r.is_valid() || !fData) return;

	if (fData->count == 0) {
		Set(r);
		return;
	}

	region* reg = Grow(1);
	if (reg) {
		reg->rects[reg->count++] = r;
		Bounds().bottom = r.bottom;
		smin(Bounds().left, r.left);
		smax(Bounds().right, r.right);
	}
}

void IRegion::OrRect(const clipping_rect& r)
{
	if (!r.is_valid() || !fData) return;

	if (fData->count == 0) {
		Set(r);
		return;
	}

	if (r.top > Bounds().bottom) {
		region* reg = Grow(1);
		reg->rects[reg->count++] = r;
		Bounds().bottom = r.bottom;
		smin(Bounds().left, r.left);
		smax(Bounds().right, r.right);
		return;
	}

	OrSelf(IRegion(r));
}

/*----------------------------------------------------------------*/

void IRegion::XOr(const IRegion& r2, IRegion* result) const
{
	IRegion tmp1, tmp2;

	r2.Sub(*this, &tmp1);
	Sub(r2, &tmp2);
	tmp1.Or(tmp2, result);
}

/*----------------------------------------------------------------*/

/*	Please forgive me for these three functions, but all these gotos
	really seem like the best way to special case the loops for speed. */

void IRegion::Sub(const IRegion& r2, IRegion* target) const
{
	CHECK_REGION(*this);
	CHECK_REGION(r2);
	
	if ((Bounds().bottom < r2.Bounds().top) ||
		(Bounds().top > r2.Bounds().bottom) ||
		(Bounds().left > r2.Bounds().right) ||
		(Bounds().right < r2.Bounds().left) ||
		(r2.IsEmpty())) {
		*target = *this;
		return;
	};

	if (IsEmpty()) {
		target->MakeEmpty();
		return;
	};

#if OPT_SIMPLE_REGIONS
	// First check if we can optimize a 1 rect - 1 rect -> 1 rect case.
	// (If we allow these situations to fall through to the generic case,
	// we will needlessly allocate data for the target region.)
	if (CountRects() == 1 && r2.CountRects() == 1) {
		if (	Bounds().top >= r2.Bounds().top &&
				Bounds().bottom <= r2.Bounds().bottom) {
			// We may be shrinking horizontally...
			if (	r2.Bounds().left <= Bounds().left &&
					r2.Bounds().right < Bounds().right) {
				// r2 is removing the left of r1...
				clipping_rect r;
				r.left = r2.Bounds().right+1;
				r.top = Bounds().top;
				r.right = Bounds().right;
				r.bottom = Bounds().bottom;
				target->Set(r);
				return;
			} else if (	r2.Bounds().right >= Bounds().right &&
						r2.Bounds().left > Bounds().left) {
				// r2 is removing the right of r1...
				clipping_rect r;
				r.left = Bounds().left;
				r.top = Bounds().top;
				r.right = r2.Bounds().left-1;
				r.bottom = Bounds().bottom;
				target->Set(r);
				return;
			}
		} else if (	Bounds().left >= r2.Bounds().left &&
					Bounds().right <= r2.Bounds().right) {
			// We may be shrinking vertically...
			if (	r2.Bounds().top <= Bounds().top &&
					r2.Bounds().bottom < Bounds().bottom) {
				// r2 is removing the top of r1...
				clipping_rect r;
				r.left = Bounds().left;
				r.top = r2.Bounds().bottom+1;
				r.right = Bounds().right;
				r.bottom = Bounds().bottom;
				target->Set(r);
				return;
			} else if (	r2.Bounds().bottom >= Bounds().bottom &&
						r2.Bounds().top > Bounds().top) {
				// r2 is removing the bottom of r1...
				clipping_rect r;
				r.left = Bounds().left;
				r.top = Bounds().top;
				r.right = Bounds().right;
				r.bottom = r2.Bounds().top-1;
				target->Set(r);
				return;
			}
		}
	}
#endif

#if DB_GROWTH_CHECK
	const int32 avail = target->CountAvail();
#endif

	clipping_rect *prevdRow=NULL,*lastd,*thisdRow,*d,*de;
	const clipping_rect *rp1=Rects(),*rp2=r2.Rects();
	const clipping_rect *sp1=rp1,*sp2=rp2;
	const clipping_rect *np1=rp1,*np2=rp2;
	const clipping_rect *ep1=rp1+CountRects(),*ep2=rp2+r2.CountRects();
	int32 y,bottom=0,tmp1,tmp2,toggle=-1,left,right;
	
	region* dst = target->ReUse(0, 0);
	if (!dst)
		return;
	
	dst->count = 0;
	d = dst->rects-1;
	de = dst->rects+dst->avail;
	
	left = 0x7FFFFFFF;
	right = -0x7FFFFFFF;

	y = sp1->top;
	while ((np1 < ep1) && (np1->top == y)) np1++;
	checkAgainFirst:
	if (np2->top <= y) {
		while ((np2 < ep2) && (np2->top == sp2->top)) np2++;
		if (sp2->bottom < y) {
			sp2 = np2;
			if (sp2 == ep2) goto doneWithSrc2;
			goto checkAgainFirst;
		};
	};
	
	while (1) {
		if (sp2 < np2) {
			rp1 = sp1;
			rp2 = sp2;
			while ((de-(d+1)) < (((np1-sp1) + (np2-sp2))*2)) {
				tmp1 = d-dst->rects;
				tmp2 = prevdRow-dst->rects;
				if (! (dst = target->GrowAvail(1)) )
					return;
				d = dst->rects + tmp1;
				de = dst->rects + dst->avail;
				if (prevdRow) prevdRow = dst->rects + tmp2;
			};
			thisdRow = d;
			lastd = prevdRow;
			if (y != bottom+1) lastd = NULL;
			toggle = 0;
			bottom = sp1->bottom;
			if (bottom > sp2->bottom) bottom = sp2->bottom;

			while (rp2->right < rp1->left) {
				rp2++;
				if (rp2 == np2) break;
			};
			d++;
			d->top = y;
			d->bottom = bottom;
			d->left = rp1->left;
			d->right = rp1->right;
			rp1++;
			if (rp2 == np2) goto doneWithRow2;
			
			while (1) {
				{
					while (d->right < rp2->left) {
						loopStart1:
						if (rp1 == np1) goto doneWithRow1;
	
						if (lastd) {
							lastd++;
							if ((lastd > thisdRow) ||
								(d->left != lastd->left) ||
								(d->right != lastd->right))
								lastd = NULL;
						};
	
						d++;
						d->top = y;
						d->bottom = bottom;
						d->left = rp1->left;
						d->right = rp1->right;
						rp1++;
					};
					while (rp2->right < d->left) {
						rp2++;
						if (rp2 == np2) goto doneWithRow2;
					};
					if (d->right < rp2->left) goto loopStart1;
				} 

				if (rp2->left <= d->left) {
					d->left = rp2->right+1;
					if (d->left > d->right) {
						if (rp1 == np1) {
							d--;
							goto doneWithRow1;
						};
						d->top = y;
						d->bottom = bottom;
						d->left = rp1->left;
						d->right = rp1->right;
						rp1++;
					};
				} else {
					tmp1 = d->right;
					d->right = rp2->left-1;
					if (tmp1 > rp2->right) {
						if (lastd) {
							lastd++;
							if ((lastd > thisdRow) ||
								(d->left != lastd->left) ||
								(d->right != lastd->right))
								lastd = NULL;
						};

						d++;
						d->top = y;
						d->bottom = bottom;
						d->left = rp2->right+1;
						d->right = tmp1;
					} else {
						if (rp1 == np1) goto doneWithRow1;

						if (lastd) {
							lastd++;
							if ((lastd > thisdRow) ||
								(d->left != lastd->left) ||
								(d->right != lastd->right))
								lastd = NULL;
						};

						d++;
						d->top = y;
						d->bottom = bottom;
						d->left = rp1->left;
						d->right = rp1->right;
						rp1++;
					};
				};
			};

			doneWithRow2:
			
			while (rp1 < np1) {
				if (lastd) {
					lastd++;
					if ((lastd > thisdRow) ||
						(d->left != lastd->left) ||
						(d->right != lastd->right))
						lastd = NULL;
				};

				d++;
				d->top = y;
				d->bottom = bottom;
				d->left = rp1->left;
				d->right = rp1->right;
				rp1++;
			};

			doneWithRow1:

			if (d > thisdRow) {
				if (lastd) {
					lastd++;
					if ((lastd == thisdRow) &&
						(d->left == lastd->left) &&
						(d->right == lastd->right)) {
						lastd = prevdRow;
						while (lastd < thisdRow)
							(++lastd)->bottom = bottom;
						d = thisdRow;
					} else
						prevdRow = thisdRow;
				} else
					prevdRow = thisdRow;

				if (left > (thisdRow+1)->left) left = (thisdRow+1)->left;
				if (right < d->right) right = d->right;
			} else
				prevdRow = NULL;

			if (bottom == sp1->bottom) {
				sp1 = np1;
				if (sp1 == ep1) goto doneWithSrc1;
				y = sp1->top;
				while ((np1 < ep1) && (np1->top == y)) np1++;
			} else
				y = bottom + 1;

			checkAgain2:
			if (y > sp2->bottom) {
				sp2 = np2;
				if (sp2 == ep2) goto doneWithSrc2;
				if (sp2->top <= y) {
					while ((np2 < ep2) && (np2->top == sp2->top)) np2++;
					goto checkAgain2;
				};
			};
		} else { // we know that (sp1 < np1) always holds
			while ((de-(d+1)) <= (np1-sp1)) {
				tmp1 = d-dst->rects;
				tmp2 = prevdRow-dst->rects;
				if (! (dst = target->GrowAvail(1)) )
					return;
				d = dst->rects + tmp1;
				de = dst->rects + dst->avail;
				if (prevdRow) prevdRow = dst->rects + tmp2;
			};
			rp1 = sp1;
			thisdRow = d;
			lastd = prevdRow;
			if ((y != bottom+1) || (toggle == 1)) lastd = NULL;
			toggle = 1;
			bottom = sp1->bottom;
			if (bottom >= sp2->top) bottom = sp2->top-1;

			d++;
			d->top = y;
			d->bottom = bottom;
			d->left = rp1->left;
			d->right = rp1->right;
			rp1++;

			if (left > d->left) left = d->left;

			while (rp1 < np1) {
				if (lastd) {
					lastd++;
					if ((lastd > thisdRow) ||
						(d->left != lastd->left) ||
						(d->right != lastd->right))
						lastd = NULL;
				};

				d++;
				d->top = y;
				d->bottom = bottom;
				d->left = rp1->left;
				d->right = rp1->right;
				rp1++;
			};

			if (lastd) {
				lastd++;
				if ((lastd == thisdRow) &&
					(d->left == lastd->left) &&
					(d->right == lastd->right)) {
					lastd = prevdRow;
					while (lastd < thisdRow)
						(++lastd)->bottom = bottom;
					d = thisdRow;
				} else
					prevdRow = thisdRow;
			} else
				prevdRow = thisdRow;

			if (right < d->right) right = d->right;

			if (bottom == sp1->bottom) {
				sp1 = rp1;
				if (sp1 == ep1) goto doneWithSrc1;
				y = sp1->top;
				while ((np1 < ep1) && (np1->top == y)) np1++;
			} else
				y = bottom + 1;

			checkAgain:
			if (sp2->top <= y) {
				while ((np2 < ep2) && (np2->top == sp2->top)) np2++;
				if (y > sp2->bottom) {
					sp2 = np2;
					if (sp2 == ep2) goto doneWithSrc2;
					goto checkAgain;
				};
			};
		};
	};

	doneWithSrc2:
	
	if (sp1 < ep1) {
		while ((de-(d+1)) <= (ep1-sp1)) {
			tmp1 = d-dst->rects;
			tmp2 = prevdRow-dst->rects;
			if (! (dst = target->GrowAvail(1)) )
				return;
			d = dst->rects + tmp1;
			de = dst->rects + dst->avail;
			if (prevdRow) prevdRow = dst->rects + tmp2;
		};
	
		thisdRow = d;
		lastd = prevdRow;
		if ((y != bottom+1) || (toggle == 1)) lastd = NULL;
	
		if (sp1 == np1) {
			y = np1->top;
			while ((np1 < ep1) && (np1->top == y)) np1++;
		};

		bottom = sp1->bottom;

		d++;
		d->left = sp1->left;
		d->right = sp1->right;
		d->bottom = bottom;
		d->top = y;
		if (left > d->left) left = d->left;
		sp1++;
	
		while (sp1 < np1) {
			if (lastd) {
				lastd++;
				if ((lastd > thisdRow) ||
					(d->left != lastd->left) ||
					(d->right != lastd->right))
					lastd = NULL;
			};

			d++;
			d->left = sp1->left;
			d->right = sp1->right;
			d->bottom = bottom;
			d->top = y;
			sp1++;
		};

		if (lastd) {
			lastd++;
			if ((lastd == thisdRow) &&
				(d->left == lastd->left) &&
				(d->right == lastd->right)) {
				lastd = prevdRow;
				while (lastd < thisdRow)
					(++lastd)->bottom = bottom;
				d = thisdRow;
			};
		};

		if (right < d->right) right = d->right;

		while (np1 < ep1) {
			while ((np1 < ep1) && (np1->top == sp1->top)) np1++;
			*++d = *sp1++;
			if (left > d->left) left = d->left;
			while (sp1 < np1) *++d = *sp1++;
			if (right < d->right) right = d->right;
		};
	};

	doneWithSrc1:

	if (d >= dst->rects) {
		if (left > d->left) left = d->left;
		if (right < d->right) right = d->right;
		dst->count = d - dst->rects + 1;
		target->Bounds().top = dst->rects[0].top;
		target->Bounds().bottom = dst->rects[dst->count-1].bottom;
		target->Bounds().left = left;
		target->Bounds().right = right;
		CHECK_REGION(*target);
	} else
		target->MakeEmpty();

#if DB_GROWTH_CHECK
	if (CountRects() == 1 && r2.CountRects() == 1 && target->CountRects() == 1
			&& avail <= 1 && target->CountAvail() > 1) {
		BOut << "*** SUB Needlessly grew to " << target->CountAvail() << "!" << endl
				<< "Source1: " << *this << endl
				<< "Source2: " << r2 << endl
				<< "Final: " << *target << endl;
	}
#endif

//	lastFuckingLabelInThisWholeGoddamnedFunction:;
};

/*----------------------------------------------------------------*/

void IRegion::And(const IRegion& r2, IRegion* target) const
{
	CHECK_REGION(*this);
	CHECK_REGION(r2);
	
	if ((Bounds().bottom < r2.Bounds().top) ||
		(Bounds().top > r2.Bounds().bottom) ||
		(Bounds().left > r2.Bounds().right) ||
		(Bounds().right < r2.Bounds().left) ||
		(IsEmpty()) ||
		(r2.IsEmpty())) {
		target->MakeEmpty();
		return;
	};

#if OPT_SIMPLE_REGIONS
	// First check if we can optimize a 1 rect & 1 rect -> 1 rect case.
	// (If we allow these situations to fall through to the generic case,
	// we will needlessly allocate data for the target region.)
	if (CountRects() == 1 && r2.CountRects() == 1) {
		// The intersection of two rectangles is always a single rectangle.
		clipping_rect r;
		r.left = Bounds().left >= r2.Bounds().left ? Bounds().left : r2.Bounds().left;
		r.top = Bounds().top >= r2.Bounds().top ? Bounds().top : r2.Bounds().top;
		r.right = Bounds().right <= r2.Bounds().right ? Bounds().right : r2.Bounds().right;
		r.bottom = Bounds().bottom <= r2.Bounds().bottom ? Bounds().bottom : r2.Bounds().bottom;
		target->Set(r);
		return;
	}
#endif

#if DB_GROWTH_CHECK
	const int32 avail = target->CountAvail();
#endif

	clipping_rect *prevdRow=NULL,*lastd,*thisdRow,*d,*de;
	const clipping_rect *rp1=Rects(),*rp2=r2.Rects();
	const clipping_rect *sp1=rp1,*sp2=rp2;
	const clipping_rect *np1=rp1,*np2=rp2;
	const clipping_rect *ep1=rp1+CountRects(),*ep2=rp2+r2.CountRects();
	int32 y,bottom=0,left,right,tmp1,tmp2;

	region* dst = target->ReUse(0, 0);
	if (!dst)
		return;
	
	dst->count = 0;
	d = dst->rects-1;
	de = dst->rects+dst->avail;
	
	left = 0x7FFFFFFF;
	right = -0x7FFFFFFF;

	while ((np1 < ep1) && (np1->top == sp1->top)) np1++;
	while ((np2 < ep2) && (np2->top == sp2->top)) np2++;
	
	y = sp1->top;
	if (y < sp2->top) {
		y = sp2->top;
		goto advancey1;
	};
	goto advancey2;
	
	while (1) {
		rp1 = sp1;
		rp2 = sp2;
		while ((de-(d+1)) < (((np1-sp1) + (np2-sp2))*2)) {
			tmp1 = d-dst->rects;
			tmp2 = prevdRow-dst->rects;
			if (! (dst = target->GrowAvail(1)) )
				return;
			d = dst->rects + tmp1;
			de = dst->rects + dst->avail;
			if (prevdRow) prevdRow = dst->rects + tmp2;
		};
		thisdRow = d;
		lastd = prevdRow;
		if (y != bottom+1) lastd = NULL;
		bottom = sp1->bottom;
		if (bottom > sp2->bottom) bottom = sp2->bottom;

		while (1) {
			while (rp1->right < rp2->left) {
				advancex1:
				rp1++;
				if (rp1 == np1) goto doneWithRow;
			};
			while (rp2->right < rp1->left) {
//				advancex2:
				rp2++;
				if (rp2 == np2) goto doneWithRow;
			};
			if (rp1->right < rp2->left) goto advancex1;

			d++;

			d->left = rp1->left;
			if (d->left < rp2->left) d->left = rp2->left;
			if (left > d->left) left = d->left;

			d->right = rp1->right;
			if (d->right > rp2->right) d->right = rp2->right;
			if (right < d->right) right = d->right;
			
			d->top = y;
			d->bottom = bottom;
			
			if (lastd) {
				lastd++;
				if ((lastd > thisdRow) ||
					(d->left != lastd->left) ||
					(d->right != lastd->right))
					lastd = NULL;
			};
			
			if (rp1->right == d->right) {
				rp1++;
				if (rp1 == np1) goto doneWithRow;
			};

			if (rp2->right == d->right) {
				rp2++;
				if (rp2 == np2) goto doneWithRow;
			};
		};
		
		doneWithRow:
		
		if (lastd == thisdRow) {
			lastd = prevdRow;
			while (lastd < thisdRow)
				(++lastd)->bottom = bottom;
			d = thisdRow;
		} else {
			prevdRow = thisdRow;
		};
		y = bottom + 1;

		advancey1:
		while (sp1->bottom < y) {
			sp1 = np1;
			if (sp1 == ep1) goto done;
			while ((np1 < ep1) && (np1->top == sp1->top)) np1++;
			if (y < sp1->top) y = sp1->top;
		};

		advancey2:
		while (sp2->bottom < y) {
			sp2 = np2;
			if (sp2 == ep2) goto done;
			while ((np2 < ep2) && (np2->top == sp2->top)) np2++;
			if (y < sp2->top) {
				y = sp2->top;
				goto advancey1;
			};
		};
	};

	done:
	if (d >= dst->rects) {
		dst->count = d - dst->rects + 1;
		target->Bounds().top = dst->rects[0].top;
		target->Bounds().bottom = dst->rects[dst->count-1].bottom;
		target->Bounds().left = left;
		target->Bounds().right = right;
		CHECK_REGION(*target);
	} else
		target->MakeEmpty();
	
#if DB_GROWTH_CHECK
	if (CountRects() == 1 && r2.CountRects() == 1 && target->CountRects() == 1
			&& avail <= 1 && target->CountAvail() > 1) {
		BOut << "*** AND Needlessly grew to " << target->CountAvail() << "!" << endl
				<< "Source1: " << *this << endl
				<< "Source2: " << r2 << endl
				<< "Final: " << *target << endl;
	}
#endif
}

/*----------------------------------------------------------------*/

void IRegion::Or(const IRegion& r2, IRegion* target) const
{
	CHECK_REGION(*this);
	CHECK_REGION(r2);
	
	if (IsEmpty()) {
		*target = r2;
		return;
	};
	
	if (r2.IsEmpty()) {
		*target = *this;
		return;
	};

#if OPT_SIMPLE_REGIONS
	// First check if we can optimize a 1 rect | 1 rect -> 1 rect case.
	// (If we allow these situations to fall through to the generic case,
	// we will needlessly allocate data for the target region.)
	if (CountRects() == 1 && r2.CountRects() == 1) {
		if (	Bounds().left >= r2.Bounds().left &&
				Bounds().top >= r2.Bounds().top &&
				Bounds().right <= r2.Bounds().right &&
				Bounds().bottom <= r2.Bounds().bottom) {
			// r1 is entirely contained in r2...
			target->Set(r2.Bounds());
			return;
		} else if (	Bounds().left <= r2.Bounds().left &&
					Bounds().top <= r2.Bounds().top &&
					Bounds().right >= r2.Bounds().right &&
					Bounds().bottom >= r2.Bounds().bottom) {
			// r2 is entirely contained in r2...
			target->Set(Bounds());
			return;
		} else if (	Bounds().top == r2.Bounds().top &&
					Bounds().bottom == r2.Bounds().bottom) {
			// We may be growing horizontally...
			if (	Bounds().left <= r2.Bounds().left &&
					Bounds().right >= (r2.Bounds().left-1)) {
				// r1 is to the left of r2...
				clipping_rect r;
				r.left = Bounds().left;
				r.top = Bounds().top;
				r.right = r2.Bounds().right;
				r.bottom = Bounds().bottom;
				target->Set(r);
				return;
			} else if (	Bounds().right >= r2.Bounds().right &&
						Bounds().left <= (r2.Bounds().right+1)) {
				// r1 is to the right of r2...
				clipping_rect r;
				r.left = r2.Bounds().left;
				r.top = Bounds().top;
				r.right = Bounds().right;
				r.bottom = Bounds().bottom;
				target->Set(r);
				return;
			}
		} else if (	Bounds().left == r2.Bounds().left &&
					Bounds().right == r2.Bounds().right) {
			// We may be growing vertically...
			if (	Bounds().top <= r2.Bounds().top &&
					Bounds().bottom >= (r2.Bounds().top-1)) {
				// r1 is above r2...
				clipping_rect r;
				r.left = Bounds().left;
				r.top = Bounds().top;
				r.right = Bounds().right;
				r.bottom = r2.Bounds().bottom;
				target->Set(r);
				return;
			} else if (	Bounds().bottom >= r2.Bounds().bottom &&
						Bounds().top <= (r2.Bounds().bottom+1)) {
				// r1 is below r2...
				clipping_rect r;
				r.left = Bounds().left;
				r.top = r2.Bounds().top;
				r.right = Bounds().right;
				r.bottom = Bounds().bottom;
				target->Set(r);
				return;
			}
		}
	}
#endif

#if DB_GROWTH_CHECK
	const int32 avail = target->CountAvail();
#endif

	clipping_rect *prevdRow=NULL,*lastd,*thisdRow,*d,*de;
	const clipping_rect *rp1=Rects(),*rp2=r2.Rects();
	const clipping_rect *sp1=rp1,*sp2=rp2;
	const clipping_rect *np1=rp1,*np2=rp2;
	const clipping_rect *ep1=rp1+CountRects(),*ep2=rp2+r2.CountRects();
	int32 y,bottom=0,tmp1,tmp2,toggle=-1,toggleCmp=1;
	
	region* dst = target->ReUse(0, 0);
	if (!dst)
		return;
	
	dst->count = 0;
	d = dst->rects-1;
	de = dst->rects+dst->avail;
	
	y = sp1->top;
	if (y > sp2->top) y = sp2->top;
	
	#define MAKE_SPACE(needed, msg)												\
		PRINT(("Target has %ld (%ld total), need %d -- growing %s\n",			\
				de-(d+1), de-dst->rects, needed, msg));							\
		tmp1 = d-dst->rects;													\
		tmp2 = prevdRow-dst->rects;												\
		if (! (dst = target->GrowAvail(1)) )									\
			return;																\
		d = dst->rects + tmp1;													\
		de = dst->rects + dst->avail;											\
		if (prevdRow) prevdRow = dst->rects + tmp2;
	
	while (1) {
		if (BCHECK(np1 < ep1 &&) y == np1->top)
			while ((np1 < ep1) && (np1->top == y)) np1++;
		if (BCHECK(np2 < ep2 &&) y == np2->top)
			while ((np2 < ep2) && (np2->top == y)) np2++;
		if (sp1 < np1) {
			if (sp2 < np2) {
				while ((de-(d+1)) <= ((np1-sp1) + (np2-sp2))) {
					MAKE_SPACE(((np1-sp1)+(np2-sp2)), "#1");
				};
				rp1 = sp1;
				rp2 = sp2;
				thisdRow = d;
				lastd = prevdRow;
				if (y != bottom+1) lastd = NULL;
				toggle = 0;
				bottom = sp1->bottom;
				if (bottom > sp2->bottom) bottom = sp2->bottom;
				if (rp2->left < rp1->left) goto loop2;
				goto loop1;
				while (1) {
					while (rp1->left <= rp2->left) {

						if (lastd) {
							lastd++;
							if ((lastd > thisdRow) ||
								(d->left != lastd->left) ||
								(d->right != lastd->right))
								lastd = NULL;
						};

						loop1:
						d++;
						d->left = rp1->left;
						d->right = rp1->right;
						d->top = y;
						d->bottom = bottom;
						rp1++;
						if (rp1 == np1) goto doneRow1;
					};
					
					while (rp2->left <= d->right+1) {
						if (rp2->right > d->right)
							d->right = rp2->right;
						rp2++;
						if (rp2 == np2) goto doneRow2;
					};

					while (rp2->left <= rp1->left) {

						if (lastd) {
							lastd++;
							if ((lastd > thisdRow) ||
								(d->left != lastd->left) ||
								(d->right != lastd->right))
								lastd = NULL;
						};

						loop2:
						d++;
						d->left = rp2->left;
						d->right = rp2->right;
						d->top = y;
						d->bottom = bottom;
						rp2++;
						if (rp2 == np2) goto doneRow2;
					};
					
					while (rp1->left <= d->right+1) {
						if (rp1->right > d->right)
							d->right = rp1->right;
						rp1++;
						if (rp1 == np1) goto doneRow1;
					};
				};

				doneRow1: {
					while ((rp2 < np2) && (rp2->left <= d->right+1)) {
						if (rp2->right > d->right)
							d->right = rp2->right;
						rp2++;
					};
	
					while (rp2 < np2) {

						if (lastd) {
							lastd++;
							if ((lastd > thisdRow) ||
								(d->left != lastd->left) ||
								(d->right != lastd->right))
								lastd = NULL;
						};

						d++;
						d->left = rp2->left;
						d->right = rp2->right;
						d->top = y;
						d->bottom = bottom;
						rp2++;
					};

					goto doneRowTotal;
				};

				doneRow2: {
					while ((rp1 < np1) && (rp1->left <= d->right+1)) {
						if (rp1->right > d->right)
							d->right = rp1->right;
						rp1++;
					};
	
					while (rp1 < np1) {

						if (lastd) {
							lastd++;
							if ((lastd > thisdRow) ||
								(d->left != lastd->left) ||
								(d->right != lastd->right))
								lastd = NULL;
						};

						d++;
						d->left = rp1->left;
						d->right = rp1->right;
						d->top = y;
						d->bottom = bottom;
						rp1++;
					};

					goto doneRowTotal;
				};

				doneRowTotal: {

					if (lastd) {
						lastd++;
						if ((lastd > thisdRow) ||
							(d->left != lastd->left) ||
							(d->right != lastd->right))
							lastd = NULL;
					};

					if (lastd == thisdRow) {
						lastd = prevdRow;
						while (lastd < thisdRow)
							(++lastd)->bottom = bottom;
						d = thisdRow;
					} else {
						prevdRow = thisdRow;
					};

					if ((bottom == sp1->bottom) && (bottom == sp2->bottom)) {
						sp1 = rp1;
						sp2 = rp2;
						if (sp1 == ep1) {
							if (sp2 == ep2) goto doneTotal;
							y = sp2->top;
							goto doneWithSrc1;
						} else if (sp2 == ep2) {
							y = sp1->top;
							goto doneWithSrc2;
						};
						y = sp1->top;
						if (y > sp2->top) y = sp2->top;
					} else {
						if (bottom == sp1->bottom) sp1 = rp1;
						if (bottom == sp2->bottom) sp2 = rp2;
						y = bottom + 1;
						if (sp1 == ep1) goto doneWithSrc1;
						if (sp2 == ep2) goto doneWithSrc2;
					};
				};

			} else {
				while ((de-(d+1)) <= (np1-sp1)) {
					MAKE_SPACE(((np1-sp1)+(np2-sp2)), "#2");
				};
				rp1 = sp1;
				thisdRow = d;
				lastd = prevdRow;
				if ((y != bottom+1) || (toggle == 1)) lastd = NULL;
				toggle = 1;
				bottom = sp1->bottom;
				if (bottom >= sp2->top) bottom = sp2->top-1;
				if (lastd) {
					while (rp1 < np1) {
						d++;
						d->top = y;
						d->bottom = bottom;
						d->left = rp1->left;
						d->right = rp1->right;
						rp1++;

						lastd++;
						if ((lastd > thisdRow) ||
							(d->left != lastd->left) ||
							(d->right != lastd->right))
							goto noLastD1;
					};
					
					if (lastd == thisdRow) {
						lastd = prevdRow;
						while (lastd < thisdRow)
							(++lastd)->bottom = bottom;
						d = thisdRow;
					} else {
						prevdRow = thisdRow;
					};
					
				} else {
					noLastD1:
					while (rp1 < np1) {
						d++;
						d->top = y;
						d->bottom = bottom;
						d->left = rp1->left;
						d->right = rp1->right;
						rp1++;
					};
					prevdRow = thisdRow;
				};
				toggle = 1;
				if (bottom == sp1->bottom) {
					sp1 = rp1;
					y = sp2->top;
					if (sp1 == ep1) goto doneWithSrc1;
					if (y > sp1->top) y = sp1->top;
				} else
					y = bottom + 1;
			};
		} else if (sp2 < np2) {
			while ((de-(d+1)) <= (np2-sp2)) {
				MAKE_SPACE(((np1-sp1)+(np2-sp2)), "#3");
			};
			rp2 = sp2;
			thisdRow = d;
			lastd = prevdRow;
			if ((y != bottom+1) || (toggle == 2)) lastd = NULL;
			toggle = 2;
			bottom = sp2->bottom;
			if (bottom >= sp1->top) bottom = sp1->top-1;
			if (lastd) {
				while (rp2 < np2) {
					d++;
					d->top = y;
					d->bottom = bottom;
					d->left = rp2->left;
					d->right = rp2->right;
					rp2++;

					lastd++;
					if ((lastd > thisdRow) ||
						(d->left != lastd->left) ||
						(d->right != lastd->right))
						goto noLastD2;
				};

				if (lastd == thisdRow) {
					lastd = prevdRow;
					while (lastd < thisdRow)
						(++lastd)->bottom = bottom;
					d = thisdRow;
				};
			} else {
				noLastD2:
				while (rp2 < np2) {
					d++;
					d->top = y;
					d->bottom = bottom;
					d->left = rp2->left;
					d->right = rp2->right;
					rp2++;
				};
				prevdRow = thisdRow;
			};
			if (bottom == sp2->bottom) {
				sp2 = rp2;
				y = sp1->top;
				if (sp2 == ep2) goto doneWithSrc2;
				if (y > sp2->top) y = sp2->top;
			} else 
				y = bottom + 1;
		};
	};

	doneWithSrc1:

	sp1 = sp2;
	ep1 = ep2;
	np1 = np2;
	toggleCmp = 2;

	doneWithSrc2:

	if (sp1 < ep1) {
		while ((de-(d+1)) <= (ep1-sp1)) {
			MAKE_SPACE(((np1-sp1)+(np2-sp2)), "#4");
		};

		thisdRow = d;
		lastd = prevdRow;
		if ((y != bottom+1) || (toggle == toggleCmp)) lastd = NULL;
	
		if (sp1 == np1) {
			y = np1->top;
			while ((np1 < ep1) && (np1->top == y)) np1++;
		};
	
		bottom = sp1->bottom;
	
		if (lastd) {
			while (sp1 < np1) {
				d++;
				d->left = sp1->left;
				d->right = sp1->right;
				d->bottom = bottom;
				d->top = y;
				sp1++;
	
				lastd++;
				if ((lastd > thisdRow) ||
					(d->left != lastd->left) ||
					(d->right != lastd->right))
					goto noLastDEnd;
			};
	
			if (lastd == thisdRow) {
				lastd = prevdRow;
				while (lastd < thisdRow)
					(++lastd)->bottom = bottom;
				d = thisdRow;
			} else {
				prevdRow = thisdRow;
			};
		} else {
			noLastDEnd:
			while (sp1 < np1) {
				d++;
				d->left = sp1->left;
				d->right = sp1->right;
				d->bottom = bottom;
				d->top = y;
				sp1++;
			};
		};
	
		while (sp1 < ep1) *++d = *sp1++;
	};

	doneTotal:
	
	#undef MAKE_SPACE
	
	if (d >= dst->rects) {
		dst->count = d - dst->rects + 1;
		target->Bounds().top = dst->rects[0].top;
		target->Bounds().bottom = dst->rects[dst->count-1].bottom;
		target->Bounds().left = Bounds().left;
		if (target->Bounds().left > r2.Bounds().left) target->Bounds().left = r2.Bounds().left;
		target->Bounds().right = Bounds().right;
		if (target->Bounds().right < r2.Bounds().right) target->Bounds().right = r2.Bounds().right;
		CHECK_REGION(*target);
	} else
		target->MakeEmpty();
	
#if DB_GROWTH_CHECK
	if (CountRects() == 1 && r2.CountRects() == 1 && target->CountRects() == 1
			&& avail <= 1 && target->CountAvail() > 1) {
		BOut << "*** OR Needlessly grew to " << target->CountAvail() << "!" << endl
				<< "Source1: " << *this << endl
				<< "Source2: " << r2 << endl
				<< "Final: " << *target << endl;
	}
#endif
};

/*----------------------------------------------------------------*/

void IRegion::OffsetBy(int32 dh, int32 dv)
{
	int32 i;
	if (IsEmpty() || (i=fData->count) <= 0 || (dh == 0 && dv == 0)) {
		return;
	}
	
	CHECK_REGION(*this);
	
	region* reg = Edit(i, i);
	if (!reg) return;
	
	if (reg != &fBounds) {
		Bounds().top += dv;
		Bounds().left += dh;
		Bounds().right += dh;
		Bounds().bottom += dv;
	}
	
	int32* ptr = &reg->rects[0].left;
	
	if (dh != 0 && dv != 0) {
		while (i-- > 0) {
			*ptr++ += dh;
			*ptr++ += dv;
			*ptr++ += dh;
			*ptr++ += dv;
		}
	} else if (dh != 0) {
		while (i-- > 0) {
			*ptr++ += dh;
			ptr++;
			*ptr++ += dh;
			ptr++;
		}
	} else {
		while (i-- > 0) {
			ptr++;
			*ptr++ += dv;
			ptr++;
			*ptr++ += dv;
		}
	}
	
	CHECK_REGION(*this);
}

void IRegion::ScaleBy(	IRegion* target, const clipping_rect& srcRect, const clipping_rect& dstRect,
						int32 offsX, int32 offsY, bool tileX, bool tileY) const
{
	const int32 dstW = dstRect.right-dstRect.left+1;
	const int32 dstH = dstRect.bottom-dstRect.top+1;
	const int32 srcW = srcRect.right-srcRect.left+1;
	const int32 srcH = srcRect.bottom-srcRect.top+1;
	if (IsEmpty() || ((dstW == srcW) && (dstH == srcH))) {
		*target = *this;
		target->OffsetBy(offsX, offsY);
		return;
	};

	CHECK_REGION(*this);
	
	if ((IsRect()) &&
		(Bounds().right-Bounds().left+1 == srcW) &&
		(Bounds().bottom-Bounds().top+1 == srcH)) {
		clipping_rect r;
		r.left = offsX;
		r.top = offsY;
		r.right = offsX + dstW - 1;
		r.bottom = offsY + dstH - 1;
		target->Set(r);
		return;
	};

	if (!tileX && !tileY) {
		region* dst = target->ReUse(CountRects(), CountRects());
		if (!dst) return;
		const clipping_rect *s = Rects();
		clipping_rect *d = dst->rects;
		for (int32 i=CountRects();i;i--) {
			d->left = offsX + s->left * dstW / srcW;
			d->right = offsX + (s->right+1) * dstW / srcW - 1;
			d->top = offsY + s->top * dstH / srcH;
			d->bottom = offsY + (s->bottom+1) * dstH / srcH - 1;
			if (d->is_valid()) d++;
			s++;
		};
		dst->count = d - dst->rects;
		target->Bounds().left = offsX + Bounds().left * dstW / srcW;
		target->Bounds().right = offsX + (Bounds().right+1) * dstW / srcW - 1;
		target->Bounds().top = offsY + Bounds().top * dstH / srcH;
		target->Bounds().bottom = offsY + (Bounds().bottom+1) * dstH / srcH - 1;
		CHECK_REGION(*target);
	} else if (!tileX && tileY) {
		int32 multHigh = ((dstH+srcH-1)/srcH);
		int32 multLow = (dstH/srcH);
		const int32 need = CountRects()*multHigh;
		region* dst = target->ReUse(need, need);
		if (!dst) return;
		const clipping_rect *s = Rects();
		clipping_rect *d = dst->rects;
		int32 low = Bounds().bottom;
		const int32 limit = offsY + dstH;
		if (multLow >= 1) {
			for (int32 i=CountRects();i;i--) {
				d->left = offsX + s->left * dstW / srcW;
				d->right = offsX + (s->right+1) * dstW / srcW - 1;
				d->top = offsY + s->top;
				d->bottom = offsY + s->bottom;
				if (d->is_valid()) d++;
				s++;
			};
			s = dst->rects;
			multLow--;

			for (int32 i=CountRects()*multLow;i;i--) {
				*d = *s++;
				d->top += srcH;
				d->bottom += srcH;
				d++;
			};

			for (int32 i=CountRects();i;i--) {
				*d = *s++;
				d->top += srcH;
				if (s->top >= limit) break;
				d->bottom += srcH;
				if (d->bottom >= limit) d->bottom = limit-1;
				low = d->bottom;
				d++;
			};
		} else {
			for (int32 i=CountRects();i;i--) {
				d->top = offsY + s->top;
				if (d->top >= limit) break;
				d->bottom = offsY + s->bottom;
				if (d->bottom >= limit) d->bottom = limit-1;
				low = d->bottom;
				d->left = offsX + s->left * dstW / srcW;
				d->right = offsX + (s->right+1) * dstW / srcW - 1;
				if (d->is_valid()) d++;
				s++;
			};
		};
		dst->count = d-dst->rects;
		target->Bounds().left = offsX + Bounds().left * dstW / srcW;
		target->Bounds().right = offsX + (Bounds().right+1) * dstW / srcW - 1;
		target->Bounds().top = offsY + Bounds().top;
		target->Bounds().bottom = low;
		CHECK_REGION(*target);
	} else if (tileX && !tileY) {
		int32 multHigh = ((dstW+srcW-1)/srcW);
		int32 multLow = (dstW/srcW);
		const int32 need = CountRects()*multHigh;
		region* dst = target->ReUse(need, need);
		if (!dst) return;
		const clipping_rect *s = Rects();
		clipping_rect *d = dst->rects;
		int32 right = Bounds().right;
		const int32 limit = offsX + dstW;
		if (multLow >= 1) {
			multLow--;
			for (int32 i=CountRects();i;i--) {
				d->left = offsX + s->left;
				d->right = offsX + s->right;
				d->top = offsY + s->top * dstH / srcH;
				d->bottom = offsY + (s->bottom+1) * dstH / srcH - 1;
				if (d->is_valid()) {
					d++;
					for (int32 j=multLow;j;j--) *d = *(d-1); d++;
					*d = *(d-1);
					d->left += srcW;
					if (d->left < limit) {
						d->right += srcW;
						if (d->right >= limit) d->right = limit-1;
						if (d->right > right) right = d->right;
						if (d->is_valid()) d++;
					};
				}
				s++;
			};
		} else {
			for (int32 i=CountRects();i;i--) {
				d->left = offsX + s->left;
				if (d->left < limit) {
					d->right = offsX + s->right;
					if (d->right >= limit) d->right = limit-1;
					if (d->right > right) right = d->right;
					d->top = offsY + s->top * dstH / srcH;
					d->bottom = offsY + (s->bottom+1) * dstH / srcH - 1;
					if (d->is_valid()) d++;
				};
				s++;
			};
		};
		dst->count = d-dst->rects;
		target->Bounds().left = offsX + Bounds().left;
		target->Bounds().right = right;
		target->Bounds().top = offsY + Bounds().top * dstH / srcH;
		target->Bounds().bottom = offsY + (Bounds().bottom+1) * dstH / srcH - 1;
		CHECK_REGION(*target);
	} else {
		// Scale as two separate operations.  This could be optimized,
		// but so it goes.
		clipping_rect xRect;
		xRect.left = dstRect.left;
		xRect.right = dstRect.right;
		xRect.top = srcRect.top;
		xRect.bottom = srcRect.bottom;
		IRegion tmp;
		ScaleBy(&tmp, srcRect, xRect, offsX, 0, true, false);
		tmp.ScaleBy(target, xRect, dstRect, 0, offsY, false, true);
	};
};

#if DB_INTEGRITY_CHECKS
static void braindead_rotate(const IRegion& orig, const DisplayRotater& rot,
							 IRegion* dest)
{
	const int32 N = orig.CountRects();
	const clipping_rect* src = orig.Rects();
	
	// Bow before my 3l33t h4x0r ski11z!!!!
	IRegion rotReg;
	IRegion tmpReg;
	IRegion* curDest = dest;
	IRegion* curSrc = &tmpReg;
	IRegion* swap;
	clipping_rect* rotRect = rotReg.CreateRects(1, 1);
	rotReg.SetRectCount(1);
	for (int32 i=0; i<N; i++) {
		rot.RotateRect(src+i, rotRect);
		if (i == 0) curDest->Set(*rotRect);
		else curSrc->Or(rotReg, curDest);
		swap = curDest;
		curDest = curSrc;
		curSrc = swap;
	}
	if (curSrc != dest) *dest = *curSrc;
}
#endif

#define Z_LIST_SIZE 32

struct z_rect {
	int32		min;
	int32		max;
	z_rect		*prev;
};

struct z_cut {
	z_cut		*prev;
	z_cut		*next;
	int32		cut;
	z_rect		*rects;	
};

struct z_header {
	z_rect		list[Z_LIST_SIZE];
	int32		count;
	z_header	*prev;
};

static void new_z_header(z_header **z_free) {
	z_header	*h;
	
	h = (z_header*)malloc(sizeof(z_header));
	h->prev = *z_free;
	h->count = 0;
	*z_free = h;
}

void IRegion::Rotate(const DisplayRotater& rot, IRegion* dest) const
{
	const int32 N = CountRects();
	const clipping_rect* src = Rects();
	
	if (N <= 0) {
		dest->MakeEmpty();
		
	} else  if (N == 1) {
		clipping_rect rect;
		rot.RotateRect(src, &rect);
		dest->Set(rect);
		return;
	
	} else {
		rect		*data;
		int32		i, j, cut_value, last_v, bottom_v, z_rect_count;
		z_cut		*nz, *curz, *first;
		z_rect		*zr;
		z_header	*curh, *z_free;
	
		z_free = NULL;
		new_z_header(&z_free);
		z_rect_count = 0;
		
		/* initialise z_cut extremes */
		first = (z_cut*)malloc(sizeof(z_cut));
		nz = (z_cut*)malloc(sizeof(z_cut));
		first->prev = NULL;
		first->next = nz;
		first->rects = NULL;
		first->cut = Bounds().left;
		nz->prev = first;
		nz->next = NULL;
		nz->rects = NULL;
		nz->cut = Bounds().right+1;
		
		/* first pass : calculate all the cuts */
		last_v = Bounds().top-1;
		curz = 0;
		for (i=0; i<N; i++) {
			if (last_v != src->top) {
				last_v = src->top;
				curz = first;
			}
			/* one pass for each extremity */
			for (j=0; j<2; j++) {
				if (j == 0)
					cut_value = src->left;
				else
					cut_value = src->right+1;
				/* find the first one smaller or equal to the current */
				while (cut_value > curz->cut)
					curz = curz->next;
				/* if strictly smaller, insert before and move back */
				if (cut_value != curz->cut) {
					nz = (z_cut*)malloc(sizeof(z_cut));
					nz->prev = curz->prev;
					nz->next = curz;
					nz->rects = NULL;
					nz->cut = cut_value;
					nz->prev->next = nz;
					curz->prev = nz;
					curz = nz;
				}
			}
			src++;
		}
	
		/* second pass : agglomerate all the rects */
		last_v = Bounds().top-1;
		bottom_v = 0;
		src = Rects();
		for (i=0; i<N; i++) {
			if (last_v != src->top) {
				last_v = src->top;
				bottom_v = src->bottom+1;
				curz = first;
			}
			/* find the left cut */
			while (src->left > curz->cut)
				curz = curz->next;
			do {
				zr = curz->rects;
				if ((zr == NULL) || (zr->max < last_v)) {
					if (z_free->count == Z_LIST_SIZE)
						new_z_header(&z_free);
					zr = z_free->list + z_free->count;
					z_free->count++;
					z_rect_count++;
					zr->min = last_v;
					zr->prev = curz->rects;
					curz->rects = zr;
				}
				zr->max = bottom_v;
				curz = curz->next;
			} while ((curz != NULL) && (curz->cut <= src->right));
			src++;
		}

		/* create the rotated region */
		data = dest->CreateRects(z_rect_count, z_rect_count);
		dest->SetRectCount(z_rect_count);
		
		/* rotation is done clock-wise */
		i = 0;
		curz = first;
		while (curz->next != NULL) {
			zr = curz->rects;
			while (zr != NULL) {
				data[i].top = rot.RotateH(curz->cut);
				data[i].left = rot.RotateV(zr->max-1);
				data[i].right = rot.RotateV(zr->min);
				data[i].bottom = rot.RotateH(curz->next->cut-1);
				zr = zr->prev;
				i++;
			}
			curz = curz->next;
		}
		
		/* rotate the bounds */
		rot.RotateRect(&Bounds(), &dest->Bounds());
		CHECK_REGION(*dest);
		
		/* clean-up : free all cut and rect */
		curz = first;
		while (curz != NULL) {
			nz = curz->next;
			free(curz);
			curz = nz;
		}
		while (z_free != NULL) {
			curh = z_free->prev;
			free(z_free);
			z_free = curh;
		}
	}
	
	#if DB_INTEGRITY_CHECKS
	IRegion test;
	braindead_rotate(*this, rot, &test);
	if (test != *dest) {
		BErr << "Created rotation: " << *dest << endl;
		BErr << "Expected rotation: " << test << endl;
		debugger("Rotate() didn't create correct region");
	}
	#endif
}

/*----------------------------------------------------------------*/

void IRegion::CompressSpans()
{
	CHECK_REGION(*this);
	
	const int32 sc=CountRects();
	if (sc <= 0) return;
	
	region* spans = Edit(sc, sc);
	
	clipping_rect *s = spans->rects;
	clipping_rect *d = s;
	clipping_rect *lasts = s + sc;
	clipping_rect *backupS,*backupD,*tmpD;
	int32 y,inc;
	
	goto startup;
	
	while (1) {
		inc = 0;
		while ((s!=lasts) && (s->top == (y+1))) {
			y = s->top;
			backupS = s;
			tmpD = backupD;
			do {
				if ((tmpD->left != s->left) ||
					(tmpD->right != s->right)) {
					s = backupS;
					goto done1;
				};
				tmpD++; s++;
				if ((tmpD==d) != ((s==lasts)||(s->top!=y))) {
					s = backupS;
					goto done1;
				};
			} while (tmpD!=d);
			inc++;
		};

		done1:

		if (inc) {
			while (backupD < d) {
				backupD->bottom += inc;
				backupD++;
			};
		};

		if (s==lasts) goto done2;

		while (s->top != (y+1)) {
			startup:
			y = s->top;
			backupD = d;
			do {
				*d++ = *s++;
				if (s == lasts) goto done2;
			} while (y == s->top);
		};
	};

	done2:

	spans->count = d - spans->rects;
	
	CHECK_REGION(*this);
};

void IRegion::CompressMemory()
{
	if (!fData) return;
	
	if (fData != &fBounds) {
		if (fData->count == 1) {
			// If this region consists of a single rectangle, make it inline.
			fBounds.refs = 10000;
			fBounds.count = 1;
			fBounds.avail = 1;
			fData->release();
			fData = &fBounds;
			return;
		} else if (fData->refs > 1) {
			// For an out-of-line region, we want to do a realloc() on it.  But
			// if the region data is shared by other objects, this would
			// actually require a malloc() and memcpy().  That would be a
			// waste.
			return;
		}
	}
	
	#if DB_INTEGRITY_CHECKS
	if (fData->avail < MIN_AVAIL) {
		debugger("CompressMemory() starting with region with bad avail count");
	}
	#endif
	
	int32 exactSize = fData->count;
	region* reg = Edit(exactSize, exactSize);
	if (!reg) return;
	
	if (exactSize < MIN_AVAIL) exactSize = MIN_AVAIL;
	if (reg != &fBounds && reg->avail > exactSize) {
		reg = (region*)realloc(reg, sizeof(region) + sizeof(clipping_rect)*(exactSize-1));
		fData = reg;
		if (reg) reg->avail = exactSize;
	}
}

/*----------------------------------------------------------------*/

int32 IRegion::FindSpanBetween(int32 top, int32 bottom) const DECLARE_RETURN(i)
{
	int32 i = -1;
	if (!fData) return i;
	const clipping_rect *rl = fData->rects;
	int32 low = 0;
	int32 high = fData->count-1;
	int32 old = -1;
	if ((bottom < Bounds().top) || (top > Bounds().bottom)) return -1;
	while (1) {
		i = (low+high)/2;
		if (i==old) return -1;
		if (rl[i].bottom >= top) {
			if (rl[i].top <= bottom) break;
			high = i-1;
		} else if (rl[i].top <= bottom) {
			low = i+1;
		}
		old = i;
	}
	return i;
}

bool IRegion::Contains(int32 h, int32 v) const
{
	int32 low,high,y,i = FindSpanBetween(v,v);
	if (i == -1) return false;

	const clipping_rect *rl = fData->rects;
	
	low = high = i;
	y = rl[i].top;

	while ((low >= 0) && (rl[low].top == y)) {
		if ((rl[low].left <= h) && (rl[low].right >= h)) return true;
		low--;
	}

	while ((high < fData->count) && (rl[high].top == y)) {
		if ((rl[high].left <= h) && (rl[high].right >= h)) return true;
		high++;
	}

	return false;
}

bool IRegion::Intersects(const clipping_rect& r) const
{
	int32	i;

	if (!fData) return false;
	const int32 count = fData->count;
	if (count <= 0) return false;

	if (!r.is_valid() ||
		(r.bottom < Bounds().top) ||
		(r.top   > Bounds().bottom) ||
		(r.right < Bounds().left) ||
		(r.left > Bounds().right))
		return false;

	if (count == 1) return true;
	
	const clipping_rect *rl = fData->rects;
	int32 low,high;
	if ((i=FindSpanBetween(r.top,r.bottom))==-1) return false;
	low = i;
	while ((low>=0) && (rl[low].bottom >= r.top)) {
		if ((rl[low].left <= r.right) && (rl[low].right >= r.left)) return true;
		low--;
	}
	high = i+1;
	while ((high<count) && (rl[high].top <= r.bottom)) {
		if ((rl[high].left <= r.right) && (rl[high].right >= r.left)) return true;
		high++;
	}

	return false;
}

/*----------------------------------------------------------------*/

IRegion::region* IRegion::EditSlow(const int32 needed, const int32 coulduse) const
{
	IRegion* This = const_cast<IRegion*>(this);
	
	if (!fData) {
		return NULL;
	}
	
	if (fData == &fBounds) {
		// The data is currently inlined; what should we do?
		if (needed <= 1) {
			// Don't need any more space, so keep it inline.
			return const_cast<region*>(&fBounds);
		}
		
		// Need to move data out-of-line; do so.  It is safe to do it
		// this way because fBounds.refs is always >> 1.
		return const_cast<region*>(
			This->fData = fBounds.edit(needed, coulduse < MIN_AVAIL ? MIN_AVAIL : coulduse));
	}
	
	if (needed <= 1 && fData->refs > 1) {
		// We are now saying that we only need a single rectangle, and
		// as it turns out the current region data is being shared.  So
		// copy the data into our inline area.
		This->fBounds.refs = 10000;
		This->fBounds.count = fData->count;
		This->fBounds.rects[0] = fData->rects[0];
		fData->release();
		return const_cast<region*>(This->fData = &fBounds);
	}
	
	// Standard case: the region is currently out-of-line, and is going
	// to stay that way.
	return const_cast<region*>(
		This->fData = fData->edit(needed, coulduse < MIN_AVAIL ? MIN_AVAIL : coulduse));
}


IRegion::region* IRegion::ReUseSlow(const int32 needed, const int32 coulduse) const
{
	IRegion* This = const_cast<IRegion*>(this);
	
	if (!fData) {
		This->MakeEmpty();
	}
	
	if (fData == &fBounds) {
		// The data is currently inlined; what should we do?
		if (needed <= 1) {
			// Don't need any more space, so keep it inline.
			return const_cast<region*>(&fBounds);
		}
		
		// Need more space, so start a new region structure out-of-line.
		return const_cast<region*>(
			This->fData = region::create(coulduse < MIN_AVAIL ? MIN_AVAIL : coulduse));
	}
	
	if (needed <= 1 && fData->refs > 1) {
		// We are now saying that we only need a single rectangle, and
		// as it turns out the current region data is being shared.  So
		// just revert to an inline region.
		This->fBounds.refs = 10000;
		This->fBounds.count = 0;
		This->Bounds() = invalRect;
		fData->release();
		return const_cast<region*>(This->fData = &fBounds);
	}
	
	// Standard case: the region is currently out-of-line, and is going
	// to stay that way.
	return const_cast<region*>(
		This->fData = fData->reuse(needed, coulduse < MIN_AVAIL ? MIN_AVAIL : coulduse));
}

#if DB_INTEGRITY_CHECKS

bool IRegion::CheckIntegrity() const
{
	bool result = true;
	if (fBounds.refs <= 9990) {
		char blah[256];
		sprintf(blah,"region at %p has bad bounds ref count!",this);
		debugger(blah);
		result = false;
	}
	if (fBounds.avail != 1) {
		char blah[256];
		sprintf(blah,"region at %p has bad bounds avail count!",this);
		debugger(blah);
		result = false;
	}
	if (fBounds.count > 1) {
		char blah[256];
		sprintf(blah,"region at %p has bad bounds rect count!",this);
		debugger(blah);
		result = false;
	}
	if (fData->refs <= 0) {
		char blah[256];
		sprintf(blah,"region at %p has no data ref count!",this);
		debugger(blah);
		result = false;
	}
	if (fData->count > fData->avail) {
		char blah[256];
		sprintf(blah,"region at %p has data rect count > avail count!",this);
		debugger(blah);
		result = false;
	}
	if (CountRects()) {
		const int32 N = CountRects();
		const clipping_rect* rects = Rects();
		clipping_rect bounds = rects[0];
		for (int32 i=0;i<N;i++) {
			if (!rects[i].is_valid()) {
				BStringIO msg;
				msg << "Region at " << this << " is corrupt:" << endl;
				msg << "invalid rect #" << i << endl;
				BOut << msg.String() << *this << endl;
				debugger(msg.String());
				result = false;
			}
			if ((i > 0)) {
				if ((rects[i].top < rects[i-1].top)) {
					BStringIO msg;
					msg << "Region at " << this << " is corrupt:" << endl;
					msg << "top of rect #" << i << " (top=" << rects[i].top
						<< ") is above previous rect (top=" << rects[i-1].top << ")" << endl;
					BOut << msg.String() << *this << endl;
					debugger(msg.String());
					result = false;
				}
				if ((rects[i].top == rects[i-1].top)) {
					if ((rects[i].bottom != rects[i-1].bottom)) {
						BStringIO msg;
						msg << "Region at " << this << " is corrupt:" << endl;
						msg << "rect #" << i << " (bot=" << rects[i].bottom
							<< ") does not create span with previous rect (bot="
							<< rects[i-1].bottom << ")" << endl;
						BOut << msg.String() << *this << endl;
						debugger(msg.String());
						result = false;
					}
					if ((rects[i].left <= rects[i-1].right)) {
						BStringIO msg;
						msg << "Region at " << this << " is corrupt:" << endl;
						msg << "rect #" << i << " (left=" << rects[i].left
							<< ") is not to right of previous span rect (right="
							<< rects[i-1].right << ")" << endl;
						BOut << msg.String() << *this << endl;
						debugger(msg.String());
						result = false;
					}
				}
			}
			if (bounds.left > rects[i].left)		bounds.left = rects[i].left;
			if (bounds.right < rects[i].right)		bounds.right = rects[i].right;
			if (bounds.top > rects[i].top)			bounds.top = rects[i].top;
			if (bounds.bottom < rects[i].bottom)	bounds.bottom = rects[i].bottom;
			if ((rects[i].left < Bounds().left) ||
				(rects[i].right > Bounds().right) ||
				(rects[i].top < Bounds().top) ||
				(rects[i].bottom > Bounds().bottom)) {
				BStringIO msg;
				msg << "Region at " << this << " is corrupt:" << endl;
				msg << "rect " << i << " = " << rects[i]
					<< ", outside bounds " << Bounds() << endl;
				BOut << msg.String() << *this << endl;
				debugger(msg.String());
				result = false;
			}
		}
		if ((bounds.left != Bounds().left) ||
			(bounds.right != Bounds().right) ||
			(bounds.top != Bounds().top) ||
			(bounds.bottom != Bounds().bottom)) {
			BStringIO msg;
			msg << "Region at " << this << " is corrupt:" << endl;
			msg << "real bounds: " << bounds << ", stated bounds " << Bounds() << endl;
			BOut << msg.String() << *this << endl;
			debugger(msg.String());
			result = false;
		}
	}
	
	return result;
};

#endif

/*----------------------------------------------------------------*/

namespace BPrivate {

#if OPT_REGION_ALLOC

inline IRegion* IRegionCache::GetRegion()
	DECLARE_RETURN(top)
{
	IRegion* top;
	acquire_fast_sem();
	top = fRegionList;
	if (top) {
		fRegionCount--;
		fRegionList = (IRegion*)top->fBounds.link;
		release_fast_sem();
		const_cast<IRegion::region*>(top->fData)->count = 0;
		top->fBounds.rects[0] = invalRect;
		top->fBounds.link = NULL;
		CHECK_REGION(*top);
		return top;
	}
	release_fast_sem();
	
	top = (IRegion*)malloc(sizeof(IRegion));
	new(top) IRegion();
	return top;
}

inline void IRegionCache::SaveRegion(IRegion* reg)
{
	CHECK_REGION(*reg);
	if (fRegionCount < 32 && reg->fData &&
			(reg->fData == &reg->fBounds || reg->fData->refs == 1)) {
		acquire_fast_sem();
		reg->fBounds.link = fRegionList;
		fRegionList = reg;
		fRegionCount++;
		release_fast_sem();
	} else {
		reg->~IRegion();
		free(reg);
	}
}
#endif

region *newregion()
{
	#if OPT_REGION_ALLOC
		return IRegionCache::GetRegion();
	#else
		return new IRegion;
	#endif
}

void kill_region(region* reg)
{
	#if OPT_REGION_ALLOC
		if (reg) IRegionCache::SaveRegion(reg);
	#else
		delete reg;
	#endif
}

BDataIO& operator<<(BDataIO& io, const IRegion& region)
{
#if SUPPORTS_STREAM_IO
	const int32 N = region.CountRects();
	
	io << "IRegion(" << region.Bounds() << ") {";
	
	if (N > 1) io << endl << "\t";
	else io << " ";
	for (int i = 0; i < N; i++) {
		io << region.Rects()[i];
		if (i < N-1) {
			if (N <= 1) io << ", ";
			else io << "," << endl << "\t";
		}
	}
	
	if (N > 1) io << "\n}";
	else io << " }";
#else
	(void)region;
#endif
	return io;
}

}
