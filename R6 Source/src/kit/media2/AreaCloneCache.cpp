#include "AreaCloneCache.h"
#include <support2/Autolock.h>

using B::Support2::BAutolock;

#include <support2/StdIO.h>

#define checkpoint \
//berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

namespace B {
namespace Private {

static	BLocker				gInstanceLock;
static	AreaCloneCache *	gInstance = 0;

AreaCloneCache *
AreaCloneCache::Instance()
{
	BAutolock _l(gInstanceLock.Lock());
	if (!gInstance) gInstance = new AreaCloneCache;
	return gInstance;
}

AreaCloneCache::AreaCloneCache()
{
}

AreaCloneCache::~AreaCloneCache()
{
	for (int32 n = mMap.CountItems()-1; n >= 0; n--)
	{
		const entry & e = mMap.ValueAt(n);
		delete_area(e.local);
	}
}

area_id 
AreaCloneCache::Clone(area_id remote, void **outPtr)
{
checkpoint
	if (remote < 0) return remote;
	BAutolock _l(mLock.Lock());
	bool found;
	entry & e = mMap.EditValueFor(remote, &found);
	if (found)
	{
		++e.refs;
		if (outPtr) *outPtr = e.base;
		return e.local;
	}
	else
	{
		entry ne;
		area_id local = clone_area("AreaCloneCache", &ne.base, B_ANY_ADDRESS,
			B_READ_AREA | B_WRITE_AREA, remote);
		if (local < 0) return local;
		if (outPtr) *outPtr = ne.base;
		ne.local = local;
		ne.refs = 1;
		mMap.AddItem(remote, ne);
		return local;
	}	
}

status_t 
AreaCloneCache::Acquire(area_id remote)
{
checkpoint
	if (remote < 0) return remote;
	BAutolock _l(mLock.Lock());
	bool found;
	entry & e = mMap.EditValueFor(remote, &found);
	if (!found) return B_BAD_VALUE;
	++e.refs;
	return B_OK;
}


status_t 
AreaCloneCache::Release(area_id remote)
{
checkpoint
	if (remote < 0) return remote;
	BAutolock _l(mLock.Lock());
	size_t index;
	if (!mMap.GetIndexOf(remote, &index)) return B_BAD_VALUE;
	entry & e = mMap.EditValueAt(index);
	if (--e.refs <= 0)
	{
checkpoint
		delete_area(e.local);
		mMap.RemoveItemsAt(index);
	}
	return B_OK;
}

} } // B::Private
