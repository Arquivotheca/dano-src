/*	$Id: DResizer.h,v 1.2 1998/10/21 12:51:46 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
*/

class DResizer : public BView
{
  public:
	DResizer(BRect frame, const char *name, BView *one, BView *two);
	
	virtual void MouseDown(BPoint);
	virtual void Draw(BRect update);

  protected:
	
	BView *m_view1, *m_view2;
	bool m_vertical;
};

