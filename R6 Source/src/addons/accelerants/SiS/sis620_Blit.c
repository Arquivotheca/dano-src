#include "sis620_Blit.h"

extern sis_card_info *ci ;
extern engine_token enginetoken;

#define BUFF_MAX (200)
#define BUFF_MIN (30)

// tested works to 6000 with 8-bits Z-buffer
// less when Z-buffer is in 16-bits mode
#define MAX_ORDERS_AT_A_TIME (200)


// May need synchronous acceleration because of app_server bug
// fast mode : no #define
// medium : #define SIS_NO_REAL_SYNCTOTOKEN
// slow : #define SIS_SYNCHRONOUS_ACCEL
//#define SIS_SYNCHRONOUS_ACCEL

#ifdef SIS_SYNCHRONOUS_ACCEL
#define SIS_NO_REAL_SYNCTOTOKEN
#endif

// ----- Token Registers -----
/* To implement the function SyncToToken, which waits for a specific order
   to be executed, we use tokens. As the SiS doesn't have a dedicated service
   for this, the hack is to write the TokenNumber into some registers.
   This token will be placed in the queue, and the register will be set to the value
   after the order is executed ( token writing is placed *after* the order ).

   It is possible to use MonoMask register or the Clipping Register.
   That's ok as long as Be is not using the harware clipping.
	
   Note that this hack is worth doing it : 2D performance is improved by 40%

   There is then a need to verify if this synchronization works, note that if you
   dont activate *both* Fill and Blit, the app_server won't sync properly.
   
 */
 
// Use MonoMask 64 bits register :
#define PRIMITIVE_HIGH_REGISTER (0x8230)
#define PRIMITIVE_LOW_REGISTER (0x822c)

// Use Clipping Registers :
//#define PRIMITIVE_HIGH_REGISTER (0x8234)
//#define PRIMITIVE_LOW_REGISTER (0x8238)

//////////////////////////////////////////////////
// -------- Sends orders to the engine -------- //
//////////////////////////////////////////////////

void sis620_send_orders(uint32 *buffer, int size) {
	bigtime_t time0 ;
	int pos = 0 ;
	int s = MAX_ORDERS_AT_A_TIME ;
	while(s) {
		int i;
		if (size-pos<MAX_ORDERS_AT_A_TIME) s = size-pos ;

		time0 = system_time();
			// wait for available space in queue
			while ( (inl(0x8240)&0x1fff) < (s/2)+10 ) snooze(20);
		ci->profiling_waittime_before_setup += system_time()-time0 ;
		
		time0 = system_time();
			// sends the orders
			for(i=0;i<s;i+=2) outl(buffer[pos+i],buffer[pos+i+1]);
		ci->profiling_setup_time += system_time()-time0 ;

		pos += s;
		}
	}

/////////////////////////////////////////////////////
// -------- add command <write synctoken> -------- //
/////////////////////////////////////////////////////

int sis620_addcommand_write_synctoken(	uint32 *buffer_start ) {
	uint32 *buf = buffer_start ;
	uint32 *primitive_low, *primitive_high ;
	
	primitive_low = (uint32*)&ci->ci_PrimitivesIssued ;
	primitive_high = primitive_low + 1 ;
	
	*buf++= PRIMITIVE_LOW_REGISTER ; *buf++= *primitive_low ;
	*buf++= PRIMITIVE_HIGH_REGISTER; *buf++= *primitive_high ;
	 
	return(buf-buffer_start);
	}

////////////////////////////////////////////////////
// -------- fill buffer with BLIT orders -------- //
////////////////////////////////////////////////////

