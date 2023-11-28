#include "sisBlit.h"

extern engine_token enginetoken;

//////////////////////////////////////////////////
// -------- Sends orders to the engine -------- //
//////////////////////////////////////////////////

void sis_send_orders(uint32 *buffer, int size) {
	int i;
	// wait for available space in queue
	while(inw(0x82a8)<size) snooze(20); // spin ?
	// sends the orders
	for(i=0;i<size;i+=2) outl(buffer[i],buffer[i+1]);
	}

// Notes for SiS 5598, 6326AGP graphics engine (0x8280-0x82a8) :
// (1) width / height : last pixel is included. So in fact draw width+1 pixel
//     so a blit with width=right-left will include the left pixel and the right pixel
// (2) Buff max defines the size of the buffer of orders before we send them
//     as the function will wait to have this amount of free place in the queue,
//     Buff_Max should not be more than the queue size
//     which is 32 for hardware queue only
//     and 30k for software queue
//     So NO BLIT MORE THAN (BUFF_MIN-2) ORDERS
// (3) for the moment, the <mono mask> register, 8298-829f is used to store synctokens
//     pay attention to this if this register someday needs to be used
// (4) register 0x82a8 is supposed to represent the tail of the software queue
//     this should be different as representing the available length of the software queue
//     but as it works this way (on SiS6326), there's no need to change it right now


#define BUFF_MAX 200
#define BUFF_MIN 20

/////////////////////////////////////////////////////
// -------- add command <write synctoken> -------- //
/////////////////////////////////////////////////////

int addcommand_write_synctoken(	uint32 *buffer_start
								) {
	uint32 *buf = buffer_start ;
	uint32 *primitive_low, *primitive_high ;
	
	primitive_low = (uint32*)&ci->ci_PrimitivesIssued ;
	primitive_high = primitive_low + 1 ;
	
	*buf++= 0x8298 ; *buf++= *primitive_low ;
	*buf++= 0x829c ; *buf++= *primitive_high ;
	 
	return(buf-buffer_start);
	}
	
////////////////////////////////////////////////////
// -------- fill buffer with BLIT orders -------- //
////////////////////////////////////////////////////

int addcommand_blit(	uint32 *buffer_start,
						uint32 src_addr, uint16 src_rowbytes,
						uint32 dst_addr, uint16 dst_rowbytes,
						uint16 width, uint16 height) {
	uint32 *buf = buffer_start ;

	uint32 command = 0x0002; // BitBlt

	if (ci->ci_Depth==16) width=2*width+1;

	if (src_addr>dst_addr) {
		command|=0x30; // counters increase
		}
	else {
		src_addr +=width + height*src_rowbytes;
		dst_addr +=width + height*dst_rowbytes;
		}
	
	*buf++= 0x8280 ; *buf++= src_addr; // source
	*buf++= 0x8284 ; *buf++= dst_addr; // destination
	*buf++= 0x8288 ; *buf++= (((uint32)dst_rowbytes)<<16) | src_rowbytes; // pitch
	*buf++= 0x828c ; *buf++= (((uint32)height)<<16)|width; // size
	*buf++= 0x8290 ; *buf++= 0xcc000000 ; // foreground rop(copy source)
	*buf++= 0x8294 ; *buf++= 0xcc000000 ; // background
	*buf++= 0x82aa ; *buf++= command ; // start (check for flags)

	return(buf-buffer_start);
	}


////////////////////////////////////////////////////
// -------- fill buffer with FILL orders -------- //
////////////////////////////////////////////////////

int addcommand_fill(	uint32* buffer_start, 
						uint32 addr, uint16 rowbytes,
						uint16 width, uint16 height,
						uint16 color ) {
	uint32 *buf = buffer_start ;
	
	if (ci->ci_Depth==16) width=width*2+1; // needs 1 more for 16 bpp mode
	
	*buf++= 0x8280 ; *buf++= 0; // source
	*buf++= 0x8284 ; *buf++= addr; // destination
	*buf++= 0x8288 ; *buf++= ((uint32)rowbytes)<<16; // dst pitch
	*buf++= 0x828c ; *buf++= (((uint32)height)<<16)|width; // size
	*buf++= 0x8290 ; *buf++= 0x0c000000 | color; // foreground rop(invertcopy) & color
	*buf++= 0x8294 ; *buf++= 0x0c000000 ; // background
	*buf++= 0x82aa ; *buf++= 0x0031 ; // start (check for flags)
		
	return(buf-buffer_start);
	}

//////////////////////////////////////////////////////
// -------- Fill buffer with INVERT orders -------- //
//////////////////////////////////////////////////////

int addcommand_invert(	uint32 *buffer_start,
						uint32 addr, uint16 rowbytes,
						uint16 width, uint16 height ) {
						
	uint32 *buf = buffer_start ;

	if (ci->ci_Depth==16) width=2*width+1;
	
	*buf++= 0x8280 ; *buf++= addr; // source
	*buf++= 0x8284 ; *buf++= addr; // destination
	*buf++= 0x8288 ; *buf++= (((uint32)rowbytes)<<16) | rowbytes; // pitch
	*buf++= 0x828c ; *buf++= (((uint32)height)<<16)|width; // size
	*buf++= 0x8290 ; *buf++= 0x03000000 ; // foreground rop(copy source)
	*buf++= 0x8294 ; *buf++= 0x03000000 ; // background
	*buf++= 0x82aa ; *buf++= 0x0032 ; // start (check for flags)

	return(buf-buffer_start);
	}

