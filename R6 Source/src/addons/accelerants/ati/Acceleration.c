#include "private.h"
#include "generic.h"
#include "SetRegisters.h"

#if 0
#include <stdio.h>
#define xprintf(a) printf a
#else
#define xprintf(a)
#endif

#define USE_GUI_STAT 0

#if USE_GUI_STAT > 0
#define wait_for_slots(numSlots) \
	{ \
	uint32 FifoReg; \
	\
	do \
	{ \
		READ_REG(GUI_STAT, FifoReg); \
		FifoReg = ((FifoReg >> 16) & 0x003f); \
	} \
	while (FifoReg && (FifoReg < numSlots)); \
	}

#else

#define LONG_WAIT 0
#if LONG_WAIT > 0
/* always wait for 16 slots */
#define wait_for_slots(numSlots) \
	{ \
	uint32 FifoReg; \
	\
	do \
	{ \
		READ_REG(FIFO_STAT, FifoReg); \
	} \
	while (FifoReg & 0xFFFF); \
	}

#else

#define NO_WAIT 0

#if NO_WAIT > 0
/* don't wait at all */
#define wait_for_slots(numSlots)

#else
#if 0
/* wait for the requested number of slots */
/*
this code seems to assume that we will
   a) never wait for more than 15 slots
   b) the last slot position is represented by bit 15, and not bit 0
*/
#define wait_for_slots(numSlots) \
	{ \
	uint32 FifoReg; \
	\
	do \
	{ \
		READ_REG(FIFO_STAT, FifoReg); \
	} \
	while ((FifoReg & 0xFFFF) >> (16 - numSlots)); \
	}
#else
#define wait_for_slots(numSlots) { while ((regs[FIFO_STAT] & 0xffff) > (0x8000 >> (numSlots))); }
#endif
#endif
#endif
#endif

#define wait_empty_idle() wait_for_slots(16) ; while ((regs[GUI_STAT] & 1) != 0)

#if 0
// init accelerant after mode change
void init_gui_engine(void) {
	uint32 scissor_x = (ai->dm.virtual_width - 1) << 16;
	uint32 scissor_y = (ai->dm.virtual_height - 1) << 16;
	WRITE_REG(SC_LEFT_RIGHT, scissor_x);
	WRITE_REG(SC_TOP_BOTTOM, scissor_y);
}
#endif

void SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT(engine_token *et, scaled_blit_params *list, uint32 count) {

	uint32 Offset, Offset_bytes, Pitch;
	uint32 BppEncoding;

	uint32 src_width, src_height;
	uint32 dest_width, dest_height;

	uint32 scissor_x = (ai->dm.virtual_width - 1) << 16;
	uint32 scissor_y = (ai->dm.virtual_height - 1) << 16;
	uint32 bytes_per_pixel;
	uint32 rowbytes;

	// Perform required calculations and do the blit.
	// start of frame buffer
	Offset_bytes = ((char *)ai->fbc.frame_buffer - (char *)si->framebuffer);
	Offset = Offset_bytes >> 3; // qword offset of the frame buffer in card memory; had
		// better be qword-aligned.
	
	Pitch = ai->dm.virtual_width;
	
	// encode pixel format
	switch (ai->dm.space & ~0x3000)
	{
	case B_CMAP8:
		BppEncoding = 0x2;
		bytes_per_pixel = 1;
		break;
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE:
		BppEncoding = 0x3;
		bytes_per_pixel = 2;
		break;
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
		BppEncoding = 0x4;
		bytes_per_pixel = 2;
		break;
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
	default:
		BppEncoding = 0x6;
		bytes_per_pixel = 4;
	}
	rowbytes = (Pitch * bytes_per_pixel);

	BppEncoding = (BppEncoding << 28) | (BppEncoding << 16) | (BppEncoding << 8)
		| (BppEncoding << 4) | BppEncoding;
	
	wait_for_slots(13);

	//WRITE_REG(SCALE_3D_CNTL, 0x00000047); // set this early, as some regs are write-prot if not
	//WRITE_REG(SCALE_3D_CNTL, 0x07000040);
	//WRITE_REG(SCALE_3D_CNTL, 0x00000040);
	WRITE_REG(SCALE_3D_CNTL, 0x00000050);

	WRITE_REG(SC_LEFT_RIGHT, scissor_x);
	WRITE_REG(SC_TOP_BOTTOM, scissor_y);
	WRITE_REG(DP_WRITE_MASK, 0xFFFFFFFF);
	WRITE_REG(DP_PIX_WIDTH, BppEncoding);
	WRITE_REG(DP_MIX, 0x00070007); // ROP = SRC
	WRITE_REG(DP_SRC, 0x00000500); // scaler/3d data
	WRITE_REG(CLR_CMP_CNTL, 0x0);

	// scaler stuff
	WRITE_REG(SCALE_PITCH, Pitch);
	WRITE_REG(GUI_TRAJ_CNTL, 0x00000003); // left->right, top->bottom
	Pitch >>= 3; // frame buffer stride in pixels*8

	// Pitch and offset had better not be out of range.
	WRITE_REG(DST_OFF_PITCH, (Pitch << 22) | Offset);

	// turn off alpha blending
	WRITE_REG(TEX_CNTL, 0);
	WRITE_REG(ALPHA_TST_CNTL, 0);

	/* update fifo count */
	ai->fifo_count += 13;
	
	/* program the blit */
	while (count--) {
		src_width = list->src_width + 1;
		src_height = list->src_height + 1;
		dest_width = list->dest_width + 1;
		dest_height = list->dest_height + 1;

		// Set up the rectangle blit

		wait_for_slots(9);

		// scalling accumulators (must be reset for each scaler op)
		WRITE_REG(SCALE_VACC, 0);
		WRITE_REG(SCALE_HACC, 0);

		// scaling factors
		WRITE_REG(SCALE_X_INC, ((src_width  << 16) / dest_width)  & 0x00fffff0); // 8.12
		WRITE_REG(SCALE_Y_INC, ((src_height << 16) / dest_height) & 0x000ffff0); // 4.12

		// source trajectory from front-end scaler
		WRITE_REG(SCALE_OFF, Offset_bytes + ((uint32)list->src_top * rowbytes) + ((uint32)list->src_left * bytes_per_pixel));
		WRITE_REG(SCALE_WIDTH, src_width);
		WRITE_REG(SCALE_HEIGHT, src_height);
		
		// destination trajectory
		WRITE_REG(DST_Y_X, ((uint32)list->dest_left << 16) | list->dest_top);
		WRITE_REG(DST_HEIGHT_WIDTH, ((uint32)dest_width << 16) | dest_height); // This triggers drawing.

		/* update fifo count */
		ai->fifo_count += 9;
		/* next one */
		list++;
	}
	// turn off the scaler
	wait_for_slots(1);
	WRITE_REG(SCALE_3D_CNTL, 0x00000000);
	ai->fifo_count++;
}



