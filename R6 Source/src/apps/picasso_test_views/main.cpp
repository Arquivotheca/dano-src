#include <stdlib.h>
#include <math.h>

#include <render2/Color.h>
#include <support2/Looper.h>
#include <support2/MessageCodes.h>
#include <support2/StdIO.h>
#include <storage2/Path.h>
#include <render2/Render.h>
#include <render2/Region.h>
#include <render2/Update.h>
#include <interface2/InterfaceEventsDefs.h>
#include <interface2/SurfaceManager.h>
#include <interface2/View.h>
#include <interface2/ViewGroup.h>
#include <interface2/ViewHVGroup.h>
#include <interface2/ViewLayoutRoot.h>

using namespace B::Support2;
using namespace B::Render2;
using namespace B::Interface2;
using namespace B::Storage2;
using namespace B::Raster2;


// ********************************************************************

class LeafView : public BView
{
		BColor 	m_color;
		BPoint	m_last_point;
		bool	m_mouse_down;

	public:
		LeafView(const BValue &attr)
		 : BView(attr),
		   m_color(attr.ValueFor("color")),
		   m_mouse_down(false)
		{
		}
		
		LeafView(const LeafView &copyFrom)
			: BView(copyFrom),
			  m_color(copyFrom.m_color),
			  m_last_point(copyFrom.m_last_point),
			  m_mouse_down(copyFrom.m_mouse_down)
		{}

		virtual	BView *		Copy() { return new LeafView(*this); }
		virtual	void		DispatchEvent(const BMessage &msg, const BPoint& where, event_dispatch_result *result)
		{
			int32 what = msg.What();
			if (result) *result = B_EVENT_ABSORBED;
			
			switch (what) {
				case (B_MOUSE_DOWN): {
					m_mouse_down = true;
					m_last_point = Transform().Transform(where);

					break;
				}

				case (B_MOUSE_UP): {
					m_mouse_down = false;
					break;
				}
				case (B_MOUSE_MOVED): {
					if (m_mouse_down) {
						BPoint parent_where = Transform().Transform(where);
						BPoint delta = parent_where - m_last_point;
						BLayoutConstraints new_constraints(ExternalConstraints());
						
						// XXX Fix: Ugly hack, assumes constraints are in pixels.
						new_constraints.axis[B_HORIZONTAL].pos.value += delta.x;
						new_constraints.axis[B_VERTICAL].pos.value += delta.y;
						
						m_last_point = parent_where;
						SetExternalConstraints(new_constraints);
					}
					break;
				}

				default: {
					break;
				}
			}
			
		}

		virtual	void		Draw(const IRender::ptr& into)
		{
			BRect r = Shape().Bounds();
			
			if (r.IsValid()) {
				into->Rect(r);
				into->Color(m_color);

				r.InsetBy(BPoint(2,2));

//				into->MoveTo(r.LeftTop());
//				into->LineTo(r.RightBottom());
//				into->Stroke();
//				into->Color(BColor(0,0,0));
				
//				into->MoveTo(r.RightTop());
//				into->LineTo(r.LeftBottom());
//				into->Stroke();
//				into->Color(BColor(0,0,0));
			}
		}
};

typedef BViewGroup TestLayoutBaseClass;

class TestGroup : public TestLayoutBaseClass
{
		BPoint	m_last_point;
		bool	m_mouse_down;

	public:
		TestGroup()
		 : TestLayoutBaseClass(BValue().Overlay("orientation",
		                                        B_HORIZONTAL)),
//		                                        B_VERTICAL)),
		   m_mouse_down(false)
		{
			IView::ptr v = new LeafView(BValue()
				.Overlay("left", "0px")
				.Overlay("top", "30px")
				.Overlay("width", "50px 100px 150px")
//				.Overlay("width", ".1nm .5nm .5nm")
				.Overlay("height", "100px 200px 200px")
				.Overlay("color", BColor(0, 1, 0))
				);
			AddChild(v);

			v = new LeafView(BValue()
				.Overlay("left", "30px")
				.Overlay("top", "0px")
				.Overlay("width", "75px 120px 250px")
//				.Overlay("width", ".1nm .3nm .3nm")
				.Overlay("height", "100px 200px 200px")
				.Overlay("color", BColor(0, 0, 1))
				);
			AddChild(v);

			v = new LeafView(BValue()
				.Overlay("left", "0px")
				.Overlay("top", "0px")
				.Overlay("width", "100px 100px 1/0px")
//				.Overlay("width", ".1nm .9nm .9nm")
				.Overlay("height", "150px 150px 150px")
				.Overlay("color", BColor(1, 0, 0))
				);
			AddChild(v);

		}
		
		TestGroup(const TestGroup &copyFrom)
		 : TestLayoutBaseClass(copyFrom),
		   m_last_point(copyFrom.m_last_point),
		   m_mouse_down(copyFrom.m_mouse_down)
		{}

