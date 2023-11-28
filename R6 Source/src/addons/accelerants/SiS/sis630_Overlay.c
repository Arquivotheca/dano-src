#include "accelerant.h"
#include "sisOverlay.h"

#include <math.h>

extern sis_card_info	*ci;

#define isa_outb(a,b) write_isa_io(0,(char*)(a),1,(b))
#define isa_inb(a)     read_isa_io(0,(char*)(a),1)

// 12 bpp : YUV 420

static uint32 overlay_spaces[] =
	{ B_YCbCr422,	B_RGB15_LITTLE,	B_RGB16_LITTLE,	B_YUV422,	B_NO_COLOR_SPACE };
static uint16      pitch_max[] =
	{ 4092,			4092, 			4092, 			4092, 		4092 };

void SetContrast(float c) ;
void SetBrightness(int b) ;
void SetOverlaySpace(uint32 space);
int bytes_per_pixel(color_space cs);


uint32 OVERLAY_COUNT(const display_mode *dm) {
	vddprintf(("sis : Overlay_Count()\n"));
	return 1;
	}

const uint32 *OVERLAY_SUPPORTED_SPACES(const display_mode *dm) {
	vddprintf(("sis : Overlay_Supported_Spaces()\n"));
	return overlay_spaces;
	}

uint32 OVERLAY_SUPPORTED_FEATURES(uint32 a_color_space) {
	uint32 result;
	vddprintf(("sis : Overlay_Supported_Features()\n"));


	switch (a_color_space) {
		case B_YCbCr422:
		case B_RGB15_LITTLE:
		case B_RGB16_LITTLE:
		case B_YUV422:
			result =
				B_OVERLAY_COLOR_KEY | 
				B_OVERLAY_CHROMA_KEY |
				B_OVERLAY_HORIZONTAL_FITLERING |
				B_OVERLAY_VERTICAL_FILTERING ;
			break;
		default:
			result = 0;
			break;
		}
	return result;
	}

overlay_token ALLOCATE_OVERLAY(void) {
	uchar t;
	int i ;

	vddprintf(("sis : Allocate_Overlay()\n"));

	if (ci->ci_Depth==32) {
		ddprintf(("sis : no overlay supported in 32bpp mode\n"));
		return 0;
		}
		
	// disable "Merge Video line buffer into CRT FIFO"
	lockBena4 (&ci->ci_SequencerLock);
	isa_outb(SEQ_INDEX,0x07);
	t = isa_inb(SEQ_DATA);
	isa_outb(SEQ_DATA, t&~0x80);
	unlockBena4 (&ci->ci_SequencerLock);
		
	// maybe test read-only CRTC index 0x26 bit D5 (0x20) : video enable
	
	// video compatible hardware cursor visibility enable : SEQ index 0x23 D3
	
	for(i=0; i<MAX_OVERLAYS;i++) if (ci->ovl_token[i].used==0) break;
	if (i >= MAX_OVERLAYS) {
		vddprintf(("sis : ALLOCATE_OVERLAY: no more overlays\n"));
		return 0;
		}
	ci->ovl_token[i].used = 1;
	vddprintf(("ALLOCATE_OVERLAY: returning token %d\n", i));
	return ( & ci->ovl_token[i] );
	}
	
status_t RELEASE_OVERLAY(overlay_token ot) {
	uchar t;
	int i;
	
	vddprintf(("sis : Release_Overlay()\n"));
	// disabled for the moment (not set on w*)
	// enable "Merge Video line buffer into CRT FIFO"
	//
	//lockBena4 (&ci->ci_SequencerLock);
	//isa_outb(SEQ_INDEX,0x07);
	//t = isa_inb(SEQ_DATA);
	//isa_outb(SEQ_DATA, t|0x80);
	//unlockBena4 (&ci->ci_SequencerLock);
	
	i = (sis_overlay_token*)ot - ci->ovl_token ;
	if ((i < 0) || (i >= MAX_OVERLAYS)) {
		ddprintf(("sis : Release_Overlay(): bad overlay_token\n"));
		return B_ERROR;
		}
	if (ci->ovl_token[i].used==0) {
		ddprintf(("sis : Release_Overlay(): releasing unused overlay\n"));
		return B_ERROR;
		}
		
	ci->ovl_token[i].used=0;
	return(B_OK);
	}
	
