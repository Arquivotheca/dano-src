
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include <string.h>

#include "vt100.h"

#define TRACE 0

void VTLine::Clear(int start, int stop, uchar _attr)
{
	int i;
	for(i=start;i<=stop;i++){
		text[i] = 0x20;
		attr[i] = _attr;
	}
}


void VTLine::Delete(int start, int count, uchar _attr)
{
	int end = start + count;
	
	while(start < width){
		if(end < width){
			text[start] = text[end];
			attr[start] = attr[end];
		} else {
			text[start] = 0x20;
			attr[start] = _attr;
		}
		start++;
		end++;
	}
}

//    ab    w
// 012345678

void VTLine::Insert(int start, int count, uchar _attr)
{
	int n;
	
	for(n = width-1; n >= start+count; n--){
		text[n] = text[n-count];
		attr[n] = attr[n-count];
	}
	while(count){
		text[start] = 0x20;
		attr[start] = _attr;
		start++;
		count--;
	}
}


VTLine::VTLine(int _width)
{
	width = _width;
	text = (uchar*) malloc(width * 2);
	attr = text + width;
}

VTLine::VTLine(VTLine *orig)
{
	width = orig->width;
	text = (uchar*) malloc(width * 2);
	attr = text + width;
	memcpy(text,orig->text,width*2);
}

VTLine::~VTLine()
{
	free(text);
}


void VT100::Push(VTLine *line)
{
	scrolltop--;
	if(scrolltop < 0) scrolltop = scrollmax-1;
	if(scrollback[scrolltop]){
		delete scrollback[scrolltop];
	}
	scrollback[scrolltop] = new VTLine(line);
}


/* struct virtscreen newscr; */

void VT100::Resize(int _rows, int _columns)
{
	int i;
	lines = (VTLine**) malloc(sizeof(VTLine*) * _rows);
	for(i=0;i<_rows;i++){
		lines[i] = new VTLine(_columns);
	}

	scrollmax = 1000;
	scrolltop = 0;
	scrollbottom = 0;
	scrollback = (VTLine**) malloc(sizeof(VTLine*) * scrollmax);
	memset(scrollback, 0, sizeof(VTLine*) * scrollmax);
	
	insert_mode = 0;
	auto_wrap = 1;
	
	bottom_scroll = _rows;
	rows = _rows;
	columns = _columns;
	cur_ansi_number = top_scroll = 0;
	ansi_elements = ansi_reading_number = 0;
	next_char_send = &VT100::std_interpret_char;
	old_attrib = attrib = 0x07;
	next_char_send = &VT100::std_interpret_char;
	for(i=0;i<rows;i++){
		Line(i)->Clear(0,columns-1,attrib);
	}
	old_xpos = old_ypos = 0;
	xpos = ypos = 0;
}

VT100::VT100(VT100Display *_display)
{
	display = _display;
	
}

void VT100::clear_virtscreen(int mode)
{
	int i;
	
	switch (mode) {
	case 0: 
		Line(ypos)->Clear(xpos,columns-1,attrib);
		for(i=ypos+1;i<rows;i++){
			Line(i)->Clear(0,columns-1, attrib);
		}
		display->InvalidateRegion(ypos, rows -1);
		break;
		
    case 1: 
		for(i=0;i<ypos;i++){
			Line(i)->Clear(0,columns-1,attrib);
		}
		Line(ypos)->Clear(0,xpos,attrib);
		display->InvalidateRegion(0,ypos);
		break;
		
    default: 
		for(i=0;i<rows;i++){
			Line(i)->Clear(0,columns-1,attrib);
		}
		display->InvalidateRegion(0,rows-1);
		xpos = ypos = 0;
		break;
	}
}

void VT100::clear_to_eol(int mode)
{
	switch(mode) {
	case 0: 
		Line(ypos)->Clear(xpos,columns-1,attrib);
		display->InvalidateRegion(ypos,xpos,columns-1);
		break;
		
	case 1:
		Line(ypos)->Clear(0,xpos,attrib);
		display->InvalidateRegion(ypos,0,xpos);
		break;
		
	default:
		Line(ypos)->Clear(0,columns-1,attrib);
		display->InvalidateRegion(ypos,0,columns-1);
	}
}

