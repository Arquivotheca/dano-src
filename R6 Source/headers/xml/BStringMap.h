
#ifndef _B_XML_STRINGMAP_H
#define _B_XML_STRINGMAP_H

#include <List.h>
#include <String.h>
#include <SupportDefs.h>


namespace B {
namespace XML {


// Forward References
// =====================================================================
class BElement;
class BXMLObject;


// Generic Set
// =====================================================================
template <class T>
class _Pointer_Set_
{
public:
				_Pointer_Set_()
					:_list()				{ };
				~_Pointer_Set_()			{ };
	void		AddItem(T item) 			{ _list.AddItem(item); };
	void		AddItem(T item, int32 i) 	{ _list.AddItem(item, i); };
	int32		CountItems() const 			{ return _list.CountItems(); };
	T			ItemAt(int32 index)			{ return (T) _list.ItemAt(index); };
	const T		ItemAt(int32 index) const	{ return (T) _list.ItemAt(index); };
	void		DeleteItems()				{	int32 c = CountItems();
												for (int32 i=0; i<c; ++i) delete ItemAt(i);
												_list.MakeEmpty();
										 	}
	void		Transfer(_Pointer_Set_<T> & to)
											{	int32 c = CountItems();
												for (int32 i=0; i<c; ++i) to.AddItem(ItemAt(i));
												_list.MakeEmpty();
										 	}
 	void		RemoveItem(T item)			{ _list.RemoveItem(item); }
 	T			RemoveItemAt(int32 index)	{ return (T) _list.RemoveItem(index); } 
 	void		MakeEmpty()					{ _list.MakeEmpty(); }
 	bool		FindItem(T item) const		{	int32 c = CountItems();
												for (int32 i=0; i<c; ++i)
													if (ItemAt(i) == item)
														return true;
												return false;
										 	}
private:
	BList _list;
};


// Some Sets of Pointers
// =====================================================================
typedef _Pointer_Set_<BElement *> ElementSet;
typedef _Pointer_Set_<BXMLObject *> XMLObjectSet;			// XXX should be fixed.
typedef _Pointer_Set_<BXMLObject *> BXMLObjectSet;
typedef _Pointer_Set_<BString *> _StringSet_;


// BMap
// =====================================================================
// Generic mapping of type K to T
template <class K, class T>
class BMap
{
public:
						BMap();
						BMap(const BMap & copy);
	BMap				& operator=(const BMap & copy);
	virtual				~BMap();
	
	void				Add(const K & key, const T & val);
	status_t			PairAt(int32 index, K & key, T & val) const;
	status_t			PairAt(int32 index, K ** key, T ** val);
	status_t			PairAt(int32 index, const K ** key, const T ** val) const;
	int32				FindIndex(const K & key) const;
	status_t			Find(const K & key, T & val) const;
	T					* Find(const K & key) const;
	int32				CountItems() const;
	
	void				MakeEmpty();
private:
	BList	_names;
	BList	_values;
};

// BStringMap
// =====================================================================
// BStringMap is distinct from BMap, because it has the AddAdopt function
class BStringMap
{
public:
						BStringMap();
						BStringMap(const BStringMap & copy);
	BStringMap			& operator=(const BStringMap & copy);
	virtual				~BStringMap();
	
	void				AddAdopt(BString & key, BString & val);
	void				Add(const BString & key, const BString & val);
	void				Add(const BString & key, int val);
	void				Add(const BString & key, unsigned int val);
	void				Add(const BString & key, uint32 val);
	void				Add(const BString & key, int32 val);
	void				Add(const BString & key, uint64 val);
	void				Add(const BString & key, int64 val);
	void 				Add(const BString & key, float val);
	
	status_t			PairAt(int32 index, BString & key, BString & val) const;
	status_t			PairAt(int32 index, const BString ** key, const BString ** val) const;
	status_t			PairAt(int32 index, BString ** key, BString ** val);
	int32				FindIndex(const BString & key) const;
	status_t			Find(const BString & key, BString & val) const;
	BString				* Find(const BString & key) const;
	int32				CountItems() const;
	
	void				MakeEmpty();
private:
	BList	_names;
	BList	_values;
};


// BStringSet
// =====================================================================
// BStringSet has slightly easier memory management from _PointerSet_ and
// and an AddAdopt function
class BStringSet
{
public:
						BStringSet();
						BStringSet(const BStringSet & copy);
	BStringSet			& operator=(const BStringSet & copy);
	virtual				~BStringSet();
	
	status_t			AddAdopt(BString & val);
	status_t			Add(const BString & val);
	status_t			Remove(const BString & val);
	bool				Exists(const BString & val) const;
	
	int32				CountItems() const;
	const BString		& ItemAt(int32 index) const;
	
	void				MakeEmpty();
	
private:
	BList	_values;
};


// Functions
// =====================================================================
// Functions that do some fun stuff to strings
bool SplitStringOnWhitespace(const BString & str, BString & split, int32 * pos);
void MushString(BString & str);
void StripWhitespace(BString & str);



}; // namespace XML
}; // namespace B



#endif // _B_XML_STRINGMAP_H