const overlay_buffer *ALLOCATE_OVERLAY_BUFFER(color_space cs, uint16 width, uint16 height) {
	int i;
	overlay_buffer *ob ;
		
	vddprintf(("sis : Allocate_Overlay_Buffer(%d, %d, %d)\n",cs,width, height));

	if (!width || !height) return 0;
	if (cs == B_NO_COLOR_SPACE) return 0;

	/* find an empty overlay buffer */
	for (i = 0; i < MAX_OVERLAY_BUFFERS; i++)
		if (ci->ovl_buffer[i].space == B_NO_COLOR_SPACE)
			break;
	if (i>=MAX_OVERLAY_BUFFERS) return 0;
	ob = &ci->ovl_buffer[i];

	/* validate color_space */
	i = 0;
	while (overlay_spaces[i]) {
		if (overlay_spaces[i] == cs) break;
		i++;
	}
	if (overlay_spaces[i] == B_NO_COLOR_SPACE) return 0;
	
	ob->bytes_per_row = bytes_per_pixel(cs) * width; 
	ob->space = cs;
	ob->width = width;
	ob->height = height;

	memset(&ci->ovl_buffer_memspec[i], 0, sizeof(ci->ovl_buffer_memspec[i]));
	ci->ovl_buffer_memspec[i].ms_PoolID			= ci->ci_PoolID;
	ci->ovl_buffer_memspec[i].ms_AddrCareBits	= 7;
	ci->ovl_buffer_memspec[i].ms_AddrStateBits	= 0;
	ci->ovl_buffer_memspec[i].ms_MR.mr_Size		= ob->bytes_per_row*ob->height ;
	ci->ovl_buffer_memspec[i].ms_AllocFlags		= 0 ;
	if ( BAllocByMemSpec(&ci->ovl_buffer_memspec[i]) < 0 ) {
		ddprintf(("sis : couldn't allocate %d kb overlay buffer\n", (int)(ci->ovl_buffer_memspec[i].ms_MR.mr_Size>>10) ));
		return(0);
		}
	ob->buffer 		= ci->ci_BaseAddr0 		+ (uint32)ci->ovl_buffer_memspec[i].ms_MR.mr_Offset ;
	ob->buffer_dma	= ci->ci_BaseAddr0_DMA	+ (uint32)ci->ovl_buffer_memspec[i].ms_MR.mr_Offset ;

	vddprintf(("sis : overlay buffer video address is 0x%08x\n",(uint)ci->ovl_buffer_memspec[i].ms_MR.mr_Offset));
	vddprintf(("sis : overlay buffer base address is 0x%08x (DMA 0x%08x)\n",(uint)ob->buffer,(uint)ob->buffer_dma));

	return(ob);
	}
	
status_t RELEASE_OVERLAY_BUFFER(const overlay_buffer *_ob) {
	overlay_buffer *ob = (overlay_buffer*) _ob ;
	int i = ob - ci->ovl_buffer;

	vddprintf(("sis : Release_Overlay_Buffer()\n"));

	// validated buffer
	if ((i < 0) || (i >= MAX_OVERLAY_BUFFERS)) {
		ddprintf(("RELEASE_OVERLAY_BUFFER: bad buffer pointer\n"));
		return B_ERROR;
		}
	if (ob->space == B_NO_COLOR_SPACE) {
		ddprintf(("RELEASE_OVERLAY_BUFFER: releasing unallocated buffer\n"));
		return B_ERROR;
		}

	// free the memory
	BFreeByMemSpec ( &ci->ovl_buffer_memspec[i] ) ;
	
	// mark as empty
	ob->space = B_NO_COLOR_SPACE;
	
	vddprintf(("sis : Release_Overlay_Buffer: OK\n"));
	return B_OK;
	}


/////////////////////////////	
// Get Overlay Constraints //
/////////////////////////////

