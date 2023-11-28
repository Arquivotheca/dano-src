#ifndef _B_SUPPORT2_STACK_H
#define _B_SUPPORT2_STACK_H

#include <support2/Vector.h>
#include <support2/Debug.h>


namespace B {
namespace Support2 {


/*--------------------------------------------------------*/
/*----- BStack class -------------------------------------*/

template <class TYPE>
class BStack
{
public:

	size_t		CountItems() const;
	
	void		Push(TYPE & item);
	TYPE &		Top();
	void		Pop();
	
	void		MakeEmpty();
	
private:
	BVector<TYPE> m_vector;
};


/*-------------------------------------------------------------*/
/*---- No user serviceable parts after this -------------------*/

template<class TYPE> inline size_t
BStack<TYPE>::CountItems() const
{
	return m_vector.CountItems();
}

template<class TYPE> inline void
BStack<TYPE>::Push(TYPE & item)
{
	m_vector.AddItem(item);
}

template<class TYPE> inline TYPE &
BStack<TYPE>::Top()
{
	size_t count = m_vector.CountItems();
	if (count == 0) debugger("stack underflow");
	return m_vector.EditItemAt(count-1);
}

template<class TYPE> inline void
BStack<TYPE>::Pop()
{
	size_t count = m_vector.CountItems();
	if (count == 0) debugger("stack underflow");
	m_vector.RemoveItemsAt(count-1);
}

template<class TYPE> inline void
BStack<TYPE>::MakeEmpty()
{
	m_vector.MakeEmpty();
}

} }	// namespace B::Support2

#endif // _B_SUPPORT2_STACK_H
