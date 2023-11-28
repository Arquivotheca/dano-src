
#ifndef _INTERFACE2_VIEWRENDERROOT_H_
#define _INTERFACE2_VIEWRENDERROOT_H_

#include <InterfaceDefs.h>
#include <View.h>

namespace B {
namespace Interface2 {

/**************************************************************************************/

class BViewRenderRoot : public BViewLayoutRoot
{
	public:

								BViewRenderRoot(const BMessage &attr);
		virtual					~BViewRenderRoot();

		virtual	status_t		HandleMessage(BMessage *message);
		virtual	void			Draw(BRect bounds, const render &into);
		virtual	void			NeedDraw(const BRegion &dirty);

	private:

		BHostedSurface *		m_surface;
};

/**************************************************************************************/

} } // namespace B::Interface2

using namespace B::Interface2;

#endif	/* _INTERFACE2_VIEWRENDERROOT_H_ */
