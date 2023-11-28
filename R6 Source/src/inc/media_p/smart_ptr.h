/*	smart_ptr.h	*/

#if !defined(_SMART_PTR_H)
#define _SMART_PTR_H

template<class T>
class smart_ptr
{
		T * it;
public:
		inline smart_ptr() {
			it = new T;
		}
		~smart_ptr() {
			delete it;
		}
		T * detach() {
			T * ret = it;
			it = NULL;
			return ret;
		}
		bool operator!() {
			return it == NULL;
		}
		T & operator*() {
			return *it;
		}
		T * operator->() {
			return it;
		}
		T & operator[](int index) {
			if (index != 0) {
				return *(T*)NULL;
			}
			else {
				return *it;
			}
		}
		operator void*() {
			return it;
		}
private:
		smart_ptr(const smart_ptr<T> &);
		smart_ptr<T> & operator=(const smart_ptr<T> &);

};

#endif /* _SMART_PTR_H */
