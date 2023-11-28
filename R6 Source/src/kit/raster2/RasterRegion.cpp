
#include <raster2/RasterRegion.h>

#include <new>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <kernel/OS.h>
#include <support2/StdIO.h>
#include <support2/Debug.h>
#include <support2_p/SupportMisc.h>

namespace B {

// We always want to compile in the region rotation function,
// even though the app_server may be built without it enabled.
//#undef ROTATE_DISPLAY
//#define ROTATE_DISPLAY 1

//#include <app_server_p/display_rotation.h>

// Optimization options
#define OPT_REGION_ALLOC 1		// Cache region memory?
#define OPT_SIMPLE_REGIONS 1	// Check for simple "1 rect OP 1 rect -> 1 rect" cases?

// Debugging options
#define DB_INTEGRITY_CHECKS 1	// Make sure region is valid after operations?
#define DB_WATCH_ALLOC 0		// Track memory allocations for leak checking?
#define DB_GROWTH_CHECK 1		// Report needless memory growth?
#define DB_MALLOC_DEBUG 0		// Add checks for compatibility with -fcheck-memory-usage?
#define DB_NO_SHARING 1			// Don't include copy-on-write optimization?

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

#define	smax(x1, x2)	if (x1 < x2) x1 = x2
#define	smin(x1, x2)	if (x1 > x2) x1 = x2
#define	MAX_OUT			0x0FFFFFFF

static const B::Raster2::BRasterRect invalRect(MAX_OUT,MAX_OUT,-MAX_OUT,-MAX_OUT);

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

namespace Private {
using namespace Raster2;

class BRasterRegionCache
{
public:
	BRasterRegionCache()
	{
		fFastSem = create_sem(0, "region alloc sem");
	}
	~BRasterRegionCache()
	{
		acquire_fast_sem();
		BRasterRegion::region* data = fDataList;
		while (data) {
			BRasterRegion::region* next = (BRasterRegion::region*)data->link;
			free(data);
			data = next;
		}
		fDataList = NULL;
		BRasterRegion* reg = fRegionList;
		while (reg) {
			BRasterRegion* next = (BRasterRegion*)reg->fBounds.link;
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

	static BRasterRegion::region* GetData(const int32 count);
	static void SaveData(BRasterRegion::region* reg);

	static BRasterRegion* GetRegion();
	static void SaveRegion(BRasterRegion* reg);
	
private:
	static int32 fFastLock;
	static sem_id fFastSem;
	static int32 fDataCount;
	static BRasterRegion::region* fDataList;
	static int32 fRegionCount;
	static BRasterRegion* fRegionList;
};

inline BRasterRegion::region* BRasterRegionCache::GetData(const int32 count)
	DECLARE_RETURN(top)
{
	acquire_fast_sem();
	BRasterRegion::region* top = fDataList;
	BRasterRegion::region** prev = &fDataList;
	while (top) {
		if (top->avail >= count) {
			fDataCount--;
			*prev = (BRasterRegion::region*)top->link;
			release_fast_sem();
			top->link = NULL;
			return top;
		}
		prev = (BRasterRegion::region**)(&top->link);
		top = (BRasterRegion::region*)top->link;
	}
	release_fast_sem();
	if (count < 8) {
		top = (BRasterRegion::region*)malloc(sizeof(BRasterRegion::region) + 7*sizeof(BRasterRect));
		if (top) top->avail = 8;
	} else {
		top = (BRasterRegion::region*)malloc(sizeof(BRasterRegion::region) + (count-1)*sizeof(BRasterRect));
		if (top) top->avail = count;
	}
	return top;
}

inline void BRasterRegionCache::SaveData(BRasterRegion::region* reg)
{
	#if DB_INTEGRITY_CHECKS
	if (reg->avail < MIN_AVAIL)
		debugger("Saving bad avail");
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

int32 BRasterRegionCache::fFastLock = 0;
sem_id BRasterRegionCache::fFastSem = B_BAD_SEM_ID;
int32 BRasterRegionCache::fDataCount = 0;
BRasterRegion::region* BRasterRegionCache::fDataList = NULL;
int32 BRasterRegionCache::fRegionCount = 0;
BRasterRegion* BRasterRegionCache::fRegionList = NULL;

static BRasterRegionCache cache;
}	// namespace Private
#endif

namespace Raster2 {
using namespace B::Private;

inline void BRasterRegion::region::acquire() const
{
	atomic_add(&refs, 1);
}

inline void BRasterRegion::region::release() const
{
	if (atomic_add(&refs, -1) == 1) {
		REM_REGION_MEMORY(sizeof(region) + sizeof(BRasterRect)*(avail-1));
		BRasterRegionCache::SaveData(const_cast<region*>(this));
	}
}

inline BRasterRegion::region* BRasterRegion::region::create(const int32 init_avail) DECLARE_RETURN(r)
{
	ADD_REGION_MEMORY(sizeof(region) + sizeof(BRasterRect)*(init_avail-1));
	region* r =  BRasterRegionCache::GetData(init_avail);
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

inline BRasterRegion::region* BRasterRegion::region::edit(const int32 needed, const int32 coulduse) const DECLARE_RETURN(r)
{
	region* r;
	
	// Are we the sole owner of this region?
	if (refs <= 1) {
		if (needed <= avail && coulduse >= (avail/4)) {
			return const_cast<region*>(this);
		}
		
		r = const_cast<region*>(this);
		ADD_REGION_MEMORY(sizeof(BRasterRect)*(coulduse-avail));
		r = (region*)realloc(r, sizeof(region) + sizeof(BRasterRect)*(coulduse-1));
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
		memcpy(r->rects, rects, sizeof(BRasterRect)*(size));
		r->count = size;
		#if DB_INTEGRITY_CHECKS
		if (r->avail < MIN_AVAIL)
			debugger("Editing bad avail");
		#endif
	}
	release();
	return r;
}

inline BRasterRegion::region* BRasterRegion::region::reuse(const int32 needed, const int32 coulduse) const
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

BRasterRegion::BRasterRegion()
{
	ADD_REGION_COUNT();
	fData = &fBounds;
	fBounds.refs = 10000;
	fBounds.count = 0;
	fBounds.avail = 1;
	Bounds() = invalRect;
	CHECK_REGION(*this);
}

BRasterRegion::BRasterRegion(const BRasterRect& r)
{
	ADD_REGION_COUNT();
	fData = &fBounds;
	fBounds.refs = 10000;
	fBounds.count = 1;
	fBounds.avail = 1;
	Bounds() = r;
	CHECK_REGION(*this);
}

BRasterRegion::BRasterRegion(const BRasterRegion& o)
	:	BFlattenable()
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
			memcpy(const_cast<region*>(fData)->rects, o.fData->rects, o.fData->count*sizeof(BRasterRect));
			const_cast<region*>(fData)->count = o.fData->count;
		}
		#else
		fData = o.fData;
		fData->acquire();
		#endif
	}
	CHECK_REGION(*this);
}

void BRasterRegion::Construct(const value_ref& ref, status_t* result)
{
	ADD_REGION_COUNT();
	fData = &fBounds;
	fBounds.refs = 10000;
	fBounds.count = 0;
	fBounds.avail = 1;
	Bounds() = invalRect;
	const status_t r = Unflatten(ref.type, ref.data, ref.length);
	if (result) *result = r;
	CHECK_REGION(*this);
}

BRasterRegion::BRasterRegion(const BValue& value, status_t* result)
{
	Construct(value_ref(value), result);
}

BRasterRegion::BRasterRegion(const value_ref& value, status_t* result)
{
	Construct(value, result);
}

BRasterRegion::~BRasterRegion()
{
	CHECK_REGION(*this);
	REM_REGION_COUNT();
	if (fData) fData->release();
}

/*----------------------------------------------------------------*/

BRasterRegion& BRasterRegion::operator=(const BRasterRegion& o)
{
	CHECK_REGION(o);
	
	if (this == &o) return *this;
	if (fData) fData->release();
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
			memcpy(const_cast<region*>(fData)->rects, o.fData->rects, o.fData->count*sizeof(BRasterRect));
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

bool BRasterRegion::operator==(const BRasterRegion& o) const
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
	if (memcmp(fData->rects, o.fData->rects, sizeof(BRasterRect)*fData->count) == 0)
		return true;
	return false;
}

void BRasterRegion::Swap(BRasterRegion& with)
{
	char buffer[sizeof(BRasterRegion)];
	memcpy(buffer, this, sizeof(BRasterRegion));
	memcpy(this, &with, sizeof(BRasterRegion));
	memcpy(&with, buffer, sizeof(BRasterRegion));
}

BValue BRasterRegion::AsValue() const
{
	return BValue::Flat(*this);
}

/*----------------------------------------------------------------*/

inline BRasterRegion::region* BRasterRegion::Edit(const int32 needed, const int32 coulduse) const
{
	// Fast case: we own the region, and don't need to resize it.
	if ((fData == &fBounds || fData->refs <= 1)&& needed <= fData->avail)
		return const_cast<region*>(fData);
	
	// All other cases.
	return EditSlow(needed, coulduse);
}

inline BRasterRegion::region* BRasterRegion::ReUse(const int32 needed, const int32 coulduse) const
{
	// Fast case: we own the region, and don't need to resize it.
	if ((fData == &fBounds || (fData && fData->refs <= 1)) && needed <= fData->avail)
		return const_cast<region*>(fData);
	
	// All other cases.
	return ReUseSlow(needed, coulduse);
}

/*----------------------------------------------------------------*/

BRasterRect* BRasterRegion::EditRects(int32 needed, int32 coulduse)
{
	if (!fData) return NULL;
	region* reg = Edit(needed, coulduse);
	if (reg) return reg->rects;
	return NULL;
}

BRasterRect* BRasterRegion::CreateRects(int32 needed, int32 coulduse)
{
	region* reg = ReUse(needed, coulduse);
	if (reg) return reg->rects;
	return NULL;
}

void BRasterRegion::SetRectCount(int32 num)
{
	if (!fData) return;
	region* reg = Edit(num, num);
	if (reg) reg->count = num;
}

/*----------------------------------------------------------------*/

void BRasterRegion::MakeEmpty()
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

void BRasterRegion::Set(const BRasterRect& r)
{
	if (r.IsValid()) {
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

void BRasterRegion::AddRect(const BRasterRect& r)
{
	if (!r.IsValid() || !fData) return;

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

void BRasterRegion::OrRect(const BRasterRect& r)
{
	if (!r.IsValid() || !fData) return;

	if (fData->count == 0) {
		Set(r);
		return;
	}

	if (r.top >= Bounds().bottom) {
		region* reg = Grow(1);
		reg->rects[reg->count++] = r;
		Bounds().bottom = r.bottom;
		smin(Bounds().left, r.left);
		smax(Bounds().right, r.right);
		return;
	}

	OrSelf(BRasterRegion(r));
}

/*----------------------------------------------------------------*/

void BRasterRegion::XOr(const BRasterRegion& r2, BRasterRegion* result) const
{
	BRasterRegion tmp1, tmp2;

	r2.Sub(*this, &tmp1);
	Sub(r2, &tmp2);
	tmp1.Or(tmp2, result);
}

/*----------------------------------------------------------------*/

/*	Please forgive me for these three functions, but all these gotos
	really seem like the best way to special case the loops for speed. */

void BRasterRegion::Sub(const BRasterRegion& r2, BRasterRegion* target) const
{
	CHECK_REGION(*this);
	CHECK_REGION(r2);

	if ((Bounds().bottom <= r2.Bounds().top) ||
		(Bounds().top >= r2.Bounds().bottom) ||
		(Bounds().left >= r2.Bounds().right) ||
		(Bounds().right <= r2.Bounds().left) ||
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
				BRasterRect r;
				r.left = r2.Bounds().right;
				r.top = Bounds().top;
				r.right = Bounds().right;
				r.bottom = Bounds().bottom;
				target->Set(r);
				return;
			} else if (	r2.Bounds().right >= Bounds().right &&
						r2.Bounds().left > Bounds().left) {
				// r2 is removing the right of r1...
				BRasterRect r;
				r.left = Bounds().left;
				r.top = Bounds().top;
				r.right = r2.Bounds().left;
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
				BRasterRect r;
				r.left = Bounds().left;
				r.top = r2.Bounds().bottom;
				r.right = Bounds().right;
				r.bottom = Bounds().bottom;
				target->Set(r);
				return;
			} else if (	r2.Bounds().bottom >= Bounds().bottom &&
						r2.Bounds().top > Bounds().top) {
				// r2 is removing the bottom of r1...
				BRasterRect r;
				r.left = Bounds().left;
				r.top = Bounds().top;
				r.right = Bounds().right;
				r.bottom = r2.Bounds().top;
				target->Set(r);
				return;
			}
		}
	}
#endif

#if DB_GROWTH_CHECK
	const int32 avail = target->CountAvail();
#endif

	BRasterRect *prevdRow=NULL,*lastd,*thisdRow,*d,*de;
	const BRasterRect *rp1=Rects(),*rp2=r2.Rects();
	const BRasterRect *sp1=rp1,*sp2=rp2;
	const BRasterRect *np1=rp1,*np2=rp2;
	const BRasterRect *ep1=rp1+CountRects(),*ep2=rp2+r2.CountRects();
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
		if (sp2->bottom <= y) {
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
			if (y != bottom) lastd = NULL;
			toggle = 0;
			bottom = sp1->bottom;
			if (bottom > sp2->bottom) bottom = sp2->bottom;

			while (rp2->right <= rp1->left) {
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
					while (d->right <= rp2->left) {
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
					while (rp2->right <= d->left) {
						rp2++;
						if (rp2 == np2) goto doneWithRow2;
					};
					if (d->right <= rp2->left) goto loopStart1;
				} 

				if (rp2->left <= d->left) {
					d->left = rp2->right;
					if (d->left >= d->right) {
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
					d->right = rp2->left;
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
						d->left = rp2->right;
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
				y = bottom;

			checkAgain2:
			if (y >= sp2->bottom) {
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
			if ((y != bottom) || (toggle == 1)) lastd = NULL;
			toggle = 1;
			bottom = sp1->bottom;
			if (bottom > sp2->top) bottom = sp2->top;

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
				y = bottom;

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
		if ((y != bottom) || (toggle == 1)) lastd = NULL;
	
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
		bout << "*** SUB Needlessly grew to " << target->CountAvail() << "!" << endl
				<< "Source1: " << *this << endl
				<< "Source2: " << r2 << endl
				<< "Final: " << *target << endl;
	}
#endif

//	lastFuckingLabelInThisWholeGoddamnedFunction:;
};

/*----------------------------------------------------------------*/

void BRasterRegion::And(const BRasterRegion& r2, BRasterRegion* target) const
{
	CHECK_REGION(*this);
	CHECK_REGION(r2);
	
	if ((Bounds().bottom <= r2.Bounds().top) ||
		(Bounds().top >= r2.Bounds().bottom) ||
		(Bounds().left >= r2.Bounds().right) ||
		(Bounds().right <= r2.Bounds().left) ||
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
		BRasterRect r;
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

	BRasterRect *prevdRow=NULL,*lastd,*thisdRow,*d,*de;
	const BRasterRect *rp1=Rects(),*rp2=r2.Rects();
	const BRasterRect *sp1=rp1,*sp2=rp2;
	const BRasterRect *np1=rp1,*np2=rp2;
	const BRasterRect *ep1=rp1+CountRects(),*ep2=rp2+r2.CountRects();
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
		if (y != bottom) lastd = NULL;
		bottom = sp1->bottom;
		if (bottom > sp2->bottom) bottom = sp2->bottom;

		while (1) {
			while (rp1->right <= rp2->left) {
				advancex1:
				rp1++;
				if (rp1 == np1) goto doneWithRow;
			};
			while (rp2->right <= rp1->left) {
//				advancex2:
				rp2++;
				if (rp2 == np2) goto doneWithRow;
			};
			if (rp1->right <= rp2->left) goto advancex1;

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
		y = bottom;

		advancey1:
		while (sp1->bottom <= y) {
			sp1 = np1;
			if (sp1 == ep1) goto done;
			while ((np1 < ep1) && (np1->top == sp1->top)) np1++;
			if (y < sp1->top) y = sp1->top;
		};

		advancey2:
		while (sp2->bottom <= y) {
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
		bout << "*** AND Needlessly grew to " << target->CountAvail() << "!" << endl
				<< "Source1: " << *this << endl
				<< "Source2: " << r2 << endl
				<< "Final: " << *target << endl;
	}
#endif
}

/*----------------------------------------------------------------*/

void BRasterRegion::Or(const BRasterRegion& r2, BRasterRegion* target) const
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
					Bounds().right >= r2.Bounds().left) {
				// r1 is to the left of r2...
				BRasterRect r;
				r.left = Bounds().left;
				r.top = Bounds().top;
				r.right = r2.Bounds().right;
				r.bottom = Bounds().bottom;
				target->Set(r);
				return;
			} else if (	Bounds().right >= r2.Bounds().right &&
						Bounds().left <= r2.Bounds().right) {
				// r1 is to the right of r2...
				BRasterRect r;
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
					Bounds().bottom >= r2.Bounds().top) {
				// r1 is above r2...
				BRasterRect r;
				r.left = Bounds().left;
				r.top = Bounds().top;
				r.right = Bounds().right;
				r.bottom = r2.Bounds().bottom;
				target->Set(r);
				return;
			} else if (	Bounds().bottom >= r2.Bounds().bottom &&
						Bounds().top <= r2.Bounds().bottom) {
				// r1 is below r2...
				BRasterRect r;
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

	BRasterRect *prevdRow=NULL,*lastd,*thisdRow,*d,*de;
	const BRasterRect *rp1=Rects(),*rp2=r2.Rects();
	const BRasterRect *sp1=rp1,*sp2=rp2;
	const BRasterRect *np1=rp1,*np2=rp2;
	const BRasterRect *ep1=rp1+CountRects(),*ep2=rp2+r2.CountRects();
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
				if (y != bottom) lastd = NULL;
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
					
					while (rp1->left <= d->right) {
						if (rp1->right > d->right)
							d->right = rp1->right;
						rp1++;
						if (rp1 == np1) goto doneRow1;
					};
				};

				doneRow1: {
					while ((rp2 < np2) && (rp2->left <= d->right)) {
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
					while ((rp1 < np1) && (rp1->left <= d->right)) {
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
						y = bottom;
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
				if ((y != bottom) || (toggle == 1)) lastd = NULL;
				toggle = 1;
				bottom = sp1->bottom;
				if (bottom > sp2->top) bottom = sp2->top;
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
					y = bottom;
			};
		} else if (sp2 < np2) {
			while ((de-(d+1)) <= (np2-sp2)) {
				MAKE_SPACE(((np1-sp1)+(np2-sp2)), "#3");
			};
			rp2 = sp2;
			thisdRow = d;
			lastd = prevdRow;
			if ((y != bottom) || (toggle == 2)) lastd = NULL;
			toggle = 2;
			bottom = sp2->bottom;
			if (bottom > sp1->top) bottom = sp1->top;
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
				y = bottom;
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
		if ((y != bottom) || (toggle == toggleCmp)) lastd = NULL;
	
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
		bout << "*** OR Needlessly grew to " << target->CountAvail() << "!" << endl
				<< "Source1: " << *this << endl
				<< "Source2: " << r2 << endl
				<< "Final: " << *target << endl;
	}
#endif
};

/*----------------------------------------------------------------*/

void BRasterRegion::OffsetBy(int32 dh, int32 dv)
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

void BRasterRegion::ScaleBy(	BRasterRegion* target, const BRasterRect& srcRect, const BRasterRect& dstRect,
						int32 offsX, int32 offsY, bool tileX, bool tileY) const
{
	const int32 dstW = dstRect.right-dstRect.left;
	const int32 dstH = dstRect.bottom-dstRect.top;
	const int32 srcW = srcRect.right-srcRect.left;
	const int32 srcH = srcRect.bottom-srcRect.top;
	if (IsEmpty() || ((dstW == srcW) && (dstH == srcH))) {
		*target = *this;
		target->OffsetBy(offsX, offsY);
		return;
	};

	CHECK_REGION(*this);
	
	if ((IsRect()) &&
		(Bounds().right-Bounds().left == srcW) &&
		(Bounds().bottom-Bounds().top == srcH)) {
		BRasterRect r;
		r.left = offsX + Bounds().left * dstW / srcW;
		r.top = offsY + Bounds().top * dstH / srcH;
		r.right = r.left + dstW;
		r.bottom = r.top + dstH;
		target->Set(r);
		return;
	};

	if (!tileX && !tileY) {
		region* dst = target->ReUse(CountRects(), CountRects());
		if (!dst) return;
		const BRasterRect *s = Rects();
		BRasterRect *d = dst->rects;
		dst->count = CountRects();
		for (int32 i=CountRects();i;i--) {
			d->left = offsX + s->left * dstW / srcW;
			d->right = offsX + s->right * dstW / srcW;
			d->top = offsY + s->top * dstH / srcH;
			d->bottom = offsY + s->bottom * dstH / srcH;
			d++; s++;
		};
		target->Bounds().left = offsX + Bounds().left * dstW / srcW;
		target->Bounds().right = offsX + Bounds().right * dstW / srcW;
		target->Bounds().top = offsY + Bounds().top * dstH / srcH;
		target->Bounds().bottom = offsY + Bounds().bottom * dstH / srcH;
		CHECK_REGION(*target);
	} else if (!tileX && tileY) {
		int32 multHigh = ((dstH+srcH-1)/srcH);
		int32 multLow = (dstH/srcH);
		const int32 need = CountRects()*multHigh;
		region* dst = target->ReUse(need, need);
		if (!dst) return;
		const BRasterRect *s = Rects();
		BRasterRect *d = dst->rects;
		int32 low = Bounds().bottom;
		const int32 limit = offsY + dstH;
		if (multLow >= 1) {
			for (int32 i=CountRects();i;i--) {
				d->left = offsX + s->left * dstW / srcW;
				d->right = offsX + s->right * dstW / srcW;
				d->top = offsY + s->top;
				d->bottom = offsY + s->bottom;
				d++; s++;
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
				if (d->bottom > limit) d->bottom = limit;
				low = d->bottom;
				d++;
			};
		} else {
			for (int32 i=CountRects();i;i--) {
				d->top = offsY + s->top;
				if (d->top >= limit) break;
				d->bottom = offsY + s->bottom;
				if (d->bottom > limit) d->bottom = limit;
				low = d->bottom;
				d->left = offsX + s->left * dstW / srcW;
				d->right = offsX + s->right * dstW / srcW;
				d++; s++;
			};
		};
		dst->count = d-dst->rects;
		target->Bounds().left = offsX + Bounds().left * dstW / srcW;
		target->Bounds().right = offsX + Bounds().right * dstW / srcW;
		target->Bounds().top = offsY + Bounds().top;
		target->Bounds().bottom = low;
		CHECK_REGION(*target);
	} else if (tileX && !tileY) {
		int32 multHigh = ((dstW+srcW-1)/srcW);
		int32 multLow = (dstW/srcW);
		const int32 need = CountRects()*multHigh;
		region* dst = target->ReUse(need, need);
		if (!dst) return;
		const BRasterRect *s = Rects();
		BRasterRect *d = dst->rects;
		int32 right = Bounds().right;
		const int32 limit = offsX + dstW;
		if (multLow >= 1) {
			multLow--;
			for (int32 i=CountRects();i;i--) {
				d->left = offsX + s->left;
				d->right = offsX + s->right;
				d->top = offsY + s->top * dstH / srcH;
				d->bottom = offsY + s->bottom * dstH / srcH;
				d++;
				for (int32 j=multLow;j;j--) *d = *(d-1); d++;
				*d = *(d-1);
				d->left += srcW;
				if (d->left < limit) {
					d->right += srcW;
					if (d->right > limit) d->right = limit;
					if (d->right > right) right = d->right;
					d++;
				};
				s++;
			};
		} else {
			for (int32 i=CountRects();i;i--) {
				d->left = offsX + s->left;
				if (d->left < limit) {
					d->right = offsX + s->right;
					if (d->right > limit) d->right = limit;
					if (d->right > right) right = d->right;
					d->top = offsY + s->top * dstH / srcH;
					d->bottom = offsY + s->bottom * dstH / srcH;
					d++;
				};
				s++;
			};
		};
		dst->count = d-dst->rects;
		target->Bounds().left = offsX + Bounds().left;
		target->Bounds().right = right;
		target->Bounds().top = offsY + Bounds().top * dstH / srcH;
		target->Bounds().bottom = offsY + Bounds().bottom * dstH / srcH;
		CHECK_REGION(*target);
	} else {
		// Scale as two separate operations.  This could be optimized,
		// but so it goes.
		BRasterRect xRect;
		xRect.left = dstRect.left;
		xRect.right = dstRect.right;
		xRect.top = srcRect.top;
		xRect.bottom = srcRect.bottom;
		BRasterRegion tmp;
		ScaleBy(&tmp, srcRect, xRect, offsX, 0, true, false);
		tmp.ScaleBy(target, xRect, dstRect, 0, offsY, false, true);
	};
};
/*
void BRasterRegion::Rotate(const DisplayRotater& rot, BRasterRegion* dest) const
{
	const int32 N = CountRects();
	const BRasterRect* src = Rects();
	
	if (N <= 0) {
		dest->MakeEmpty();
		
	} else  if (N == 1) {
		BRasterRect rect;
		rot.RotateRect(Rects(), &rect);
		dest->Set(rect);
		return;
	
	} else {
		// Bow before my 3l33t h4x0r ski11z!!!!
		BRasterRegion rotReg;
		BRasterRegion tmpReg;
		BRasterRegion* curDest = dest;
		BRasterRegion* curSrc = &tmpReg;
		BRasterRegion* swap;
		BRasterRect* rotRect = rotReg.CreateRects(1, 1);
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
}
*/
/*----------------------------------------------------------------*/

void BRasterRegion::CompressSpans()
{
#warning BRasterRegion::CompressSpans() -- fix for right-side-exclusive
	CHECK_REGION(*this);
	
	const int32 sc=CountRects();
	region* spans = Edit(sc, sc);
	
	BRasterRect *s = spans->rects;
	BRasterRect *d = s;
	BRasterRect *lasts = s + sc;
	BRasterRect *backupS,*backupD,*tmpD;
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

void BRasterRegion::CompressMemory()
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
	
	const int32 exactSize = fData->count;
	region* reg = Edit(exactSize, exactSize);
	if (!reg) return;
	
	if (reg != &fBounds && reg->avail > exactSize) {
		reg = (region*)realloc(reg, sizeof(region) + sizeof(BRasterRect)*(exactSize-1));
		fData = reg;
		if (reg) reg->avail = exactSize;
	}
}

/*----------------------------------------------------------------*/

int32 BRasterRegion::FindSpanAt(int32 y) const DECLARE_RETURN(i)
{
	int32 i = -1;
	if (!fData) return i;
	const BRasterRect *rl = fData->rects;
	int32 low = 0;
	int32 high = fData->count-1;
	int32 old = -1;
	if ((y < Bounds().top) || (y >= Bounds().bottom)) return -1;
	while (1) {
		i = (low+high)/2;
		if (i==old) return -1;
		if (rl[i].bottom > y) {
			if (rl[i].top <= y) break;
			high = i-1;
		} else if (rl[i].top <= y) {
			low = i+1;
		}
		old = i;
	}
	return i;
}

int32 BRasterRegion::FindSpanBetween(int32 top, int32 bottom) const DECLARE_RETURN(i)
{
	int32 i = -1;
	if (!fData) return i;
	const BRasterRect *rl = fData->rects;
	int32 low = 0;
	int32 high = fData->count-1;
	int32 old = -1;
	if ((bottom <= Bounds().top) || (top >= Bounds().bottom)) return -1;
	while (1) {
		i = (low+high)/2;
		if (i==old) return -1;
		if (rl[i].bottom > top) {
			if (rl[i].top < bottom) break;
			high = i-1;
		} else if (rl[i].top < bottom) {
			low = i+1;
		}
		old = i;
	}
	return i;
}

bool BRasterRegion::Contains(int32 h, int32 v) const
{
	int32 low,high,y,i = FindSpanAt(v);
	if (i == -1) return false;

	const BRasterRect *rl = fData->rects;
	
	low = high = i;
	y = rl[i].top;

	while ((low >= 0) && (rl[low].top == y)) {
		if ((rl[low].left <= h) && (rl[low].right > h)) return true;
		low--;
	}

	while ((high < fData->count) && (rl[high].top == y)) {
		if ((rl[high].left <= h) && (rl[high].right > h)) return true;
		high++;
	}

	return false;
}

bool BRasterRegion::Intersects(const BRasterRect& r) const
{
	int32	i;

	if (!fData) return false;
	const int32 count = fData->count;
	if (count <= 0) return false;

	if (!r.IsValid() ||
		(r.bottom <= Bounds().top) ||
		(r.top   >= Bounds().bottom) ||
		(r.right <= Bounds().left) ||
		(r.left >= Bounds().right))
		return false;

	if (count == 1) return true;
	
	const BRasterRect *rl = fData->rects;
	int32 low,high;
	if ((i=FindSpanBetween(r.top,r.bottom))==-1) return false;
	low = i;
	while ((low>=0) && (rl[low].bottom > r.top)) {
		if ((rl[low].left < r.right) && (rl[low].right > r.left)) return true;
		low--;
	}
	high = i+1;
	while ((high<count) && (rl[high].top < r.bottom)) {
		if ((rl[high].left < r.right) && (rl[high].right > r.left)) return true;
		high++;
	}

	return false;
}

/*----------------------------------------------------------------*/

void BRasterRegion::PrintToStream(ITextOutput::arg io, uint32 flags) const
{
	const int32 N = CountRects();
	
	if (flags&B_PRINT_STREAM_HEADER) io << "BRasterRegion(";
	else io << "(";
	
	if (N == 0) {
		io << "<empty>)";
	
	} else if (N == 1) {
		io << Bounds() << ")";
	
	} else {
		io << Bounds() << ") {" << endl << indent;
		for (int i = 0; i < N; i++) {
			io << Rects()[i];
			if (i < N-1) {
				if (N <= 1) io << ", ";
				else io << "," << endl;
			}
		}
		io << dedent << "\n}";
	}
}

status_t BRasterRegion::printer(ITextOutput::arg io, const value_ref& val, uint32 flags)
{
	status_t result;
	BRasterRegion obj(val, &result);
	if (result == B_OK) obj.PrintToStream(io, flags);
	return result;
}

/*----------------------------------------------------------------*/

bool BRasterRegion::IsFixedSize() const
{
	return false;
}

type_code BRasterRegion::TypeCode() const
{
	return B_REGION_TYPE;
}

ssize_t BRasterRegion::FlattenedSize() const
{
	return (CountRects()+1) * sizeof(BRasterRect);
}

status_t BRasterRegion::Flatten(void *buffer, ssize_t size) const
{
	const ssize_t N = CountRects();
	if (size >= static_cast<ssize_t>((N+1)*sizeof(BRasterRect))) {
		*static_cast<BRasterRect*>(buffer) = Bounds();
		if (N > 0) {
			memcpy(	static_cast<BRasterRect*>(buffer)+1,
					Rects(),
					N*sizeof(BRasterRect));
		}
		return B_OK;
	} else {
		return B_BAD_VALUE;
	}
}

bool BRasterRegion::AllowsTypeCode(type_code code) const
{
	return code == B_REGION_TYPE;
}

status_t BRasterRegion::Unflatten(type_code c, const void *buf, ssize_t size)
{
	if (c == B_REGION_TYPE) {
		if (size >= static_cast<ssize_t>(sizeof(BRasterRect))) {
			const int32 count = ((size/sizeof(BRasterRect))-1);
			BRasterRect* data = CreateRects(count, count);
			if (data) {
				SetRectCount(count);
				Bounds() = *static_cast<const BRasterRect*>(buf);
				if (count > 0) {
					memcpy(	data,
							static_cast<const BRasterRect*>(buf)+1,
							count*sizeof(BRasterRect));
				}
				return B_OK;
			} else {
				return B_NO_MEMORY;
			}
		} else {
			return B_BAD_VALUE;
		}
	} else {
		return B_BAD_TYPE;
	}
}
		
/*----------------------------------------------------------------*/

BRasterRegion::region* BRasterRegion::EditSlow(const int32 needed, const int32 coulduse) const
{
	BRasterRegion* This = const_cast<BRasterRegion*>(this);
	
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


BRasterRegion::region* BRasterRegion::ReUseSlow(const int32 needed, const int32 coulduse) const
{
	BRasterRegion* This = const_cast<BRasterRegion*>(this);
	
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

bool BRasterRegion::CheckIntegrity() const
{
	int32 left,right,top,bottom;
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
		left = Rects()[0].left;
		top = Rects()[0].top;
		right = Rects()[0].right;
		bottom = Rects()[0].bottom;
		for (int32 i=0;i<CountRects();i++) {
			if (left > Rects()[i].left) left = Rects()[i].left;
			if (right < Rects()[i].right) right = Rects()[i].right;
			if (top > Rects()[i].top) top = Rects()[i].top;
			if (bottom < Rects()[i].bottom) bottom = Rects()[i].bottom;
			if ((Rects()[i].left < Bounds().left) ||
				(Rects()[i].right > Bounds().right) ||
				(Rects()[i].top < Bounds().top) ||
				(Rects()[i].bottom > Bounds().bottom)) {
				char blah[256];
				printf("rect %ld = (%ld,%ld)-(%ld,%ld), outside bounds (%ld,%ld)-(%ld,%ld)\n",
					i,Rects()[i].left,Rects()[i].top,Rects()[i].right,Rects()[i].bottom,
					Bounds().left,Bounds().top,Bounds().right,Bounds().bottom);
				sprintf(blah,"region at %p is corrupt!",this);
				debugger(blah);
				result = false;
			};
		};
		if ((left != Bounds().left) ||
			(right != Bounds().right) ||
			(top != Bounds().top) ||
			(bottom != Bounds().bottom)) {
			char blah[256];
			printf("real bounds: (%ld,%ld)-(%ld,%ld), stated bounds (%ld,%ld)-(%ld,%ld)\n",
				left,top,right,bottom,
				Bounds().left,Bounds().top,Bounds().right,Bounds().bottom);
			sprintf(blah,"region at %p is corrupt!",this);
			debugger(blah);
			result = false;
		};
	};
	
///	PrintToStream(bout, B_PRINT_STREAM_HEADER); bout << "\n";
	return result;
};

#endif

void BMoveBefore(BRasterRegion* to, BRasterRegion* from, size_t count)
{
	memcpy(to, from, sizeof(BRasterRegion)*count);
}

void BMoveAfter(BRasterRegion* to, BRasterRegion* from, size_t count)
{
	memmove(to, from, sizeof(BRasterRegion)*count);
}

ITextOutput::arg operator<<(ITextOutput::arg io, const BRasterRegion& region)
{
	region.PrintToStream(io, B_PRINT_STREAM_HEADER);
	return io;
}

}	// namespace Raster2

/*----------------------------------------------------------------*/

#if OPT_REGION_ALLOC

namespace Private {

inline BRasterRegion* BRasterRegionCache::GetRegion()
	DECLARE_RETURN(top)
{
	BRasterRegion* top;
	acquire_fast_sem();
	top = fRegionList;
	if (top) {
		fRegionCount--;
		fRegionList = (BRasterRegion*)top->fBounds.link;
		release_fast_sem();
		const_cast<BRasterRegion::region*>(top->fData)->count = 0;
		top->fBounds.rects[0] = invalRect;
		top->fBounds.link = NULL;
		CHECK_REGION(*top);
		return top;
	}
	release_fast_sem();
	
	top = (BRasterRegion*)malloc(sizeof(BRasterRegion));
	new(top) BRasterRegion();
	return top;
}

inline void BRasterRegionCache::SaveRegion(BRasterRegion* reg)
{
	CHECK_REGION(*reg);
	if (fRegionCount < 32 && reg->fData &&
			(reg->fData == &reg->fBounds || reg->fData->count <= 1)) {
		acquire_fast_sem();
		reg->fBounds.link = fRegionList;
		fRegionList = reg;
		fRegionCount++;
		release_fast_sem();
	} else {
		reg->~BRasterRegion();
		free(reg);
	}
}

}	// namespace Private
#endif

}	// namespace B