void VT100::position_console(int _ypos, int _xpos, int rel)
{
	if (rel) {
		xpos += _xpos;
		ypos += _ypos;
	} else {
		xpos = _xpos;
		ypos = _ypos;
	}
	if (xpos < 0) xpos = 0;
	if (xpos >= columns) xpos = columns-1;
	if (ypos < 0) ypos = 0;
	if (ypos >= rows) ypos = rows-1;
	move_cursor();
}

void VT100::scroll_virt_up_at_cursor(int dir)
{
	VTLine *save;
	int i;
	int bottom = bottom_scroll - 1;
	
	if((ypos < top_scroll) || (ypos >= bottom_scroll)) return;
	
	if(dir){
		/* scrolling up */
		save = Line(ypos);
		for(i=ypos;i<bottom;i++){
			lines[i] = lines[i+1];
		}
	} else {
		/* scrolling down */
		save = Line(bottom);
		for(i=bottom;i>ypos;i--){
			lines[i] = lines[i-1];
		}
	}
	lines[i] = save;
	Push(save);
	save->Clear(0,columns-1,attrib);
	display->InvalidateRegion(top_scroll,bottom);
}

void VT100::scroll_virtscreen()
{
	int i,bottom;
	bottom = bottom_scroll - 1;
	VTLine *save = Line(top_scroll);
	for(i=top_scroll;i<bottom;i++){
		lines[i] = lines[i+1];
	}
	lines[i] = save;
	Push(save);
	save->Clear(0,columns-1,attrib);
	display->InvalidateRegion(top_scroll,bottom);
}

void VT100::set_scroll_region(int low, int high)
{
	low--;
	high = (high >= rows) ? rows : high;
	low = (low < 0) ? 0 : low;
	
	top_scroll = low;
	bottom_scroll = high;
//	fprintf(stderr,"[%d-%d]",low,high);
}

void 
VT100::special_xterm_ansi(unsigned char ch)
{
	if(ch == 7) {
		char *x = str;
		str[str_len] = 0;
		while(*x && (*x != ';')) x++;
		if(*x == ';') x++;
		display->SetTitle((const char*) x);
		next_char_send = &VT100::std_interpret_char;
	} else {
		if(str_len < 255){
			str[str_len++] = ch;
		}
	}
}

void VT100::special_ansi_charset_0(unsigned char ch)
{
	next_char_send = &VT100::std_interpret_char;
}

void VT100::special_ansi_charset_1(unsigned char ch)
{
	next_char_send = &VT100::std_interpret_char;
}

void VT100::std_interpret_ansi(unsigned char ch)
{
	switch (ch) {
	case '(':  
		next_char_send = &VT100::special_ansi_charset_0;
		return;
		
	case ')':  
		next_char_send = &VT100::special_ansi_charset_1;
		return;
		
	case '[':  
		cur_ansi_number = 0;
		ansi_elements = 0;
		ansi_reading_number = 0;
		next_char_send = &VT100::special_reading_ansi;
		return;
		
	case ']':
		next_char_send = &VT100::special_xterm_ansi;
		str_len = 0;
		return;
		
    case '7':
		old_attrib = attrib;
		old_xpos = xpos;
		old_ypos = ypos;
		break;
		
    case '8':
		change_attribute(old_attrib);
		position_console(ypos,xpos,0);
		break;
		
    case 'E':  
		xpos = 0;
		
    case 'D':  
		ypos++;
		if (ypos >= bottom_scroll) {
			scroll_virtscreen();
			ypos--;
		}
		move_cursor();
		break; 
		
	case 'M':  
		ypos--;
		if (ypos < top_scroll) {
			ypos++;
			scroll_virt_up_at_cursor(0);
		}
		move_cursor();
		break;    /* recalculate screen pos */
			
	default:
		if(ch != 27){
			if((ch > ' ') && (ch < 127)) {
				fprintf(stderr,"('%c')\n",ch);
			} else {
				fprintf(stderr,"(%02x)\n",ch);
			}
		}
		
	}
	if (ch != 27) next_char_send = &VT100::std_interpret_char;
}

