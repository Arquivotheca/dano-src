
#include <content2/ContentList.h>

namespace B {
namespace Content2 {

BContentList::BContentList()
{
}

BContentList::~BContentList()
{
}

status_t 
BContentList::AddContent(const content &child, const BMessage &attr)
{
	BAutolock _auto1(m_listLock);
	m_childMap.AddItem(child.ptr(),m_children.AddItem(child));
	return B_OK;
}

status_t 
BContentList::RemoveContent(const content &child)
{
	BAutolock _auto1(m_listLock);

	const ssize_t i = m_childMap.IndexOf(child.ptr());
	if (i >= 0) {
		m_children.RemoveItemsAt(m_childMap.ValueAt(i));
		m_childMap.RemoveItemsAt(i);
		return B_OK;
	}
	
	return ENOENT;
}

} }	// namespace B::Content2
