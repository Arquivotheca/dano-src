#ifndef SIS620_BLIT_H
#define SIS620_BLIT_H

#include "accelerant.h"

// Graphics Card Hooks

void		sis620_fill_rectangle (engine_token *et, uint32 color, fill_rect_params *list,uint32	count);
void		sis620_invert_rectangle (engine_token *et, fill_rect_params *list, uint32 count);
void		sis620_screen_to_screen_blit (engine_token *et, blit_params *list, uint32 count);
void		sis620_waitengineidle (void);
status_t	sis620_synctotoken (sync_token *st);

void		sis620_send_orders(uint32 *buffer, int size) ;
int			sis620_addcommand_write_synctoken(	uint32 *buffer_start ) ;
int			sis620_addcommand_blit(	uint32 *buffer_start,
						uint16 depth,
						uint32 src_addr, uint16 src_rowbytes,
						uint16 srcX, uint16 srcY,
						uint32 dst_addr, uint16 dst_rowbytes,
						uint16 dstX, uint16 dstY,
						uint16 width, uint16 height) ;
int			sis620_addcommand_fill(	uint32* buffer_start, 
						uint16 depth,
						uint32 addr, uint16 rowbytes,
						uint16 startx, uint16 starty,
						uint16 width, uint16 height,
						uint32 color ) ;
int			sis620_addcommand_invert(	uint32 *buffer_start,
						uint16 depth,
						uint32 addr, uint16 rowbytes,
						uint16 left, uint16 top,
						uint16 right, uint16 bottom ) ;

#endif