void SCREEN_TO_SCREEN_TRANSPARENT_BLIT(engine_token *et, uint32 trans_color, blit_params *list, uint32 count) {

	uint32 Offset, Pitch;
	uint32 BppEncoding;
	uint32 XDir;
	uint32 YDir;

	uint32 src_top, src_left;
	uint32 dest_top, dest_left;
	uint32 width, height;

	uint32 scissor_x = (ai->dm.virtual_width - 1) << 16;
	uint32 scissor_y = (ai->dm.virtual_height - 1) << 16;

	// Perform required calculations and do the blit.
	// start of frame buffer
	Offset = ((char *)ai->fbc.frame_buffer - (char *)si->framebuffer);
	Offset = Offset >> 3; // qword offset of the frame buffer in card memory; had
		// better be qword-aligned.
	
	Pitch = ai->dm.virtual_width;
	Pitch = (Pitch + 7) >> 3; // frame buffer stride in pixels*8
	
	// encode pixel format
	switch (ai->dm.space & ~0x3000)
	{
	case B_CMAP8:
		BppEncoding = 0x2;
		trans_color &= 0xff;
		trans_color |= trans_color << 8;
		trans_color |= trans_color << 16;
		break;
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE:
		BppEncoding = 0x3;
		trans_color &= 0xffff;
		trans_color |= trans_color << 16;
		break;
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
		BppEncoding = 0x4;
		trans_color &= 0xffff;
		trans_color |= trans_color << 16;
		break;
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
	default:
		BppEncoding = 0x6;
	}
	
	BppEncoding = (BppEncoding << 28) | (BppEncoding << 16) | (BppEncoding << 8)
		| (BppEncoding << 4) | BppEncoding;
	
	wait_for_slots(12);

	WRITE_REG(SC_LEFT_RIGHT, scissor_x);
	WRITE_REG(SC_TOP_BOTTOM, scissor_y);
	WRITE_REG(DP_WRITE_MASK, 0xFFFFFFFF);
	WRITE_REG(DP_PIX_WIDTH, BppEncoding);
	WRITE_REG(DP_MIX, 0x00070003); // ROP = SRC
	WRITE_REG(DP_SRC, 0x00000300);
	WRITE_REG(CLR_CMP_CNTL, 0x01000005);
	WRITE_REG(CLR_CMP_MASK, 0xffffffff);
	WRITE_REG(CLR_CMP_CLR, trans_color);


	// Pitch and offset had better not be out of range.
	WRITE_REG(SRC_OFF_PITCH, (Pitch << 22) | Offset);
	WRITE_REG(DST_OFF_PITCH, (Pitch << 22) | Offset);
	WRITE_REG(SRC_CNTL, 0x0);
	/* update fifo count */
	ai->fifo_count += 12;
	
	/* program the blit */
	while (count--) {
		src_left = list->src_left;
		src_top = list->src_top;
		dest_left = list->dest_left;
		dest_top = list->dest_top;
		width = list->width + 1; /* + 1 because it seems to be non-inclusive */
		height = list->height + 1;

		// Adjust direction of blitting to allow for overlapping source and destination.
		if (src_left < dest_left)
		{
			XDir = 0; // right to left
			src_left += (width - 1);
			dest_left += (width - 1);
		}
		else
			XDir = 1; // left to right

		if (src_top < dest_top)
		{
			YDir = 0; // bottom to top
			src_top += (height - 1);
			dest_top += (height - 1);
		}
		else
			YDir = 1; // top to bottom


		// Set up the rectangle blit

		// Critital section - accessing card registers.
		//lock_card();


		wait_for_slots(5);
		WRITE_REG(SRC_WIDTH1, width);
		WRITE_REG(SRC_Y_X, ((uint32)src_left << 16) | src_top);

		// Pitch and offset had better not be out of range.
		WRITE_REG(DST_CNTL, (YDir << 1) | XDir);
		WRITE_REG(DST_Y_X, ((uint32)dest_left << 16) | dest_top);
		WRITE_REG(DST_HEIGHT_WIDTH, ((uint32)width << 16) | height); // This triggers drawing.

		/* update fifo count */
		ai->fifo_count += 5;
		/* next one */
		list++;
	}
}


void SCREEN_TO_SCREEN_BLIT(engine_token *et, blit_params *list, uint32 count) {

	uint32 Offset, Pitch;
	uint32 BppEncoding;
	uint32 XDir;
	uint32 YDir;

	uint32 src_top, src_left;
	uint32 dest_top, dest_left;
	uint32 width, height;

	uint32 scissor_x = (ai->dm.virtual_width - 1) << 16;
	uint32 scissor_y = (ai->dm.virtual_height - 1) << 16;

	// Perform required calculations and do the blit.
	// start of frame buffer
	Offset = ((char *)ai->fbc.frame_buffer - (char *)si->framebuffer);
	Offset = Offset >> 3; // qword offset of the frame buffer in card memory; had
		// better be qword-aligned.
	
	Pitch = ai->dm.virtual_width;
	Pitch = (Pitch + 7) >> 3; // frame buffer stride in pixels*8
	
	// encode pixel format
	switch (ai->dm.space & ~0x3000)
	{
	case B_CMAP8:
		BppEncoding = 0x2;
		break;
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE:
		BppEncoding = 0x3;
		break;
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
		BppEncoding = 0x4;
		break;
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
	default:
		BppEncoding = 0x6;
	}
	
	BppEncoding = (BppEncoding << 28) | (BppEncoding << 16) | (BppEncoding << 8)
		| (BppEncoding << 4) | BppEncoding;
	
	wait_for_slots(10);

	WRITE_REG(SC_LEFT_RIGHT, scissor_x);
	WRITE_REG(SC_TOP_BOTTOM, scissor_y);
	WRITE_REG(DP_WRITE_MASK, 0xFFFFFFFF);
	WRITE_REG(DP_PIX_WIDTH, BppEncoding);
	WRITE_REG(DP_MIX, 0x00070007); // ROP = SRC
	WRITE_REG(DP_SRC, 0x00000300);
	WRITE_REG(CLR_CMP_CNTL, 0x0);

	// Pitch and offset had better not be out of range.
	WRITE_REG(SRC_OFF_PITCH, (Pitch << 22) | Offset);
	WRITE_REG(DST_OFF_PITCH, (Pitch << 22) | Offset);
	WRITE_REG(SRC_CNTL, 0x0);
	/* update fifo count */
	ai->fifo_count += 10;
	
	/* program the blit */
	while (count--) {
		src_left = list->src_left;
		src_top = list->src_top;
		dest_left = list->dest_left;
		dest_top = list->dest_top;
		width = list->width + 1; /* + 1 because it seems to be non-inclusive */
		height = list->height + 1;

		// Adjust direction of blitting to allow for overlapping source and destination.
		if (src_left < dest_left)
		{
			XDir = 0; // right to left
			src_left += (width - 1);
			dest_left += (width - 1);
		}
		else
			XDir = 1; // left to right

		if (src_top < dest_top)
		{
			YDir = 0; // bottom to top
			src_top += (height - 1);
			dest_top += (height - 1);
		}
		else
			YDir = 1; // top to bottom


		// Set up the rectangle blit

		// Critital section - accessing card registers.
		//lock_card();


		wait_for_slots(5);

		WRITE_REG(SRC_WIDTH1, width);
		WRITE_REG(SRC_Y_X, ((uint32)src_left << 16) | src_top);

		// Pitch and offset had better not be out of range.
		WRITE_REG(DST_CNTL, (YDir << 1) | XDir);
		WRITE_REG(DST_Y_X, ((uint32)dest_left << 16) | dest_top);
		WRITE_REG(DST_HEIGHT_WIDTH, ((uint32)width << 16) | height); // This triggers drawing.

		/* update fifo count */
		ai->fifo_count += 5;
		/* next one */
		list++;
	}
}

