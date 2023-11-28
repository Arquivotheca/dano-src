#ifndef __SET__
#define __SET__

// stl-like set
//	Copyright 1998, Be Incorporated

#include <SortedArray.h>
#include <SmallSortedArray.h>
#include <HashedArray.h>

template <class Key, int Size, class Container, class Compare = less<Key> >
class rt_set {

public:
	typedef Key key_type;
	typedef Key data_type;
	typedef Key value_type;
	typedef Compare key_compare;
	typedef Compare value_compare;

private:
	typedef Container rep_type;
	rep_type t;

public:
	typedef typename rep_type::reference reference;
	typedef typename rep_type::const_reference const_reference;
	typedef typename rep_type::iterator iterator;
	typedef typename rep_type::const_iterator const_iterator;
	typedef typename rep_type::size_type size_type;

	
	pair<iterator, bool> insert(const value_type& e)
		{
			pair<typename rep_type::iterator, bool> p = t.insert_unique(e); 
			return pair<iterator, bool>(p.first, p.second);
		}
	size_type erase(const key_type& e)
		{ return t.erase(e); }
	void erase(iterator i)
		{ t.erase(i); }

	iterator find(const key_type &e)
		{ return t.find(e); }
	const_iterator find(const key_type &e) const
		{ return t.find(e); }

	iterator lower_bound(const key_type &e)
		{ return t.lower_bound(e); }
	const_iterator lower_bound(const key_type &e) const
		{ return t.lower_bound(e); }
	iterator upper_bound(const key_type &e)
		{ return t.upper_bound(e); }
	const_iterator upper_bound(const key_type &e) const
		{ return t.upper_bound(e); }

	iterator begin()
		{ return t.begin (); }
	iterator end()
		{ return t.end(); }
	const_iterator begin() const
		{ return t.begin(); }
	const_iterator end() const
		{ return t.end(); }
	

	bool empty() const
		{ return t.empty(); }
	size_type size() const
		{ return t.size(); }
	size_type max_size() const
		{ return t.max_size (); }

	void clear()
		{ t.clear(); }
	
	void dump()
		{ t.dump(); }
};

// following glue needed to work around different flavors of less in
// Modena and standard STL
template <class Type>
struct _ident : public unary_function<Type, Type> {
	const Type &operator()(const Type &x) const
		{ return x; }
};

#endif
