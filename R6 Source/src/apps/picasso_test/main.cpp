#include <stdlib.h>

#include <render2/RenderDefs.h>
#include <render2/Color.h>
#include <support2/Looper.h>
#include <support2/MessageCodes.h>
#include <support2/StdIO.h>
#include <storage2/Path.h>
#include <render2/Render.h>
#include <render2/Region.h>
#include <render2/Update.h>
#include <render2/Pixmap.h>
#include <interface2/InterfaceEventsDefs.h>
#include <interface2/SurfaceManager.h>
#include <interface2/View.h>

using namespace B::Support2;
using namespace B::Render2;
using namespace B::Interface2;
using namespace B::Storage2;
using namespace B::Raster2;


// ********************************************************************

static const uint32 SIZE = 256;

class RootView : public BView
{
	BStaticPixmap::ptr m_bitmap;
	BStaticPixmap::ptr m_bitmap1;

	public:
		RootView()
		{
			m_bitmap = new BStaticPixmap(SIZE,SIZE,pixel_format(0,0),4*SIZE);
			for (uint32 y=0;y<SIZE;y++) {
				uint32 *p = (uint32*)m_bitmap->PixelsForRow(y);
				for (uint32 x=0;x<SIZE;x++) {
					p[x] = y*0x00000001 + x*0x00000100 + 0xFFFF0000;
				}
			}
			m_bitmap1 = new BStaticPixmap(SIZE,SIZE,pixel_format(0,0),4*SIZE);
			for (uint32 y=0;y<SIZE;y++) {
				uint32 *p = (uint32*)m_bitmap1->PixelsForRow(y);
				for (uint32 x=0;x<SIZE;x++) {
					p[x] = y*0x00010000 + x*0x00000100 + 0xFF0000FF;
				}
			}
		}

		virtual	BView *		Copy() { return this; }

		virtual	void		DispatchEvent(const BMessage &msg, const BPoint& where, event_dispatch_result *result)
		{
			//bout << where << endl;
			if (result) *result = B_EVENT_ABSORBED;
		}

		virtual	void		Draw(const IRender::ptr& into)
		{
			const BRect r = Shape().Bounds();

//			B2dTransform tr = 	B2dTransform::MakeTranslate(-320, -240) *
//								B2dTransform::MakeScale(0.5,0.5) *
//								B2dTransform::MakeRotate((15 * 3.14)/180) *
//								B2dTransform::MakeTranslate(320, 240);					
//			into->Transform(tr);


			into->Rect(r);
			into->Color(BColor(0,0,1.0));


			into->PushState();
			into->Transform(B2dTransform::MakeTranslate(20, 10));

			m_bitmap->Display(into);
			into->PopState();

IRender::ptr child = into->Branch();//IRender::B_ASYNCHRONOUS);

			into->MoveTo(BPoint(100,100));
			into->Text("This is a beautifull antialiased text...");
			into->Color(BColor(0,0,0));

child->PushState();
child->Transform(B2dTransform::MakeTranslate(200, 20));
m_bitmap1->Display(child);
child->PopState();

		}
};

// ********************************************************************
// #pragma mark -

#include <render2_p/Edges.h>
#include <render2_p/Stroke.h>

B::Support2::IBinder::ptr
root(const B::Support2::BValue &/*params*/)
{
	IPath::ptr edges = new BEdges();
	IPath::ptr p = new BStroker(edges,5,B_ROUND_CAP,B_ROUND_JOIN);
	p->Rect(BRect(0,0,100,100));
	return static_cast<BView*>(new RootView());
}

int main(int , char **)
{
	BValue v;
	BLooper::SetRootObject(root(v));
	return BLooper::Loop();
}