status_t GET_OVERLAY_CONSTRAINTS(const display_mode *dm, const overlay_buffer *ob, overlay_constraints *oc) {

	vddprintf(("sis : Get_Overlay_Constraint()\n"));

	/* minimum size of view */
	oc->view.width.min = 1;
	oc->view.height.min = 1;
	/* view may not be larger than the buffer or 1024, which ever is lower */
	oc->view.width.max = ob->width > 1024 ? 1024 : ob->width;
	oc->view.height.max = ob->height > 1024 ? 1024 : ob->height;
	/* view alignment */
	oc->view.h_alignment = 0;
	oc->view.v_alignment = 0;
	oc->view.width_alignment = 0;
	oc->view.height_alignment = 0;
	
	/* minium size of window */
	oc->window.width.min = 1;
	oc->window.height.min = 1;
	/* upper usefull size is limited by screen realestate */
	oc->window.width.max = 1600;
	oc->window.height.max = 1200;
	/* window alignment */
	oc->window.h_alignment = 0;
	oc->window.v_alignment = 0;
	oc->window.width_alignment = 0;
	oc->window.height_alignment = 0;
	
	/* overall scaling factors */
	oc->h_scale.min = 1.0 / 32.0;
	oc->v_scale.min = 1.0 / 32.0;
	oc->h_scale.max = 32.0;	/* a lie, but a convienient one */
	oc->v_scale.max = 32.0; /* being accurate requires knowing either the window or the view size exactly */

	return B_OK;
	}
	
