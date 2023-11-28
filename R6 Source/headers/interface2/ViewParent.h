
#ifndef _INTERFACE2_VIEWPARENT_H_
#define _INTERFACE2_VIEWPARENT_H_

#include <support2/Binder.h>
#include <render2/RenderDefs.h>
#include <render2/2dTransform.h>
#include <support2/IInterface.h>
#include <interface2/InterfaceDefs.h>

namespace B {
namespace Render2 {
	class BUpdate;
}}

namespace B {
namespace Interface2 {

class IView;
class BLayoutConstraints;

/**************************************************************************************/

class IViewParent : public IInterface
{
	public:

		B_DECLARE_META_INTERFACE(ViewParent)

							enum {
								pretraversal = 	0x00000001,
								posttraversal =	0x00000002,
								constrain = 	0x00000004,
								layout = 		0x00000008
							};

		virtual	status_t	ConstrainChild(	const atom_ptr<IView> &child,
											const BLayoutConstraints &constraints) = 0;

		virtual	status_t	InvalidateChild(const atom_ptr<IView> &child,
											const BUpdate &update) = 0;

		virtual	void		MarkTraversalPath(int32 type) = 0;
};

/**************************************************************************************/

class LViewParent : public LInterface<IViewParent>
{
		virtual	status_t				Told(BValue &in);
};

/**************************************************************************************/

} } // namespace B::Interface2

#endif	/* _INTERFACE2_VIEWPARENT_H_ */
