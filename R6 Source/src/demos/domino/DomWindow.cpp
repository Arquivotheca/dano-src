//*****************************************************************************
//
//	File:		 DomWindow.cpp
//
//	Description: WindowScreen class playing dominos.
//
//	Copyright 1996, Be Incorporated
//
//  Thanks to Guillaume Desmaret for all the textures and the icon.
//
//*****************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <Alert.h>
#include "Domino.h"
#include "DomWindow.h"

//*********************************************************************

// Enum use for internal reference of horizontal or vertical dominos.
enum { HOR, VER };

// Index of the blit graphics hook.
#define  INDEX_BLIT   7
#define  INDEX_SYNC   10

// Macro used to test a specific key in the key_map returned by GetKeys.
#define	TestCode(x)		(theKey.key_states[x>>3]&(128>>(x&7)))

// Type definition of the blit graphics hook.
typedef long     (*BLIT)(long,long,long,long,long,long);
typedef long	 (*SYNC)();

// Blit graphics hook pointer.
static BLIT      blit_jmp;
static SYNC		 sync_jmp;

// A few prototypes (as everything is in one file, you will find nothing
// in the header file and a lot of static variables and function. Don't
// hesitate to do something cleaner...
static long      Play(void *param);
static void      InitBoard();
static void      DrawDigit();
static void      DrawBand(long h0, long h1, long v0, long v1);

// This flag indicated if the access to the screen is allow. Access to it
// must be protected by the "lock" semaphore.
static int       drawing_enable = FALSE;

// Used to initialise the game on the first ScreenConnected(TRUE).
static int       First = TRUE;

// Flag used to ask the domino player to abort itself. Then the ender
// semaphore is used to symchronised the window thread.
static int       kill_play = FALSE;

// Index of the current displayed buffer (as we're using double buffer).
static int       page_num;

// Current number of dominos on the board.
static int       domino_count;

// Use for the crc random generator.
static long      crc, crc_key;

// RowByte (BytesPerRow) of the frame buffer.
static long      row;

// 2 dimensional coordinates of :
// dim : size of the display rectangle.
// pos : position of the top left corner of the display rectangle in the board.
// max : size of the board
// focus : current play position (has to be visible). 
static long      dimh, dimv, posh, posv, maxh, maxv, focush, focusv;

// Base adress of the current offscreen buffer (the one we're drawing into).
static uchar     *draw_base, *base;

// Offscreen memory buffer used to render a square before copying it into the
// graphic card frame_buffer (small enough to stay in the cache).
static uchar     *buffer = 0L;

// Board buffer. That buffer store and 2D array the size of the board (of
// maxh*maxv ulong) coding all the information needed to draw the board,
// the borders of the board, the dominos, the shadows and informations to
// help the calculator find a place to put new dominos. Each long codes an
// 32x32 square, corresponding to half a domino.
// Meaning by bits :
// 0-2 : Board Background code. The Board background is draw by 64x64 textures,
//       (2x2 32x32 squares). This code indicate the index of the texture in
//       a choice of 8 textures. The horizontal and vertical parity indicate
//       which of the 4 squares need to be used. The 8 different textures are
//       supposed to loop together (That's what Guillaume said).
// 3-6 : Shape code. If the square contains something more than the background
//       of the board, this bits indicates what the shape is. 16 shapes are
//       available :
//         0 : top half vertical domino,
//         1 : left half horizontal domino,
//         2 : bottom half vertical domino,
//         3 : right half horizontal domino,          4  5  6
//         4-11 : borders of the board, like that --> 11    7
//                                                    10 9  8
//         12-15 : borders with screw 5->12, 7->13, 9->14, 11->15.
// 7 : Empty square code. If set to 0, the Shape code is valid, if set to 1, the
//     square is empty.
// 8-10 : Figure code. 7 figures are available for the dominos. 1 to 6 are coded
//        by 0 to 5, and 6 codes the "Be" logo. 7 is still available.
// 11-15 : Shadow mask. Mask code for the shadows of the dominos (1 bit per shadow).
//         11 : corner shadow (projected in n+1, m+1)
//         12 : partial vertical shadow (projected in n+1, m)
//         13 : complete vertical shadow (projected in n+1, m)
//         14 : partial horizontal shadow (projected in n, m+1)
//         15 : complete horizontal shadow (projected in n, m+1)
// 16-29 : Calculator mask. This mask indicates for each square the status of each
//         figure. The first bit indicate if the figure is allowed (0) or blocked
//         by a neighboor, a border, or a specific band property (1). The second
//         bit indicates if the figure would be linked with another identical figure
//         in the neighboorhood.
//            Figures :                1    2    3    4    5    6   Be
//         Allowed(0)/Blocked(1)      16   18   20   22   24   26   28 
//         Linked(0)/Not Linked(1)    17   19   21   23   25   27   29
// 30-31 : Band mask. Indicate the bands incrusted into the board background when
//         necessary. Band are managed as background texture (64x64).
//         30 : Horizontal band
//         31 : Vertical band
static ulong     *board = 0L;

