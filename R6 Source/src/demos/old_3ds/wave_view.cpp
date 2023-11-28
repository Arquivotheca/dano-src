#include "wave_view.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>



//--------------------------------------------------------------


TWaveView::TWaveView (BPoint where, BView *parent)
  : BView (BRect(where.x+0, where.y, where.x + 29, where.y + 132), "main view",
		   B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED)
{
	parent->AddChild(this);
	SetViewColor(200,200,200);
	bitview = new BBitmap(BRect(0,0,26,128), B_COLOR_8_BIT);
	k = (rand()%32768)/1500.0;
	init_wave(0);
	clear_bitmap();
	ImageWave();
}

//--------------------------------------------------------------

void	TWaveView::init_wave(float v)
{
	long	i;
	
	for (i = 0; i < 128; i++) {
		values[i] = (10.0*sin(v+(i/k)));
	}
}

//--------------------------------------------------------------

void	TWaveView::clear_bitmap()
{
	char	*ptr;
	long	count;
	long	rb;
	
	ptr = (char *)bitview->Bits();
	rb = bitview->BytesPerRow();
	
	count = bitview->BitsLength();
	
	memset(ptr, 26, count);
}

//--------------------------------------------------------------

void	TWaveView::ImageWave()
{
	char	*ptr;
	long	rb;
	long	i;
	long	v;
	
	ptr = (char *)bitview->Bits();
	rb = bitview->BytesPerRow();
	
	for (i = 0; i < 128; i++) {
		v = values[i];
		if (v < -15) v = -15;
		if (v > 15) v = 15;
		*(ptr+v+15) = 0x11;
		ptr += rb;
	}
}

//--------------------------------------------------------------

TWaveView::~TWaveView()
{
}


//--------------------------------------------------------------


void	TWaveView::Draw(BRect rr)
{
	BRect	r;
	
	
	r = Bounds();
	DrawBitmap(bitview, BPoint(2,2));
	SetHighColor(240,240,240);
	MovePenTo(BPoint(r.right, r.top));
	StrokeLine(BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.left, r.bottom));
	SetHighColor(170,170,170);
	StrokeRect(BRect(r.left + 1, r.top + 1, r.right -1, r.bottom - 1));
	SetHighColor(200,200,200);
	MovePenTo(BPoint(r.left, r.bottom - 1));
	StrokeLine(BPoint(r.left, r.top + 0));
	StrokeLine(BPoint(r.right - 1, r.top + 0));
}


//--------------------------------------------------------------

void	TWaveView::UpdateWave()
{
	//init_wave(system_time()/100000.0);
	clear_bitmap();
	ImageWave();
	DrawBitmap(bitview, BPoint(2,2));
	k += ((rand()%1000)-500.0)/2000.0;
}

//--------------------------------------------------------------

void	TWaveView::SetSample(long val)
{
	long	i;
	
	for (i = 1; i < 128; i++)
		values[i-1] = values[i];
	values[127] = val;
}

//--------------------------------------------------------------

void	TWaveView::Pulse()
{
	UpdateWave();
}


//--------------------------------------------------------------

