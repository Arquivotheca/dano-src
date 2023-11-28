
#ifndef _CONTENT2_CONTENT_H_
#define _CONTENT2_CONTENT_H_

#include <support2/Binder.h>
#include <interface2/InterfaceDefs.h>
#include <interface2/View.h>

namespace B {
namespace Support2 {
class BString;
}}

namespace B {
namespace Content2 {

using namespace Support2;
using namespace Interface2;

/**************************************************************************************/

class IContent : public IInterface
{
	public:
	
		B_DECLARE_META_INTERFACE(Content)

		virtual	IContent::ptr			Parent() const = 0;
		virtual	status_t				SetParent(const IContent::ptr& parent) = 0;
		virtual	IView::ptr				CreateView(const BMessage &attr) = 0;
		virtual void					DispatchEvent(const BMessage &msg, const IView::ptr& view = NULL) = 0;
};

/**************************************************************************************/

class LContent : public LInterface<IContent>
{
	public:
		virtual	status_t				Told(value &in);
		virtual	status_t				Asked(const value &outBindings, value &out);
};

/**************************************************************************************/

class BContent : public LContent
{
	public:
		virtual	IContent::ptr	Parent() const;
		virtual	status_t		SetParent(const IContent::ptr &parent);
		virtual void			DispatchEvent(const BMessage &msg, const IView::ptr& view = NULL);
		virtual void			WheelMoved(const BMessage &msg, coord xDelta, coord yDelta);
		virtual	void			KeyDown(const BMessage &msg, const char *bytes, int32 numBytes);
		virtual	void			KeyUp(const BMessage &msg, const char *bytes, int32 numBytes);

	private:
		void do_key_up(const BMessage &, const IView::ptr&);
		void do_key_dn(const BMessage &, const IView::ptr&);

		IContent::ref			m_parent;
};

/**************************************************************************************/

} } // namespace B::Content2

#endif	/* _INTERFACE2_CONTENT_H_ */