int sis620_addcommand_blit(	uint32 *buffer_start,
						uint16 depth,
						uint32 src_addr, uint16 src_rowbytes,
						uint16 srcX, uint16 srcY,
						uint32 dst_addr, uint16 dst_rowbytes,
						uint16 dstX, uint16 dstY,
						uint16 width, uint16 height) {

	uint32 *buf = buffer_start ;
	uint32 command ;

	if (depth != ci->ci_Depth) {
		ddprintf(("sis620: addcommand_blit error : depth(%d) != ScreenDepth(%d)\n",depth,ci->ci_Depth));
		return(0);
		}
		
	command = 0x0000cc00 ; // ROP = copy source
			
	if (srcY>dstY) command|=0x00020000;
	else {
		srcY+=height;
		dstY+=height;
		}
	if (srcX>dstX) command|=0x00010000;
	else {
		srcX+=width ;
		dstX+=width;
		}

	*buf++ = 0x8200 ; *buf++ = src_addr ;
	*buf++ = 0x8204 ; *buf++ = src_rowbytes ;	
	*buf++ = 0x8208 ; *buf++ = (srcX<<16) | srcY ;
	*buf++ = 0x820c ; *buf++ = (dstX<<16) | dstY ;
	*buf++ = 0x8210 ; *buf++ = dst_addr ;
	*buf++ = 0x8214 ; *buf++ = (0x7fff<<16) | dst_rowbytes ; // height set to maximum...
	*buf++ = 0x8218 ; *buf++ = ((height+1)<<16) | (width+1) ;
	*buf++ = 0x821c ; *buf++ = 0xff00ff00 ; // foreground pattern
	*buf++ = 0x823c ; *buf++ = command ; // Command

	return(buf-buffer_start);
	}

////////////////////////////////////////////////////
// -------- fill buffer with FILL orders -------- //
////////////////////////////////////////////////////

// --- On SiS620, Hardware Fill Depth is directly related to screen depth

int sis620_addcommand_fill_hwdepth(	uint32* buffer_start, 
						uint32 addr, uint16 rowbytes,
						uint16 startx, uint16 starty,
						uint16 width, uint16 height,
						uint32 color ) {

	uint32 *buf = buffer_start ;

	// uses the trapezoid fill function
	*buf++= 0x8208 ; *buf++= (starty<<16) | (height+1) ; // Y start, trapezoid height
	*buf++= 0x820c ; *buf++= ((startx+width+1)<<16) | startx ;
	*buf++= 0x8210 ; *buf++= addr ;
	*buf++= 0x8214 ; *buf++= (0x1fff<<16) | rowbytes ; // height ?
  	*buf++= 0x821c ; *buf++= color ; // foreground color
  	*buf++= 0x8220 ; *buf++= color ; // background color
		
	*buf++= 0x8244 ; *buf++= 0x7fff0000 ; // deltaX/Y left
	*buf++= 0x8248 ; *buf++= 0x7fff0000 ; // deltaX/Y right
	*buf++= 0x824c ; *buf++= -1 ; // left error
	*buf++= 0x8250 ; *buf++= -1 ; // right error

	*buf++= 0x823c ; *buf++= 0x0000f005 ; // Command = Trapezoid Fill; Raster OP=copy pattern

	return(buf-buffer_start);
	}

// --- So to fill a rectangle of any depth, we must adapt datas
// But this call doesn't make it possible to fill with a 32 bits pattern if you are in 8 bit mode

int sis620_addcommand_fill(	uint32* buffer_start, 
						uint16 depth,
						uint32 addr, uint16 rowbytes,
						uint16 startx, uint16 starty,
						uint16 width, uint16 height,
						uint32 color ) {

	uint32 *buf = buffer_start ;

	switch( depth ) {
		case 8: {
			color |= (color<<8) ;
			color |= (color<<16);
			switch(ci->ci_Depth) {
				case 8:
					break;
				case 16:
					width = (width+1)/2 ;
					break;
				case 32:
					width = (width+3)/4 ;
					break;
				default:
					vddprintf(("sis accel : fill 8bpp - unsupported screen mode\n"));
					return(0);
				}
			break;
			}
			
		case 16: {
			color |= (color<<16);
			switch(ci->ci_Depth) {
				case 16:
					break;
				case 32:
					width = (width+1) / 2 ;
					break;
				default:
					vddprintf(("sis accel : fill 16bpp - unsupported screen mode\n"));
					return(0);
				}
			}			
			break;
		
		case 32:
			if (ci->ci_Depth!=32) {
				vddprintf(("sis accel : fill 32bpp - unsupported screen mode\n"));
				return(0);
				}
			break;			
		default:
			ddprintf(("sis620: addcommand_fill unsupported depth %d\n",depth));
			return( 0 ) ;
		} // End of switch ( depth )
	
	// uses the trapezoid fill function
	*buf++= 0x8208 ; *buf++= (0<<16) | (height+1) ; // Y start, trapezoid height
	*buf++= 0x820c ; *buf++= ((width+1)<<16) | 0 ;
	*buf++= 0x8210 ; *buf++= addr + starty*rowbytes + startx*(depth>>3) ;
	*buf++= 0x8214 ; *buf++= (0x1fff<<16) | rowbytes ; // height ?
  	*buf++= 0x821c ; *buf++= color ; // foreground color
  	*buf++= 0x8220 ; *buf++= color ; // background color
		
	*buf++= 0x8244 ; *buf++= 0x7fff0000 ; // deltaX/Y left
	*buf++= 0x8248 ; *buf++= 0x7fff0000 ; // deltaX/Y right
	*buf++= 0x824c ; *buf++= -1 ; // left error
	*buf++= 0x8250 ; *buf++= -1 ; // right error

	*buf++= 0x823c ; *buf++= 0x0000f005 ; // Command = Trapezoid Fill; Raster OP=copy pattern

	return(buf-buffer_start);	
	
	} // End sis620_addcommand_fill
		
	


	
