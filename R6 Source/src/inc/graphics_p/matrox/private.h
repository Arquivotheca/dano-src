//////////////////////////////////////////////////////////////////////////////
// Matrox Private Header
//    This whole "private" thing has me a little confused.  WTF is private?
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Defines ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Goddess Only Knows...

#define MGA_PRIVATE_DATA_MAGIC  0x0002 /* a private driver rev, of sorts */


//////////////////////////////////////////////////////////////////////////////
// Kernel Driver Commands

#define MKD_MOVE_CURSOR       (0x00000001)
#define MKD_PROGRAM_CLUT      (0x00000002)
#define MKD_SET_START_ADDR    (0x00000004)
#define MKD_SET_CURSOR        (0x00000008)
#define MKD_HANDLER_INSTALLED (0x80000000)

enum
{
  MGA_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
  MGA_GET_PCI,
  MGA_SET_PCI,
  MGA_MAP_ROM,
  MGA_RUN_INTERRUPTS,

  MGA_WAIT_FOR_VBLANK = (1 << 0)
};


//////////////////////////////////////////////////////////////////////////////
//  Typedefs /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Shared Info

typedef struct
{
  uint16 vendor_id;
  uint16 device_id;

  uint8 revision;
  uint8 interrupt_line;

  vuchar *regs;

  area_id regs_area;

  void *framebuffer;
  void *framebuffer_pci;

  area_id fb_area;

  sem_id vblank;

  int32 flags;
  int32 start_addr;

  uint16 cursor_x;
  uint16 cursor_y;
  uint16 first_color;
  uint16 color_count;

  uint8 color_data[3 * 256];
  uint8 cursor0[512];
  uint8 cursor1[512];
} SHARED_INFO;


//////////////////////////////////////////////////////////////////////////////
// MGA Get Set PCI

typedef struct
{
  uint32 magic;         // magic number to make sure the caller groks us
  uint32 offset;
  uint32 size;
  uint32 value;
} MGA_GET_SET_PCI;


//////////////////////////////////////////////////////////////////////////////
// MGA Set Boolean State

typedef struct
{
  uint32 magic;         // magic number to make sure the caller groks us
  bool do_it;
} MGA_SET_BOOL_STATE;


//////////////////////////////////////////////////////////////////////////////
// MGA Get Private Data

typedef struct
{
  uint32 magic;         // magic number to make sure the caller groks us
  SHARED_INFO *si;
} mga_get_private_data;


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
