/*****************************************************************************
// 
// File:		 vesa.cpp
/
//	Description: add-on for graphics cards using s3 chips.
//
//	Copyright 1996, Be Incorporated
//
//****************************************************************************/

#ifdef __INTEL__
#include <OS.h>
#include <PCI.h>
#include <bioscall.h>

#include "vesa.h"

extern status_t _kset_dprintf_enabled_(int);
extern void _kdprintf_ (const char *format, ...);
#define ddprintf _kdprintf_

extern int read_isa_io (int bus, long address, int size);
extern int write_isa_io (int bus, long address, int size, int value);
#define control_graphics_card vesa_control_graphics_card
extern long control_graphics_card(ulong message,void *buf);

/* static variables */
static  vesa_state     state;


/* static tables */
#define VESA_MODE_1_COUNT  12
static uint16  vesa_modes_1[VESA_MODE_1_COUNT] = {
  0x101, 0x103, 0x105, 0x107, 0x111, 0x112, 0x114, 0x115, 0x117, 0x118, 0x11a, 0x11b
};

static uint8   be_vesa_modes_1[VESA_MODE_1_COUNT] = {
  0x00,  0x01,  0x02,  0x03,  0x05,  0x0a,  0x06,  0x0b,  0x07,  0x0c,  0x08,  0x0d
};

#define VESA_MODE_2_COUNT  4
static uint16  vesa_modes_2[VESA_MODE_2_COUNT] = {
  0x110, 0x113, 0x116, 0x119
};

static uint8   be_vesa_modes_2[VESA_MODE_2_COUNT] = {
  0x05,  0x06,  0x07,  0x08
};

#define BE_RES_COUNT 18
static uint16 be_res_defs[3*18] =
{
  640,  480,  8,
  800,  600,  8,
  1024, 768,  8,
  1280, 1024, 8,
  1600, 1200, 8,
  640,  480,  16,
  800,  600,  16,
  1024, 768,  16,
  1280, 1024, 16,
  1600, 1200, 16,
  640,  480,  32,
  800,  600,  32,
  1024, 768,  32,
  1280, 1024, 32,
  1600, 1200, 32,
  1152, 900,  8,
  1152, 900,  16,
  1152, 900,  32
};

/* Benaphore description (long for atomic lock, sem_id for semaphore). */
static sem_id	io_sem = -1;
static long	    io_lock;
static char     FAKE_BUFFER[4];

/* Create the benaphores (if possible) */
void init_locks()
{
	if (io_sem == -1) {
		io_lock = 0L;
		io_sem = create_sem(0,"vga io sem");
	}
}

/* Free the benaphore (if created) */
void dispose_locks()
{
	if (io_sem >= 0)
		delete_sem(io_sem);
}

/* Protect the access to the general io-registers by locking a benaphore. */
static void lock_io()
{
	int	old;

	old = atomic_add (&io_lock, 1);
	if (old >= 1) {
		acquire_sem(io_sem);	
	}	
}

/* Release the protection on the general io-registers */
static void unlock_io()
{
	int	old;

	old = atomic_add (&io_lock, -1);
	if (old > 1) {
		release_sem(io_sem);
	}
}	

static void set_color_map() {
  bios_args       ba;
  bios_data_block viblock;

  viblock.offset = 0;
  viblock.size = 768;
  viblock.dataptr = state.my_ct.data;
  viblock.next = NULL;
  
  ba.data = &viblock;
  ba.eax = 0x1012;
  ba.ebx = 0;
  ba.ecx = 256;
  ba.edx = 0;
  ba.sig = 'BIOS';
  ba.sum = user_bios_args_sum(&ba);
	
  call_bios(0x10, &ba);
}

static void print_mode_infos(vesa_mode *m)
{
  dprintf("Mode : 0x%03x [%4dx%4d, %2d bpp] -> ",
		  m->code, m->width, m->height, m->bits_per_pixel);
  if (m->be_code != 255)
	dprintf("Be %d\n", m->be_code);
  else
	dprintf("Unusable\n");
}