status_t CONFIGURE_OVERLAY(overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov) {
	int offset ;
	uchar t;
	uint32 addr = 0;
	int32 left, right, top, bottom ;
	float h_factor, v_factor ;
	uint16 hsff, hsfi, vusf ;	// Horizontal Scaling Factor Fraction/Integer
								// Vertical Up-Scaling Factor
	int new_width ;	

	sis_overlay_token *sis_ot = (sis_overlay_token*) ot ;
	int i = sis_ot - &(ci->ovl_token[0]);

	vddprintf(("sis : Configure Overlay()\n"));

	// validate the overlay token
	if ((i < 0) || (i >= MAX_OVERLAYS)) {
		ddprintf(("sis : Configure_Overlay(): invalid overlay_token\n"));
		return B_ERROR;
		}
	
	if (!sis_ot->used) {
		ddprintf(("sis : Configure_Overlay(): trying to configure un-allocated overlay\n"));
		return B_ERROR;
		}


	// Initialization

	lockBena4 (&ci->ci_CRTCLock);
	
	isa_outb(CRTC_INDEX,0x80); // Unlock Video registers
	isa_outb(CRTC_DATA, 0x86);
	t=isa_inb(CRTC_DATA);
	if (t!=0xa1) ddprintf(("sis : Video not unlocked, code should be A1 and is 0x%02x\n",t));

	//isa_outb(CRTC_INDEX, 0x98);
	//isa_outb(CRTC_DATA, 0x00) ; // disable video playback

	if ((ob == 0) || (ow == 0) || (ov == 0)) { // disable overlay
		vddprintf(("sis : disabling overlay\n"));
		isa_outb(CRTC_INDEX, 0x98);
		isa_outb(CRTC_DATA, 0x00) ; // disable video playback
		unlockBena4 (&ci->ci_CRTCLock);
		return B_OK;
		}

	// Zoom Factors
	
	h_factor = (float)(ow->width -ow->offset_right -ow->offset_left) / (float)ob->width ; 
	v_factor = (float)(ow->height -ow->offset_top -ow->offset_bottom) / (float)ob->height ;
	vddprintf(("sis : ovl_window(%d, %d) ovl_buffer(%d, %d)\n",(int)ow->width,(int)ow->height,(int)ob->width,(int)ob->height));
	vddprintf(("sis : after offsets : ovl_window(%d, %d) \n",(int)ow->width -ow->offset_right -ow->offset_left,(int)ow->height-ow->offset_top -ow->offset_bottom));
	vddprintf(("sis : required zoom factors : h=%d/1000 v=%d/1000\n",(int)(1000*h_factor),(int)(1000*v_factor) ));

	// Horizontal Zoom 
		
	// formula is h_factor = 1 / (hsfi + hsff/64)
	if (h_factor>1) { // up-scaling
		hsff = (uint16) ceil(64.0 / h_factor ); // +0.91 : manually tuned constant.... else doesn't work !!!
		hsfi = 0 ;
		new_width = (int) ceil((ob->width*64.0)/hsff);
		}
	else { // down-scaling
		hsfi = (uint16) floor( 1 / h_factor );
		hsff = (uint16) ceil( 64.0/h_factor - 64.0*hsfi ) ; // +1.90 ?
		new_width = (int) ceil( ob->width * (1.0 / ( hsfi + hsff/64.0) ) );
		}
	vddprintf(("sis :     hsff=%d hsfi=%d\n",(int)hsff,(int)hsfi));
	if (hsff>=64) {
		hsff=63;
		vddprintf(("sis :     hsff sature a 63...\n"));
		}
	vddprintf(("sis :     hmmmm=%d\n",(int)floor(3.1)));

	vddprintf(("sis :     h_factor=%d/1000\n",(int)(1000 /(hsfi + hsff/64.0) ) ));
	isa_outb(CRTC_INDEX, 0x92);
	isa_outb(CRTC_DATA, 0x80 | hsff ); 	// 2-phase interpolation
										// 0x00 : replication 
										// 0x40 : 2-phase
										// 0x80 : 4-phase
										// 0xc0 : 8-phase
	isa_outb(CRTC_INDEX, 0x94);
	t = 0xf0 & isa_inb(CRTC_DATA) ;
	isa_outb(CRTC_DATA, hsfi | t );
	
	// Vertical Zoom and Offset

	vddprintf(("sis : bytes_per_row = %d\n",ob->bytes_per_row));
	offset = ob->bytes_per_row; // 32 bits

	while (v_factor<1) {
		offset *= 2;
		v_factor *=2 ;
		} 
	vddprintf(("sis : offset = %d\n",offset));
	offset = offset >> 2 ; // 32 bits
	isa_outb(CRTC_INDEX, 0x8c);
	isa_outb(CRTC_DATA, offset&0xff );
	isa_outb(CRTC_INDEX, 0x8e);
	isa_outb(CRTC_DATA, (offset>>8)&0x0f );

	if (v_factor==1) {
		vusf = 0;
		}
	else if (v_factor>1) {
		vusf = (int) (64.0 / v_factor) ;
		}

	isa_outb(CRTC_INDEX,0x93);
	t= 0xc0 & isa_inb(CRTC_DATA);
	isa_outb(CRTC_DATA, t | vusf );
	
	// Overlay Window

	left = ow->h_start + ow->offset_left ;
	left += ((ow->width -ow->offset_right -ow->offset_left) - new_width )/2; // center gray borders

	if (left<0) {
		float hor_move = (-left) * bytes_per_pixel(ob->space) ;
		addr += ( (uint)(hor_move / h_factor) ) & ~(bytes_per_pixel(ob->space)-1) ;
		left=0;
		}
	else if (left >= ci->ci_CurDispMode.timing.h_display-1) left=ci->ci_CurDispMode.timing.h_display-2;
	
	// right = ow->h_start + ow->width - 1 - ow->offset_right ;
	// recomputed right :
	right = ow->h_start + new_width ; 
	right += ((ow->width -ow->offset_right -ow->offset_left) - new_width )/2; // center gray borders
	vddprintf(("sis : new width = %d\n",(int)new_width));
	if (right<=0) right=1;
	else if ((float)right >= ci->ci_CurDispMode.timing.h_display*1.13) right=(int)(ci->ci_CurDispMode.timing.h_display*1.13)-1;
	
	top = ow->v_start + ow->offset_top ;
	if (top<0) top=0;
	else if (top >= ci->ci_CurDispMode.timing.v_display-1) top=ci->ci_CurDispMode.timing.v_display-2;

	bottom = ow->v_start + ow->height - 1 - ow->offset_bottom ;
	if (bottom<=0) bottom=1;
	else if (bottom >= ci->ci_CurDispMode.timing.v_display) bottom=ci->ci_CurDispMode.timing.v_display-1;
	
	vddprintf(("sis : overlay window (<l=%d, t=%d> to <r=%d, b=%d>)\n", left, top, right, bottom));

	isa_outb(CRTC_INDEX,0x83);
	isa_outb(CRTC_DATA, ((left>>8)&0x07) | ((right>>4)&0x70) );
	isa_outb(CRTC_INDEX,0x82);
	isa_outb(CRTC_DATA, right & 0xff);
	isa_outb(CRTC_INDEX,0x81);
	isa_outb(CRTC_DATA, left & 0xff);
	
	isa_outb(CRTC_INDEX,0x86);
	isa_outb(CRTC_DATA, ((top>>8)&0x07) | ((bottom>>4)&0x70) );
	isa_outb(CRTC_INDEX,0x85);
	isa_outb(CRTC_DATA, bottom & 0xff);
	isa_outb(CRTC_INDEX,0x84);
	isa_outb(CRTC_DATA, top & 0xff);

	
	// Video Playback threshold
	
	isa_outb(CRTC_INDEX, 0x9e);
	isa_outb(CRTC_DATA, 0x50); // low
	isa_outb(CRTC_INDEX, 0x9f);
	isa_outb(CRTC_DATA, 0x7f); // high

	// Line Buffer Size limit
	
	isa_outb(CRTC_INDEX, 0xa0);
	isa_outb(CRTC_DATA, 0x70); // sis 620

	SetOverlaySpace(ob->space);
	
	// Color Keying
	
	if (ow->flags & B_OVERLAY_COLOR_KEY) {
		switch (ci->ci_CurDispMode.space) {
			case B_RGB32_LITTLE:
				isa_outb(CRTC_INDEX, 0xa1); // high values
				isa_outb(CRTC_DATA, ow->blue.value | ~ow->blue.mask );
				isa_outb(CRTC_INDEX, 0xa2);
				isa_outb(CRTC_DATA, ow->green.value | ~ow->green.mask );
				isa_outb(CRTC_INDEX, 0xa3);
				isa_outb(CRTC_DATA, ow->red.value | ~ow->red.mask );

				isa_outb(CRTC_INDEX, 0x95); // low values
				isa_outb(CRTC_DATA, ow->blue.value & ow->blue.mask );
				isa_outb(CRTC_INDEX, 0x96);
				isa_outb(CRTC_DATA, ow->green.value & ow->green.mask );
				isa_outb(CRTC_INDEX, 0x97);
				isa_outb(CRTC_DATA, ow->red.value & ow->red.mask );

				isa_outb(CRTC_INDEX, 0xa9);
				isa_outb(CRTC_DATA, 0x0f); // temporary : no color keying - always on
				break ;
			case B_RGB16_LITTLE: {
				uint16 mask = (((uint16)ow->red.mask & 0x1f) << 11) | (((uint16)ow->green.mask & 0x3f) << 5) | (((uint16)ow->blue.mask & 0x1f));
				uint16 value = (((uint16)ow->red.value & 0x1f) << 11) | (((uint16)ow->green.value & 0x3f) << 5) | (((uint16)ow->blue.value & 0x1f));
				vddprintf(("sis : color key 16 bpp : mask=0x%08x value=0x%08x\n",mask,value));
				isa_outb(CRTC_INDEX, 0xa1); // high value
				isa_outb(CRTC_DATA, (value|~mask) &0xff);
				isa_outb(CRTC_INDEX, 0xa2);
				isa_outb(CRTC_DATA, ((value|~mask)>>8) &0xff);
				isa_outb(CRTC_INDEX, 0x95); // low value
				isa_outb(CRTC_DATA, (value&mask)&0xff );
				isa_outb(CRTC_INDEX, 0x96);
				isa_outb(CRTC_DATA, ((value&mask)>>8)&0xff );

				isa_outb(CRTC_INDEX, 0xa9);
				isa_outb(CRTC_DATA, 0x03); // Color Keying
				}
				break ;
			case B_CMAP8: {
				uint8 mask = (ow->red.mask | ow->green.mask | ow->blue.mask);
				uint8 value = (ow->red.value | ow->green.value | ow->blue.value);
				isa_outb(CRTC_INDEX, 0xa1); // high value
				isa_outb(CRTC_DATA, value | ~mask );
				isa_outb(CRTC_INDEX, 0x95); // low value
				isa_outb(CRTC_DATA, value & mask );
				// in fact all this may result into low=high=255 which is the transparent color...
				vddprintf(("sis : CMAP8 color key low=%d high=%d\n",(int)(value&mask), (int)(value|~mask) ));
				isa_outb(CRTC_INDEX, 0xa9);
				isa_outb(CRTC_DATA, 0x03); // color keying only
				}
				break ;	
			}
		}
	else {
		isa_outb(CRTC_INDEX, 0xa9);
		isa_outb(CRTC_DATA, 0x0f);
		}

	// Contrast
	
	SetContrast(1.3);
	
	// Brightness
	
	SetBrightness(0);

	// Special SIS620 (ONLY !!! : not SiS530 !!!): disable SGRAM burst timing
	// but this is necessary for 32 bpp mode
	// SiS620 : rev 2a
	// SiS530 : rev a2
	if ((ci->ci_DeviceId==SIS620_DEVICEID)
		&&(ci->ci_Device_revision != 0xa2)
		&&(ci->ci_Depth!=32)
		) {
		isa_outb(SEQ_INDEX, 0x35);
		t=isa_inb(SEQ_DATA);
		isa_outb(SEQ_DATA, t|0x20); // disable SGRAM burst timings
		}
		
	// Set Overlay Buffer address
	
	addr += (uint32)ob->buffer - (uint32)ci->ci_BaseAddr0 ;
	addr = addr >> 2 ; // 32 bits
	vddprintf(("sis : overlay buffer address is 0x%08x (%d kb)\n",addr,addr>>10));
	
	isa_outb(CRTC_INDEX, 0x8a);
	isa_outb(CRTC_DATA, addr&0xff );
	isa_outb(CRTC_INDEX, 0x8b);
	isa_outb(CRTC_DATA, (addr>>8)&0xff);

	isa_outb(CRTC_INDEX, 0x89); // last to take effect
	switch(ci->ci_DeviceId) {
		case SIS6326_DEVICEID:
			isa_outb(CRTC_DATA, (addr>>12)&0xf0);
			break;
		case SIS620_DEVICEID:
			isa_outb(CRTC_DATA, (addr>>13)&0xf8);
			break;
		} ;

	// Start

	isa_outb(CRTC_INDEX, 0x98);
	t=isa_inb(CRTC_DATA);
	isa_outb(CRTC_DATA, t | 0x02) ; // enable video playback

	unlockBena4 (&ci->ci_CRTCLock);
	return(B_OK);
	}