// Semaphore used to synchronise access to the screen and the drawing_enable flag.
static sem_id    lock;

// Semaphore to get the window thread waiting for the death of the domino player.
static sem_id    ender;

// Copy of the WindowScreen pointer.
static DomWindow *window;

// Code used to convert board code into texture map index.
static uchar      background[8] = { 8, 10, 40, 42, 72, 74, 104, 106 };
static uchar      digit[10] = { 65, 66, 67, 68, 69, 70, 71, 100, 101, 112 };
static uchar      croix[4] = { 0, 102, 115, 113 };
static uchar      form[16] = {
	0, 1, 2, 3,
	80, 81, 82, 83, 84, 85, 86, 87, 96, 97, 98, 99,
};

// Mask used to set the band inhibition.
static ulong mask0[7] = {
	0x15540000,
	0x15510000,
	0x15450000,
	0x15150000,
	0x14550000,
	0x11550000,
	0x05550000,
};

// Mask used to update the neighboor property after putting a domino figure.
static ulong mask[7] = {
	0x15560000,
	0x15590000,
	0x15650000,
	0x15950000,
	0x16550000,
	0x19550000,
	0x25550000,
};

// Indicate which combination of calculator code allow to put a domino. 
static uchar allow[16] = {
	0,0,1,0,
	0,0,0,0,
	1,0,2,0,
	0,0,0,0,
};

//*********************************************************************
// WindowScreen specific stuff.

// to enable debugger mode.
#undef DEBUG
#define DEBUG FALSE