//////////////////////////////////////////////////////
// -------- Fill buffer with INVERT orders -------- //
//////////////////////////////////////////////////////

int sis620_addcommand_invert(	uint32 *buffer_start,
						uint16 depth,
						uint32 addr, uint16 rowbytes,
						uint16 left, uint16 top,
						uint16 right, uint16 bottom ) {

	uint32 *buf = buffer_start ;

	if (depth != ci->ci_Depth) {
		ddprintf(("sis620: addcommand_invert error : depth(%d) != ScreenDepth(%d)\n",depth,ci->ci_Depth));
		return(0);
		}

	// uses the bitblt function
	*buf++ = 0x8200 ; *buf++ = addr ;
	*buf++ = 0x8204 ; *buf++ = rowbytes ;	
	*buf++ = 0x8208 ; *buf++ = (left<<16) | top ;
	*buf++ = 0x820c ; *buf++ = (left<<16) | top ;
	*buf++ = 0x8210 ; *buf++ = addr ;
	*buf++ = 0x8214 ; *buf++ = (0x7fff<<16) | rowbytes ; // height set to maximum...
	*buf++ = 0x8218 ; *buf++ = ((bottom-top+1)<<16) | (right-left+1) ;
	*buf++ = 0x821c ; *buf++ = 0xff00ff00 ; // foreground pattern
	*buf++ = 0x823c ; *buf++ = 0x00033300 ; // Command : Raster OP= invert

	return(buf-buffer_start);
	}


// -------- 2D *Classic* hooks --------

// -------- Screen To Screen Blit --------

void sis620_screen_to_screen_blit (engine_token *et, blit_params *list, uint32 count) {
	int size = 0;
	if (et != &enginetoken) return;

#ifdef SIS_NO_REAL_SYNCTOTOKEN
	lockBena4(&ci->ci_EngineRegsLock);
#endif
	ci->ci_PrimitivesIssued ++;

	while(count--) {
		uint32 buffer[BUFF_MAX];
		size += sis620_addcommand_blit(buffer+size,
						ci->ci_Depth,
						ci->ci_FBBase_offset, ci->ci_BytesPerRow,
						list->src_left, list->src_top,
						ci->ci_FBBase_offset, ci->ci_BytesPerRow,
						list->dest_left, list->dest_top,
						list->width, list->height
						);
		if ( (!count) | (size>=BUFF_MAX-BUFF_MIN) ) {
			if (count==0) size+=sis620_addcommand_write_synctoken(buffer+size);	
			sis620_send_orders(buffer,size);
			size = 0;
			}
		list++;
		}

#ifdef SIS_NO_REAL_SYNCTOTOKEN
	unlockBena4(&ci->ci_EngineRegsLock);
#endif	

#ifdef SIS_SYNCHRONOUS_ACCEL
	sis620_waitengineidle();
#endif
	}

// -------- Fill Rectangle --------

void sis620_fill_rectangle (engine_token *et, uint32 color, fill_rect_params *list,uint32	count){
	int size = 0;
	if (et != &enginetoken) return;

#ifdef SIS_NO_REAL_SYNCTOTOKEN
	lockBena4(&ci->ci_EngineRegsLock);
#endif

	ci->ci_PrimitivesIssued ++;

	while(count--) {
		uint32 buffer[BUFF_MAX];
	
		size += sis620_addcommand_fill_hwdepth(buffer+size,
						ci->ci_FBBase_offset,
						ci->ci_BytesPerRow,
						list->left, list->top ,
						list->right-list->left, list->bottom-list->top,
						color
						);
		if ( (!count) | (size>=BUFF_MAX-BUFF_MIN) ) {
			if (count==0) size+=sis620_addcommand_write_synctoken(buffer+size);	
			sis620_send_orders(buffer,size);
			size = 0;
			}
		list++;
		}

#ifdef SIS_NO_REAL_SYNCTOTOKEN
	unlockBena4(&ci->ci_EngineRegsLock);
#endif	

#ifdef SIS_SYNCHRONOUS_ACCEL
	sis620_waitengineidle();
#endif
	}


