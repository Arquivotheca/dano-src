
#include "termview.h"

#include <Window.h>
#include <Font.h>
#include <Bitmap.h>
#include <Region.h>
#include <Message.h>

#include <unistd.h>

#include "vt100.h"

#include <Debug.h>

rgb_color black = { 0, 0, 0 };
rgb_color red = { 255, 0, 0 };
rgb_color green = { 0, 255, 0 };
rgb_color yellow = { 255, 255, 0};
rgb_color blue = { 0, 0, 255 };
rgb_color magenta = { 255, 0, 255};
rgb_color cyan = { 0, 255, 255};
rgb_color white = { 255, 255, 255};

rgb_color foreground = { 0, 220, 0};

rgb_color *colortable[8] = {
	&black, &red, &green, &yellow, &blue, &magenta, &cyan, &white
};


#define FGColor(attr) *(((attr & 0x77) == 0x07) ? &foreground : colortable[attr&0x07])
#define BGColor(attr) *(colortable[(attr&0x70)>>4])

// charxy -> bitmapxy
#define X0(x) (((float)(x))*fw)
#define Y0(y) (((float)(y))*fh)

// charxy -> bitmapxy for second corner of a rectangle
#define X1(x) (((float)(x+1))*fw-1.0)
#define Y1(y) (((float)(y+1))*fh-1.0)


void 
bTermView::SetFD(int _fd)
{
	fd = _fd;
}

void 
bTermView::MessageReceived(BMessage *msg)
{
	if(msg->what == 42){
	}
}

void 
bTermView::KeyDown(const char *bytes, int32 numBytes)
{
	if(fd == -1){
		fprintf(stderr,"eek\n");
		return;
	}
	
	if(numBytes == 1){
		BMessage *msg = Window()->CurrentMessage();
		uint modifiers = msg->FindInt32( "modifiers");	
		
		modifiers &= B_SHIFT_KEY | B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY | B_MENU_KEY;
		
		if((bytes[0] < 27) && (modifiers == B_CONTROL_KEY)){
			write(fd,bytes,1); // Control Key
			return;
		}
			
		switch(bytes[0]){
		case B_RETURN: // newline should be CR
			write(fd,"\r",1);
			break;
		case B_UP_ARROW: // up
			write(fd,"\033[A",3);
			break;
		case B_DOWN_ARROW: // down
			write(fd,"\033[B",3);
			break;
		case B_LEFT_ARROW: // left
			write(fd,"\033[D",3);
			break;
		case B_RIGHT_ARROW: // right
			write(fd,"\033[C",3);
			break;
						
		case B_INSERT:
			fprintf(stderr,"INS\n");
			break;
			
		case B_HOME:
			if(modifiers & B_SHIFT_KEY) ScrollTo(0,0);			
			break;
			
		case B_END:
			fprintf(stderr,"END\n");
			break;
			
		case B_PAGE_UP:
			if(modifiers & B_SHIFT_KEY) {
				ScrollBy(0,-fh * 24);			
				scrolled_to -= 24;
			}
			break;
			
		case B_PAGE_DOWN:
			if(modifiers & B_SHIFT_KEY) {
				if(scrolled_to < 0) {
					ScrollBy(0,fh * 24);
					scrolled_to += 24;
				}
			}
			break;
			
		default:
#if 0
			fprintf(stderr,"<%02x>",bytes[0]);
#endif
			write(fd, bytes, 1);		
		}
		
	} else {
		if(fd != -1) write(fd,bytes,numBytes);
	}
#if 0	
	int i;
	for(i=0;i<numBytes;i++){
		PRINT(("%02x\n",bytes[i]));
	}
#endif
}

bTermView::bTermView(int width, int height)
 : BView(BRect(0,0,1,1),"btermview",B_FOLLOW_NONE,B_WILL_DRAW)
{
	font_height f;
	
	fd = -1;
	
	rows = height;
	cols = width;
	
	cursorx = -1;
	cursory = -1;
	scrolled_to = 0;
	
	be_fixed_font->GetHeight(&f);
	SetFont(be_fixed_font);
	
	fw = StringWidth("M");
	fh = f.ascent + f.descent + 1.0;
	fa = f.ascent;
		
	SetViewColor(B_TRANSPARENT_COLOR);
	
	ResizeTo(fw*cols-1,fh*rows-1);	
	
	BRect r = Bounds();
	r.bottom += 1;
	r.right += 1;
	
	vt100 = new VT100(dynamic_cast<VT100Display*>(this));
	vt100->Resize(rows,cols);
}

