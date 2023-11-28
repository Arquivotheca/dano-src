
/*	This file implements an IBinderVector interface specialization. */

#include <support2/Binder.h>

namespace B {
namespace Support2 {

static const BValue g_keyAddChild		("b:addbrat");
static const BValue g_keyRemoveChild	("b:removebrat");
static const BValue g_keyChildAtIndex	("b:childatindex");
static const BValue g_keyChildAtName	("b:childatname");
static const BValue g_keyIndexOf		("b:indexof");
static const BValue g_keyNameOf			("b:nameofbrat");
static const BValue g_keyCount			("b:countbrats");

} } // namespace B::Support2



#define COMBINE_TOKENS(a,b) a##b

/**************************************************************************************/

#if IMPLEMENT_INTERFACE

const BValue IBinderVector<CHILD_TYPE>::descriptor(BValue::TypeInfo(THIS_TYPE));

#endif /* IMPLEMENT_INTERFACE */

/**************************************************************************************/

#if IMPLEMENT_REMOTE

template<class CHILD_TYPE>
status_t RBinderVector<CHILD_TYPE>::AddChild(const atom_ptr<CHILD_TYPE> &child, const BValue &attr)
{
	return Remote()->Put(BValue(g_keyAddChild, child));
}

template<class CHILD_TYPE>
status_t RBinderVector<CHILD_TYPE>::RemoveChild(const atom_ptr<CHILD_TYPE> &child)
{
	return Remote()->Put(BValue(g_keyRemoveChild, child));
}

template<class CHILD_TYPE>
atom_ptr<CHILD_TYPE> RBinderVector<CHILD_TYPE>::ChildAt(int32 index) const
{
	return CHILD_TYPE::AsInterface(Remote()->Invoke(BValue::Int32(index), g_keyChildAtIndex));
}

template<class CHILD_TYPE>
atom_ptr<CHILD_TYPE> RBinderVector<CHILD_TYPE>::ChildAt(const char *name) const
{
	return CHILD_TYPE::AsInterface(Remote()->Invoke(name, g_keyChildAtName));
}

template<class CHILD_TYPE>
int32 RBinderVector<CHILD_TYPE>::IndexOf(const atom_ptr<CHILD_TYPE> &child) const
{
	return Remote()->Invoke(child, g_keyIndexOf).AsInteger();
}

template<class CHILD_TYPE>
BString RBinderVector<CHILD_TYPE>::NameOf(const atom_ptr<CHILD_TYPE> &child) const
{
	return Remote()->Invoke(child, g_keyNameOf).AsString();
}

template<class CHILD_TYPE>
int32 RBinderVector<CHILD_TYPE>::Count() const
{
	return Remote()->Get(BValue(g_keyCount)).AsInteger();
}

#endif /* IMPLEMENT_REMOTE */

/**************************************************************************************/

#if IMPLEMENT_CONCRETE

template<class CHILD_TYPE>
status_t LBinderVector<CHILD_TYPE>::Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out)
{
	if (out) {
		BValue val;
		if ((val = in[g_keyNameOf])) {
			
			atom_ptr<CHILD_TYPE> child = CHILD_TYPE::AsInterface(val);
			
			if (child != NULL)
				(*out) += outBindings * BValue(g_keyNameOf, NameOf(child));
		
		} else if ((val = in[g_keyChildAtIndex])) {
			
			(*out) += outBindings * BValue(g_keyChildAtIndex, ChildAt(val.AsInteger())->AsBinder());
			
		} else if ((val = in[g_keyChildAtName])) {
			
			(*out) += outBindings * BValue(g_keyChildAtName, ChildAt(val.AsString())->AsBinder());
	
		} else if ((val = in[g_keyIndexOf])) {
			
			atom_ptr<CHILD_TYPE> child = CHILD_TYPE::AsInterface(val);
			
			if (child != NULL)
				(*out) += outBindings * BValue(g_keyIndexOf, NameOf(child));
		}
	}
	
	return BBinder::Effect(in,inBindings,outBindings,out);
}

template<class CHILD_TYPE>
status_t LBinderVector<CHILD_TYPE>::Told(BValue &map)
{	
	status_t status = B_UNSUPPORTED;
	
	BValue val;
	
	if ((val = map[g_keyAddChild])) {
		atom_ptr<CHILD_TYPE> child = CHILD_TYPE::AsInterface(val);
		status = AddChild(child);
	}
	
	if ((val = map[g_keyRemoveChild])) {
		atom_ptr<CHILD_TYPE> child = CHILD_TYPE::AsInterface(val);
		status = AddChild(child);
	}
	
	return status;
}

template<class CHILD_TYPE>
status_t LBinderVector<CHILD_TYPE>::Asked(const BValue &outBindings, BValue &out)
{
	out += outBindings * BValue().Overlay(g_keyCount, BValue::Int32(Count()));
	return B_OK;
}

#endif /* IMPLEMENT_CONCRETE */

#if IMPLEMENT_BASE

/**************************************************************************************/

template<class CHILD_TYPE>
status_t BBinderVector<CHILD_TYPE>::_AddChild(const atom_ptr<CHILD_TYPE> &child, const BValue &attr)
{
	m_listMap.AddItem(child.ptr(),m_list.AddItem(list_entry(attr["name"].AsString(),child)));
	return B_OK;
}

