
#ifndef _INTERFACE2_VIEWLIST_H_
#define _INTERFACE2_VIEWLIST_H_

#include <interface2/InterfaceDefs.h>
#include <support2/List.h>
#include <support2/String.h>

namespace B {
namespace Interface2 {

/**************************************************************************************/

typedef IBinderVector<IView> IViewList;
typedef LBinderVector<IView> LViewList;
typedef BBinderVector<IView> BViewList;

/**************************************************************************************/

} } // namespace B::Interface2

#endif	/* _INTERFACE2_VIEWLIST_H_ */
