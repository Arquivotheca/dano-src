
#ifndef _SUPPORT2_LIST_H_
#define _SUPPORT2_LIST_H_

#include <support2/Vector.h>
#include <support2/KeyedVector.h>
#include <support2/Binder.h>
#include <support2/Locker.h>
#include <support2/Autolock.h>

namespace B {
namespace Support2 {

/**************************************************************************************/

template <class CHILD_TYPE>
class IBinderVector : virtual public BAtom
{
	public:
		static	const BValue					descriptor;
		static	atom_ptr< IBinderVector<CHILD_TYPE> >	AsInterface(const IBinder::ptr &o);

		virtual	status_t						AddChild(const atom_ptr<CHILD_TYPE> &child, const BValue &attr = BValue::undefined) = 0;
		virtual	status_t						RemoveChild(const atom_ptr<CHILD_TYPE> &child) = 0;
		virtual	atom_ptr<CHILD_TYPE>			ChildAt(int32 index) const = 0;
		virtual	atom_ptr<CHILD_TYPE>			ChildAt(const char *name) const = 0;
		virtual	int32							IndexOf(const atom_ptr<CHILD_TYPE> &child) const = 0;
		virtual	BString							NameOf(const atom_ptr<CHILD_TYPE> &child) const = 0;
		virtual	int32							Count() const = 0;
};

/**************************************************************************************/

template <class CHILD_TYPE>
inline atom_ptr< IBinderVector<CHILD_TYPE> > 
IBinderVector<CHILD_TYPE>::AsInterface(const IBinder::ptr &o)
{
	return dynamic_cast<IBinderVector<CHILD_TYPE>*>(o->Get(descriptor).AsAtom().ptr());
}

/**************************************************************************************/

template <class CHILD_TYPE>
class LBinderVector : public LInterface< IBinderVector<CHILD_TYPE> >
{
		virtual	status_t						Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out);
		virtual	status_t						Told(BValue &in);
		virtual	status_t						Asked(const BValue &outBindings, BValue &out);
};

/**************************************************************************************/

template <class CHILD_TYPE>
class RBinderVector : public RInterface< IBinderVector<CHILD_TYPE> >
{
	public:
												RBinderVector(IBinder::arg remote) : RInterface< IBinderVector<CHILD_TYPE> >(remote) {};
													
		virtual	status_t						AddChild(const atom_ptr<CHILD_TYPE> &child, const BValue &attr = BValue::undefined);
		virtual	status_t						RemoveChild(const atom_ptr<CHILD_TYPE> &child);
		virtual	atom_ptr<CHILD_TYPE>			ChildAt(int32 index) const;
		virtual	atom_ptr<CHILD_TYPE>			ChildAt(const char *name) const;
		virtual	int32							IndexOf(const atom_ptr<CHILD_TYPE> &child) const;
		virtual	BString							NameOf(const atom_ptr<CHILD_TYPE> &child) const;
		virtual	int32							Count() const;
};

/**************************************************************************************/

template <class CHILD_TYPE>
class BBinderVector : public LBinderVector<CHILD_TYPE>
{
	public:
												BBinderVector();
												BBinderVector(const BBinderVector<CHILD_TYPE>& o);
												
		virtual	status_t						AddChild(const atom_ptr<CHILD_TYPE> &child, const BValue &attr = BValue::undefined);
		virtual	status_t						RemoveChild(const atom_ptr<CHILD_TYPE> &child);
		virtual	atom_ptr<CHILD_TYPE>			ChildAt(int32 index) const;
		virtual	atom_ptr<CHILD_TYPE>			ChildAt(const char *name) const;
		virtual	int32							IndexOf(const atom_ptr<CHILD_TYPE> &child) const;
		virtual	BString							NameOf(const atom_ptr<CHILD_TYPE> &child) const;
		virtual	int32							Count() const;

	protected:
		virtual									~BBinderVector();
		
				status_t						_AddChild(const atom_ptr<CHILD_TYPE> &child, const BValue &attr);
				status_t						_RemoveChild(const atom_ptr<CHILD_TYPE> &child);
				status_t						_ReplaceChild(int32 i, const atom_ptr<CHILD_TYPE> &child);
				atom_ptr<CHILD_TYPE>			_ChildAt(int32 index) const;
				atom_ptr<CHILD_TYPE>			_ChildAt(const char *name) const;
				int32							_IndexOf(const atom_ptr<CHILD_TYPE> &child) const;
				BString							_NameOf(const atom_ptr<CHILD_TYPE> &child) const;
				int32							_Count() const;

		virtual	status_t						Acquired(const void* id);
		virtual	status_t						Released(const void* id);

	private:


				typedef BKeyedVector<CHILD_TYPE*,int32> index_map;

				struct list_entry {
					list_entry() {};
					list_entry(const BString &_name, const atom_ptr<CHILD_TYPE> &_child) : name(_name), child(_child) {};

					BString					name;
					atom_ptr<CHILD_TYPE>	child;
				};
	
		mutable	BLocker					m_listLock;
				BVector<list_entry>		m_list;
				index_map				m_listMap;
	
	public:
		// gross hack
				void						_instantiate_all_the_list();
};

/**************************************************************************************/

} } // namespace B::Support2

#endif	/* _SUPPORT2_LIST_H_ */