template<class CHILD_TYPE>
status_t BBinderVector<CHILD_TYPE>::_RemoveChild(const atom_ptr<CHILD_TYPE> &child)
{
	const ssize_t i = m_listMap.IndexOf(child.ptr());
	if (i >= 0) {
		m_list.RemoveItemsAt(m_listMap.ValueAt(i));
		m_listMap.RemoveItemsAt(i);
		return B_OK;
	}
	
	return ENOENT;
}

template<class CHILD_TYPE>
status_t BBinderVector<CHILD_TYPE>::_ReplaceChild(int32 i, const atom_ptr<CHILD_TYPE> &child)
{
	m_listMap.RemoveItemFor(m_list[i].child.ptr());
	m_list.EditItemAt(i).child = child;
	m_listMap.AddItem(child.ptr(),i);
	return B_OK;
}

template<class CHILD_TYPE>
atom_ptr<CHILD_TYPE> BBinderVector<CHILD_TYPE>::_ChildAt(int32 index) const
{
	if ((index < 0) && (index >= static_cast<int32>(m_list.CountItems()))) return NULL;
	return m_list[index].child;
}

template<class CHILD_TYPE>
atom_ptr<CHILD_TYPE> BBinderVector<CHILD_TYPE>::_ChildAt(const char *name) const
{
	for (size_t i=0;i<m_list.CountItems();i++) {
		if (m_list[i].name == name) return m_list[i].child;
	}

	return NULL;
}

template<class CHILD_TYPE>
int32 BBinderVector<CHILD_TYPE>::_IndexOf(const atom_ptr<CHILD_TYPE> &child) const
{
	const ssize_t index = m_listMap.IndexOf(child.ptr());
	return index >= B_OK ? index : -1;
}

template<class CHILD_TYPE>
BString BBinderVector<CHILD_TYPE>::_NameOf(const atom_ptr<CHILD_TYPE> &child) const
{
	const ssize_t index = m_listMap.IndexOf(child.ptr());
	return index >= B_OK ? m_list[index].name : BString();
}

template<class CHILD_TYPE>
int32 BBinderVector<CHILD_TYPE>::_Count() const
{
	return m_list.CountItems();
}

/**************************************************************************************/

template<class CHILD_TYPE>
BBinderVector<CHILD_TYPE>::BBinderVector()
{
}

template<class CHILD_TYPE>
BBinderVector<CHILD_TYPE>::BBinderVector(const BBinderVector<CHILD_TYPE>& o)
	:	LBinderVector<CHILD_TYPE>(),
		m_list(o.m_list), m_listMap(o.m_listMap)
{
}

template<class CHILD_TYPE>
BBinderVector<CHILD_TYPE>::~BBinderVector()
{
}

template<class CHILD_TYPE>
status_t BBinderVector<CHILD_TYPE>::Acquired(const void* id)
{
	return LBinderVector<CHILD_TYPE>::Acquired(id);
}

template<class CHILD_TYPE>
status_t BBinderVector<CHILD_TYPE>::Released(const void* id)
{
	m_list.MakeEmpty();
	m_listMap.MakeEmpty();
	return LBinderVector<CHILD_TYPE>::Released(id);
}

template<class CHILD_TYPE>
status_t BBinderVector<CHILD_TYPE>::AddChild(const atom_ptr<CHILD_TYPE> &child, const BValue &attr)
{
	BAutolock _auto(m_listLock.Lock());
	return _AddChild(child,attr);
}

template<class CHILD_TYPE>
status_t BBinderVector<CHILD_TYPE>::RemoveChild(const atom_ptr<CHILD_TYPE> &child)
{
	BAutolock _auto(m_listLock.Lock());
	return _RemoveChild(child);
}

template<class CHILD_TYPE>
atom_ptr<CHILD_TYPE> BBinderVector<CHILD_TYPE>::ChildAt(int32 index) const
{
	BAutolock _auto(m_listLock.Lock());
	return _ChildAt(index);
}

template<class CHILD_TYPE>
atom_ptr<CHILD_TYPE> BBinderVector<CHILD_TYPE>::ChildAt(const char *name) const
{
	BAutolock _auto(m_listLock.Lock());
	return _ChildAt(name);
}

template<class CHILD_TYPE>
int32 BBinderVector<CHILD_TYPE>::IndexOf(const atom_ptr<CHILD_TYPE> &child) const
{
	BAutolock _auto(m_listLock.Lock());
	return _IndexOf(child);
}

template<class CHILD_TYPE>
BString BBinderVector<CHILD_TYPE>::NameOf(const atom_ptr<CHILD_TYPE> &child) const
{
	BAutolock _auto(m_listLock.Lock());
	return _NameOf(child);
}

template<class CHILD_TYPE>
int32 BBinderVector<CHILD_TYPE>::Count() const
{
	return _Count();
}

template<class CHILD_TYPE>
void BBinderVector<CHILD_TYPE>::_instantiate_all_the_list()
{
	_ReplaceChild(0, _ChildAt(1));
}

static BBinderVector<CHILD_TYPE>* instantiate_template()
{
	return new BBinderVector<CHILD_TYPE>();
}

static BBinderVector<CHILD_TYPE>* instantiate_template2(BBinderVector<CHILD_TYPE>& o)
{
	o._instantiate_all_the_list();
	return new BBinderVector<CHILD_TYPE>(o);
}

/**************************************************************************************/
#endif /* IMPLEMENT_BASE */

