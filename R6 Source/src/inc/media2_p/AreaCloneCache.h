#ifndef _MEDIA2_AREACLONECACHE_PRIVATE_
#define _MEDIA2_AREACLONECACHE_PRIVATE_

#include <support2/KeyedVector.h>
#include <support2/Locker.h>
#include <OS.h>

namespace B {
namespace Private {

using namespace Support2;

class AreaCloneCache
{
public:
	static	AreaCloneCache *	Instance();
								AreaCloneCache();
								~AreaCloneCache();

			area_id				Clone(area_id remote, void ** outPtr = 0);
			status_t			Acquire(area_id remote);
			status_t			Release(area_id remote);

private:
	struct entry {
		entry() : local(-1), base(0), refs(0) {}
		area_id		local;
		void *		base;
		int32		refs;
	};
	typedef	BKeyedVector<area_id, entry> map_t;
			map_t				mMap;
			BLocker				mLock;
};

} } // B::Private
#endif // _MEDIA2_AREACLONECACHE_PRIVATE_
