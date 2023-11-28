#ifndef _VT_100_H
#define _VT_100_H

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <OS.h>

#define MAX_ANSI_ELEMENTS 16
#define char_to_virtscreen(ch) ((this->*next_char_send)(ch))

#define Line(num) (lines[num])

class VT100Display
{
public:
	virtual void MoveCursor(int x, int y) = 0;
	virtual void SetTitle(const char *title) = 0;
	virtual void InvalidateRegion(int line) = 0;
	virtual void InvalidateRegion(int startline, int stopline) = 0;
	virtual void InvalidateRegion(int line, int start, int stop) = 0;
};

#define move_cursor() (display->MoveCursor(xpos,ypos))

struct VTLine
{
	int dirty;
	int width;
	uchar *text;
	uchar *attr;
	
	void Clear(int start, int stop, uchar attr);
	void Delete(int start, int count, uchar attr);
	void Insert(int start, int count, uchar attr);	
	VTLine(int width);
	VTLine(VTLine *orig);
	~VTLine();
};

class VT100
{
	int rows;
	int columns; 
	
	VTLine **lines;
	VTLine **scrollback;
	
	int scrollmax;
	int scrolltop;
	int scrollbottom;
	
	int xpos;
	int ypos;
	int top_scroll;       // first line of scroll region
	int bottom_scroll;    // last line of scroll region + 1
	int attrib;
	
	int old_attrib;
	int old_xpos;
	int old_ypos;
	
	int insert_mode;
	int auto_wrap;
	
	int cur_ansi_number;
	int ansi_elements;
	int ansi_reading_number;
	int ansi_element[MAX_ANSI_ELEMENTS];
	
	char str[256];
	int str_len;
	
	void (VT100::*next_char_send)(unsigned char ch);
	
	void clear_virtscreen(int mode);
	void clear_to_eol(int mode);
	void position_console(int ypos, int xpos, int rel);
	void scroll_virt_up_at_cursor(int dir);
	void scroll_virtscreen();
	void set_scroll_region(int low, int high);
	void special_ansi_charset_0(unsigned char ch);
	void special_ansi_charset_1(unsigned char ch);
	void std_interpret_char(unsigned char ch);
	void std_interpret_ansi(unsigned char ch);
	void special_xterm_ansi(unsigned char ch);
	void special_reading_ansi(unsigned char ch);
	void special_reading_ansi2(unsigned char ch);

	void change_attribute(int _attrib) { attrib = _attrib;	}

	VT100Display *display;
	
	void Push(VTLine *line);
public:
	VT100(VT100Display *display);
	
	void Resize(int rows, int columns);
	
	void Write(char byte);
	void Write(const char *string);
	void Write(const char *string, int len);

	int FirstLine() { return 0; }
	int LastLine() { return rows; }

	VTLine *LineAt(int line){
		if(line < 0){
			line = -(line+1); // 0-index into scrollback ring	
			if(line >= scrollmax) return NULL;
//			fprintf(stderr,"[%d/%d/%d/%d]\n",line,scrolltop,scrollmax,(line+scrolltop)%scrollmax);
//			
			return scrollback[(line + scrolltop) % scrollmax];			
		} else {
			if(line >= rows) {
				return NULL;
			} else {
				return lines[line];
			}
		}
	}

	void DumpHistory(int fd){
		VTLine *line;
		int i;
		int n;
		char c = '\n';

		if(fd < 0) return;
		for(i = scrollmax-1; i >= 0; i--) {
			line = scrollback[(i+scrolltop) % scrollmax];
			if(line == NULL) continue;
			n = write(fd, line->text, line->width);
			if(n != line->width) {
				return;
			}
			write(fd, &c, 1);
		}
		for(i = 0; i < rows; i++) {
			line = lines[i];
			if(line == NULL) continue;
			n = write(fd, line->text, line->width);
			if(n != line->width) {
				return;
			}
			write(fd, &c, 1);
		}
	}
	
};

#endif /* _VT_100_h */