/* main entry of the vesa graphics driver. */
#pragma export on
long control_graphics_card(ulong message,void *buf)
{
  int             i, j, err, ecart, dx, dy, total, min, mode;
  uint8           *data;
  uint16          *mode_list;
  uint32          code, space_mask;
  pci_info        h;
  vesa_mode       *v_mode;
  bios_args       ba;
  vesa_info_t     vi;
  vesa_mode_info  vmi;
  bios_data_block viblock;
  
  err = B_NO_ERROR;
  switch (message) {

  case B_GET_GRAPHICS_CARD_INFO :
	if (state.cur_mode == 0xffff) {
	  ((graphics_card_info*)buf)->height = 1;
	  ((graphics_card_info*)buf)->width = 1;
	  ((graphics_card_info*)buf)->bits_per_pixel = 8;
	  ((graphics_card_info*)buf)->frame_buffer = (void*)FAKE_BUFFER;
	  ((graphics_card_info*)buf)->bytes_per_row = 0;
	}
	else {
	  v_mode = state.modes+state.cur_mode;
	  ((graphics_card_info*)buf)->height = v_mode->height;
	  ((graphics_card_info*)buf)->width = v_mode->width;
	  ((graphics_card_info*)buf)->bits_per_pixel = v_mode->bits_per_pixel;
	  ((graphics_card_info*)buf)->frame_buffer = (void*)(state.pci_base+v_mode->base_addr);
	  ((graphics_card_info*)buf)->bytes_per_row = v_mode->bytes_per_row;
	}
	((graphics_card_info*)buf)->flags = state.be_flags;
	((graphics_card_info*)buf)->rgba_order[0] = 'b';
	((graphics_card_info*)buf)->rgba_order[1] = 'g';
	((graphics_card_info*)buf)->rgba_order[2] = 'r';
	((graphics_card_info*)buf)->rgba_order[3] = 'a';
	((graphics_card_info*)buf)->version = 2;
	((graphics_card_info*)buf)->id = 0;
	break;
		
  case B_GET_GRAPHICS_CARD_HOOKS :
	for (i=0;i<B_HOOK_COUNT;i++)
	  ((graphics_card_hook*)buf)[i] = 0L;
	break;
	
  case B_OPEN_GRAPHICS_CARD :
	/* save PCI informations */
	state.pci_base = (uint8*)((graphics_card_spec*)buf)->screen_base;

	/* get and save the physical adress */
	for (j = 0; j<64 ; j++) {	  
	  if (get_nth_pci_info (j, &h) != B_NO_ERROR)
		break;
	  for(i=0; i<6; i++ )
		if (h.u.h0.base_registers[i] == (uint32)state.pci_base) {
		  state.phys_pci_base = (uint32)h.u.h0.base_registers_pci[i];
		  goto physical_found;
		}
	}
	state.phys_pci_base = 0;
	dprintf("Couldn't translate the virtual frame-buffer adress,");
  physical_found: 
	
	/* get version and ID strings */	
	viblock.offset = 0;
	viblock.size = sizeof(vi);
	viblock.dataptr = &vi;
	viblock.next = NULL;

	vi.sig[0] = 'V';
	vi.sig[1] = 'B';
	vi.sig[2] = 'E';
	vi.sig[3] = '2';
	
	ba.data = &viblock;
	ba.eax = 0x4f00;
	ba.edi = 0;	  
	ba.sig = 'BIOS';
	ba.sum = user_bios_args_sum(&ba);
	
	call_bios(0x10, &ba);

	/* If this call fails, then no hope to do anything with that card */
	if (ba.eax != 0x4f) {
	  ddprintf("BIOS unknown or too old. Can't be driven by the VESA driver\n");
	  err = B_ERROR;
	  break;
	}

	/* Print out strings informations */
	ddprintf("\n#### VESA BIOS EXTENDED INFOS ####\n");
	ddprintf("VBE signature = %c%c%c%c\n",  vi.sig[0], vi.sig[1], vi.sig[2], vi.sig[3]);
	ddprintf("VBE version   = %04x\n", vi.version);
	ddprintf("Capablilities = %08x\n", vi.cap);
	ddprintf("Total memory  = %d KB\n", vi.totalmemory*64);
	ddprintf("OEM name      = %s\n", far_to_charp(vi.OEMstringptr));
	ddprintf("Vendor name   = %s\n", far_to_charp(vi.vendorname));
	ddprintf("Product name  = %s\n", far_to_charp(vi.productname));
	ddprintf("Revision id   = %s\n\n", far_to_charp(vi.revisionstring));

	/* Translate the version id into likely capabilities */
	for (i=0; i<FUNCTION_COUNT; i++)
	  state.functions[i] = 0;
	if (vi.version == 0x0100) {
	  for (i=0; i<6; i++)
		state.functions[i] = 1;
	}
	else if (vi.version == 0x0101) {
	  for (i=0; i<8; i++)
		state.functions[i] = 1;
	}
	else if (vi.version == 0x0102) {
	  for (i=0; i<9; i++)
		state.functions[i] = 1;
	}
	else if ((vi.version>>8) >= 2) {
	  for (i=0; i<11; i++)
		state.functions[i] = 1;
	}
	else if ((vi.version>>8) == 1) {
	  ddprintf("This is an unknown subversion of version 1\n");
	  for (i=0; i<9; i++)
		state.functions[i] = 1;
	}
	else {
	  ddprintf("Unknown revision. Enable only minimal support\n");
	  for (i=0; i<6; i++)
		state.functions[i] = 1;
	}

	/* Translate the capabilities infos */
	state.capabilities = vi.cap;

	/* memorize the graphic modes available */
	state.mode_count = 0;
	mode_list = (uint16*)far_to_charp(vi.modeptr);
	for (i=0; i<MODE_COUNT_MAX; i++) {
	  if (mode_list[i] == 0xffff)
		break;
	  if (mode_list[i] & 0x0100)
		state.modes[state.mode_count++].code = mode_list[i]; 
	}
	if (state.mode_count == 0) {
	  ddprintf("This BIOS doesn't support any extended graphic mode.\n");
	  err = B_ERROR;
	  break;
	}
	
	/* Translate the vesa mode into equivalent be modes */
	for (i=0; i<state.mode_count; i++) {
	  state.modes[i].be_code = 0xff;
	  for (j=0; j<VESA_MODE_1_COUNT; j++)
		if (state.modes[i].code == vesa_modes_1[j]) {
		  state.modes[i].be_code = be_vesa_modes_1[j];
		  break;
		}
	}

	/* do the 5/6/5 -> 1/5/5/5 substitution if needed */
	for (i=0; i<state.mode_count; i++)
	  for (j=0; j<VESA_MODE_2_COUNT; j++)
		if (state.modes[i].be_code == be_vesa_modes_2[j])
		  goto not_needed;
	for (i=0; i<state.mode_count; i++) {
	  for (j=0; j<VESA_MODE_1_COUNT; j++)
		if (state.modes[i].code == vesa_modes_2[j]) {
		  state.modes[i].be_code = be_vesa_modes_2[j];
		  break;
		}
	}
  not_needed:

	/* check the 640x400 safety */
	for (i=0; i<state.mode_count; i++)
	  if (state.modes[i].be_code == 0)
		goto not_needed2;
	for (i=0; i<state.mode_count; i++)
	  if (state.modes[i].code == 0x100) {
		state.modes[i].be_code = 0;
		break;
	  }
  not_needed2:

	/* query all non-identified mode to improve matching,
	   at least if function 1 is available. */
	if (state.functions[0x01]) {
	  viblock.offset = 0;
	  viblock.size = sizeof(vmi);
	  viblock.dataptr = &vmi;
	  viblock.next = NULL;
	  memset(&vmi, 0, 256);
	  
	  for (i=0; i<state.mode_count; i++) {
		ba.data = &viblock;
		ba.eax = 0x4f01;
		ba.ecx = state.modes[i].code;
		ba.edi = 0;	  
		ba.sig = 'BIOS';
		ba.sum = user_bios_args_sum(&ba);
		
		call_bios(0x10, &ba);
		
		/* check error code */
		if (ba.eax != 0x4f)
		  continue;
		  
		/* record graphic context */
		state.modes[i].bits_per_pixel = vmi.BitsPerPixel;
		state.modes[i].height = vmi.YResolution;
		state.modes[i].width = vmi.XResolution;
		state.modes[i].bytes_per_row = vmi.BytesPerScanLine;
		if (state.phys_pci_base)
		  state.modes[i].base_addr = vmi.PhysBasePtr - state.phys_pci_base;
		else
		  state.modes[i].base_addr = 0;
		
		/* check mode attributes */
		if ((vmi.ModeAttributes & 0x009b) != 0x009b) {
		  dprintf("Mode 0x%03x attributes : 0x%04x/0x009b\n",
				  state.modes[i].code, vmi.ModeAttributes);
		  state.modes[i].be_code = 0xff;
		  continue;
		}

		if (state.modes[i].be_code == 0xff) {
		  /* check if the graphic mode uses the proper color encoding */
		  switch (vmi.BitsPerPixel) {
		  case 8 :
			if (vmi.MemoryModel != 5) {
			wrong_memory_model:
			  ddprintf("Warning : Mode 0x%03x with %d bpp uses memory model %d\n",
					   state.modes[i].code, vmi.BitsPerPixel, vmi.MemoryModel);
			  continue;
			}
			break;
		  case 16 :
			if ((vmi.MemoryModel != 4) && (vmi.MemoryModel != 6))
			  goto wrong_memory_model;
			break;
		  case 32 :
			if ((vmi.MemoryModel != 4) && (vmi.MemoryModel != 6))
			  goto wrong_memory_model;
			break;
		  }
		  
		  /* check if the resolution is a good match for an undefined mode */
		  total = vmi.XResolution*vmi.XResolution + vmi.YResolution*vmi.YResolution;
		  min = total>>6;
		  mode = 0xff;
		  
		  for (j=0; j<BE_RES_COUNT; j++)
			if (be_res_defs[j*3+2] == vmi.BitsPerPixel) {
			  dx = vmi.XResolution - be_res_defs[j*3+0];
			  dy = vmi.YResolution - be_res_defs[j*3+1];
			  ecart = dx*dx + dy*dy;
			  if (ecart < min) {
				min = ecart;
				mode = j;
			  }
			}
		  /* set the best match */
		  state.modes[i].be_code = mode;		  
		}
	  }
	}
	  
	/* check at least one 8bits mode for safety */
	for (i=0; i<state.mode_count; i++)
	  if (state.modes[i].be_code != 0xff)	  
		goto got_one_be_mode;
	ddprintf("This BIOS doesn't support any Be compatible graphic mode.\n");
	err = B_ERROR;
	break;
	
  got_one_be_mode:
	/* check that function 2 is supported */
	if (!state.functions[2]) {
	  dprintf("Graphic mode setting not implemented in this BIOS\n");
	  err = B_ERROR;
	  break;
	}
	
	/* print out the result of the analysis */
	for (i=0; i<state.mode_count; i++)
	  print_mode_infos(state.modes+i);
	
	/* init default state variables */
	state.cur_mode = 0xffff;
	state.my_ct.data[0] = 0;
	state.my_ct.data[1] = 0;
	state.my_ct.data[2] = 0;
	
	init_locks();
	break;

  case B_CLOSE_GRAPHICS_CARD :
	/* Blank the screen. The add-on prefers dying in the darkness... */
	
	/* Free the benaphore ressources (if necessary). */
	dispose_locks();
	break;

  case B_SET_INDEXED_COLOR_TABLE :
	data = (uint8*)((indexed_color_table*)buf)->data;

	for (i=((indexed_color_table*)buf)->from*3; i<((indexed_color_table*)buf)->to*3; i++)
	  state.my_ct.data[i] = data[i] >> 2;

	set_color_map();
	break;
	
  case B_SET_INDEXED_COLOR :
	ba.data = NULL;
	ba.eax = 0x1010;
	ba.ebx = ((indexed_color*)buf)->index & 0xff;
	ba.ecx = ((((indexed_color*)buf)->color.green<<6) |
	          (((indexed_color*)buf)->color.blue>>2)) & 0x3f3f;
	ba.edx = (((indexed_color*)buf)->color.red<<6) & 0x3f00;
	ba.sig = 'BIOS';
	ba.sum = user_bios_args_sum(&ba);
	
	call_bios(0x10, &ba);
	break;
	
  case B_GET_SCREEN_SPACES:
	space_mask = 0;
	for (i=0; i<state.mode_count; i++)
	  space_mask |= (1<<state.modes[i].be_code);
	((graphics_card_config*)buf)->space = space_mask;
	break;
	
  case B_CONFIG_GRAPHICS_CARD:
	/* look for the proper mode. */
	for (i=0; i<state.mode_count; i++)
	  if (((graphics_card_config*)buf)->space & (1<<state.modes[i].be_code)) {
		/* set that mode. */
		ba.data = NULL;
		ba.eax = 0x4f02;
		ba.ebx = state.modes[i].code | 0x4000;
		ba.sig = 'BIOS';
		ba.sum = user_bios_args_sum(&ba);

		dprintf("Set ");
		print_mode_infos(state.modes+i);
		call_bios(0x10, &ba);

		if (state.modes[i].bits_per_pixel == 8)
		  set_color_map();
		
		if (ba.eax != 0x4f) {
		  dprintf("########################### Setting failed !!\n");
		  snooze(10000000);
		  err = B_ERROR;
		}
		else
		  state.cur_mode = i;
		goto exit;
	  }
	err = B_ERROR;
	break;
	
  case B_GET_REFRESH_RATES :
	((refresh_rate_info*)buf)->min = 72.0;
	((refresh_rate_info*)buf)->max = 72.0;
	((refresh_rate_info*)buf)->current = 72.0;
	break;
	
  default :
	err = B_ERROR;
	break;
  }
exit:
  return err;
}
#pragma export off

