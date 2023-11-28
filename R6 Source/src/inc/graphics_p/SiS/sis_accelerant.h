#ifndef SIS_ACCELERANT_H
#define SIS_ACCELERANT_H

#include <Accelerant.h>
#include <video_overlay.h>
#include <surface/genpool.h>

#include "bena4.h"
#include "sis_protos.h"

// long queue (62k) can not work in asynchronous mode
// while 30k queue can
// For the moment, there is no reason to use 62k that I know.
//#define SIS620_USE_62k_QUEUE


// -------- Hooks --------

enum {
	SIS_GET_GENPOOL_TOKENID = B_ACCELERANT_PRIVATE_START+1,
	SIS_GET_CARD_INFO,
	SIS_SET_SECONDARY_DISPLAY_MODE,
	SIS_RESTORE_PRIMARY_DISPLAY_MODE,
	MAX_SIS_PRIVATE_HOOK
	};
	
typedef status_t (*restore_primary_display_mode)(void);

// -------- SiS card info --------

typedef struct sis_overlay_token {
	int	used ;
	} sis_overlay_token ;

#define MAX_OVERLAYS (1)
#define MAX_OVERLAY_BUFFERS (1)

typedef struct sis_card_info {	
	
	uint16			ci_DeviceId;		// SIS5598_DEVICEID | SIS6326_DEVICEID | SIS620_DEVICEID
	uchar			ci_Device_revision ;// a2 for SiS530, 2a for SiS620
	accelerant_device_info	ci_ADI;		/*  "Identifying marks"						*/
	char			ci_DevName[B_OS_NAME_LENGTH];

	// Memory Mapping
	
	uint32			ci_MemSize;			//  Total video RAM in kb
	int				ci_Shared_Mode; 	// if false, then local video ram mode
	uint32			ci_PoolID;			// identificator for memspool registered video memory
										// warning: the first 256k have already been allocated
	void			*ci_BaseAddr0_DMA;	// physical address of video memory
	volatile uint8	*ci_BaseAddr0;		// virtual (mapped) address of video memory
	volatile uint8	*ci_BaseAddr1;		// virtual address of MMIO registers and channels
	volatile uint8	*ci_BaseAddr2;		// virtual address of Relocated IO registers and channels
	volatile uchar	*ci_RegBase;		// = BaseAddr1
	volatile uchar	*ci_IORegBase;		// = BaseAddr2

	BMemSpec		ci_TurboQueue_ms;	// MemSpec for the Turbo Queue

	// Appserver Allocations

	BMemSpec		ci_FBMemSpec;		// MemSpec of the frame buffer
	void			*ci_FBBase;			//  Pointer to BeOS screen framebuffer (mapped memory)
	void			*ci_FBBase_DMA;		//  Physical FB addr for DMA
	uint32			ci_FBBase_offset;	// position of Frame Buffer in Video Memory
	uint32			ci_Cursor_offset;	// position of Cursor in Video Memory


	//  Hardware arbitration locks
	//  MUST Obtain multiple locks in the order listed here.
	
	struct Bena4	ci_CRTCLock;		/*  CRTC, AR, SR, and MISC regs						*/
	struct Bena4	ci_CLUTLock;		/*  256-entry palette								*/
	struct Bena4	ci_EngineLock;		/*  HW renderer										*/
	struct Bena4	ci_EngineRegsLock;	/*  Accelerant internal lock for engine registers	*/
	struct Bena4	ci_SequencerLock;	/*  almost only used for the cursor					*/
	sem_id			ci_VBlankLock;		/*  WaitVBL support									*/

	display_mode	*ci_DispModes;
	int				ci_NDispModes;
	float			ci_Clock_Max;
  	display_mode	ci_CurDispMode;		/*  May contain custom mode					*/

	uint16			ci_sis630_AGPBase;	// Backup of the 2D Engine AGPBase register ( 0x8206~0x8207 )
	uint16			ci_Depth;			/*  Bits per pixel							*/
	uint16			ci_BytesPerPixel;
	uint16			ci_BytesPerRow;

	int16			ci_MonSizeX;		/*  Display size on monitor					*/
	int16			ci_MonSizeY;		/*  <50 - narrower, >50 - wider				*/
	int16			ci_MonPosX;			/*  Display position on monitor				*/
	int16			ci_MonPosY;			/*  <50 - left, >50 - right					*/

	int16			ci_MousePosX;		/*  Position of mouse						*/
	int16			ci_MousePosY;
	int16			ci_MouseHotX;		/*  Hotpoint of mouse						*/
	int16			ci_MouseHotY;

	//int32			ci_IRQFlags;		/*  Things for IRQ to do					*/

	//  sync_token stuff
	uint64				ci_PrimitivesIssued;
	uint64				ci_PrimitivesCompleted;
	
	// Overlay ------------
	
	sis_overlay_token	ovl_token[MAX_OVERLAYS];
	overlay_buffer		ovl_buffer[MAX_OVERLAY_BUFFERS];
	BMemSpec			ovl_buffer_memspec[MAX_OVERLAY_BUFFERS];
	
	// 2d Accelerated functions
	struct sis_protos	ci_protos ;

	bigtime_t		profiling_waittime_before_setup ;
	bigtime_t		profiling_setup_time ;

	uint32			vbl_calls ;
		
	} sis_card_info;

#endif