void 
bTermView::Write(const char *data, size_t len)
{
	int cx,cy;
	cx = cursorx;
	cy = cursory;
	invalid.MakeEmpty();	
	vt100->Write(data,len);
	if((cx != cursorx) || (cy != cursory)){
		BRect r;
		r.Set(X0(cursorx),Y0(cursory)+fa,X1(cursorx),Y1(cursory));
		invalid.Include(r);
		r.Set(X0(cx),Y0(cy)+fa,X1(cx),Y1(cy));
		invalid.Include(r);
	}
	Invalidate(&invalid);
	if(scrolled_to && invalid.CountRects()){
		ScrollTo(0,0);
		scrolled_to = 0;
	}
}

void 
bTermView::Draw(BRect update)
{	
	VTLine *line;
	uchar *text,*attr;
	uchar lastattr = 0xff;
	BPoint loc;
	BRect r;
	int x,y,top,bottom,left,right,right2,first,last;
	int x1,n;
	
	top = update.top / fh;
	bottom = update.bottom / fh + 1;
	left = update.left / fw;
	right = update.right / fw + 1;

	if(top*fh > update.top) top--;
	if(bottom*fh < update.bottom) bottom++;
	
	first = vt100->FirstLine();
	last = vt100->LastLine();
	
#if 0
		PRINT(("%d/%d/%d/%d  %d/%d/%d/%d\n",
			   (int)update.left,(int)update.top,(int)update.right,(int)update.bottom,
			   left,top,right,bottom));
#endif
	for(y=top;y<bottom;y++){
		line = vt100->LineAt(y);
		if(!line){
			r.Set(update.left,Y0(y),update.right,Y1(y));			
			SetLowColor(black);
			FillRect(r,B_SOLID_LOW);	 
			lastattr = 0xff;
			continue;
		}
		
		loc.y = Y0(y) + fa;
		text = line->text;
		attr = line->attr;
		if(line->width < right) {
			right2 = line->width;
		} else {
			right2 = right;
		}
		
		
		for(x=left;x<right2;x+=n){
			loc.x = X0(x);
			
			if(attr[x] != lastattr){
				SetHighColor(FGColor(attr[x]));
				SetLowColor(BGColor(attr[x]));
				lastattr = attr[x];
			}
			x1 = x + 1;
			n = 1;
			while((x1 < right2) && (attr[x1] == lastattr)){
				n++;
				x1++;
			}
			
			r.Set(X0(x),Y0(y),X1(x+n),Y1(y)+1.0);			
			FillRect(r,B_SOLID_LOW);	 
			DrawString((const char*)text+x,n,loc);
		}
	}
	
	r.Set(X0(cursorx),Y0(cursory)+fa,X1(cursorx),Y1(cursory));
	SetHighColor(white);
	FillRect(r);
}

// ------------------

void bTermView::MoveCursor(int x, int y)
{
	cursorx = x;
	cursory = y;
	//	PRINT(("Cursor(%d,%d)\n",x,y));
}

void bTermView::SetTitle(const char *title)
{
	Window()->SetTitle(title);
}

void bTermView::InvalidateRegion(int line)
{
	BRect r;
	r.Set(X0(0),Y0(line),X1(cols-1),Y1(line));			
//	Invalidate(r);
	invalid.Include(r);	
}

void bTermView::InvalidateRegion(int line, int start, int stop)
{
	BRect r;
	r.Set(X0(start),Y0(line),X1(stop),Y1(line));			
//	Invalidate(r);
	invalid.Include(r);	
}

void bTermView::InvalidateRegion(int startline, int stopline)
{
	BRect r;
	r.Set(X0(0),Y0(startline),X1(cols-1),Y1(stopline));			
//	Invalidate(r);
	invalid.Include(r);	
}