void VT100::std_interpret_char(unsigned char ch)
{
	switch (ch) {
	case 27: 
		next_char_send = &VT100::std_interpret_ansi;
		return;
		
	case 12: 
		clear_virtscreen(2);
		break;
		
	case 13: 
		xpos = 0;        /* return = back to begin */
		break;
		
	case 10: 
		ypos++;
		if (ypos >= bottom_scroll) {
			/* if we're at bottom */
			if (ypos == bottom_scroll) scroll_virtscreen();        /* and scroll it! */
			ypos--;            /* go back up a line */
		}
//		xpos=0;
		break;  
		
	case 9:  
		position_console(ypos,xpos+(8-(xpos % 8)),0);
		break;
		
	case 8:  
		xpos--;               /* backspace on screen */
		if (xpos<0) {
			xpos = columns - 1;
			ypos--;//XXX?
			if (ypos<0) ypos=0;
		}
		break;
		
	case 7: // bell
		break;
		
    default: 
		if (!ch) break;
		if ((ch < ' ') ){
			fprintf(stderr,"[%02x]",ch);
			break;
		}
		
		if(insert_mode) {
			Line(ypos)->Insert(xpos,1,attrib);
		}
			
		Line(ypos)->text[xpos] = ch;
		Line(ypos)->attr[xpos] = attrib;
		display->InvalidateRegion(ypos,xpos,insert_mode ? columns-1 : xpos);
		
		xpos++;
		if (xpos >= columns) {
			if(auto_wrap){
				xpos = 0;
				ypos++;
				if (ypos == bottom_scroll) { //xxx
					ypos--;
					scroll_virtscreen();
				}
			} else {
				xpos = columns-1;
			}
		}
		break;
	}
	move_cursor();
}

void VT100::special_reading_ansi2(unsigned char ch)
{
	int op;
	
	if ((ch>='0') && (ch<='9')) {
		cur_ansi_number = (cur_ansi_number * 10) + (ch - '0');
		ansi_reading_number = 1;
		return;
	}

	if ((ansi_reading_number) || (ch == ';')) {
		if (ansi_elements<MAX_ANSI_ELEMENTS)
			ansi_element[ansi_elements++] = cur_ansi_number;
		ansi_reading_number = 0;
	}
	cur_ansi_number = 0;

	op = ansi_elements ? ansi_element[0] : 0;
		
	switch(ch){
	case ';':
		return;
		
	case 'l':
		switch(op){
		case 25: // form feed?!
			break;
			
		case 7:
			auto_wrap = 0;
			break;
			
		default: 
			fprintf(stderr,"^[?%dl",op);
		}
		break;
		
	case 'h':
		switch(op){
		case 25: // form feed?!
			break;
			
		case 7:
			auto_wrap = 1;
			break;
			
		default: 
			fprintf(stderr,"^[?%dh",op);
		}
		break;
		
	}
	next_char_send = &VT100::std_interpret_char;
	
}