void SetOverlaySpace(uint32 space) {
	uchar t;
	isa_outb(CRTC_INDEX,0x93);
	t=isa_inb(CRTC_DATA);
	switch(space) {
		case B_YCbCr422:
			vddprintf(("sis : SetOverlaySpace(B_YCbCr422)\n"));
			t&=~0x40;
			t|=0x80;
			goto set_YUV;
			break;
		case B_YUV422:
			vddprintf(("sis : SetOverlaySpace(B_YUV422)\n"));
			t&=~0xc0;
			goto set_YUV;
			break;
		case B_RGB15_LITTLE:
			vddprintf(("sis : SetOverlaySpace(B_RGB15)\n"));
			t&=~0xc0;
			goto set_RGB;
			break;
		case B_RGB16_LITTLE:
			vddprintf(("sis : SetOverlaySpace(B_RGB16)\n"));
			t|=0x40;
			t&=~0x80;
			goto set_RGB;
			break;
		default:
			ddprintf(("sis : SetOverlaySpace() failed - unsupported mode\n"));
			return;
		}
set_YUV:
	isa_outb(CRTC_DATA, t);

	isa_outb(CRTC_INDEX, 0x98);
	t=isa_inb(CRTC_DATA);
	t|=0x40;
	isa_outb(CRTC_DATA,t);
	return;

set_RGB:
	isa_outb(CRTC_DATA, t);

	isa_outb(CRTC_INDEX, 0x98);
	t=isa_inb(CRTC_DATA);
	t&=~0x40;
	isa_outb(CRTC_DATA,t);
	return;
	}
	

void SetContrast(float c) {
	uint8 step ;
	if (c<1.0) c=1.0;
	if (c>1.4375) c=1.4375;
	step=16.0*(c-1.0) ;
	isa_outb(CRTC_INDEX, 0xb5);
	isa_outb(CRTC_DATA, step & 0x07 );
	}
	
void SetBrightness(int b) {
	char t;
	if (b>127) t=127;
	else if (b<-128) b=-128;
	else t=(char)b;
	isa_outb(CRTC_INDEX, 0xb4);
	isa_outb(CRTC_DATA, t);
	}

int bytes_per_pixel(color_space cs) {
	switch(cs) {
		case B_YCbCr422:
		case B_RGB15_LITTLE:
		case B_RGB16_LITTLE:
			return(2);
			break;
		default:
			ddprintf(("sis : warning !!! bytes_per_pixel unknown for mode %d\n",(int)cs));
			return(2);
			break;
		}
	}
	
