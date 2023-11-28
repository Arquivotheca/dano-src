
#ifndef _CONTENT2_CONTENTVIEW_H_
#define _CONTENT2_CONTENTVIEW_H_

#include <interface2/View.h>
#include <content2/ContentDefs.h>

namespace B {
namespace Render2 {
	class BUpdate;
}}

namespace B {
namespace Content2 {

/**************************************************************************************/

class BContentView : public BView
{
	public:

										BContentView(
											const IContent::ptr &theContent,
											const BMessage &attr = BMessage());
										BContentView(const BContentView &copyFrom);
		virtual							~BContentView();

		virtual	BView *					Copy();

		virtual	IView::ptr				PostTraversal(BUpdate &outDirty);

		virtual	void 					Constrain(const BLayoutConstraints &internalConstraints);
		virtual	void					Invalidate(const BRegion &dirty = BRegion::full);
		virtual	void					Draw(const IRender::ptr& into);

		virtual	BLayoutConstraints		Constraints() const;
		virtual	IContent::ptr			Content() const;

		virtual	void					DispatchEvent(const BMessage &msg, const BPoint& where, event_dispatch_result *result = NULL));
		virtual	event_dispatch_result	MouseMoved(const BMessage &msg, const BPoint& where, uint32 transit, const BMessage &drag);
		virtual	event_dispatch_result	MouseDown(const BMessage &msg, const BPoint& where, int32 buttons);
		virtual	event_dispatch_result	MouseUp(const BMessage &msg, const BPoint& where, int32 buttons);

	protected:
	
		virtual	status_t				Acquired(const void* id);
		virtual	status_t				Released(const void* id);
		
	private:

				BLayoutConstraints		m_aggregateConstraints;
				IContent::ptr			m_content;
};

/**************************************************************************************/

} } // namespace B::Content2

#endif	/* _CONTENT2_CONTENTVIEW_H_ */
