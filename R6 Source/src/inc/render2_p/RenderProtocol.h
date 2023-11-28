
#ifndef _RENDER2_PROTOCOL_H_
#define _RENDER2_PROTOCOL_H_

namespace B {
namespace Render2 {

enum {
	B_RENDER_TRANSACTION_DATA = 'REND',
	B_RENDER_TRANSACTION_CMDS = 'RENC'
};

// Nb of IRender commands
#define	GRP_NB_COMMANDS		29

//	-----------------------------------------------------------------------------------------
// Command 						Opcode				Parameters				Size (bytes)
//	-----------------------------------------------------------------------------------------
enum RenderCommands {
	GRP_NOP					=	0x00,		//			-						0
	GRP_FINISHED			=	0x01,		//			-						0
	GRP_BRANCH				=	0x02,		//			II						8
	GRP_RLE					=	0x80,		//			-						-

	// Primitive path creation and manipulation
	GRP_MOVE_TO				=	0x10,		//			P						8
	GRP_LINE_TO				=	0x11,		//			P						8
	GRP_BEZIER_TO			=	0x12,		//			PPP						24
	GRP_ARC_TO				=	0x13,		//			PPF						20
	GRP_ARC					=	0x14,		//			PFFF					20
	GRP_ARC_CONECTED		=	0x15,		//			PFFF					20
	GRP_TEXT				=	0x16,		//			ICp						4+?
	GRP_TEXT_ESCAPEMENT		=	0x17,		//			ICp,escapement_delta	4+?+8
	GRP_CLOSE				=	0x18,		//			-						0
	GRP_CLEAR				=	0x19,		//			-						0

	// Composite path operations

	GRP_STROKE				=	0x1A,		//			-						0
	GRP_TEXT_ON_PATH		=	0x1B,		//			ICp						4+?

	// Path filling

	GRP_FILL				=	0x1C,		//			-						0
	GRP_COLOR				=	0x1D,		//			BColor					16
	GRP_SHADE				=	0x1E,		//			BGradient				(fixed)

	// Path generation state

	GRP_SET_FONT			=	0x20,		//			BFont					(fixed)
	GRP_SET_WINDING_RULE	=	0x21,		//			I						4
	GRP_SET_STROKE			=	0x22,		//			FIIFFp					16+?

	// Composition stacks

	GRP_TRANSFORM_2D		=	0x2A,		//			Tr2D					(fixed)
	GRP_TRANSFORM_COLOR		=	0x2B,		//			TrColor					(fixed)
	GRP_BEGIN_COMPOSE		=	0x2C,		//			-						0
	GRP_END_COMPOSE			=	0x2D,		//			-						0

	// Save/restore state

	GRP_PUSH_STATE			=	0x2E,		//			-						0
	GRP_POP_STATE			=	0x2F,		//			-						0

	// Pixmap/sample support

	GRP_BEGIN_PIXELS		=	0x30,		//			BPixelDescription		20
	GRP_PLACE_PIXELS		=	0x31,		//			P, BPixelData			32+?
	GRP_END_PIXELS			=	0x32,		//			-						0

	// Drawing a cached visual

	GRP_DISPLAY_CACHED		=	0x33,		//			binder,II				20
};

} } // namespace B::Render2

#endif	/* _RENDER2_PROTOCOL_H_ */
