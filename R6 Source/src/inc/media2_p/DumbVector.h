#ifndef _MEDIA2_DUMBVECTOR_PRIVATE_
#define _MEDIA2_DUMBVECTOR_PRIVATE_

#include <support2/Vector.h>

namespace B {
namespace Private {

class BDumbVector : private B::Support2::BAbstractVector
{
public:
							BDumbVector();
							BDumbVector(const BDumbVector & clone);
	virtual					~BDumbVector();
	
			BDumbVector &	operator=(const BDumbVector & clone);
			
			void			SetCapacity(size_t total_space);
			void			SetExtraCapacity(size_t extra_space);
			size_t			Capacity() const;
			
			size_t			CountItems() const;

	inline	const void *	operator[](size_t i) const { return ItemAt(i); }
			const void *	ItemAt(size_t i) const;
			void *			EditItemAt(size_t i);
			
			ssize_t			AddItem();
			ssize_t			AddItem(void * item);
			ssize_t			AddItemAt(size_t index);
			ssize_t			AddItemAt(void * item, size_t index);
			
			ssize_t			ReplaceItemAt(void * item, size_t index);
	
			ssize_t			AddVector(const BDumbVector& o);
			ssize_t			AddVectorAt(const BDumbVector& o, size_t index);
			
			void			MakeEmpty();
			void			RemoveItemsAt(size_t index, size_t count = 1);
	
			void			Swap(BDumbVector& o);
	
protected:
	virtual	void			PerformConstruct(void* base, size_t count) const;
	virtual	void			PerformCopy(void* to, const void* from, size_t count) const;
	virtual	void			PerformDestroy(void* base, size_t count) const;
	
	virtual	void			PerformMoveBefore(void* to, void* from, size_t count) const;
	virtual	void			PerformMoveAfter(void* to, void* from, size_t count) const;
	
	virtual	void			PerformAssign(void* to, const void* from, size_t count) const;
};

} } // B::Private
#endif //_MEDIA2_DUMBVECTOR_PRIVATE_
