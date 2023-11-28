/* $Id: //depot/blt/srv/console2/vt100.h#1 $ */
/* VT100 Engine Copyright 1997 Daniel Marks <dmarks@uiuc.edu> */  

#ifndef _VT_100_H
#define _VT_100_H

#define loc_in_virtscreen(cur,y,x) (((cur)->data)+(((y)*((cur)->columns))+(x)))
#define MAX_ANSI_ELEMENTS 16
#define char_to_virtscreen(cur,ch) (((cur)->next_char_send)((cur),(ch)))

struct virtscreen
{
  int rows;
  int columns; 
  int num_bytes;
  ushort *data;
  ushort *back;
  ushort *data_off_scr;

  int xpos;
  int ypos;
  int top_scroll;
  int bottom_scroll;
  int attrib;
  int old_attrib;
  int old_xpos;
  int old_ypos;

  int cur_ansi_number;
  int ansi_elements;
  int ansi_reading_number;
  int ansi_element[MAX_ANSI_ELEMENTS];

  void (*next_char_send)(struct virtscreen *cur, unsigned char ch);

  int lock;
};


int init_virtscreen(struct virtscreen *cur, int rows, int columns);
#endif /* _VT_100_h */