void hardware_rectangle(engine_token *et, uint32 colorIndex, fill_rect_params *list, uint32 count, uint32 ROP) {
	uint32 Offset;
	uint32 Pitch;
	uint32 Y;
	uint32 X;
	uint32 Width;
	uint32 Height;
	uint32 Colour;
	uint32 BppEncoding;

	uint32 scissor_x = (ai->dm.virtual_width - 1) << 16;
	uint32 scissor_y = (ai->dm.virtual_height - 1) << 16;

	// Perform required calculations and do the blit.
	// start of frame buffer
	Offset = ((char *)ai->fbc.frame_buffer - (char *)si->framebuffer);
	Offset = Offset >> 3; // qword offset of the frame buffer in card memory; had
		// better be qword-aligned.
	
	Pitch = ai->dm.virtual_width;
	Pitch = (Pitch + 7) >> 3; // frame buffer stride in pixels*8

	// Fill colour dword with specified colour, though only ls should be necessary.
	switch (ai->dm.space & ~0x3000)
	{
	case B_CMAP8:
		BppEncoding = 0x2;
		colorIndex &= 0xFF;
		Colour = colorIndex | (colorIndex << 8) | (colorIndex << 16) | (colorIndex << 24);
		break;
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE:
		BppEncoding = 0x3;
		colorIndex &= 0xFFFF;
		Colour = colorIndex | (colorIndex << 16);
		break;
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
		BppEncoding = 0x4;
		colorIndex &= 0xFFFF;
		Colour = colorIndex | (colorIndex << 16);
		break;
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
	default:
		BppEncoding = 0x6;
		Colour = colorIndex;
	}

	BppEncoding = (BppEncoding << 28) | (BppEncoding << 16) | (BppEncoding << 8) | (BppEncoding << 4) | BppEncoding;
	
	wait_for_slots(11);
	// This seems to be needed even though we aren't using a source trajectory.
	WRITE_REG(SRC_CNTL, 0x0);
	
	// Pitch and offset had better not be out of range.
	WRITE_REG(DST_OFF_PITCH, (Pitch << 22) | Offset);
	
	WRITE_REG(DP_FRGD_CLR, Colour);
	WRITE_REG(DP_WRITE_MASK, 0xFFFFFFFF);
	WRITE_REG(DP_PIX_WIDTH, BppEncoding);
	WRITE_REG(DP_MIX, ROP);
	WRITE_REG(DP_SRC, 0x00000100);
	
	WRITE_REG(CLR_CMP_CNTL, 0x0);
	WRITE_REG(GUI_TRAJ_CNTL, 0x3);
	WRITE_REG(SC_LEFT_RIGHT, scissor_x);
	WRITE_REG(SC_TOP_BOTTOM, scissor_y);
	/* update fifo count */
	ai->fifo_count += 11;

	while (count--) {

		X = list->left;
		Y = list->top;
		Width = list->right;
		Width = Width - X + 1;
		Height = list->bottom;
		Height = Height - Y + 1;

		wait_for_slots(2);

		xprintf(("x,y: %d,%d  w,h: %d,%d\n", X,Y, Width,Height));
		// X and Y should have been clipped to frame buffer, and so should be in range.
		WRITE_REG(DST_Y_X, (X << 16) | Y);
		WRITE_REG(DST_HEIGHT_WIDTH, (Width << 16) | Height); // This triggers drawing.

		/* update fifo count */
		ai->fifo_count += 2;
		/* next rect */
		list++;
	}

}

