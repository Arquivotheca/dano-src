#include "sis630_Blit.h"
#include "sis630defs.h"

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

void sis630_send_orders(uint32 *buffer, int size) {
	bigtime_t time0 ;
	int pos = 0 ;
	int s = MAX_ORDERS_AT_A_TIME ;
	while(s) {
		int i;
		if (size-pos<MAX_ORDERS_AT_A_TIME) s = size-pos ;

		time0 = system_time();
			// wait for available space in queue
			while ( (inl(0x8240)&0xffff) < (s/2)+10 ) snooze(20);
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

int sis630_addcommand_write_synctoken(	uint32 *buffer_start ) {
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

int sis630_addcommand_blit(	uint32 *buffer_start,
						uint16 depth,
						uint32 src_addr, uint16 src_rowbytes,
						uint16 srcX, uint16 srcY,
						uint32 dst_addr, uint16 dst_rowbytes,
						uint16 dstX, uint16 dstY,
						uint16 width, uint16 height) {

	uint32 *buf = buffer_start ;
	uint32 command ;

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

	ci->ci_sis630_AGPBase &= ~(0x3<<14) ;
	if (depth ==  8 ) ci->ci_sis630_AGPBase |= (0x00<<14);
	if (depth == 16 ) ci->ci_sis630_AGPBase |= (0x02<<14);
	if (depth == 32 ) ci->ci_sis630_AGPBase |= (0x03<<14);

	*buf++ = 0x8204 ; *buf++ = (ci->ci_sis630_AGPBase<<16) | src_rowbytes ;
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

int sis630_addcommand_fill(	uint32* buffer_start, 
						uint16 depth,
						uint32 addr, uint16 rowbytes,
						uint16 startx, uint16 starty,
						uint16 width, uint16 height,
						uint32 color ) {

	uint32 *buf = buffer_start ;

	ci->ci_sis630_AGPBase &= ~(0x3<<14) ;
	if (depth ==  8 ) ci->ci_sis630_AGPBase |= (0x00<<14);
	if (depth == 16 ) ci->ci_sis630_AGPBase |= (0x02<<14);
	if (depth == 32 ) ci->ci_sis630_AGPBase |= (0x03<<14);

	*buf++ = 0x8204 ; *buf++ = (ci->ci_sis630_AGPBase<<16) ;

	// uses the trapezoid fill function
	*buf++= 0x8208 ; *buf++= (starty<<16) | (height+1) ; // Y start, trapezoid height
	*buf++= 0x820c ; *buf++= ((startx+width+1)<<16) | startx ;
	*buf++= 0x8210 ; *buf++= addr ;
	*buf++= 0x8214 ; *buf++= (0x1fff<<16) | rowbytes ; // height ?
  	*buf++= 0x821c ; *buf++= color ; // foreground color
  	*buf++= 0x8220 ; *buf++= color ; // background color
		
	*buf++= 0x8244 ; *buf++= 0x07ff0000 ; // deltaX/Y left
	*buf++= 0x8248 ; *buf++= 0x07ff0000 ; // deltaX/Y right
	*buf++= 0x824c ; *buf++= -1 ; // left error
	*buf++= 0x8250 ; *buf++= -1 ; // right error

	*buf++= 0x823c ; *buf++= 0x0000f005 ; // Command = Trapezoid Fill; Raster OP=copy pattern

	return(buf-buffer_start);
	}
	
//////////////////////////////////////////////////////
// -------- Fill buffer with INVERT orders -------- //
//////////////////////////////////////////////////////

int sis630_addcommand_invert(	uint32 *buffer_start,
						uint16 depth,
						uint32 addr, uint16 rowbytes,
						uint16 left, uint16 top,
						uint16 right, uint16 bottom ) {

	uint32 *buf = buffer_start ;


	ci->ci_sis630_AGPBase &= ~(0x3<<14) ;
	if (depth ==  8 ) ci->ci_sis630_AGPBase |= (0x00<<14);
	if (depth == 16 ) ci->ci_sis630_AGPBase |= (0x02<<14);
	if (depth == 32 ) ci->ci_sis630_AGPBase |= (0x03<<14);

	*buf++ = 0x8200 ; *buf++ = addr ;
	*buf++ = 0x8204 ; *buf++ = (ci->ci_sis630_AGPBase<<16) | rowbytes ;
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

void sis630_screen_to_screen_blit (engine_token *et, blit_params *list, uint32 count) {
	int size = 0;
	if (et != &enginetoken) return;

	ci->ci_PrimitivesIssued ++;

	while(count--) {
		uint32 buffer[BUFF_MAX];
		size += sis630_addcommand_blit(buffer+size,
						ci->ci_Depth,
						ci->ci_FBBase_offset, ci->ci_BytesPerRow,
						list->src_left, list->src_top,
						ci->ci_FBBase_offset, ci->ci_BytesPerRow,
						list->dest_left, list->dest_top,
						list->width, list->height
						);
		if ( (!count) | (size>=BUFF_MAX-BUFF_MIN) ) {
			if (count==0) size+=sis630_addcommand_write_synctoken(buffer+size);	
			sis630_send_orders(buffer,size);
			size = 0;
			}
		list++;
		}
	}

// -------- Fill Rectangle --------

// Temporary Fix : Fill doesn't seem to work with height >= 128

int fixed_sis630_addcommand_fill(	uint32* buffer_start, 
						uint16 depth,
						uint32 addr, uint16 rowbytes,
						uint16 startx, uint16 starty,
						uint16 width, uint16 height,
						uint32 color ) {
	int size = 0 ;
	int h = height ;
	while (h>=0) {
		height = h;
		if (height>125) height=125 ;
		size += sis630_addcommand_fill( buffer_start+size,
					depth,
					addr, rowbytes,
					startx, starty,
					width, height,
					color );
		starty += height+1 ;
		h -= height+1 ;
		}
	return(size);
	}

void sis630_fill_rectangle (engine_token *et, uint32 color, fill_rect_params *list,uint32	count){
	int size = 0;
	if (et != &enginetoken) return;

	ci->ci_PrimitivesIssued ++;

	while(count--) {
		uint32 buffer[4*BUFF_MAX]; // Fixme : due to degrouping of fills

		// Fix SiS630 : height over 128 invalid ??? #?O!?
		uint16 height, starty ;
		starty = list->top ;		

		while (starty <= list->bottom ) {
			height = list->bottom - starty ;

			if ( height > 125 ) height = 125 ;
	
			size += sis630_addcommand_fill(buffer+size,
						ci->ci_Depth,
						ci->ci_FBBase_offset,
						ci->ci_BytesPerRow,
						list->left, starty ,
						list->right-list->left, height,
						color
						);
			if ( (!count) | (size>=BUFF_MAX-BUFF_MIN) ) {
				if (count==0) size+=sis630_addcommand_write_synctoken(buffer+size);	
				sis630_send_orders(buffer,size);
				size = 0;
				}
			
			starty += height+1 ;
			}

		list++;
		} // end count--
	}


// -------- Invert Rectangle --------

void sis630_invert_rectangle (engine_token *et, fill_rect_params *list, uint32 count) {
	int size = 0;
	if (et != &enginetoken) return;

	ci->ci_PrimitivesIssued ++;

	while(count--) {
		uint32 buffer[BUFF_MAX];
		size += sis630_addcommand_invert(buffer+size,
						ci->ci_Depth,
						ci->ci_FBBase_offset,
						ci->ci_BytesPerRow,
						list->left, list->top,
						list->right, list->bottom
						);
		if ( (!count) | (size>=BUFF_MAX-BUFF_MIN) ) {
			if (count==0) size+=sis630_addcommand_write_synctoken(buffer+size);	
			sis630_send_orders(buffer,size);
			size = 0;
			}
		list++;
		}		
	}


/////////////////////////////////////
// -------- Sync To Token -------- //
/////////////////////////////////////

status_t sis630_synctotoken (sync_token *st) {
	vvddprintf(("synctotoken(%d)\n",(int)st->counter ));
	vvddprintf(("issued : %d completed %d\n",(int)ci->ci_PrimitivesIssued,(int)ci->ci_PrimitivesCompleted ));

	while (st->counter > ci->ci_PrimitivesCompleted) {
		uint64 primitive_high = (uint64)inl(PRIMITIVE_HIGH_REGISTER); // reading in this order make sure about data consistency  (if both reading are not separed by more than 2^32-1 operations...)
		uint64 primitive_low = (uint64)inl(PRIMITIVE_LOW_REGISTER);
		uint64 new_primitive ;
		uint64 wait_time ;
		new_primitive = (primitive_high<<32) | (primitive_low);
		ci->ci_PrimitivesCompleted = new_primitive; // this writing should be atomic
		// *(double*)&ci->ci_PrimitivesCompleted = *(double*)&new_primitive ; // double uses 64 bits move which is atomic

		vvddprintf(("    issued : %d completed %d\n",(int)ci->ci_PrimitivesIssued,(int)ci->ci_PrimitivesCompleted ));
		
		// fastest is 400krect/s e.g. 2.5 microsec per rect. let's say 2
		wait_time = 2* (st->counter-ci->ci_PrimitivesCompleted);
		if (wait_time > 500) snooze(500);
		else if (wait_time>20) snooze(wait_time); // spin ?
		}

	return(B_OK);
	}


////////////////////////////////////////
// -------- Wait Engine Idle -------- //
////////////////////////////////////////

void sis630_waitengineidle (void) {

	while ( !( inl(0x8240)&(1<<21) ) ) snooze(300);
	
	}

/////////////////////////////////////////
// -------- Move Display Area -------- //
/////////////////////////////////////////

status_t	sis630_Move_DisplayArea(int16 screenX, int16 screenY) {
	register display_mode	*dm;
	uint32			addr;
	int			bytespp;
	uchar		t;
	
	dm = &ci->ci_CurDispMode;

	if (screenX + dm->timing.h_display > dm->virtual_width  ||
	    screenY + dm->timing.v_display > dm->virtual_height) {
	    ddprintf(("sis : Move_Display_Area refused\n"));
		return (B_ERROR);
		}

	bytespp	= (ci->ci_Depth + 7) >> 3;
	addr	= ci->ci_FBBase_offset ;
	addr	+= (screenY * dm->virtual_width + screenX ) * bytespp;
	addr	= (addr>>2) ; // Unit of DW
	
	outl( 0x8540 , addr ) ;
	return (B_OK);
	}

