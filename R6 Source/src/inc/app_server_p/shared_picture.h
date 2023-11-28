
//******************************************************************************
//
//	File:		shared_picture.h
//
//	Description:	Picture opcodes.  Shared between the app_server and 
//				the interface kit.
//	
//	Written by:	George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef SHARED_PICTURE_H
#define SHARED_PICTURE_H


enum PictureOps {
	SPIC_NOOP=0,

	SPIC_MOVE_PEN=0x10,

	/* Rendering primitives */
	SPIC_STROKE_LINE=0x0100,

	SPIC_STROKE_RECT,
	SPIC_FILL_RECT,

	SPIC_STROKE_ROUNDRECT,
	SPIC_FILL_ROUNDRECT,

	SPIC_STROKE_BEZIER,
	SPIC_FILL_BEZIER,

	SPIC_STROKE_ARC,
	SPIC_FILL_ARC,

	SPIC_STROKE_ELLIPSE,
	SPIC_FILL_ELLIPSE,

	SPIC_STROKE_POLYGON,
	SPIC_FILL_POLYGON,

	SPIC_STROKE_PATH,
	SPIC_FILL_PATH,

	SPIC_DRAW_STRING,

	SPIC_PIXELS,
	SPIC_BLIT,

	SPIC_PLAY_PICTURE,

	SPIC_INSCRIBE_STROKE_ARC,
	SPIC_INSCRIBE_FILL_ARC,

	SPIC_INSCRIBE_STROKE_ELLIPSE,
	SPIC_INSCRIBE_FILL_ELLIPSE,

	/* State manipulation */
	SPIC_SET_STATE=0x0200,
	SPIC_CLIP_TO_RECTS,
	SPIC_CLIP_TO_PICTURE,
	SPIC_PUSH_STATE,
	SPIC_POP_STATE,
	SPIC_CLEAR_CLIP,

	/* State attributes */
	SPIC_ORIGIN=0x0300,
	SPIC_LOCATION,
	SPIC_DRAW_OP,
	SPIC_LINE_MODE,
	SPIC_PEN_SIZE,
	SPIC_SCALE,
	SPIC_FORE_COLOR,
	SPIC_BACK_COLOR,
	SPIC_STIPPLE,
	SPIC_FONT,
	SPIC_BLENDING_MODE,
	SPIC_TRANSFORM,

	/* Font attributes */
	SPIC_FAMILY=0x0380,
	SPIC_STYLE,
	SPIC_SPACING,
	SPIC_ENCODING,
	SPIC_FLAGS,
	SPIC_SIZE,
	SPIC_ROTATE,
	SPIC_SHEAR,
	SPIC_BPP,
	SPIC_FACES,

	/* Path elements */
	SPIC_MOVETO=0x0400,
	SPIC_LINE,
	SPIC_BEZIER,
	SPIC_ARC
};

#endif
