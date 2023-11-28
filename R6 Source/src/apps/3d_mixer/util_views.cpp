
LineView::LineView(BRect r) :
		  BView(r, "line", B_FOLLOW_NONE,B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
}

//--------------------------------------------------------

LineView::~LineView() {}

//--------------------------------------------------------

void	LineView::Draw(BRect r)
{
	BRect	tr;

	tr = Bounds();

	SetHighColor(180,180,180);
	MovePenTo(BPoint(tr.left,tr.top));
	StrokeLine(BPoint(tr.right,tr.top));

	SetHighColor(240,240,240);
	MovePenTo(BPoint(tr.left,tr.bottom));
	StrokeLine(BPoint(tr.right,tr.bottom));
}

//--------------------------------------------------------

ToolView::ToolView(BRect r) :
		  BView(r, "line", B_FOLLOW_NONE,B_WILL_DRAW)
{
	BScreen	s;

	SetViewColor(s.ColorForIndex(25));
}

//--------------------------------------------------------

ToolView::~ToolView()
{}

//--------------------------------------------------------

void	ToolView::Draw(BRect r)
{
	BRect	tr;

	tr = Bounds();

	SetHighColor(180,180,180);
	MovePenTo(BPoint(tr.right-1,tr.top));
	StrokeLine(BPoint(tr.right-1,tr.bottom));
	
	SetHighColor(240,240,240);
	MovePenTo(BPoint(tr.right,tr.top));
	StrokeLine(BPoint(tr.right,tr.bottom));

	SetHighColor(130,130,140);
	MovePenTo(BPoint(0,tr.bottom-1));
	StrokeLine(BPoint(4096, tr.bottom-1)); 
	SetHighColor(160,160,160);
	MovePenTo(BPoint(0,tr.bottom));
	StrokeLine(BPoint(4096, tr.bottom)); 
}

//--------------------------------------------------------

THSB::THSB(char *name, BRect view_bound)
	: BScrollBar(view_bound, name, 0, 0, 8000*100.0, B_HORIZONTAL)
{
}

/*------------------------------------------------------------*/

void	THSB::UpdateSpeed(float zoom)
{
	float	small;
	
	//printf("zoom = %f\n", zoom);
	
	small = zoom * 100.0 * 128.0;
	if (small < 1)
		small = 1;

	SetSteps(small*2, small * 16);
}

/*------------------------------------------------------------*/

		ZoomControl::ZoomControl(BRect r)
	  	: BView(r, "zoom", B_FOLLOW_BOTTOM, B_WILL_DRAW)
{
	off = new BBitmap(BRect(0, 0, 109, 20),
						  	B_COLOR_8_BIT,
						  	TRUE);
	off->AddChild(off_view = new BView(BRect(0,0,110,20),
									  "",
									  B_FOLLOW_ALL,
									  B_WILL_DRAW));

	zlevel = 12;

	SetViewColor(B_TRANSPARENT_32_BIT);
	off->Lock();
	draw_internal(off_view);
	off->Unlock();
}


/*------------------------------------------------------------*/

void	ZoomControl::draw_internal(BView *v)
{
	long	i;
	long	d;
	float	e;
	long	i1;

	for (i = 16; i < 94; i++) {
		v->SetHighColor(i*2, i*2, i*2);
		d = (i-6)/19;
		v->MovePenTo(BPoint(i, 6 - d));
		v->StrokeLine(BPoint(i, 6 + d));
		e = (i-6.0)/19.0;
		e -= d;
		i1 = (255-i*2);
		v->SetHighColor(255-(e*i1),255-(e*i1),255-(e*i1));
		
		v->MovePenTo(BPoint(i, 6 - d - 1));
		v->StrokeLine(BPoint(i, 6 - d - 1));
		
		v->MovePenTo(BPoint(i, 6 + d + 1));
		v->StrokeLine(BPoint(i, 6 + d + 1));
			
	}

	v->Sync();
}

/*------------------------------------------------------------*/

		ZoomControl::~ZoomControl()
{
	delete off;
}

/*------------------------------------------------------------*/

void	ZoomControl::Draw(BRect r)
{
	BRect	br;
	
	br = Bounds();

	DrawBitmap(off, BPoint(0, 1));
	SetHighColor(160,160,160);
	MovePenTo(BPoint(br.left, br.top));
	StrokeLine(BPoint(br.right, br.top));


	SetHighColor(200, 0, 0);
	SetDrawingMode(B_OP_BLEND);

	MovePenTo(BPoint(zlevel + 16, 1));
	StrokeLine(BPoint(zlevel + 16, br.bottom - 1));
	MovePenTo(BPoint(zlevel + 16 + 1, 1));
	StrokeLine(BPoint(zlevel + 16 + 1, br.bottom - 1));
	SetDrawingMode(B_OP_COPY);
}

/*------------------------------------------------------------*/

void	ZoomControl::HandleValue(float v)
{
	TrackView	*tv;
	THSB		*sb;

	tv = (TrackView *)Window()->FindView("tv");
	if (tv)
		tv->SetZoom(v);	
	
	sb = (THSB *)Window()->FindView("_HSB_");

	if (sb)
		sb->UpdateSpeed(v);
}

/*------------------------------------------------------------*/

void	ZoomControl::MouseDown(BPoint where)
{
	ulong		but;
	long		level0;
	TrackView	*tv;
	float		z;

	tv = (TrackView *)Window()->FindView("tv");

	do {
		level0 = zlevel;

		zlevel = (int)where.x - 16;
		if (zlevel > (94-16))
			zlevel = 94-16;
	
		if (zlevel < 0)
			zlevel = 0;

		if (zlevel != level0) {
			Draw(BRect(0,0,1000,1000));
			z = zlevel / 90.0;
			z = z*z;
			HandleValue(1.0/44100.0 + z);
		}
		else {
			Window()->Unlock();
			snooze(32000);
			Window()->Lock();
		}

		GetMouse(&where, &but);
	} while(but);
}

/*------------------------------------------------------------*/

void	THSB::ValueChanged(float v)
{
	TrackView	*tv;

	tv = (TrackView*)Window()->FindView("tv");
	
	tv->SetHPos(v/100.0);
}