void VT100::special_reading_ansi(unsigned char ch)
{
	if ((ch>='0') && (ch<='9')) {
		cur_ansi_number = (cur_ansi_number * 10) + (ch - '0');
		ansi_reading_number = 1;
		return;
	}

	if ((ansi_reading_number) || (ch == ';')) {
		if (ansi_elements<MAX_ANSI_ELEMENTS)
			ansi_element[ansi_elements++] = cur_ansi_number;
		ansi_reading_number = 0;
	}
	cur_ansi_number = 0;
	switch (ch) {
	case '?':
		ansi_elements = 0;
		ansi_reading_number = 0;
		next_char_send = &VT100::special_reading_ansi2;
		return;
		
	case ';':   
		return;
		
	case 'D':   
		if(ansi_elements && !ansi_element[0]) ansi_element[0] = 1;
		position_console(0,(ansi_elements) ?
						 -ansi_element[0] : -1,1);
		break;
		
	case 'a':
	case 'C':   
		if(ansi_elements && !ansi_element[0]) ansi_element[0] = 1;
		position_console(0,(ansi_elements) ?
						 ansi_element[0] : 1,1);
		break;
		
	case 'A':   
		if(ansi_elements && !ansi_element[0]) ansi_element[0] = 1;
		position_console((ansi_elements) ? 
						 -ansi_element[0] : -1,0,1);
		break;
		
	case 'e':
	case 'B':   
		if(ansi_elements && !ansi_element[0]) ansi_element[0] = 1;
		position_console((ansi_elements) ?
						 ansi_element[0] : 1,0,1);
		break;
		
	case '`':
	case 'G': {
		int temp = ansi_elements ? ansi_element[0] : 1;
		if (temp) temp--;
		position_console(ypos,temp,0);
		break;
	}

	case 'E':   
		position_console(ypos + 
						 ((ansi_elements) ? ansi_element[0] : 1),
						 0,0);
		break;
		
	case 'F':   
		position_console(ypos - 
						 ((ansi_elements) ? ansi_element[0] : 1),
						 0,0);
		break;
		
	case 'd': {
		int temp = ansi_elements ? ansi_element[0] : 1;
		if (temp) temp--;
		position_console(temp,xpos,0);
		break;
	}
	
	case 'l':
		if(ansi_elements && (ansi_element[0] == 4)){
			insert_mode = 0;
		}
		break;
		
	case 'h':
		if(ansi_elements && (ansi_element[0] == 4)){
			insert_mode = 1;
		}
		break;
		
	case 'f':
	case 'H': {
		int row = (ansi_elements > 0) ? ansi_element[0] : 1;
		int col = (ansi_elements > 1) ? ansi_element[1] : 1;
		if (row) row--;
		if (col) col--;
		position_console(row,col,0);
		break;
	}
	
	case 'J':   
		clear_virtscreen((ansi_elements) ?
						 ansi_element[0]: 0);
		break;
		
	case 'L': {
		int lines = (ansi_elements) ? ansi_element[0] : 1;
		while (lines>0) {
			scroll_virt_up_at_cursor(0);
			lines--;
		}
		break;
	}
	
	case 'M': {
		int lines = (ansi_elements) ? ansi_element[0] : 1; 
		while (lines>0) {
			scroll_virt_up_at_cursor(1);
			lines--;
		}
		break;
	}
	
	case '@':
		Line(ypos)->Insert(xpos,ansi_elements ? ansi_element[0] : 1, attrib);
		display->InvalidateRegion(ypos,xpos,columns-1);
		break;
		
	case 'P':   
		Line(ypos)->Delete(xpos,ansi_elements ? ansi_element[0] : 1,attrib);
		display->InvalidateRegion(ypos,xpos,columns-1);
		break;
		
	case 'K':   
		clear_to_eol(ansi_elements ? 
					 ansi_element[0] : 0);
		break;
		
	case 's':   
		old_xpos = xpos;
		old_ypos = ypos;
		break;
		
	case 'u':   
		position_console(old_ypos,old_xpos,0);
		break;
			
	case 'r': {
		int low = (ansi_elements > 0) ? ansi_element[0] : 1;
		int high = (ansi_elements > 1) ? ansi_element[1] : rows;
		if (low<=high) set_scroll_region(low,high);
		break;
	}
	
	case 'm': {
#if 1
		int count = 0;
		int cthing;
		if (!ansi_elements) change_attribute(0x07);
		while (count < ansi_elements) {
			cthing = ansi_element[count];
			switch (cthing) {
			case 0:
			case 27: 
				change_attribute(0x07);
				break;
			case 1:  
				change_attribute(attrib | 0x08);
				break;
			case 5:  
//				change_attribute(attrib | 0x80);
				break;
			case 7: 
				change_attribute(0x70);
				break;
			case 21:
			case 22: 
				change_attribute(attrib & 0xF7);
				break;
			case 25: 
//				change_attribute(attrib & 0x7F);
				break;
			default:
				if ((cthing>=30) && (cthing<=37)){
					change_attribute((attrib & 0xF8) | (cthing-30));
				}
				if ((cthing>=40) && (cthing<=47)) {
					change_attribute((attrib & 0x8F) | 
									 ((cthing-40) << 4));
				}
				break;
			}
			count++;
		}
#endif
		break;
	}
	
	default:{
		int i;
		if((ch > ' ') && (ch < 127)) {
			fprintf(stderr,"<'%c'",ch);
		} else {
			fprintf(stderr,"<%02x",ch);
		}
		for(i=0;i<ansi_elements;i++){
			fprintf(stderr,":%02x",ansi_element[i]);
		}
		fprintf(stderr,">");
	}
	}
	next_char_send = &VT100::std_interpret_char;
}

void VT100::Write(const char *_string, int length)
{
	uchar *string = (uchar *) _string;
	while (length--) {
#if TRACE
		{
			uchar c = *string;
			if(c < ' '){
				switch(c){
				case 10: fprintf(stderr,"(\\n)"); break;
				case 13: fprintf(stderr,"(\\r)"); break;
				case 27: fprintf(stderr,"^"); break;
				}
			} else {
				fprintf(stderr,"%c",c);
			}
		}
#endif
		char_to_virtscreen(*(string++));
	}
}

void VT100::Write(const char *_string)
{
	uchar *string = (uchar *) _string;
	while (*string) {
		char_to_virtscreen(*(string++));
	}
}


void VT100::Write(char byte)
{
	char_to_virtscreen((uchar)byte);
}




