
#ifndef _SUPPORT2_SUPPORTMISC_H_
#define _SUPPORT2_SUPPORTMISC_H_

#include <support2/SupportDefs.h>

#include <stdlib.h>

// optimization for gcc
#if defined(__GNUC__) && __GNUC__ < 3
#define DECLARE_RETURN(x) return x
#else
#define DECLARE_RETURN(x)
#endif

namespace B {
namespace Private {

using namespace B::Support2;
struct shared_buffer {
	mutable int32 users;
	size_t length;
	
	inline const void* data() const { return this+1; }
	inline void* data() { return this+1; }
	
	inline static shared_buffer* create(size_t length)
	{
		shared_buffer* sb =
			reinterpret_cast<shared_buffer*>(malloc(sizeof(shared_buffer) + length));
		if (sb) {
			sb->users = 1;
			sb->length = length;
		}
		return sb;
	}
	
	inline void inc_users() const { atomic_add(&users, 1); }
	inline void dec_users() const
	{
		if (atomic_add(&users, -1) == 1)
			free(const_cast<shared_buffer*>(this));
	}
	
	inline shared_buffer* edit(size_t newLength) const
	{
		shared_buffer* result;
		if (users <= 1) {
			if (length == newLength) result = const_cast<shared_buffer*>(this);
			else if ((result=reinterpret_cast<shared_buffer*>(
							realloc(const_cast<shared_buffer*>(this),
									sizeof(shared_buffer) + newLength))) != NULL) {
				result->length = newLength;
			}
		} else {
			result = create(newLength);
			if (result) {
				memcpy(result->data(), data(), newLength < length ? newLength : length);
				dec_users();
			}
		}
		return result;
	}
	
};

} }	// namespace B::Private

#endif	/* _SUPPORT2_BINDERIPC_H_ */