DomWindow::DomWindow (long space, char *name, status_t *error)
:BWindowScreen(name,space,error,DEBUG)
{
	int       	i, nrj;
	bool		sSwapped;
	char		*buf;
	BRect     	rect;
	BAlert		*quit_alert;
	key_map		*map;
	rgb_color 	c_list[256];
	
// default warning dialog, if running is an option.
	if (CanControlFrameBuffer() && (*error == B_OK)) {
		get_key_map(&map, &buf);
		sSwapped = (map->left_control_key==0x5d) && (map->left_command_key==0x5c);
		free(map);
		free(buf);
		quit_alert = new BAlert("QuitAlert", sSwapped ?
		                        "This demo runs only in full screen mode.\n"
		                        "While running, press 'Ctrl-Q' to quit.":
		                        "This demo runs only in full screen mode.\n"
		                        "While running, press 'Alt-Q' to quit.",
		                        "Quit", "Start demo", NULL,
		                        B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		if (quit_alert->Go() == 0)
			*error = B_ERROR;
	}
// create the semaphore
	lock = create_sem(1,"domino lock sem"); 
	ender = create_sem(0,"domino kill sem");
// spawn the domino player thread
	draw = spawn_thread(Play,"Domino player",B_NORMAL_PRIORITY,(void*)this);
	RegisterThread(draw);
	resume_thread(draw);
// Create the fake view used to read the keyboard.
	rect.Set(0.0, 0.0, 9.0, 9.0);
	view = new BView(rect, "", 0, 0);
	AddChild(view);
}


void DomWindow::MessageReceived(BMessage *message)
{
	int8		key_code;

	switch(message->what) {
	// Switch between full-screen mode and windowed mode.
	case B_KEY_DOWN :
		if (message->FindInt8("byte", &key_code) != B_OK)
			break;
		if (key_code == B_ESCAPE)
			PostMessage(B_QUIT_REQUESTED);
		break;
	}
}

void DomWindow::Quit()
{
// Disconnect the screen control. In that case, it's just for the fun as the kill
// domino player mechanism doesn't need it.
	Disconnect();
// If the domino player is running, ask for its death and wait for it...
	if (draw != 0L) {
		kill_play = TRUE;
		acquire_sem(ender);
	}
// free the semaphores
	delete_sem(ender);
	delete_sem(lock);
// Quit the WindowScreen.
	BWindowScreen::Quit();
}


void DomWindow::ScreenConnected(bool active)
{
	short    height;

// Better to block anything else before modifying any graphic settings or
// the drawing_enable flag.
	acquire_sem(lock);
	if (active == TRUE) {
	// The first time we got the screen, we need to init the game and set
	// the screen in double buffer mode.
		if (First) {
			InitBoard();
			if (SetFrameBuffer(640, 960) == B_NO_ERROR)
				goto ok;
			PostMessage(B_QUIT_REQUESTED);
			goto end;
		ok:
			First = FALSE;
		}
	// Each time we get the screen again, we have to reinitialise the
	// double buffer management, redraw the frame_buffer and read all
	// the graphics card parameters.
		page_num = 0;  // current display : page 0
		MoveDisplayArea(0, 0); // set page 0
		row = CardInfo()->bytes_per_row; // read graphics parameters
		base = draw_base = (uchar*)CardInfo()->frame_buffer;
		blit_jmp = (BLIT)CardHookAt(INDEX_BLIT);
		sync_jmp = (SYNC)CardHookAt(INDEX_SYNC);
		DrawBand(0, dimh, 0, dimv); // draw the board
		DrawDigit(); // draw the counter
		draw_base += dimv*32*row; // move the draw_base to the other buffer.
	// allow the drawing in the frame buffer (when lock release).
		drawing_enable = TRUE;
	}
	else
	// stop drawing.
		drawing_enable = FALSE;
 end:
// Release the main semaphore.
	release_sem(lock);
}

//***********************************************************************
// Domino player stuff.

// Draw a square from the texture map into the standard draw buffer.
// Color 0 is transparent.
static void DrawShape(int index) {
	long    i, j;
	uchar   *to, *from;

	from = bitmap + (index&15)*32 + (index>>4)*(400*32);
	to = buffer;
	for (i=0;i<32;i++) {
		for (j=0;j<32;j++)
			if (from[j] != 0)
				to[j] = from[j];
		to += 32;
		from += 400;
	}
}

// Draw a square of the board (with all the layers), then copy it in
// the graphics card frame_buffer.
static void DrawSquare(long h, long v, ulong code, int digit_ref) {
	int    val, i, j;
	double *f, *t;

// draw the background	
	DrawShape(background[code&7]+((h+posh)&1)+((v+posv)&1)*16);
// draw the band
	if (code>>30)
		DrawShape(croix[code>>30]+((h+posh)&1)+((v+posv)&1)*16);
// draw the shadows
	if (code&(1<<11)) DrawShape(54);
	if (code&(1<<12)) DrawShape(52);
	if (code&(1<<13)) DrawShape(53);
	if (code&(1<<14)) DrawShape(64);
	if (code&(1<<15)) DrawShape(55);
// draw the domino or the board border
	val = (code>>8)&7;
	if (val == 7) {
		if (((code>>3)&31) < 16)
			DrawShape(form[(code>>3)&31]);
	}
	else
		DrawShape((val+(val&6))*4+((code>>3)&3));
// draw the counter
	if (digit_ref >= 0)
		DrawShape(digit[digit_ref]);
// copy the standard buffer in the frame_buffer.
	t = (double*)draw_base + (row*v+h)*4;
	f = (double*)buffer;
	for (i=0;i<32;i++) {
		t[0] = *f++;
		t[1] = *f++;
		t[2] = *f++;
		t[3] = *f++;
		t += row>>3;
	}
}

// Redraw part the part of the current visible board, included in any
// specific rectangle.
static void DrawBand(long h0, long h1, long v0, long v1) {
	long     h, v;

	if (h0 < 0) h0 = 0;
	if (h1 > dimh) h1 = dimh;
	if (v0 < 0) v0 = 0;
	if (v1 > dimv) v1 = dimv;
	for (v=v0; v<v1; v++)
		for (h=h0; h<h1; h++)
			DrawSquare(h, v, board[(v+posv)*maxh+(h+posh)], -1);
}

// Draw the counter in the right bottom corner of the display.
static void DrawDigit() {
	int      h, cnt, base;

	base = 1000;
	cnt = domino_count;
	for (h=4; h>0; h--) {
		if (cnt >= base)
			DrawSquare(dimh-h, dimv-1,
					   board[(dimv-1+posv)*maxh+(dimh-h+posh)],
					   (cnt/base)%10);
		base /= 10;
	}
}

// Crc random generator.
static long alea(int loop) {
	for (;loop>0;loop--) {
		crc <<= 1;
		if (crc < 0) crc ^= crc_key;
	}
	return crc;
}

// Initialize the game
static void InitBoard() {
	int     i, j;
	ulong   back;
	bigtime_t time;

// initialize the crc generator.
	crc = (long)(system_time()&0xffffffff) | 1;
	crc_key = 0x07890ef1L;

// display area : 20x15 squares
	dimh = 20;
	dimv = 15;
// board : 100x100 squares
	maxh = 100;
	maxv = 100;
// initial position 0,0
	posh = 0;
	posv = 0;
// allocate the board buffer.
	board = (ulong*)malloc(sizeof(ulong)*maxh*maxv);
// allocate the standard graphic buffer
	buffer = (uchar*)malloc(1024);
// reset the domino count
	domino_count = 0;
// Initialize the background texture of the board. Incrusts the 7 horizontal
// bands and the 7 vertical bands.
	for (j=0;j<maxv;j+=2)
		for (i=0;i<maxh;i+=2) {
			back = 0x00000780L+(alea(3)&7);
			if ((i%14) == 8)
				back |= mask0[i/14] | 0x40000000;
			if ((j%14) == 8)
				back |= mask0[j/14] | 0x80000000;
			board[j*maxh+i] = back;
			board[j*maxh+i+1] = back;
			board[(j+1)*maxh+i] = back;
			board[(j+1)*maxh+i+1] = back;
		}
// Put the 4 corners of the border.
	board[0]             = (board[0]&0xc0000007)             | 0x15550720L;
	board[maxh-1]        = (board[maxh-1]&0xc0000007)        | 0x15550730L;
	board[maxh*(maxv-1)] = (board[maxh*(maxv-1)]&0xc0000007) | 0x15550750L;
	board[maxh*maxv-1]   = (board[maxh*maxv-1]&0xc0000007)   | 0x15550740L;
// Put the 4 main part of the borders, incrusting screws.
	for (i=1;i<maxh-1;i++) {
		if (i & 3) {
			board[i]               = (board[i]&0xc0000007)               | 0x15550728L;
			board[maxh*(maxv-1)+i] = (board[maxh*(maxv-1)+i]&0xc0000007) | 0x15550748L;
		}
		else {
			board[i]               = (board[i]&0xc0000007)               | 0x15550760L;
			board[maxh*(maxv-1)+i] = (board[maxh*(maxv-1)+i]&0xc0000007) | 0x15550770L;
		}
	}
	for (i=1;i<maxv-1;i++) {
		if (i & 3) {
			board[maxh-1+maxh*i]   = (board[maxh-1+maxh*i]&0xc0000007)   | 0x15550738L;
			board[maxh*i]          = (board[maxh*i]&0xc0000007)          | 0x15550758L;
		}
		else {
			board[maxh-1+maxh*i]   = (board[maxh-1+maxh*i]&0xc0000007)   | 0x15550768L;
			board[maxh*i]          = (board[maxh*i]&0xc0000007)          | 0x15550778L;
		}
	}
}

// Update the board buffer to incrustate an horizontal or vertical dominos at a
// specific position, with specific figures. Increase the domino counter.
static void PutDomino(int val0, int val1, int sens, long h, long v) {
	long      pos;

// get the lock before modifying the board
	acquire_sem(lock);
	pos = v*maxh+h;
	if (sens == HOR) {
		board[pos]   = (board[pos]   & 0xfffff8ff) | (val0<<8);
		board[pos+1] = (board[pos+1] & 0xfffff8ff) | (val1<<8);
		board[pos]        |= 0x15550008;
		board[pos+1]      |= 0x15550018;
		board[pos+2]      |= 0x00001000 | mask[val1];
		board[pos+maxh]   |= 0x00004000 | mask[val0];
		board[pos+maxh+1] |= 0x00008000 | mask[val1];
		board[pos+maxh+2] |= 0x00000800;
		board[pos-1]      |=              mask[val0];
		board[pos-maxh]   |=              mask[val0];
		board[pos-maxh+1] |=              mask[val1];
	}
	else {
		board[pos]      = (board[pos]      & 0xfffff8ff) | (val0<<8);
		board[pos+maxh] = (board[pos+maxh] & 0xfffff8ff) | (val1<<8);
		board[pos]          |= 0x15550000;
		board[pos+maxh]     |= 0x15550010;
		board[pos+1]        |= 0x00001000 | mask[val0];
		board[pos+maxh+1]   |= 0x00002000 | mask[val1];
		board[pos+2*maxh]   |= 0x00004000 | mask[val1];
		board[pos+2*maxh+1] |= 0x00000800;
		board[pos-1]        |=              mask[val0];
		board[pos-maxh]     |=              mask[val0];
		board[pos+maxh-1]   |=              mask[val1];
	}
	domino_count++;
// release the lock.
	release_sem(lock);
}

// Copy the useful part of the displayed buffer in the offscreen buffer,
// depending of the current scrolling, using the hardware blitter.
static void CopyBuffer(int dh, int dv) {
	int      h0, v0, h1, v1, sh, sv;

	if (dh < 0) {
		h0 = 0;
		h1 = -32*dh;
		sh = (dimh+dh)*32-1;
	}
	else {
		h0 = 32*dh;
		h1 = 0;
		sh = (dimh-dh)*32-1;
	}
	if (dv < 0) {
		v0 = 0;
		v1 = -32*dv;
		sv = (dimv+dv)*32-1;
	}
	else {
		v0 = 32*dv;
		v1 = 0;
		sv = (dimv-dv)*32-1;
	}
	if (page_num == 0)
		v1 += dimv*32;
	else
		v0 += dimv*32;
	if ((sh > 0) && (sv > 0)) {
		if (blit_jmp != 0L) {
			(blit_jmp)(h0,v0,h1,v1,sh,sv);
			// don't forget to sync, or Trey won't be happy ;-)
			if (sync_jmp != 0L)
				(sync_jmp)();
		}
		else {
			int      i;
			double   *from, *to;
			
			from = (double*)(base+h0+v0*row);
			to = (double*)(base+h1+v1*row);
			sh >>= 3;
			for (;sv>=0;sv--) {
				for (i=0;i<=sh;i++)
					to[i] = from[i];
				to += row>>3;
				from += row>>3;
			}
		}
	}
}

// Switch buffers.
static void SwitchBuffer() {
	if (page_num == 0) {
		window->MoveDisplayArea(0, dimv*32);
		page_num = 1;
		draw_base -= row*dimv*32;
	}
	else {
		window->MoveDisplayArea(0, 0);
		page_num = 0;
		draw_base += row*dimv*32;
	}
}

// Move the focus. Check that the new focus is still visible. if not, do
// a scrolling to get it in the middle (if possible) of the displayed area.
static void MoveFocus(int h, int v) {
	int      dh, dv, first;
	bigtime_t   time;

// Get the lock before doing anything
	acquire_sem(lock);
	
	focush = h;
	focusv = v;
// Do the drawing only if allowed.
	if (drawing_enable) {
	// Do we need to scroll, or is the new focus already visible ?
		if ((h < (posh+1)) || (h >= (posh+dimh-2)) ||
			(v < (posv+1)) || (v >= (posv+dimv-2))) {
		// We need to scroll. First calculate the scroll translation.
			h -= dimh/2;
			v -= dimv/2;
			if (h < 0) h = 0;
			else if (h > (maxh-dimh)) h = maxh-dimh;
			if (v < 0) v = 0;
			else if (v > (maxv-dimv)) v = maxv-dimv;
			h -= posh;
			v -= posv;
		// Use the first falg to draw the new domino if necessary.
			first = TRUE;
			while ((h != 0) || (v != 0)) {
				time = system_time();

			// Calculate the elementary scrolling step.
				if (h == 0) dh = 0;
				else if (h < 0) dh = -1;
				else dh = 1;
				if (v == 0) dv = 0;
				else if (v < 0) dv = -1;
				else dv = 1;
			// Scroll the main part
				CopyBuffer(dh, dv);
			// Update the position of the display (needed to call DrawBand).
				posh += dh;
				posv += dv;
			// Erase the old position of the counter
				DrawBand(dimh-4-dh, dimh-dh, dimv-1-dv, dimv-dv);
			// Update the scrolling move.
				h -= dh;
				v -= dv;
			// Update the bands on the borders (where necessary after the scroll).
				if (dh < 0) DrawBand(0, 1, 0, dimv);
				else if (dh > 0) DrawBand(dimh-1, dimh, 0, dimv);
				if (dv < 0) DrawBand(0, dimh, 0, 1);
				else if (dv > 0) DrawBand(0, dimh, dimv-1, dimv);
			// If it's the first move, we need to update the rectangle in the neighboor
			// of the domino, in case part of it was already visible. 
				if (first) {
					DrawBand(focush-posh, focush-posh+3, focusv-posv, focusv-posv+3);
					first = FALSE;
				}
			// Draw the counter.
				DrawDigit();
			// Switch buffer
				SwitchBuffer();

			// synchornizes the scrolling speed.
				time = time+20000-system_time();
				if (time > 0)
					snooze(time);
			}
		}
		else {
		// Just copy everything from the visible buffer to the hidden one.
			CopyBuffer(0, 0);
		// Update teh new domino and its neighboorhood.
			DrawBand(focush-posh, focush-posh+3, focusv-posv, focusv-posv+3);
		// Draw the counter
			DrawDigit();
		// Swicth buffer.
			SwitchBuffer();
		}
	}
// If drawing is not allowed, just move the position (as it's virtual it's fast !)
// link with the new focus.
	else {
		posh = h-dimh/2;
		posv = v-dimv/2;
		if (posh < 0) posh = 0;
		if (posh > (maxh-dimh)) posh = maxh-dimh;
		if (posv < 0) posv = 0;
		if (posv > (maxv-dimv)) posv = maxv-dimv;
	}
// Release the lock.	
	release_sem(lock);
}

// Try to put a new domino with the specified figures (val1, val2) near the
// current focus (in the rectangle -dh, +dh, -dv, +dv). Put the domino if
// a solution is found. Use the closest available solution, with a real
// preference when the domino is linked by both figures.
static bool LookForDomino(int val0, int val1, int dh, int dv) {
	int      pos, h ,v, h1, h2, v1, v2, pash, pasv, d;
	int      hm, vm, dm, sm, val0m, val1m;
	int      c0, c1, ch, b0, b1, bh, bonus;

// Initial impossibly bad match
	dm = 1000000;
// Determine the search area.
	h1 = focush-dh;
	h2 = focush+dh;
	v1 = focusv-dv;
	v2 = focusv+dv;
	if (h1 < 1) h1 = 1;
	if (h2 > maxh-1) h2 = maxh-1;
	if (v1 < 1) v1 = 1;
	if (v2 > maxv-1) v2 = maxv-1;
// Select random order or research.
	pash = 1;
	pasv = 1;
	if (alea(1)&1) {
		h = h1;
		h1 = h2-1;
		h2 = h-1;
		pash = -1;
	}
	if (alea(1)&1) {
		v = v1;
		v1 = v2-1;
		v2 = v-1;
		pasv = -1;
	}
// for each possible first square...
	for (v=v1;v!=v2;v+=pasv)
		for (h=h1;h!=h2;h+=pash) {
			pos = maxh*v+h;
		// read the board code of the square and its 2 right and bottom neighboors.
			c0 = ((board[pos] >> (2*val0+16)) & 3);
			c1 = ((board[pos+1] >> (2*val0+16)) & 3);
			ch = ((board[pos+maxh] >> (2*val0+16)) & 3);
			b0 = ((board[pos] >> (2*val1+16)) & 3);
			b1 = ((board[pos+1] >> (2*val1+16)) & 3);
			bh = ((board[pos+maxh] >> (2*val1+16)) & 3);
		// calculate distance to current focus
			d = (h-focush)*(h-focush) + (v-focusv)*(v-focusv);		
			if (d > (dm+100)) continue;
		// look for the 4 dominos beginning in that square, using those figures.
			if (bonus = allow[c0*4+b1])
				if ((d-50*bonus) < dm) {
					val0m = val0;
					val1m = val1;
					sm = HOR;
					hm = h;
					vm = v;
					dm = d-50*bonus;
				}
			if (bonus = allow[c0*4+bh])
				if ((d-50*bonus) < dm) {
					val0m = val0;
					val1m = val1;
					sm = VER;
					hm = h;
					vm = v;
					dm = d-50*bonus;
				}
			if (bonus = allow[b0*4+c1])
				if ((d-50*bonus) < dm) {
					val0m = val1;
					val1m = val0;
					sm = HOR;
					hm = h;
					vm = v;
					dm = d-50*bonus;
				}
			if (bonus = allow[b0*4+ch])
				if ((d-50*bonus) < dm) {
					val0m = val1;
					val1m = val0;
					sm = VER;
					hm = h;
					vm = v;
					dm = d-50*bonus;
				}
		}
// if no solution was found, just exit...
	if (dm == 1000000)
		return FALSE;
// in the other case, put the domino
	PutDomino(val0m, val1m, sm, hm, vm);
// and move the focus
	MoveFocus(hm, vm);
	return TRUE;
}

// Main thread of the domino player.
static long Play(void *param)
{
	int       dh, dv, i, j, k, val;
	uchar     values[49];
	bigtime_t time;
	key_info  theKey;

// Get a copy of the window object.
	window = (DomWindow*)param;
// Wait for the first ScreenConnected(TRUE). Sorry, it's a pooler (add a semaphore
// if you want...)
	while (!drawing_enable) {
	// Kill the domino player when ask.	
		if (kill_play) goto end;
		snooze(200000);
	}
// Delay just for the fun
	snooze(300000);
// Put a first domino, a double Be.
	PutDomino(6, 6, HOR, 4, 4);
	MoveFocus(4, 4);
// One second to look at that wonderful double Be domino !
	snooze(1000000);
// Let's go with the domino demo.
	while (TRUE) {
		time = system_time();
	// Kill the domino player when ask.	
		if (kill_play) goto end;
	// Reset the table of tried dominos (can be optimized using symetry).
		for (i=0;i<49;i++)
			values[i] = 0;
	// Try the 49 possibility in random order.
		for (i=49;i>0;i--) {
			val = (alea(7)&0x7fffffff)%i;
			j = 0;
			while ((val+values[j]) > 0)
				val += values[j++]-1;
			val = j;
		// For the first, we only try to put them in the near neighboor
			if (LookForDomino(val/7, val%7, 5, 5)) break;
		// For the 20 next, we also try a bigger area
			if (i < 40) if (LookForDomino(val/7, val%7, 15, 15)) break;
		// example of debugger call.
			if (i == 38)
				window->Suspend("Stop there...");
		// At the end, we try anywhere.
			if (i < 20) if (LookForDomino(val/7, val%7, maxh, maxv)) break;
			values[val] = 1;
		}
	// if it failed...
		if (i == 0) {
		// do it again...
			for (i=0;i<49;i++)
				values[i] = 0;
			for (i=49;i>0;i--) {
				val = (alea(7)&0x7fffffff)%i;
				j = 0;
				while ((val+values[j]) > 0)
					val += values[j++]-1;
				val = j;
			// but try all of them everywhere.
				if (LookForDomino(val/7, val%7, maxh, maxv)) break;
				values[val] = 1;
			}
		// If it failed again, then the demo it's finished...
			if (i == 0) break;
		}

	// Synchronize to only 8 dominos per second.
		time = time+130000-system_time();
		if (time > 4000)
			snooze(time);
	}
// When all the dominos are in place...
	while (TRUE) {
		time = system_time();
	// Kill the domino player when ask.	
		if (kill_play) goto end;
	// Read the keyboard.		
//		get_key_info(&theKey);
    // Move the display area.
		acquire_sem(lock);
		if (drawing_enable) {
//			dh = 0;
//			dv = 0;
//			if (TestCode(0x38))
//				if (posv > 0) dv = -1;
//			if (TestCode(0x48))
//				if (posh > 0) dh = -1;
//			if (TestCode(0x59))
//				if (posv < (maxv-dimv)) dv = 1;
//			if (TestCode(0x4a))
//				if (posh < (maxh-dimh)) dh = 1;
			BPoint  pt;
			ulong   button;
			
			window->Lock();
			window->view->GetMouse(&pt, &button);
			window->Unlock();

			set_mouse_position(320, 240);
			
			if ((pt.x < 320) && (posh > 0))
				dh = -1;
			else if ((pt.x > 320) && (posh < (maxh-dimh)))
				dh = 1;
			else
				dh = 0;
			if ((pt.y < 240) && (posv > 0))
				dv = -1;
			else if ((pt.y > 240) && (posv < (maxv-dimv)))
				dv = 1;
			else
				dv = 0;

			if ((dh != 0) || (dv != 0)) {
				CopyBuffer(dh, dv);
				posh += dh;
				posv += dv;
				DrawBand(dimh-4-dh, dimh-dh, dimv-1-dv, dimv-dv);
				if (dh < 0) DrawBand(0, 1, 0, dimv);
				else if (dh > 0) DrawBand(dimh-1, dimh, 0, dimv);
				if (dv < 0) DrawBand(0, dimh, 0, 1);
				else if (dv > 0) DrawBand(0, dimh, dimv-1, dimv);
				DrawDigit();
				SwitchBuffer();
			}
		}
		else
			snooze(200000);
		release_sem(lock);
	// Synchronise the speed to 33 moves/s
		time = time+30000-system_time();
		if (time > 0)
			snooze(time);
	}
 end:
// do some clean-up...
	if (buffer != 0L) free(buffer);
	if (board != 0L) free(board);
// release the semaphore blocking the window thread
	release_sem(ender);
	return 0;
}

















