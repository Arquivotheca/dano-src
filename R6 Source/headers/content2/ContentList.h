
#ifndef _CONTENT2_CONTENTLIST_H_
#define _CONTENT2_CONTENTLIST_H_

#include <support2/Binder.h>
#include <content2/ContentDefs.h>
#include <interface2/InterfaceDefs.h>

namespace B {
namespace Content2 {

/**************************************************************************************/

typedef BAssociativeVector<IContent*,int32> BContentIndexMap;

/**************************************************************************************/

class IContentList : virtual public BAtom
{
	public:
	
		static	property_id				Interface() { static property_id id; if (!id) id = "IContentList"; return id; }
		static	atom_ptr<IContentList>	AsInterface(const binder &o);
		virtual	binder					AsBinder() = 0;

		virtual	status_t				AddContent(const content &child, const BMessage &attr) = 0;
		virtual	status_t				RemoveContent(const content &child) = 0;
};

/**************************************************************************************/

class LContentList : public IContentList, virtual public BBinder { public: BINDER_CALLS; binder AsBinder(); };

/**************************************************************************************/

class BContentList : public LContentList
{
	public:
	
										BContentList();
		virtual							~BContentList();

		virtual	status_t				AddContent(const content &child, const BMessage &attr);
		virtual	status_t				RemoveContent(const content &child);

	private:

				BLocker					m_listLock;
				BVector<content>		m_children;
				BContentIndexMap		m_childMap;
};

/**************************************************************************************/

} } // namespace B::Content2

#endif	/* _INTERFACE2_CONTENTLIST_H_ */