// -------- 2D *Classic* hooks --------

void sis_screen_to_screen_blit (engine_token *et, blit_params *list, uint32 count) {
	int size = 0;
	if (et != &enginetoken) return;

	ci->ci_PrimitivesIssued ++;

	while(count--) {
		uint32 buffer[BUFF_MAX];
		size += addcommand_blit(buffer+size,
						ci->ci_FBBase_offset + (list->src_left*ci->ci_Depth)/8 + list->src_top*ci->ci_BytesPerRow,
						ci->ci_BytesPerRow,
						ci->ci_FBBase_offset + (list->dest_left*ci->ci_Depth)/8 + list->dest_top*ci->ci_BytesPerRow,
						ci->ci_BytesPerRow,
						list->width, list->height
						);
		if ( (!count) | (size>=BUFF_MAX-BUFF_MIN) | (ci->ci_DeviceId==SIS5598_DEVICEID) ) {
			if (count==0) size+=addcommand_write_synctoken(buffer+size);	
			sis_send_orders(buffer,size);
			size = 0;
			}
		list++;
		}
	}

void sis_fill_rectangle (engine_token *et, uint32 color, fill_rect_params *list,uint32	count) {
	int size = 0;
	if (et != &enginetoken) return;

	ci->ci_PrimitivesIssued ++;

	while(count--) {
		uint32 buffer[BUFF_MAX];
		size += addcommand_fill(buffer+size,
						ci->ci_FBBase_offset + (list->left*ci->ci_Depth)/8 + list->top*ci->ci_BytesPerRow,
						ci->ci_BytesPerRow,
						list->right-list->left, list->bottom-list->top,
						color
						);
		if ( (!count) | (size>=BUFF_MAX-BUFF_MIN) | (ci->ci_DeviceId==SIS5598_DEVICEID) ) {
			if (count==0) size+=addcommand_write_synctoken(buffer+size);	
			sis_send_orders(buffer,size);
			size = 0;
			}
		list++;
		}
	}

void sis_invert_rectangle (engine_token *et, fill_rect_params *list, uint32 count) {
	int size = 0;
	if (et != &enginetoken) return;

	ci->ci_PrimitivesIssued ++;

	while(count--) {
		uint32 buffer[BUFF_MAX];
		size += addcommand_invert(buffer+size,
						ci->ci_FBBase_offset + (list->left*ci->ci_Depth)/8 + list->top*ci->ci_BytesPerRow,
						ci->ci_BytesPerRow,
						list->right-list->left, list->bottom-list->top
						);
		if ( (!count) | (size>=BUFF_MAX-BUFF_MIN) | (ci->ci_DeviceId==SIS5598_DEVICEID) ) {
			if (count==0) size+=addcommand_write_synctoken(buffer+size);	
			sis_send_orders(buffer,size);
			size = 0;
			}
		list++;
		}		
	}


/////////////////////////////////////
// -------- Sync to Token -------- //
/////////////////////////////////////

// The reading of the 0x8298-0x829f register, and writing it into ci->ci_PrimitivesCompleted
// is not atomic. So ci->ci_PrimitivesCompleted is not garanteed to be up to date when you read it.
// This is not a problem, because the synctotoken will just update until it as the required value,
// and the value contained in 0x8298-0x829f is in any case the right one.
// So it is just used as a "cache" for register 0x8298-0x829f.
// Although it is important to be sure that the value contained in ci_PrimitivesCompleted
// make senses, that is that writting any value in it is atomic.

status_t synctotoken (sync_token *st) {
	while (st->counter > ci->ci_PrimitivesCompleted) {
		uint64 primitive_low = (uint64)inl(0x8298);
		uint64 primitive_high = (uint64)inl(0x829c);
		uint64 new_primitive ;
		uint64 wait_time ;
		new_primitive = (primitive_high<<32) | (primitive_low);
		ci->ci_PrimitivesCompleted = new_primitive; // this writing should be atomic
		vvddprintf(("issued : 0x%08x completed 0x%08x\n",(uint32)ci->ci_PrimitivesIssued,(uint32)ci->ci_PrimitivesCompleted ));
		
		// fastest is 400krect/s so 2.5 microsec per rect. let's say 2
		wait_time = 2* (st->counter-ci->ci_PrimitivesCompleted);
		if (wait_time > 500) snooze(500);
		else if (wait_time>20) snooze(wait_time); // spin ?
		}
	return(B_OK);
	}


////////////////////////////////////////
// -------- Wait Engine Idle -------- //
////////////////////////////////////////

void sis_waitengineidle (void) {
	int t;
	vvddprintf(("sis_accelerant: B_WAITENGINEIDLE\n"));

	t=inb(0x82ab) ;
	while ( (t&0x40) || // GE busy or Harware command queue not empty
  		  (!(t&0x80))  ) // Hardware queue empty
  		{
  		snooze(100); // spin ?
  		t=inb(0x82ab);
  		}
	vvddprintf(("sis_accelerant: Engine is now idle\n"));
	}
