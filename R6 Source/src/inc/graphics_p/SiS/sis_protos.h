#ifndef SIS_PROTOS_H
#define SIS_PROTOS_H

#include <Accelerant.h>

	// 2d Accelerated functions
typedef struct sis_protos {

	status_t	(*sis_getsynctoken)
						(engine_token *et, sync_token *st);
	void		(*sis_waitengineidle)
						(void);
	status_t	(*sis_synctotoken)
						(sync_token *st);

	void		(*sis_send_orders)
						(uint32 *buffer, int size) ;

	int			(*sis_addcommand_write_synctoken)
						(uint32 *buffer_start ) ;

	int			(*sis_addcommand_blit)
						(uint32 *buffer_start,
						uint16 depth,
						uint32 src_addr, uint16 src_rowbytes,
						uint16 srcX, uint16 srcY,
						uint32 dst_addr, uint16 dst_rowbytes,
						uint16 dstX, uint16 dstY,
						uint16 width, uint16 height) ;

	int			(*sis_addcommand_fill)
						(uint32* buffer_start, 
						uint16 depth,
						uint32 addr, uint16 rowbytes,
						uint16 startx, uint16 starty,
						uint16 width, uint16 height,
						uint32 color ) ;

	int			(*sis_addcommand_invert)
						(uint32 *buffer_start,
						uint16 depth,
						uint32 addr, uint16 rowbytes,
						uint16 left, uint16 top,
						uint16 right, uint16 bottom ) ;

	} sis_protos ;
	
#endif
