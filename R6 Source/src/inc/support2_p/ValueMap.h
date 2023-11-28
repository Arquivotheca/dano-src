/******************************************************************************
/
/	File:			ValueMap.h
/
/	Description:	A container of arbitrary BValue->BValue mappings.
/
/	Copyright 2001, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _SUPPORT2_VALUEMAP_H
#define _SUPPORT2_VALUEMAP_H

#include <support2/IByteStream.h>
#include <support2/Value.h>
#include <support2/Vector.h>

#include <support2_p/ValueMapFormat.h>

namespace std {
	struct nothrow_t;
}

namespace B {
namespace Support2 {

class BParcel;

// This is a comparison between values with two nice properties:
// * It is significantly faster than the standard BValue comparison,
//   as it doesn't try to put things in a "nice" order.  In practice,
//   the speedup is about 2x.
// * It has a clearly defined order, where-as the normal ordering of
//   BValues will change whenever we add special cases for other types.
//   This allows us to proclaim the following algorithm as the offical
//   order for items in flattened data.
inline int32 fast_compare(const value_ref& v1, const value_ref& v2)
{
	if (v1.type != v2.type)
		return v1.type < v2.type ? -1 : 1;
	if (v1.length != v2.length)
		return v1.length < v2.length ? -1 : 1;
	// Special case compare to put integers in a nice order.
	if (v1.length == 4) {
		return (*reinterpret_cast<const int32*>(v1.data))
				!= (*reinterpret_cast<const int32*>(v2.data))
			? ( (*reinterpret_cast<const int32*>(v1.data))
					< (*reinterpret_cast<const int32*>(v2.data))
				? -1 : 1
				)
			: 0;
	}
	return memcmp(v1.data, v2.data, v1.length);
}

inline int32 CompareMap(const BValue& key1, const BValue& value1,
						const BValue& key2, const BValue& value2)
{
	int32 c;
	if ((c=fast_compare(value_ref(key1),value_ref(key2))) == 0 && key1.IsNull())
		c = fast_compare(value_ref(value1), value_ref(value2));
	return c;
}

class BValueMap
{
public:
							BValueMap();
							BValueMap(const BValueMap& o);
	
			struct pair {
				BValue key;
				BValue value;
				
				inline pair() { }
				inline pair(const BValue& k, const BValue& v)	:	key(k), value(v) { }
				inline pair(const pair& o)						:	key(o.key), value(o.value) { }
				inline ~pair() { }
				
				inline pair& operator=(const pair& o) { key = o.key; value = o.value; return *this; }
				
				int32 compare(const pair& o) const {
					int32 c;
					if ((c=fast_compare(value_ref(key), value_ref(o.key))) == 0 && key.IsNull())
						c = fast_compare(value_ref(value),value_ref(o.value));
					return c;
				}
				int32 compare(const value_ref& k, const value_ref& v) const {
					int32 c;
					if ((c=fast_compare(value_ref(key),k)) == 0 && k == value_ref::null)
						c = fast_compare(value_ref(value),v);
					return c;
				}
			};
			
			void			IncUsers() const;
			void			DecUsers() const;
			bool			IsShared() const;
	
			ssize_t			FlattenedSize() const;
			ssize_t			Flatten(void *buffer, ssize_t size, BParcel* offsets) const;
			ssize_t			Flatten(IByteOutput::arg stream, BParcel* offsets) const;
	
			ssize_t			Unflatten(const void *buffer, size_t avail);
			ssize_t			Unflatten(IByteInput::arg stream);
	
			status_t		SetFirstMap(const BValue& key, const BValue& value);
			ssize_t			OverlayMap(const BValue& key, const BValue& value);
			ssize_t			OverlayMap(const value_ref& key, const BValue& value);
			ssize_t			InheritMap(const BValue& key, const BValue& value);
			ssize_t			InheritMap(const value_ref& key, const BValue& value);
			ssize_t			InheritMap(const value_ref& key, const value_ref& value);
			status_t		RemoveMap(	const value_ref& key,
										const value_ref& value = value_ref());
			status_t		RenameMap(	const value_ref& old_key,
										const BValue& new_key);
			ssize_t			IndexFor(	const value_ref& key,
										const value_ref& value = value_ref()) const;
			const pair&		MapAt(size_t index) const;
			BValue*			BeginEditMapAt(size_t index);
			void			EndEditMapAt();
			void			RemoveMapAt(size_t index);
			bool			IsEditing() const;
			
			size_t			CountMaps() const;
			int32			Compare(const BValueMap& o) const;
			int32			FastCompare(const BValueMap& o) const;
			
			void 			AssertEditing() const;
			
private:
							~BValueMap();
			
			bool			GetIndexOf(	const value_ref& k,
										const value_ref& v, size_t* index) const;
			bool			GetIndexOf(const pair& o, size_t* index) const;

			B_IMPLEMENT_SIMPLE_TYPE_FUNCS(pair);
	
	mutable	int32			m_users;
			size_t			m_dataSize;
			BVector<pair>	m_map;
			int32			m_editIndex;
			size_t			m_editSize;
};

inline BValueMap::BValueMap()
	:	m_users(1), m_dataSize(0), m_editIndex(-1)
{
}

inline BValueMap::BValueMap(const BValueMap& o)
	:	m_users(1),
		m_dataSize(o.m_dataSize), m_map(o.m_map),
		m_editIndex(-1)
{
}
			
inline BValueMap::~BValueMap()
{
}

inline void BValueMap::IncUsers() const
{
	AssertEditing();
	
	atomic_add(&m_users, 1);
}

inline void BValueMap::DecUsers() const
{
	AssertEditing();
	
	if (atomic_add(&m_users, -1) == 1)
		delete const_cast<BValueMap*>(this);
}

inline bool BValueMap::IsShared() const
{
	return m_users > 1;
}

inline ssize_t BValueMap::IndexFor(const value_ref& key, const value_ref& value) const
{
	size_t index;
	return GetIndexOf(key, value, &index) ? index : B_NAME_NOT_FOUND;
}

inline const BValueMap::pair& BValueMap::MapAt(size_t index) const
{
	return m_map.ItemAt(index);
}

inline void BValueMap::RemoveMapAt(size_t index)
{
	const pair& p = m_map.ItemAt(index);
	m_dataSize -= chunk_align(p.key.Length()) + chunk_align(p.value.Length());
	m_map.RemoveItemsAt(index);
}

inline size_t BValueMap::CountMaps() const
{
	return m_map.CountItems();
}

inline bool	BValueMap::IsEditing() const
{
	return m_editIndex >= 0;
}

inline void BValueMap::AssertEditing() const
{
	if (IsEditing()) {
		debugger("This operation can not be performed while editing a value");
	}
}

} }	// namespace B::Support2

#endif