		virtual	void		Draw(const IRender::ptr& into)
		{
			BRect r = Shape().Bounds();
			
			if (r.IsValid()) {
				into->Rect(r);
				into->Color(BColor(.8,.8,.8));

				r.InsetBy(BPoint(2,2));

//				into->MoveTo(r.LeftTop());
//				into->LineTo(r.RightBottom());
//				into->Stroke();
//				into->Color(BColor(0,0,0));

//				into->MoveTo(r.RightTop());
//				into->LineTo(r.LeftBottom());
//				into->Stroke();
//				into->Color(BColor(0,0,0));
			}
			
			TestLayoutBaseClass::Draw(into);
		}

		virtual	void		DispatchEvent(const BMessage &msg, const BPoint& where, event_dispatch_result *result)
		{
			event_dispatch_result local_result;
			TestLayoutBaseClass::DispatchEvent(msg, where, &local_result);
			if (result) *result = local_result;
			if (local_result != B_EVENT_ABSORBED || m_mouse_down) {
				BRect bounds(Shape().Bounds());
				int32 what = msg.What();
				if (result) *result = B_EVENT_ABSORBED;
				
				switch (what) {
					case (B_MOUSE_DOWN): {
						BRect corner(-20, -20, 0, 0);
						corner.OffsetBy(bounds.RightBottom());
						if (corner.Contains(where)) {
							m_mouse_down = true;
							m_last_point = Transform().Transform(where);
						}		
						break;
					}
	
					case (B_MOUSE_UP): {
						m_mouse_down = false;
						break;
					}
					case (B_MOUSE_MOVED): {
						if (m_mouse_down) {
							BPoint parent_where = Transform().Transform(where);
							BPoint delta = parent_where - m_last_point;
							BLayoutConstraints new_constraints(ExternalConstraints());
							
							// XXX Fix: Ugly hack, assumes constraints are in pixels.
							if (new_constraints.axis[B_HORIZONTAL].pref.is_undefined()) {
								new_constraints.axis[B_HORIZONTAL].pref = dimth(bounds.Width(), dimth::pixels);
							}
							new_constraints.axis[B_HORIZONTAL].pref.value += delta.x;
							if (new_constraints.axis[B_VERTICAL].pref.is_undefined()) {
								new_constraints.axis[B_VERTICAL].pref = dimth(bounds.Height(), dimth::pixels);
							}
							new_constraints.axis[B_VERTICAL].pref.value += delta.y;
							
							m_last_point = parent_where;
							SetExternalConstraints(new_constraints);
						}
						break;
					}
	
					default: {
						break;
					}
				}
			}
			
		}

		virtual	BView *		Copy() { return new TestGroup(*this); }
};

class TestLayoutRoot : public BViewLayoutRoot {
	public:
		TestLayoutRoot(/* const IView::ptr &child, */ const BValue &attr = BValue::undefined)
		: BViewLayoutRoot(new TestGroup(), attr)
		{
		}
		
		TestLayoutRoot(const TestLayoutRoot &copyFrom)
		: BViewLayoutRoot(copyFrom) { }
		
		virtual void DoConstrain() { };

		virtual void DoLayout() {
		}

		virtual void Draw(const IRender::ptr &into)
		{
			BRect r = Shape().Bounds();
			if (r.IsValid()) {
				into->Rect(r);
				into->Color(BColor(.9,.9,.9));
			}
			into->PushState();
			into->Transform(Child()->Transform());
			Child()->Display(into);
			into->PopState();
		}

		virtual void DispatchEvent(const BMessage &msg,
		                           const BPoint& where,
		                           event_dispatch_result *result = NULL)
		{
//			bigtime_t start = system_time();

			Child()->DispatchEvent(msg, where, result);

//			bigtime_t end = system_time();
//			printf("\e[0;31mThread %ld: TestLayoutRoot::DispatchEvent() took %Ld usecs\e[0m\n",
//				find_thread(NULL), end - start);
		}

		virtual	BView *Copy() { return new TestLayoutRoot(*this); }
		
		
		status_t InvalidateChild(const IView::ptr&, const BUpdate &update)
		{
			IViewParent::ptr p = Parent();
			if (p != NULL) {	
				BUpdate my_update(BUpdate::B_OPAQUE, Transform(), Shape());
				my_update.ComposeWithChildren(update);
				return p->InvalidateChild(this, my_update);
			}
			return B_OK;
		}
};


// ********************************************************************
// #pragma mark -

B::Support2::IBinder::ptr
root(const B::Support2::BValue &/*params*/)
{
	return static_cast<BView*>(new TestGroup());
//	return static_cast<BView*>(new TestLayoutRoot());
}

int main(int , char **)
{
	BValue v;
	BLooper::SetRootObject(root(v));
	return BLooper::Loop();
}

