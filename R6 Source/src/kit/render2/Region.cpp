
#include <render2/Region.h>

#include <new>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>

#include <kernel/OS.h>
#include <support2/Debug.h>
#include <support2/StdIO.h>
#include <support2_p/SupportMisc.h>
#include <render2/2dTransform.h>

namespace B {
namespace Render2 {

// gcc gets VERY UPSET if you bypass this temporary holding cell and
// use INFINITY directly when instantiating BRegion::full.
static const float inf = INFINITY;
const BRegion BRegion::empty;
const BRegion BRegion::full(BRect(-inf, -inf, inf, inf));

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

int32 max_bregion_mem = 0;
int32 cur_bregion_mem = 0;
int32 max_bregion_cnt = 0;
int32 cur_bregion_cnt = 0;

/*----------------------------------------------------------------*/

#if DB_WATCH_ALLOC
static void	add_region_memory(int32 amount)
{
	//printf("Adding %ld bytes...\n", amount);
	amount = atomic_add(&cur_bregion_mem, amount) + amount;
	int32 current;
	while ((current=max_bregion_mem) < amount) {
		compare_and_swap32(&max_bregion_mem, current, amount);
	}
	//printf("New total = %ld, new maximum = %ld\n", cur_bregion_mem, max_bregion_mem);
}

static void	rem_region_memory(int32 amount)
{
	//printf("Removing %ld bytes....\n", amount);
	atomic_add(&cur_bregion_mem, -amount);
	//printf("New total = %ld\n", cur_bregion_mem);
}

static void	add_region_count()
{
	const int32 amount = atomic_add(&cur_bregion_cnt, 1) + 1;
	int32 current;
	while ((current=max_bregion_cnt) < amount) {
		compare_and_swap32(&max_bregion_cnt, current, amount);
	}
}

static void	rem_region_count()
{
	atomic_add(&cur_bregion_cnt, -1);
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

class BRegionCache
{
public:
	BRegionCache()
	{
		fFastSem = create_sem(0, "region alloc sem");
	}
	~BRegionCache()
	{
		acquire_fast_sem();
		BRegion::region* data = fDataList;
		while (data) {
			BRegion::region* next = (BRegion::region*)data->link;
			free(data);
			data = next;
		}
		fDataList = NULL;
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

	static BRegion::region* GetData(const int32 count);
	static void SaveData(BRegion::region* reg);

private:
	static int32 fFastLock;
	static sem_id fFastSem;
	static int32 fDataCount;
	static BRegion::region* fDataList;
};

inline BRegion::region* BRegionCache::GetData(const int32 count)
	DECLARE_RETURN(top)
{
	acquire_fast_sem();
	BRegion::region* top = fDataList;
	BRegion::region** prev = &fDataList;
	while (top) {
		if (top->avail >= count) {
			fDataCount--;
			*prev = (BRegion::region*)top->link;
			release_fast_sem();
			top->link = NULL;
			return top;
		}
		prev = (BRegion::region**)(&top->link);
		top = (BRegion::region*)top->link;
	}
	release_fast_sem();
	if (count < 8) {
		top = (BRegion::region*)malloc(sizeof(BRegion::region) + 7*sizeof(BRect));
		if (top) top->avail = 8;
	} else {
		top = (BRegion::region*)malloc(sizeof(BRegion::region) + (count-1)*sizeof(BRect));
		if (top) top->avail = count;
	}
	return top;
}

inline void BRegionCache::SaveData(BRegion::region* reg)
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

int32 BRegionCache::fFastLock = 0;
sem_id BRegionCache::fFastSem = B_BAD_SEM_ID;
int32 BRegionCache::fDataCount = 0;
BRegion::region* BRegionCache::fDataList = NULL;


BRegionCache BRegion::cache;
#endif

inline void BRegion::region::acquire() const
{
	atomic_add(&refs, 1);
}

inline void BRegion::region::release() const
{
	if (atomic_add(&refs, -1) == 1) {
		REM_REGION_MEMORY(sizeof(region) + sizeof(BRect)*(avail-1));
		BRegionCache::SaveData(const_cast<region*>(this));
	}
}

inline BRegion::region* BRegion::region::create(const int32 init_avail) DECLARE_RETURN(r)
{
	ADD_REGION_MEMORY(sizeof(region) + sizeof(BRect)*(init_avail-1));
	region* r =  BRegionCache::GetData(init_avail);
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

inline BRegion::region* BRegion::region::edit(const int32 needed, const int32 coulduse) const DECLARE_RETURN(r)
{
	region* r;
	
	// Are we the sole owner of this region?
	if (refs <= 1) {
		if (needed <= avail && coulduse >= (avail/4)) {
			return const_cast<region*>(this);
		}
		
		r = const_cast<region*>(this);
		ADD_REGION_MEMORY(sizeof(BRect)*(coulduse-avail));
		r = (region*)realloc(r, sizeof(region) + sizeof(BRect)*(coulduse-1));
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
		memcpy(r->rects, rects, sizeof(BRect)*(size));
		r->count = size;
		#if DB_INTEGRITY_CHECKS
		if (r->avail < MIN_AVAIL)
			debugger("Editing bad avail");
		#endif
	}
	release();
	return r;
}

inline BRegion::region* BRegion::region::reuse(const int32 needed, const int32 coulduse) const
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

//
// Code that's common to the copy constructor and assignment
// operator.  It's "Unsafe" because it doesn't check for
// self-assignment and doesn't call fData->release().
//
inline void BRegion::UnsafeCopyFrom(const BRegion &src)
{
	fBounds = src.fBounds;
	if (src.fData == &src.fBounds || !src.fData) {
		fData = &fBounds;
		fBounds.refs = 10000;
	} else {
		#if DB_INTEGRITY_CHECKS
		if (src.fData->avail < MIN_AVAIL)
			debugger("Bad incoming avail");
		#endif
		#if DB_NO_SHARING
		fData = region::create(src.fData->count);
		if (fData) {
			memcpy(const_cast<region*>(fData)->rects, src.fData->rects, src.fData->count*sizeof(BRect));
			const_cast<region*>(fData)->count = src.fData->count;
		}
		#else
		fData = src.fData;
		fData->acquire();
		#endif
	}
	CHECK_REGION(*this);
}

/*----------------------------------------------------------------*/

void BRegion::ConstructCommon()
{
	ADD_REGION_COUNT();
	fData = &fBounds;
	fBounds.refs = 10000;
	fBounds.avail = 1;
}

/*----------------------------------------------------------------*/

void BRegion::Construct(const BRect &rect)
{
	ConstructCommon();
	fBounds.count = 1;
	BoundsRef() = rect;
}

/*----------------------------------------------------------------*/

BRegion::BRegion() : BFlattenable()
{
	ConstructCommon();
	fBounds.count = 0;
	// Default BRect constructor makes fBounds.rects[0] invalid.
	CHECK_REGION(*this);
}

/*----------------------------------------------------------------*/

BRegion::BRegion(BRect rect)
	:	BFlattenable()
{
	Construct(rect);
	CHECK_REGION(*this);
}

/*----------------------------------------------------------------*/

BRegion::BRegion(const BRegion &src)
	:	BFlattenable()
{
	CHECK_REGION(src);

	UnsafeCopyFrom(src);
	ADD_REGION_COUNT();
}

/*----------------------------------------------------------------*/

BRegion::BRegion(const BRasterRegion &src, const B2dTransform &trans)
	:	BFlattenable()
{
	int32 count = src.CountRects();
	
	if (count == 0) {
		ConstructCommon();
		fBounds.count = 0;
	}
	else if (count == 1) {
		Construct(BRect(src.Bounds().left,  src.Bounds().top,
		                src.Bounds().right, src.Bounds().bottom));
	}
	else {
		fBounds.refs = 10000;
		fBounds.avail = 1;
		fBounds.count = 1;
		BoundsRef().Set(src.Bounds().left,  src.Bounds().top,
		                src.Bounds().right, src.Bounds().bottom);
		
		const BRasterRect *src_rect = src.Rects();
		region *reg;
		if (src_rect && (reg = region::create(count))) {
			reg->count = count;
			for (BRect *dest_rect = reg->rects; count > 0;
			     --count, ++dest_rect, ++src_rect)
			{
				 dest_rect->Set(src_rect->left,  src_rect->top,
				                src_rect->right, src_rect->bottom);
			}
			fData = reg;
		}
		else {
			fData = &fBounds;
		}
	}
	CHECK_REGION(*this);

	Transform(trans);
}

/*----------------------------------------------------------------*/

BRegion::BRegion(const BValue &flat, status_t *result)
	:	BFlattenable()
{
	Construct(value_ref(flat), result);
}

/*----------------------------------------------------------------*/

BRegion::BRegion(const value_ref &flat, status_t *result)
	:	BFlattenable()
{
	Construct(flat, result);
}

/*----------------------------------------------------------------*/

void BRegion::Construct(const value_ref& flat, status_t* result)
{
	ADD_REGION_COUNT();
	fData = &fBounds;
	fBounds.refs = 10000;
	fBounds.count = 0;
	fBounds.avail = 1;
	// Default BRect constructor makes fBounds.rects[0] invalid.
	const status_t r = Unflatten(flat.type, flat.data, flat.length);
	if (result) *result = r;
	CHECK_REGION(*this);
}

/*----------------------------------------------------------------*/

BRegion	&BRegion::operator=(const BRegion &src)
{
	CHECK_REGION(src);

	if (this == &src) return *this;
	if (fData) fData->release();

	UnsafeCopyFrom(src);
	return *this;
}

bool BRegion::operator==(const BRegion& o) const
{
	if (!fData || !o.fData)
		return fData == o.fData;
	if (fData->count != o.fData->count)
		return false;
	if ((BoundsRef().top != o.BoundsRef().top) ||
		(BoundsRef().left != o.BoundsRef().left) ||
		(BoundsRef().right != o.BoundsRef().right) ||
		(BoundsRef().bottom != o.BoundsRef().bottom))
		return false;
	if (fData == o.fData)
		return true;
	if (memcmp(fData->rects, o.fData->rects, sizeof(BRect)*fData->count) == 0)
		return true;
	return false;
}


/*----------------------------------------------------------------*/

BValue BRegion::AsValue() const
{
	return BValue::Flat(*this);
}

/*----------------------------------------------------------------*/

void BRegion::Swap(BRegion& with)
{
	char buffer[sizeof(BRegion)];
	memcpy(buffer, this, sizeof(BRegion));
	memcpy(this, &with, sizeof(BRegion));
	memcpy(&with, buffer, sizeof(BRegion));
}

/*----------------------------------------------------------------*/

BRegion::~BRegion()
{
	CHECK_REGION(*this);
	REM_REGION_COUNT();
	if (fData) fData->release();
}

/*----------------------------------------------------------------*/

BRect BRegion::Bounds() const
{
	return BoundsRef();
}

/*----------------------------------------------------------------*/

bool BRegion::Intersects(BRect a_rect) const
{
	if (!fData) return false;
	const int32 count = fData->count;
	if (count <= 0) return false;

	if (!a_rect.IsValid() ||
		(a_rect.bottom <= BoundsRef().top) ||
		(a_rect.top    >= BoundsRef().bottom) ||
		(a_rect.right  <= BoundsRef().left) ||
		(a_rect.left   >= BoundsRef().right))
		return false;

	if (count == 1) return true;
	
	const BRect *rl = fData->rects;
	int32 i = FindSpanBetween(a_rect.top, a_rect.bottom);
	
	if (i == -1) return false;
	int32 low = i;
	while ((low >= 0) && (rl[low].bottom > a_rect.top)) {
		if ((rl[low].left < a_rect.right) && (rl[low].right > a_rect.left)) return true;
		low--;
	}
	int32 high = i+1;
	while ((high < count) && (rl[high].top < a_rect.bottom)) {
		if ((rl[high].left < a_rect.right) && (rl[high].right > a_rect.left)) return true;
		high++;
	}

	return false;
}

/*----------------------------------------------------------------*/

int32 BRegion::FindSpanBetween(coord top, coord bottom) const DECLARE_RETURN(i)
{
	int32 i = -1;
	if (!fData) return i;
	const BRect *rl = fData->rects;
	int32 low = 0;
	int32 high = fData->count-1;
	int32 old = -1;
	if ((bottom <= BoundsRef().top) || (top >= BoundsRef().bottom)) return -1;
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

/*----------------------------------------------------------------*/

bool BRegion::Contains(BPoint a_point) const
{
	int32 i = FindSpanAt(a_point.y);
	int32 low, high;
	coord y;
	
	if (i == -1) return false;

	const BRect *rl = fData->rects;
	
	low = high = i;
	y = rl[i].top;

	while ((low >= 0) && (rl[low].top == y)) {
		if ((rl[low].left <= a_point.x) && (rl[low].right > a_point.x)) return true;
		low--;
	}

	while ((high < fData->count) && (rl[high].top == y)) {
		if ((rl[high].left <= a_point.x) && (rl[high].right > a_point.x)) return true;
		high++;
	}

	return false;
}

/*----------------------------------------------------------------*/

int32 BRegion::FindSpanAt(coord y) const DECLARE_RETURN(i)
{
	int32 i = -1;
	if (!fData) return i;
	const BRect *rl = fData->rects;
	int32 low = 0;
	int32 high = fData->count-1;
	int32 old = -1;
	if ((y < BoundsRef().top) || (y >= BoundsRef().bottom)) return -1;
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

/*----------------------------------------------------------------*/

void BRegion::Include(const BRegion &other)
{
	OrSelf(other);
}

/*----------------------------------------------------------------*/

void BRegion::Exclude(const BRegion &other)
{
	SubSelf(other);
}

/*----------------------------------------------------------------*/

void BRegion::IntersectWith(const BRegion &other)
{
	AndSelf(other);
}

/*----------------------------------------------------------------*/

void BRegion::Exclude(BRect r)
{
	BRegion tmp(r);
	SubSelf(tmp);
}

void BRegion::Include(BRect r)
{
	if (!r.IsValid() || !fData) return;

	if (fData->count == 0) {
		Set(r);
		return;
	}

	if (r.top >= BoundsRef().bottom) {
		region* reg = Grow(1);
		reg->rects[reg->count++] = r;
		BoundsRef().bottom = r.bottom;
		smin(BoundsRef().left, r.left);
		smax(BoundsRef().right, r.right);
		return;
	}

	OrSelf(BRegion(r));
}

/*----------------------------------------------------------------*/

inline BRegion::region* BRegion::Edit(const int32 needed, const int32 coulduse) const
{
	// Fast case: we own the region, and don't need to resize it.
	if ((fData == &fBounds || fData->refs <= 1)&& needed <= fData->avail)
		return const_cast<region*>(fData);
	
	// All other cases.
	return EditSlow(needed, coulduse);
}

inline BRegion::region* BRegion::ReUse(const int32 needed, const int32 coulduse) const
{
	// Fast case: we own the region, and don't need to resize it.
	if ((fData == &fBounds || (fData && fData->refs <= 1)) && needed <= fData->avail)
		return const_cast<region*>(fData);
	
	// All other cases.
	return ReUseSlow(needed, coulduse);
}

BRect * BRegion::CreateRects(int32 needed, int32 coulduse)
{
	region* reg = ReUse(needed, coulduse);
	if (reg) return reg->rects;
	return NULL;
}

void BRegion::SetRectCount(int32 num)
{
	if (!fData) return;
	region* reg = Edit(num, num);
	if (reg) reg->count = num;
}

void BRegion::MakeEmpty()
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
	fBounds.rects[0] = BRect();
}

BRegion::region* BRegion::EditSlow(const int32 needed, const int32 coulduse) const
{
	BRegion* This = const_cast<BRegion*>(this);
	
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

BRegion::region* BRegion::ReUseSlow(const int32 needed, const int32 coulduse) const
{
	BRegion* This = const_cast<BRegion*>(this);
	
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
		This->BoundsRef() = BRect();
		fData->release();
		return const_cast<region*>(This->fData = &fBounds);
	}
	
	// Standard case: the region is currently out-of-line, and is going
	// to stay that way.
	return const_cast<region*>(
		This->fData = fData->reuse(needed, coulduse < MIN_AVAIL ? MIN_AVAIL : coulduse));
}

//-------------------------------------------------

void BRegion::Set(BRect r)
{
	if (r.IsValid()) {
		region* reg = ReUse(1, 1);
		if (reg) {
			reg->count = 1;
			reg->rects[0] = fBounds.rects[0] = r;
		}
		CHECK_REGION(*this);
	} else {
		MakeEmpty();
	}
}

/*----------------------------------------------------------------*/

void BRegion::OffsetBy(BPoint delta)
{
	int32 i;
	if (IsEmpty() || (i = fData->count) <= 0 || (delta.x == 0 && delta.y == 0)) {
		return;
	}
	
	CHECK_REGION(*this);
	
	region* reg = Edit(i, i);
	if (!reg) return;
	
	if (reg != &fBounds) {
		BoundsRef().top += delta.y;
		BoundsRef().left += delta.x;
		BoundsRef().right += delta.x;
		BoundsRef().bottom += delta.y;
	}
	
	coord* ptr = &reg->rects[0].left;
	
	if (delta.x != 0 && delta.y != 0) {
		while (i-- > 0) {
			*ptr++ += delta.x;
			*ptr++ += delta.y;
			*ptr++ += delta.x;
			*ptr++ += delta.y;
		}
	} else if (delta.x != 0) {
		while (i-- > 0) {
			*ptr++ += delta.x;
			ptr++;
			*ptr++ += delta.x;
			ptr++;
		}
	} else {
		while (i-- > 0) {
			ptr++;
			*ptr++ += delta.y;
			ptr++;
			*ptr++ += delta.y;
		}
	}
	
	CHECK_REGION(*this);
}


void BRegion::And(const BRegion& r2, BRegion* dest) const
{
	CHECK_REGION(*this);
	CHECK_REGION(r2);
	
	if ((BoundsRef().bottom <= r2.BoundsRef().top) ||
		(BoundsRef().top >= r2.BoundsRef().bottom) ||
		(BoundsRef().left >= r2.BoundsRef().right) ||
		(BoundsRef().right <= r2.BoundsRef().left) ||
		(IsEmpty()) ||
		(r2.IsEmpty())) {
		dest->MakeEmpty();
		return;
	};

#if OPT_SIMPLE_REGIONS
	// First check if we can optimize a 1 rect & 1 rect -> 1 rect case.
	// (If we allow these situations to fall through to the generic case,
	// we will needlessly allocate data for the dest region.)
	if (CountRects() == 1 && r2.CountRects() == 1) {
		// The intersection of two rectangles is always a single rectangle.
		BRect r;
		r.left = BoundsRef().left >= r2.BoundsRef().left ? BoundsRef().left : r2.BoundsRef().left;
		r.top = BoundsRef().top >= r2.BoundsRef().top ? BoundsRef().top : r2.BoundsRef().top;
		r.right = BoundsRef().right <= r2.BoundsRef().right ? BoundsRef().right : r2.BoundsRef().right;
		r.bottom = BoundsRef().bottom <= r2.BoundsRef().bottom ? BoundsRef().bottom : r2.BoundsRef().bottom;
		dest->Set(r);
		return;
	}
#endif

#if DB_GROWTH_CHECK
	const int32 avail = dest->CountAvail();
#endif

	BRect *prevdRow=NULL,*lastd,*thisdRow,*d,*de;
	const BRect *rp1=Rects(),*rp2=r2.Rects();
	const BRect *sp1=rp1,*sp2=rp2;
	const BRect *np1=rp1,*np2=rp2;
	const BRect *ep1=rp1+CountRects(),*ep2=rp2+r2.CountRects();
	coord bottom = 0;
	coord y, left, right;
//	int32 tmp1, tmp2;

	region* dst = dest->ReUse(0, 0);
	if (!dst)
		return;
	
	dst->count = 0;
	d = dst->rects-1;
	de = dst->rects+dst->avail;
	
	left = FLT_MAX;
	right = -FLT_MAX;

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
			int32 tmp1 = d-dst->rects;
			int32 tmp2 = prevdRow-dst->rects;
			if (! (dst = dest->GrowAvail(1)) )
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
		dest->BoundsRef().top = dst->rects[0].top;
		dest->BoundsRef().bottom = dst->rects[dst->count-1].bottom;
		dest->BoundsRef().left = left;
		dest->BoundsRef().right = right;
		CHECK_REGION(*dest);
	} else
		dest->MakeEmpty();
	
#if DB_GROWTH_CHECK
	if (CountRects() == 1 && r2.CountRects() == 1 && dest->CountRects() == 1
			&& avail <= 1 && dest->CountAvail() > 1) {
		bout << "*** AND Needlessly grew to " << dest->CountAvail() << "!" << endl
				<< "Source1: " << *this << endl
				<< "Source2: " << r2 << endl
				<< "Final: " << *dest << endl;
	}
#endif
}

void BRegion::Or(const BRegion& r2, BRegion* dest) const
{
	CHECK_REGION(*this);
	CHECK_REGION(r2);
	
	if (IsEmpty()) {
		*dest = r2;
		return;
	};
	
	if (r2.IsEmpty()) {
		*dest = *this;
		return;
	};

#if OPT_SIMPLE_REGIONS
	// First check if we can optimize a 1 rect | 1 rect -> 1 rect case.
	// (If we allow these situations to fall through to the generic case,
	// we will needlessly allocate data for the dest region.)
	if (CountRects() == 1 && r2.CountRects() == 1) {
		if (	BoundsRef().left >= r2.BoundsRef().left &&
				BoundsRef().top >= r2.BoundsRef().top &&
				BoundsRef().right <= r2.BoundsRef().right &&
				BoundsRef().bottom <= r2.BoundsRef().bottom) {
			// r1 is entirely contained in r2...
			dest->Set(r2.BoundsRef());
			return;
		} else if (	BoundsRef().left <= r2.BoundsRef().left &&
					BoundsRef().top <= r2.BoundsRef().top &&
					BoundsRef().right >= r2.BoundsRef().right &&
					BoundsRef().bottom >= r2.BoundsRef().bottom) {
			// r2 is entirely contained in r2...
			dest->Set(BoundsRef());
			return;
		} else if (	BoundsRef().top == r2.BoundsRef().top &&
					BoundsRef().bottom == r2.BoundsRef().bottom) {
			// We may be growing horizontally...
			if (	BoundsRef().left <= r2.BoundsRef().left &&
					BoundsRef().right >= r2.BoundsRef().left) {
				// r1 is to the left of r2...
				BRect r;
				r.left = BoundsRef().left;
				r.top = BoundsRef().top;
				r.right = r2.BoundsRef().right;
				r.bottom = BoundsRef().bottom;
				dest->Set(r);
				return;
			} else if (	BoundsRef().right >= r2.BoundsRef().right &&
						BoundsRef().left <= r2.BoundsRef().right) {
				// r1 is to the right of r2...
				BRect r;
				r.left = r2.BoundsRef().left;
				r.top = BoundsRef().top;
				r.right = BoundsRef().right;
				r.bottom = BoundsRef().bottom;
				dest->Set(r);
				return;
			}
		} else if (	BoundsRef().left == r2.BoundsRef().left &&
					BoundsRef().right == r2.BoundsRef().right) {
			// We may be growing vertically...
			if (	BoundsRef().top <= r2.BoundsRef().top &&
					BoundsRef().bottom >= r2.BoundsRef().top) {
				// r1 is above r2...
				BRect r;
				r.left = BoundsRef().left;
				r.top = BoundsRef().top;
				r.right = BoundsRef().right;
				r.bottom = r2.BoundsRef().bottom;
				dest->Set(r);
				return;
			} else if (	BoundsRef().bottom >= r2.BoundsRef().bottom &&
						BoundsRef().top <= r2.BoundsRef().bottom) {
				// r1 is below r2...
				BRect r;
				r.left = BoundsRef().left;
				r.top = r2.BoundsRef().top;
				r.right = BoundsRef().right;
				r.bottom = BoundsRef().bottom;
				dest->Set(r);
				return;
			}
		}
	}
#endif

#if DB_GROWTH_CHECK
	const int32 avail = dest->CountAvail();
#endif

	BRect *prevdRow=NULL,*lastd,*thisdRow,*d,*de;
	const BRect *rp1=Rects(),*rp2=r2.Rects();
	const BRect *sp1=rp1,*sp2=rp2;
	const BRect *np1=rp1,*np2=rp2;
	const BRect *ep1=rp1+CountRects(),*ep2=rp2+r2.CountRects();
	coord bottom = 0;
	coord y;
//	int32 tmp1, tmp2;
	int32 toggle = -1;
	int32 toggleCmp = 1;
	
	region* dst = dest->ReUse(0, 0);
	if (!dst)
		return;
	
	dst->count = 0;
	d = dst->rects-1;
	de = dst->rects+dst->avail;
	
	y = sp1->top;
	if (y > sp2->top) y = sp2->top;
	
	#define MAKE_SPACE(needed, msg)	{											\
		PRINT(("Target has %ld (%ld total), need %d -- growing %s\n",			\
				de-(d+1), de-dst->rects, needed, msg));							\
		int32 tmp1 = d-dst->rects;													\
		int32 tmp2 = prevdRow-dst->rects;												\
		if (! (dst = dest->GrowAvail(1)) )									\
			return;																\
		d = dst->rects + tmp1;													\
		de = dst->rects + dst->avail;											\
		if (prevdRow) prevdRow = dst->rects + tmp2; }
	
	while (1) {
		while ((np1 < ep1) && (np1->top == y)) np1++;
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
		dest->BoundsRef().top = dst->rects[0].top;
		dest->BoundsRef().bottom = dst->rects[dst->count-1].bottom;
		dest->BoundsRef().left = BoundsRef().left;
		if (dest->BoundsRef().left > r2.BoundsRef().left) dest->BoundsRef().left = r2.BoundsRef().left;
		dest->BoundsRef().right = BoundsRef().right;
		if (dest->BoundsRef().right < r2.BoundsRef().right) dest->BoundsRef().right = r2.BoundsRef().right;
		CHECK_REGION(*dest);
	} else
		dest->MakeEmpty();
	
#if DB_GROWTH_CHECK
	if (CountRects() == 1 && r2.CountRects() == 1 && dest->CountRects() == 1
			&& avail <= 1 && dest->CountAvail() > 1) {
		bout << "*** OR Needlessly grew to " << dest->CountAvail() << "!" << endl
				<< "Source1: " << *this << endl
				<< "Source2: " << r2 << endl
				<< "Final: " << *dest << endl;
	}
#endif
}

void BRegion::Sub(const BRegion& r2, BRegion* dest) const
{
	CHECK_REGION(*this);
	CHECK_REGION(r2);
	
	if ((BoundsRef().bottom <= r2.BoundsRef().top) ||
		(BoundsRef().top >= r2.BoundsRef().bottom) ||
		(BoundsRef().left >= r2.BoundsRef().right) ||
		(BoundsRef().right <= r2.BoundsRef().left) ||
		(r2.IsEmpty())) {
		*dest = *this;
		return;
	};

	if (IsEmpty()) {
		dest->MakeEmpty();
		return;
	};

#if OPT_SIMPLE_REGIONS
	// First check if we can optimize a 1 rect - 1 rect -> 1 rect case.
	// (If we allow these situations to fall through to the generic case,
	// we will needlessly allocate data for the dest region.)
	if (CountRects() == 1 && r2.CountRects() == 1) {
		if (	BoundsRef().top >= r2.BoundsRef().top &&
				BoundsRef().bottom <= r2.BoundsRef().bottom) {
			// We may be shrinking horizontally...
			if (	r2.BoundsRef().left <= BoundsRef().left &&
					r2.BoundsRef().right < BoundsRef().right) {
				// r2 is removing the left of r1...
				BRect r;
				r.left = r2.BoundsRef().right;
				r.top = BoundsRef().top;
				r.right = BoundsRef().right;
				r.bottom = BoundsRef().bottom;
				dest->Set(r);
				return;
			} else if (	r2.BoundsRef().right >= BoundsRef().right &&
						r2.BoundsRef().left > BoundsRef().left) {
				// r2 is removing the right of r1...
				BRect r;
				r.left = BoundsRef().left;
				r.top = BoundsRef().top;
				r.right = r2.BoundsRef().left;
				r.bottom = BoundsRef().bottom;
				dest->Set(r);
				return;
			}
		} else if (	BoundsRef().left >= r2.BoundsRef().left &&
					BoundsRef().right <= r2.BoundsRef().right) {
			// We may be shrinking vertically...
			if (	r2.BoundsRef().top <= BoundsRef().top &&
					r2.BoundsRef().bottom < BoundsRef().bottom) {
				// r2 is removing the top of r1...
				BRect r;
				r.left = BoundsRef().left;
				r.top = r2.BoundsRef().bottom;
				r.right = BoundsRef().right;
				r.bottom = BoundsRef().bottom;
				dest->Set(r);
				return;
			} else if (	r2.BoundsRef().bottom >= BoundsRef().bottom &&
						r2.BoundsRef().top > BoundsRef().top) {
				// r2 is removing the bottom of r1...
				BRect r;
				r.left = BoundsRef().left;
				r.top = BoundsRef().top;
				r.right = BoundsRef().right;
				r.bottom = r2.BoundsRef().top;
				dest->Set(r);
				return;
			}
		}
	}
#endif

#if DB_GROWTH_CHECK
	const int32 avail = dest->CountAvail();
#endif

	BRect *prevdRow=NULL,*lastd,*thisdRow,*d,*de;
	const BRect *rp1=Rects(),*rp2=r2.Rects();
	const BRect *sp1=rp1,*sp2=rp2;
	const BRect *np1=rp1,*np2=rp2;
	const BRect *ep1=rp1+CountRects(),*ep2=rp2+r2.CountRects();
	coord bottom = 0;
	coord y, left, right;
//	int32 tmp1, tmp2;
	int32 toggle = -1;
	
	region* dst = dest->ReUse(0, 0);
	if (!dst)
		return;
	
	dst->count = 0;
	d = dst->rects-1;
	de = dst->rects+dst->avail;
	
	left = FLT_MAX;
	right = -FLT_MAX;

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
				int32 tmp1 = d-dst->rects;
				int32 tmp2 = prevdRow-dst->rects;
				if (! (dst = dest->GrowAvail(1)) )
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
					coord tmp_r = d->right;
					d->right = rp2->left;
					if (tmp_r > rp2->right) {
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
						d->right = tmp_r;
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
				int32 tmp1 = d-dst->rects;
				int32 tmp2 = prevdRow-dst->rects;
				if (! (dst = dest->GrowAvail(1)) )
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
			int32 tmp1 = d-dst->rects;
			int32 tmp2 = prevdRow-dst->rects;
			if (! (dst = dest->GrowAvail(1)) )
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
		dest->BoundsRef().top = dst->rects[0].top;
		dest->BoundsRef().bottom = dst->rects[dst->count-1].bottom;
		dest->BoundsRef().left = left;
		dest->BoundsRef().right = right;
		CHECK_REGION(*dest);
	} else
		dest->MakeEmpty();

#if DB_GROWTH_CHECK
	if (CountRects() == 1 && r2.CountRects() == 1 && dest->CountRects() == 1
			&& avail <= 1 && dest->CountAvail() > 1) {
		bout << "*** SUB Needlessly grew to " << dest->CountAvail() << "!" << endl
				<< "Source1: " << *this << endl
				<< "Source2: " << r2 << endl
				<< "Final: " << *dest << endl;
	}
#endif

//	lastFuckingLabelInThisWholeGoddamnedFunction:;
}

void BRegion::XOr(const BRegion& r2, BRegion* dest) const
{
	BRegion tmp1, tmp2;

	r2.Sub(*this, &tmp1);
	Sub(r2, &tmp2);
	tmp1.Or(tmp2, dest);
}


/*----------------------------------------------------------------*/

bool BRegion::IsEmpty() const
{
	return (!fData || fData->count == 0);
}

/*----------------------------------------------------------------*/

bool BRegion::IsFull() const
{
	return (*this == full);
}

/*----------------------------------------------------------------*/

void BRegion::Rasterize(BRasterRegion* target) const
{
	if (fData && fData->count != 0) {
		target->SetRectCount(fData->count);
		BRasterRect *target_rect = target->EditRects(fData->count, fData->count);
		if (target_rect) {
			const BRect *source_rect = fData->rects;
			for (int i = fData->count; i > 0;
			     --i, ++target_rect, ++source_rect)
			{
				*target_rect = BRasterRect(*source_rect);
			}
			target->Bounds() = BRasterRect(BoundsRef());
			return;
		}
	}
	target->MakeEmpty();
}

/*----------------------------------------------------------------*/

void BRegion::Rasterize(BRasterRegion* target, const B2dTransform& t) const
{
	if (t.Operations() == 0) Rasterize(target);
	else {
		BRegion reg(*this);
		reg.Transform(t);
		reg.Rasterize(target);
	}
}

/*----------------------------------------------------------------*/

void BRegion::Transform(const B2dTransform& t)
{
	// Cheesy-ass implementation until BRegion actually becomes vector-based.
	const uint32 op = t.Operations();
	if (op) {
		if (!IsEmpty() && !IsFull()) {
			if ((op&~(B_TRANSFORM_TRANSLATE|B_TRANSFORM_SCALE)) == 0) {
				const BPoint origin = t.Origin();
				if ((op&B_TRANSFORM_SCALE) != 0) {
					BRegion result;
					BRect src(BoundsRef());
					BRect dst(src);
					t.DeltaTransformBounds(&dst);
					ScaleBy(&result, src, dst, origin);
					*this = result;
				} else {
					OffsetBy(origin);
				}
			} else {
				BRect bounds(BoundsRef());
				t.TransformBounds(&bounds);
				Set(bounds);
			}
		}
	}
	CHECK_REGION(*this);
}

/*----------------------------------------------------------------*/

void BRegion::ScaleBy(BRegion* target, const BRect& srcRect, const BRect& dstRect,
						BPoint offset) const
{
	const coord dstW = dstRect.Width();
	const coord dstH = dstRect.Height();
	const coord srcW = srcRect.Width();
	const coord srcH = srcRect.Height();
	if (IsEmpty() || ((dstW == srcW) && (dstH == srcH))) {
		*target = *this;
		target->OffsetBy(offset);
		CHECK_REGION(*target);
		return;
	};

	CHECK_REGION(*this);
	
	if ((IsRect()) &&
		(BoundsRef().Width()  == srcW) &&
		(BoundsRef().Height() == srcH))
	{
		BRect r;
		r.left = offset.x + BoundsRef().left * dstW / srcW;
		r.top = offset.y + BoundsRef().top * dstH / srcH;
		r.right = r.left + dstW;
		r.bottom = r.top + dstH;
		target->Set(r);
		CHECK_REGION(*target);
		return;
	};

	region* dst = target->ReUse(CountRects(), CountRects());
	if (!dst) return;
	const BRect *s = fData->rects;
	BRect *d = dst->rects;
	dst->count = CountRects();
	for (int32 i=CountRects(); i; i--) {
		d->left = offset.x + s->left * dstW / srcW;
		d->right = offset.x + s->right * dstW / srcW;
		d->top = offset.y + s->top * dstH / srcH;
		d->bottom = offset.y + s->bottom * dstH / srcH;
		d++; s++;
	};
	target->BoundsRef().left = offset.x + BoundsRef().left * dstW / srcW;
	target->BoundsRef().right = offset.x + BoundsRef().right * dstW / srcW;
	target->BoundsRef().top = offset.y + BoundsRef().top * dstH / srcH;
	target->BoundsRef().bottom = offset.y + BoundsRef().bottom * dstH / srcH;
	CHECK_REGION(*target);
}

/*----------------------------------------------------------------*/

#if DB_INTEGRITY_CHECKS

bool BRegion::CheckIntegrity() const
{
	coord left, right, top, bottom;
	bool result = true;
	if (fBounds.refs <= 9990) {
		char blah[256];
		sprintf(blah,"BRegion at %p has bad bounds ref count!",this);
		printf("%s", blah);
		debugger(blah);
		result = false;
	}
	if (fBounds.avail != 1) {
		char blah[256];
		sprintf(blah,"BRegion at %p has bad bounds avail count!",this);
		printf("%s", blah);
		debugger(blah);
		result = false;
	}
	if (fBounds.count > 1) {
		char blah[256];
		sprintf(blah,"BRegion at %p has bad bounds rect count!",this);
		printf("%s", blah);
		debugger(blah);
		result = false;
	}
	if (fData->refs <= 0) {
		char blah[256];
		sprintf(blah,"BRegion at %p has no data ref count!",this);
		printf("%s", blah);
		debugger(blah);
		result = false;
	}
	if (fData->count > fData->avail) {
		char blah[256];
		sprintf(blah,"BRegion at %p has data rect count > avail count!",this);
		printf("%s", blah);
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
				printf("rect %ld = (%f,%f)-(%f,%f), outside bounds (%f,%f)-(%f,%f)\n",
					i,Rects()[i].left,Rects()[i].top,Rects()[i].right,Rects()[i].bottom,
					Bounds().left,Bounds().top,Bounds().right,Bounds().bottom);
				sprintf(blah,"BRegion at %p is corrupt!",this);
				printf("%s", blah);
				debugger(blah);
				result = false;
			};
		};
		if ((left != Bounds().left) ||
			(right != Bounds().right) ||
			(top != Bounds().top) ||
			(bottom != Bounds().bottom)) {
			char blah[256];
			printf("real bounds: (%f,%f)-(%f,%f), stated bounds (%f,%f)-(%f,%f)\n",
				left,top,right,bottom,
				Bounds().left,Bounds().top,Bounds().right,Bounds().bottom);
			sprintf(blah,"BRegion at %p is corrupt!",this);
			printf("%s", blah);
			debugger(blah);
			result = false;
		};
	};

	return result;
};

#endif // DB_INTEGRITY_CHECKS

/*----------------------------------------------------------------*/

void BRegion::PrintToStream(ITextOutput::arg io, uint32 flags) const
{
	if (flags&B_PRINT_STREAM_HEADER) io << "BRegion";
	const int32 N = CountRects();
	
	io << "(";
	
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

/*----------------------------------------------------------------*/

status_t BRegion::printer(ITextOutput::arg io, const value_ref& val, uint32 flags)
{
	status_t result;
	BRegion obj(val, &result);
	if (result == B_OK) obj.PrintToStream(io, flags);
	return result;
}

/*----------------------------------------------------------------*/

void BMoveBefore(BRegion* to, BRegion* from, size_t count)
{
	memcpy(to, from, sizeof(BRegion)*count);
}

void BMoveAfter(BRegion* to, BRegion* from, size_t count)
{
	memmove(to, from, sizeof(BRegion)*count);
}

/*----------------------------------------------------------------*/

bool BRegion::IsFixedSize() const
{
	return false;
}

type_code BRegion::TypeCode() const
{
	return B_REGION_TYPE;
}

ssize_t BRegion::FlattenedSize() const
{
	return (CountRects()+1) * sizeof(BRect);
}

status_t BRegion::Flatten(void *buffer, ssize_t size) const
{
	const ssize_t N = CountRects();
	if (size >= static_cast<ssize_t>((N+1)*sizeof(BRect))) {
		*static_cast<BRect*>(buffer) = Bounds();
		if (N > 0) {
			memcpy(	static_cast<BRect*>(buffer)+1,
					Rects(),
					N*sizeof(BRect));
		}
		return B_OK;
	} else {
		return B_BAD_VALUE;
	}
}

bool BRegion::AllowsTypeCode(type_code code) const
{
	return code == B_REGION_TYPE;
}

status_t BRegion::Unflatten(type_code c, const void *buf, ssize_t size)
{
	if (c == B_REGION_TYPE) {
		if (size >= static_cast<ssize_t>(sizeof(BRect))) {
			const int32 count = ((size/sizeof(BRect))-1);
			BRect* data = CreateRects(count, count);
			if (data) {
				SetRectCount(count);
				BoundsRef() = *static_cast<const BRect*>(buf);
				if (count > 0) {
					memcpy(	data,
							static_cast<const BRect*>(buf)+1,
							count*sizeof(BRect));
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
		


ITextOutput::arg operator<<(ITextOutput::arg io, const BRegion& region)
{
	region.PrintToStream(io, B_PRINT_STREAM_HEADER);
	return io;
}

} }	// namespace B::Render2