#endif






#if 0

typedef struct {
	uint16	offset;
	uint16	segment;
} far_ptr;

typedef struct {
	char	sig[4];
	uint16	version;
	far_ptr	OEMstringptr;
	uint32	cap;
	far_ptr	modeptr;
	uint16	totalmemory;
	uint16	rev;
	far_ptr	vendorname;
	far_ptr	productname;
	far_ptr	revisionstring;
	char	reserved[222];
	char	OemData[256];
} vesa_info_t;

#define far_to_charp(x) ((char*)(((x).offset+((x).segment<<4))))

int test(int testnum)
{
	switch(testnum) {
	case 1:
		{
			bios_args ba;
			vesa_info_t vi;
			bios_data_block viblock;
			viblock.offset = 0;
			viblock.size = sizeof(vi);
			viblock.dataptr = &vi;
			viblock.next = NULL;
		
			ba.data = &viblock;
			vi.sig[0] = 'V';
			vi.sig[1] = 'B';
			vi.sig[2] = 'E';
			vi.sig[3] = '2';
			//copytobios(0, (char*)&vi, sizeof(vi));
			ba.eax = 0x4f00;
			ba.edi = 0;
			
			ba.sig = 'BIOS';
			ba.sum = user_bios_args_sum(&ba);
			call_bios(0x10, &ba);
			printf("call_bios returned, eax = 0x%x\n", ba.eax);
			//copyfrombios((char*)&vi, 0, sizeof(vi));

			printf("vbe sig = %c%c%c%c\n",  vi.sig[0],vi.sig[1],
					vi.sig[2],vi.sig[3]);
			/*			*((char*)patova(0x1004)),*((char*)patova(0x1005)),
			 *((char*)patova(0x1006)),*((char*)patova(0x1007)));*/
			printf("VbeVersion = 0x%04x\n", vi.version);
			printf("OemString at 0x%x:0x%x\n", vi.OEMstringptr.segment, 
					vi.OEMstringptr.offset);
			printf("OemString at 0x%.8x\n", far_to_charp(vi.OEMstringptr));
			printf("OemString: %s\n", far_to_charp(vi.OEMstringptr));
			printf("capablilities: 0x%08x\n", vi.cap);
			printf("total memory: %d KB\n", vi.totalmemory*64);
			printf("vendor: %s\n", far_to_charp(vi.vendorname));
			printf("product: %s\n", far_to_charp(vi.productname));
			printf("revision: %s\n", far_to_charp(vi.revisionstring));
		} break;
	case 2: 
		{
		} break;
	default:
		return -1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	bios_args ba;
	int int_num;

	if(argc == 2) {
		if(test(atol(argv[1])) == 0) {
			return 0;
		}
	}

	if(argc < 6) {
		printf("use: %s testnum\n", argv[0]);
		printf("     %s intnum eax ebx ecx edx\n", argv[0]);
		return 0;
	}
	int_num = atol(argv[1]);
	ba.eax = atol(argv[2]);
	ba.ebx = atol(argv[3]);
	ba.ecx = atol(argv[4]);
	ba.edx = atol(argv[5]);
	ba.data = NULL;
	ba.sig = 'BIOS';
	ba.sum = user_bios_args_sum(&ba);

	call_bios(int_num, &ba);

	printf("eax = 0x%04x\n", ba.eax);
	printf("ebx = 0x%04x\n", ba.ebx);
	printf("ecx = 0x%04x\n", ba.ecx);
	printf("edx = 0x%04x\n", ba.edx);
	return 0;
}

#endif




















