#ifndef _R6MER_H_
#define _R6MER_H_

#ifdef __cplusplus
extern "C" {
#endif  //	__cplusplus

#define	PM4_MICROCODE_SIZE	256					// Size of PM4 microcode in QWORD

//	Empty headers of type-0,1,2 packets

#define PM4_PACKET0_NOP		0x00000000			// Empty header of type-0 packtes
#define PM4_PACKET1_NOP		0x40000000			// Empty header of type-1 packtes
#define PM4_PACKET2_NOP		0x80000000			// Empty header of type-2 packtes (reserved)

#define PM4_COUNT_SHIFT             16  

//	IT_OPCODEs for type-3 packets without the DP_GUI_MASTER_CNTL block.
#define PM4_PACKET3_NOP					0xC0001000		// Do nothing.
#define PM4_PACKET3_PAINT				0xC0001100		// Paint with brush using a solid colour.
#define PM4_PACKET3_BITBLT				0xC0001200		// Bitblt and source copy.
#define PM4_PACKET3_SMALLTEXT			0xC0001300		// Draw small glyphs on the screen.
#define PM4_PACKET3_HOSTDATA_BLT		0xC0001400		// Draw large glyphs or bitmap through
														// hostdata register writes.
#define PM4_PACKET3_POLYLINE			0xC0001500		// Draw a polyline.
#define PM4_PACKET3_SCALING				0xC0001600		// Scaling operation for ROP3 and pattern.
#define PM4_PACKET3_TRANS_SCALING		0xC0001700		// Transparent scaling.
#define PM4_PACKET3_POLYSCANLINES		0xC0001800		// Draw polyscanlines or scanlines.
#define PM4_PACKET3_NEXT_CHAR			0xC0001900		// Draw large glyphs or bitmap through
														// hostdata register writes without fore- and
														// back-ground colour writes.
#define PM4_PACKET3_PAINT_MULTI			0xC0001A00		// Paint multiple Rect. y_x and height_width with brush 
									// without the setup body..
#define PM4_PACKET3_BITBLT_MULTI		0xC0001B00		// Multiple BITBLTs without the setup body.
#define PM4_PACKET3_PLY_NEXTSCAN		0xC0001D00		// Draw polyscanlines using current setup. 
#define PM4_PACKET3_SET_SCISSORS		0xC0001E00		// Set up scissors.
#define PM4_PACKET3_SET_MODE24BPP		0xC0001F00		// Set Rage128 to the 24bpp mode.

//	IT_OPCODEs for type-3 packets with the DP_GUI_MASTER_CNTL block.
#define PM4_PACKET3_CNTL_PAINT			0xC0009100		// Paint Rect. y_x and bottom_right with brush using a solid colour.
#define PM4_PACKET3_CNTL_BITBLT			0xC0009200		// Bitblt and source copy.
#define PM4_PACKET3_CNTL_SMALLTEXT		0xC0009300		// Draw small glyphs on the screen.
#define PM4_PACKET3_CNTL_HOSTDATA_BLT	0xC0009400		// Draw large glyphs or bitmap through
														// hostdata register writes.
#define PM4_PACKET3_CNTL_POLYLINE		0xC0009500		// Draw a polyline.		
#define PM4_PACKET3_CNTL_SCALING		0xC0009600		// Scaling operation for ROP3 and pattern.
#define PM4_PACKET3_CNTL_TRANS_SCALING	0xC0009700		// Transparent scaling.
#define PM4_PACKET3_CNTL_POLYSCANLINES	0xC0009800		// Draw polyscanlines or scanlines.
#define PM4_PACKET3_CNTL_NEXT_CHAR		0xC0009900		// Draw large glyphs or bitmap through
														// hostdata register writes without fore- and
														// back-ground colour writes.
#define PM4_PACKET3_CNTL_PAINT_MULTI	0xC0009A00		// Paint multiple Rect. y_x and height_width with brush  .
#define PM4_PACKET3_CNTL_BITBLT_MULTI	0xC0009B00		// Multiple Bitblits .
#define PM4_PACKET3_CNTL_TRANS_BITBLT	0xC0009C00		// 3D Transparent BitBlt.

//	IT_OPCODEs for 3D packets
#define	PM4_PACKET3_3D_SAVE_CONTEXT		0xC0002000		// General purpose context saving.
#define	PM4_PACKET3_3D_PLAY_CONTEXT		0xC0002100		// Play back the context saved previously.
#define	PM4_PACKET3_3D_RNDR_GEN_INDX_PRIM	0xC0002300	// Draw objetcs through the Vertex Walker.
#define	PM4_PACKET3_LOAD_MICROCODE	0xC0002400			// Load Microcode of PM4
#define	PM4_PACKET3_3D_RNDR_GEN_PRIM	0xC0002500		// Draw points, lines,strips, fans, and
														// independent triangles.
#define	PM4_PACKET3_WAIT_FOR_IDLE		0xC0002600		// Waite for the engine to be idle. 
#define PM4_PACKET3_3D_DRAW_VBUF		0xC0002800		// draw primitives using a vertex buffer
#define PM4_PACKET3_3D_DRAW_IMMD		0xC0002900		// draw primitives using immediate vertices in the packet
#define PM4_PACKET3_3D_DRAW_INDX		0xC0002A00		// draw primitives using vertex buffer and indicies in packet
#define PM4_PACKET3_LOAD_PALETTE		0xC0002C00		// Load palette onto Rage128. 
#define PM4_PACKET3_PURGE				0xC0002D00		// Purge Pixel Cache. 
#define PM4_PACKET3_NEXT_VERTEX_BUNDLE	0xC0002E00		// load pointers to vertex buffer
#define PM4_PACKET3_LOAD_VBPNTR			0xC0002F00		// Load palette onto Rage128. 
#define PM4_PACKET3_CLEAR_ZMASK			0xC0003200		// Clear a portion of the ZMASK memory

extern	unsigned long aPM4_Microcode[PM4_MICROCODE_SIZE][2];	// PM4 microcode

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  //  _R6MER_H_
