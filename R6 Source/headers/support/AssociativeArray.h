
#ifndef _ASSOCIATIVEARRAY_H_
#define _ASSOCIATIVEARRAY_H_

#include <SmartArray.h>
#include <Gehnaphore.h>

template <class KEY, class VALUE>
class AssociativeArray
{
	protected:

		struct pair {
			KEY key; VALUE value;
			pair() {};
			pair(const pair &copyFrom) : key(copyFrom.key), value(copyFrom.value) {};
		};
		SmartArray<pair> 	m_list;
		NestedGehnaphore	m_lock;
	
		bool Find(const KEY &key, int32 &index) const
		{
			int32 mid, low = 0, high = m_list.CountItems()-1;
			while (low <= high) {
				mid = (low + high)/2;
				if (key == m_list[mid].key) {
					index = mid;
					return true;
				} else if (key < m_list[mid].key) {
					high = mid-1;
				} else
					low = mid+1;
			}
			
			index = low;
			return false;
		}
	
	public:
	
		AssociativeArray<KEY,VALUE> () { };

		AssociativeArray<KEY,VALUE> (AssociativeArray<KEY,VALUE> &copyFrom) {
			copyFrom.m_lock.Lock();
			m_list = copyFrom.m_list;
			copyFrom.m_lock.Unlock();
		}
	
		void Lock() { m_lock.Lock(); };
		void Unlock() { m_lock.Unlock(); };

		pair & operator[](int i) const
		{
			AssociativeArray<KEY,VALUE> *This = const_cast<AssociativeArray<KEY,VALUE>*>(this);
			NestedGehnaphore &g = This->m_lock;
			NestedGehnaphoreAutoLock _auto(g);
			return This->m_list[i];
		}
	
		VALUE Lookup(const KEY &key) const
		{
			int32 i;
			NestedGehnaphore &g = const_cast<AssociativeArray*>(this)->m_lock;
			NestedGehnaphoreAutoLock _auto(g);
			if (!Find(key,i)) return VALUE();
			return m_list[i].value;
		}

		bool Lookup(const KEY &key, VALUE &value) const
		{
			int32 i;
			NestedGehnaphore &g = const_cast<AssociativeArray*>(this)->m_lock;
			NestedGehnaphoreAutoLock _auto(g);
			if (!Find(key,i)) return false;
			value = m_list[i].value;
			return true;
		}

		void Insert(const KEY &key, const VALUE &value)
		{
			int32 i;
			m_lock.Lock();
			if (!Find(key,i)) m_list.InsertItem(i);
			m_list[i].key = key;
			m_list[i].value = value;
			m_lock.Unlock();
		}	

		void RemoveIndex(int32 i)
		{
			m_lock.Lock();
			m_list.RemoveItem(i);
			m_lock.Unlock();
		}	

		void Remove(const KEY &key)
		{
			int32 i;
			m_lock.Lock();
			if (Find(key,i)) m_list.RemoveItem(i);
			m_lock.Unlock();
		}	

		bool Exists(const KEY &key)
		{
			int32 i;
			m_lock.Lock();
			bool exists = Find(key,i);
			m_lock.Unlock();
			return exists;
		}

		void MakeEmpty()
		{
			m_lock.Lock();
			m_list.MakeEmpty();
			m_lock.Unlock();
		}

		int32 Count()
		{
			m_lock.Lock();
			int32 c = m_list.Count();
			m_lock.Unlock();
			return c;
		}

		status_t AssertSize(int32 c)
		{
			m_lock.Lock();
			m_list.AssertSize(c);
			m_lock.Unlock();
			return c;
		}
};

#endif