// -------- Invert Rectangle --------

void sis620_invert_rectangle (engine_token *et, fill_rect_params *list, uint32 count) {
	int size = 0;
	if (et != &enginetoken) return;

#ifdef SIS_NO_REAL_SYNCTOTOKEN
	lockBena4(&ci->ci_EngineRegsLock);
#endif

	ci->ci_PrimitivesIssued ++;

	while(count--) {
		uint32 buffer[BUFF_MAX];
		size += sis620_addcommand_invert(buffer+size,
						ci->ci_Depth,
						ci->ci_FBBase_offset,
						ci->ci_BytesPerRow,
						list->left, list->top,
						list->right, list->bottom
						);
		if ( (!count) | (size>=BUFF_MAX-BUFF_MIN) ) {
			if (count==0) size+=sis620_addcommand_write_synctoken(buffer+size);	
			sis620_send_orders(buffer,size);
			size = 0;
			}
		list++;
		}		

#ifdef SIS_NO_REAL_SYNCTOTOKEN
	unlockBena4(&ci->ci_EngineRegsLock);
#endif
	
#ifdef SIS_SYNCHRONOUS_ACCEL
	sis620_waitengineidle();
#endif
	}


/////////////////////////////////////
// -------- Sync To Token -------- //
/////////////////////////////////////

#define SIS_3D_IDLE_AND_3DQUEUE_EMPTY (inl(0x89fc)&2)
#define SIS_2D_IDLE_3D_IDLE_AND_QUEUE_EMPTY ((inl(0x8240)&0xe0000000)==0xe0000000)

status_t sis620_synctotoken (sync_token *st) {
	vvddprintf(("synctotoken(%d)\n",(int)st->counter ));
	vvddprintf(("issued : %d completed %d\n",(int)ci->ci_PrimitivesIssued,(int)ci->ci_PrimitivesCompleted ));

#ifdef SIS_NO_REAL_SYNCTOTOKEN
	
	sis620_waitengineidle();

#else

	while (st->counter > ci->ci_PrimitivesCompleted) {
		uint64 primitive_high = (uint64)inl(PRIMITIVE_HIGH_REGISTER); // reading in this order make sure about data consistency  (if both reading are not separed by more than 2^32-1 operations...)
		uint64 primitive_low = (uint64)inl(PRIMITIVE_LOW_REGISTER);
		uint64 new_primitive ;
		uint64 wait_time ;
		new_primitive = (primitive_high<<32) | (primitive_low);
		ci->ci_PrimitivesCompleted = new_primitive; // this writing should be atomic
		// *(double*)&ci->ci_PrimitivesCompleted = *(double*)&new_primitive ; // double uses 64 bits move which is atomic

		vvddprintf(("    issued : %d completed %d\n",(int)ci->ci_PrimitivesIssued,(int)ci->ci_PrimitivesCompleted ));
		
		// fastest is 400krect/s so 2.5 microsec per rect. let's say 2
		wait_time = 2* (st->counter-ci->ci_PrimitivesCompleted);
		if (wait_time > 500) snooze(500);
		else if (wait_time>20) snooze(wait_time); // spin ?
		}
#endif

	return(B_OK);
	}


////////////////////////////////////////
// -------- Wait Engine Idle -------- //
////////////////////////////////////////

void sis620_waitengineidle (void) {

#ifdef SIS_NO_REAL_SYNCTOTOKEN
	lockBena4(&ci->ci_EngineRegsLock);
#endif

	while ( !SIS_3D_IDLE_AND_3DQUEUE_EMPTY ) snooze(300);
	
	while( ! SIS_2D_IDLE_3D_IDLE_AND_QUEUE_EMPTY ) snooze(500);

#ifdef SIS_NO_REAL_SYNCTOTOKEN
	unlockBena4(&ci->ci_EngineRegsLock);
#endif

	}
