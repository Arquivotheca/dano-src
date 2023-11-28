
#ifndef _SUPPORT2_HASHTABLE_H_
#define _SUPPORT2_HASHTABLE_H_

#include <support2/SupportDefs.h>
#include <support2/KeyedVector.h>

namespace B {
namespace Support2 {

/**************************************************************************************/

class BHasher
{
	public:
	
					BHasher(int32 bits);
		uint32		BaseHash(const void *data, int32 len) const;

	private:

		int32		m_bits;
		uint32		m_crcxor[256];
};

template <class KEY, class VALUE>
class BHashTable : public BHasher
{
	public:
	
		BHashTable(int32 bits);
		~BHashTable();

		uint32 Hash(const KEY &key) const;

		const VALUE& Lookup(const KEY &key) const
		{
			return m_table[Hash(key)].ValueFor(key);
		}

		bool Lookup(const KEY &key, VALUE &value) const
		{
			bool found;
			value = m_table[Hash(key)].ValueFor(key, &found);
			return found;
		}

		void Insert(const KEY &key, const VALUE &value)
		{
			m_table[Hash(key)].AddItem(key,value);
		}	

		void Remove(const KEY &key)
		{
			m_table[Hash(key)].RemoveItemFor(key);
		}	

	private:

		BKeyedVector<KEY,VALUE> *	m_table;
};

template<class KEY, class VALUE>
BHashTable<KEY, VALUE>::BHashTable(int32 bits) : BHasher(bits)
{
	m_table = new BKeyedVector<KEY,VALUE> [1<<bits];
}

template<class KEY, class VALUE>
BHashTable<KEY, VALUE>::~BHashTable()
{
}


/**************************************************************************************/

} } // namespace B::Support2

#endif	/* _SUPPORT2_HASHTABLE_H_ */
