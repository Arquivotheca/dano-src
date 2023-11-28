#ifndef __CURSOR_H__
#define __CURSOR_H__

status_t cursorCreate();
void cursorLoad( uint16 w, uint16 h, uint16 hx, uint16 hy, uint8 *and, uint8 *xor );
void cursorMove( uint16 x, uint16 y );
void cursorShow( uint8 show );
void cursorDestroy();


#endif
