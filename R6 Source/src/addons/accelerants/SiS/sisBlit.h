#ifndef SISBLIT_H
#define SISBLIT_H

#include "accelerant.h"

// Graphics Card Hooks

void sis_fill_rectangle (engine_token *et, uint32 color, fill_rect_params *list,uint32	count);
void sis_invert_rectangle (engine_token *et, fill_rect_params *list, uint32 count);
void sis_screen_to_screen_blit (engine_token *et, blit_params *list, uint32 count);
void sis_waitengineidle(void);
status_t synctotoken (sync_token *st);

#endif

/*
 * The ROP code directly specifies the output of three binary
 * inputs (in the Windoze case, Pattern, Source, and Destination).  For three
 * binary sources, there are a total of eight possible input states.  Each of
 * the bits in the MinTerm value itself defines the result of each possible
 * state.  The MinTerm bits are defined as follows:
 *
 *      7       6       5       4       3       2       1       0
 *    P S D   P S D   P S D   P S D   P S D   P S D   P S D   P S D
 *    1 1 1   1 1 0   1 0 1   1 0 0   0 1 1   0 1 0   0 0 1   0 0 0
 *
 * So for example : COPY SOURCE = 0xCC
 *                  COPY PATTERN = 0xF0
 */
 