void FILL_RECTANGLE(engine_token *et, uint32 colorIndex, fill_rect_params *list, uint32 count) {
	hardware_rectangle(et, colorIndex, list, count, 0x00070007); // ROP = SRC
}

void INVERT_RECTANGLE(engine_token *et, fill_rect_params *list, uint32 count) {
	hardware_rectangle(et, 0xFFFFFFFF, list, count, 0x00000000); // ROP = !DST
}

void FILL_SPAN(engine_token *et, uint32 colorIndex, uint16 *list, uint32 count) {
	uint32 Offset;
	uint32 Pitch;
	uint32 Y;
	uint32 X;
	uint32 Width;
	uint32 Height;
	uint32 Colour;
	uint32 BppEncoding;


	uint32 scissor_x = (ai->dm.virtual_width - 1) << 16;
	uint32 scissor_y = (ai->dm.virtual_height - 1) << 16;

	// Perform required calculations and do the blit.
	// start of frame buffer
	Offset = ((char *)ai->fbc.frame_buffer - (char *)si->framebuffer);
	Offset = Offset >> 3; // qword offset of the frame buffer in card memory; had
		// better be qword-aligned.
	
	Pitch = ai->dm.virtual_width;
	Pitch = (Pitch + 7) >> 3; // frame buffer stride in pixels*8

	// Fill colour dword with specified colour, though only ls should be necessary.
	switch (ai->dm.space & ~0x3000)
	{
	case B_CMAP8:
		BppEncoding = 0x2;
		colorIndex &= 0xFF;
		Colour = colorIndex | (colorIndex << 8) | (colorIndex << 16) | (colorIndex << 24);
		break;
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE:
		BppEncoding = 0x3;
		colorIndex &= 0xFFFF;
		Colour = colorIndex | (colorIndex << 16);
		break;
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
		BppEncoding = 0x4;
		colorIndex &= 0xFFFF;
		Colour = colorIndex | (colorIndex << 16);
		break;
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
	default:
		BppEncoding = 0x6;
		Colour = colorIndex;
	}

	BppEncoding = (BppEncoding << 28) | (BppEncoding << 16) | (BppEncoding << 8) | (BppEncoding << 4) | BppEncoding;


	wait_for_slots(11);
	// This seems to be needed even though we aren't using a source trajectory.
	WRITE_REG(SRC_CNTL, 0x0);
	
	// Pitch and offset had better not be out of range.
	WRITE_REG(DST_OFF_PITCH, (Pitch << 22) | Offset);
	
	WRITE_REG(DP_FRGD_CLR, Colour);
	WRITE_REG(DP_WRITE_MASK, 0xFFFFFFFF);
	WRITE_REG(DP_PIX_WIDTH, BppEncoding);
	WRITE_REG(DP_MIX, 0x00070007); // ROP = SRC
	WRITE_REG(DP_SRC, 0x00000100);
	
	WRITE_REG(CLR_CMP_CNTL, 0x0);
	WRITE_REG(GUI_TRAJ_CNTL, 0x3);
	WRITE_REG(SC_LEFT_RIGHT, scissor_x);
	WRITE_REG(SC_TOP_BOTTOM, scissor_y);
	/* update fifo count */
	ai->fifo_count += 11;

	/* span lines are always one pixel tall */
	Height = 1;
	while (count--) {

		Y = (uint32)*list++;
		X = (uint32)*list++;
		Width = (uint32)*list++;
		Width = Width - X + 1;

		wait_for_slots(2);

		// X and Y should have been clipped to frame buffer, and so should be in range.
		WRITE_REG(DST_Y_X, (X << 16) | Y);
		WRITE_REG(DST_HEIGHT_WIDTH, (Width << 16) | Height); // This triggers drawing.

		/* update fifo count */
		ai->fifo_count += 2;
	}

}
