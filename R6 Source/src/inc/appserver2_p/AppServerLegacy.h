
#ifndef	_APPSERVER2_LEGACY_H_
#define	_APPSERVER2_LEGACY_H_

#include <render2/RenderDefs.h>

/*----------------------------------------------------------------*/

enum {
	B_FIRST_MOUSE_BUTTON = 0x01,
	B_SECOND_MOUSE_BUTTON = 0x02,
	B_THIRD_MOUSE_BUTTON = 0x04,
	
	// or if you prefer...
	B_MOUSE_BUTTON_1 = B_FIRST_MOUSE_BUTTON,
	B_MOUSE_BUTTON_2 = B_SECOND_MOUSE_BUTTON,
	B_MOUSE_BUTTON_3 = B_THIRD_MOUSE_BUTTON
};

enum {
	B_ENTERED_VIEW = 0,
	B_INSIDE_VIEW,
	B_EXITED_VIEW,
	B_OUTSIDE_VIEW
};

enum {
	B_UPDATE_INVALIDATED	= 0x00000001,
	B_UPDATE_SCROLLED		= 0x00000002,
	B_UPDATE_RESIZED		= 0x00000004,
	B_UPDATE_EXPOSED		= 0x00000008
};

enum {
	B_NO_EVENTS				= 0x00000000,
	B_POINTER_EVENTS		= 0x00000001,
	B_KEYBOARD_EVENTS		= 0x00000002
};

enum {
	B_LOCK_WINDOW_FOCUS		= 0x00000001,
	B_SUSPEND_VIEW_FOCUS	= 0x00000002,
	B_NO_POINTER_HISTORY	= 0x00000004
};

const uint32 _VIEW_TOP_ 	= 1UL;
const uint32 _VIEW_LEFT_ 	= 2UL;
const uint32 _VIEW_BOTTOM_ 	= 3UL;
const uint32 _VIEW_RIGHT_ 	= 4UL;
const uint32 _VIEW_CENTER_ 	= 5UL;

inline uint32 _rule_(uint32 r1, uint32 r2, uint32 r3, uint32 r4)
	{ return ((r1 << 12) | (r2 << 8) | (r3 << 4) | r4); }

const uint32 B_FULL_UPDATE_ON_RESIZE 	= 0x80000000UL;	/* 31 */
const uint32 _B_RESERVED1_ 				= 0x40000000UL;	/* 30 */
const uint32 B_WILL_DRAW 				= 0x20000000UL;	/* 29 */
const uint32 B_PULSE_NEEDED 			= 0x10000000UL;	/* 28 */
const uint32 B_NAVIGABLE_JUMP 			= 0x08000000UL;	/* 27 */
const uint32 B_FRAME_EVENTS				= 0x04000000UL;	/* 26 */
const uint32 B_NAVIGABLE 				= 0x02000000UL;	/* 25 */
const uint32 B_SUBPIXEL_PRECISE 		= 0x01000000UL;	/* 24 */
const uint32 B_DRAW_ON_CHILDREN 		= 0x00800000UL;	/* 23 */
const uint32 B_INPUT_METHOD_AWARE 		= 0x00400000UL;	/* 22 */
const uint32 _B_RESERVED7_ 				= 0x00200000UL;	/* 21 */
const uint32 B_OVERLAPPING_CHILDREN		= 0x00100000UL;	/* 20 */

#define B_FOLLOW_NONE 0
#define B_FOLLOW_ALL			_rule_(_VIEW_TOP_, _VIEW_LEFT_, _VIEW_BOTTOM_, _VIEW_RIGHT_)
#define B_FOLLOW_LEFT			_rule_(0, _VIEW_LEFT_, 0, _VIEW_LEFT_)
#define B_FOLLOW_RIGHT			_rule_(0, _VIEW_RIGHT_, 0, _VIEW_RIGHT_)
#define B_FOLLOW_LEFT_RIGHT		_rule_(0, _VIEW_LEFT_, 0, _VIEW_RIGHT_)
#define B_FOLLOW_H_CENTER		_rule_(0, _VIEW_CENTER_, 0, _VIEW_CENTER_)
#define B_FOLLOW_TOP			_rule_(_VIEW_TOP_, 0, _VIEW_TOP_, 0)
#define B_FOLLOW_BOTTOM			_rule_(_VIEW_BOTTOM_, 0, _VIEW_BOTTOM_, 0)
#define B_FOLLOW_TOP_BOTTOM		_rule_(_VIEW_TOP_, 0, _VIEW_BOTTOM_, 0)
#define B_FOLLOW_V_CENTER		_rule_(_VIEW_CENTER_, 0, _VIEW_CENTER_, 0)

enum ViewMessages {
	GR_ADD_VIEW = 0x500,
	GR_MOVE_VIEW,
	GR_MOVETO_VIEW,
	GR_SIZE_VIEW,
	GR_SIZETO_VIEW,
	GR_REMOVE_VIEW,
	GR_FIND_VIEW,
	GR_GET_VIEW_BOUND,
	GR_VIEW_FLAGS,
	GR_VIEW_SET_EVENT_MASK,
	GR_VIEW_AUGMENT_EVENT_MASK,
	GR_SCREWED_UP_MOVETO_VIEW,
	GR_SET_VIEW_CURSOR,
	GR_SET_VIEW_DOUBLE_BUFFER,
};

/*----------------------------------------------------------------*/

struct screen_id { int32 id; };
extern const struct screen_id B_MAIN_SCREEN_ID;
extern const color32 B_TRANSPARENT_COLOR;

/*----------------------------------------------------------------*/

#define B_ANY_BYTES_PER_ROW -1

/*----------------------------------------------------------------*/

enum {
	B_BITMAP_CLEAR_TO_WHITE				= 0x00000001,
	B_BITMAP_ACCEPTS_VIEWS				= 0x00000002,
	B_BITMAP_IS_AREA					= 0x00000004,
	B_BITMAP_IS_LOCKED					= 0x00000008 | B_BITMAP_IS_AREA,
	B_BITMAP_IS_CONTIGUOUS				= 0x00000010 | B_BITMAP_IS_LOCKED,
	B_BITMAP_IS_OFFSCREEN				= 0x00000020,
	B_BITMAP_WILL_OVERLAY				= 0x00000040 | B_BITMAP_IS_OFFSCREEN,
	B_BITMAP_RESERVE_OVERLAY_CHANNEL	= 0x00000080
};

/*----------------------------------------------------------------*/

typedef enum
{
	B_NO_COLOR_SPACE =	0x0000,	/* byte in memory order, high bit first				*/
	
	/* linear color space (little endian is the default) */
	B_RGB32 = 			0x0008,	/* B[7:0]  G[7:0]  R[7:0]  -[7:0]					*/
	B_RGBA32 = 			0x2008,	/* B[7:0]  G[7:0]  R[7:0]  A[7:0]					*/
	B_RGB24 = 			0x0003,	/* B[7:0]  G[7:0]  R[7:0]							*/
	B_RGB16 = 			0x0005,	/* G[2:0],B[4:0]  R[4:0],G[5:3]						*/
	B_RGB15 = 			0x0010,	/* G[2:0],B[4:0]  	   -[0],R[4:0],G[4:3]			*/
	B_RGBA15 = 			0x2010,	/* G[2:0],B[4:0]  	   A[0],R[4:0],G[4:3]			*/
	B_CMAP8 = 			0x0004,	/* D[7:0]  											*/
	B_GRAY8 = 			0x0002,	/* Y[7:0]											*/
	B_GRAY1 = 			0x0001,	/* Y0[0],Y1[0],Y2[0],Y3[0],Y4[0],Y5[0],Y6[0],Y7[0]	*/

	/* big endian version, when the encoding is not endianess independant */
	B_RGB32_BIG =		0x1008,	/* -[7:0]  R[7:0]  G[7:0]  B[7:0]					*/
	B_RGBA32_BIG = 		0x3008,	/* A[7:0]  R[7:0]  G[7:0]  B[7:0]					*/
	B_RGB24_BIG = 		0x1003,	/* R[7:0]  G[7:0]  B[7:0]							*/
	B_RGB16_BIG = 		0x1005,	/* R[4:0],G[5:3]  G[2:0],B[4:0]						*/
	B_RGB15_BIG = 		0x1010,	/* -[0],R[4:0],G[4:3]  G[2:0],B[4:0]				*/
	B_RGBA15_BIG = 		0x3010,	/* A[0],R[4:0],G[4:3]  G[2:0],B[4:0]				*/

	/* little-endian declarations, for completness */
	B_RGB32_LITTLE = 	B_RGB32,
	B_RGBA32_LITTLE =	B_RGBA32,
	B_RGB24_LITTLE =	B_RGB24,
	B_RGB16_LITTLE =	B_RGB16,
	B_RGB15_LITTLE =	B_RGB15,
	B_RGBA15_LITTLE =	B_RGBA15,

	/* non linear color space -- note that these are here for exchange purposes;	*/
	/* a BBitmap or BView may not necessarily support all these color spaces.	*/

	/* Loss/Saturation points are Y 16-235 (absoulte); Cb/Cr 16-240 (center 128) */

	B_YCbCr422 = 		0x4000,	/* Y0[7:0]  Cb0[7:0]  Y1[7:0]  Cr0[7:0]  Y2[7:0]...	*/
								/* Cb2[7:0]  Y3[7:0]  Cr2[7:0]						*/
	B_YCbCr411 = 		0x4001,	/* Cb0[7:0]  Y0[7:0]  Cr0[7:0]  Y1[7:0]  Cb4[7:0]...*/
								/* Y2[7:0]  Cr4[7:0]  Y3[7:0]  Y4[7:0]  Y5[7:0]...	*/
								/* Y6[7:0]  Y7[7:0]	 								*/
	B_YCbCr444 = 		0x4003,	/* Y0[7:0]  Cb0[7:0]  Cr0[7:0]		*/
	B_YCbCr420 = 		0x4004,	/* Non-interlaced only, Cb0  Y0  Y1  Cb2 Y2  Y3  on even scan lines ... */
								/* Cr0  Y0  Y1  Cr2 Y2  Y3  on odd scan lines */

	/* Extrema points are Y 0 - 207 (absolute) U -91 - 91 (offset 128) V -127 - 127 (offset 128) */
	/* note that YUV byte order is different from YCbCr */
	/* USE YCbCr, not YUV, when that's what you mean! */
	B_YUV422 =			0x4020, /* U0[7:0]  Y0[7:0]   V0[7:0]  Y1[7:0] ... */
								/* U2[7:0]  Y2[7:0]   V2[7:0]  Y3[7:0]  */
	B_YUV411 =			0x4021, /* U0[7:0]  Y0[7:0]  Y1[7:0]  V0[7:0]  Y2[7:0]  Y3[7:0]  */
								/* U4[7:0]  Y4[7:0]  Y5[7:0]  V4[7:0]  Y6[7:0]  Y7[7:0]  */
	B_YUV444 =			0x4023,	/* U0[7:0]  Y0[7:0]  V0[7:0]  U1[7:0]  Y1[7:0]  V1[7:0] */
	B_YUV420 = 			0x4024,	/* Non-interlaced only, U0  Y0  Y1  U2 Y2  Y3  on even scan lines ... */
								/* V0  Y0  Y1  V2 Y2  Y3  on odd scan lines */
	B_YUV9 = 			0x402C,	/* planar?	410?								*/
	B_YUV12 = 			0x402D,	/* planar?	420?								*/

	B_UVL24 =			0x4030,	/* U0[7:0] V0[7:0] L0[7:0] ... */
	B_UVL32 =			0x4031,	/* U0[7:0] V0[7:0] L0[7:0] X0[7:0]... */
	B_UVLA32 =			0x6031,	/* U0[7:0] V0[7:0] L0[7:0] A0[7:0]... */

	B_LAB24 =			0x4032,	/* L0[7:0] a0[7:0] b0[7:0] ...  (a is not alpha!) */
	B_LAB32 =			0x4033,	/* L0[7:0] a0[7:0] b0[7:0] X0[7:0] ... (b is not alpha!) */
	B_LABA32 =			0x6033,	/* L0[7:0] a0[7:0] b0[7:0] A0[7:0] ... (A is alpha) */

	/* red is at hue = 0 */

	B_HSI24 =			0x4040,	/* H[7:0]  S[7:0]  I[7:0]							*/
	B_HSI32 =			0x4041,	/* H[7:0]  S[7:0]  I[7:0]  X[7:0]					*/
	B_HSIA32 =			0x6041,	/* H[7:0]  S[7:0]  I[7:0]  A[7:0]					*/

	B_HSV24 =			0x4042,	/* H[7:0]  S[7:0]  V[7:0]							*/
	B_HSV32 =			0x4043,	/* H[7:0]  S[7:0]  V[7:0]  X[7:0]					*/
	B_HSVA32 =			0x6043,	/* H[7:0]  S[7:0]  V[7:0]  A[7:0]					*/

	B_HLS24 =			0x4044,	/* H[7:0]  L[7:0]  S[7:0]							*/
	B_HLS32 =			0x4045,	/* H[7:0]  L[7:0]  S[7:0]  X[7:0]					*/
	B_HLSA32 =			0x6045,	/* H[7:0]  L[7:0]  S[7:0]  A[7:0]					*/

	B_CMY24 =			0xC001,	/* C[7:0]  M[7:0]  Y[7:0]  			No gray removal done		*/
	B_CMY32 =			0xC002,	/* C[7:0]  M[7:0]  Y[7:0]  X[7:0]	No gray removal done		*/
	B_CMYA32 =			0xE002,	/* C[7:0]  M[7:0]  Y[7:0]  A[7:0]	No gray removal done		*/
	B_CMYK32 =			0xC003,	/* C[7:0]  M[7:0]  Y[7:0]  K[7:0]					*/

	/* compatibility declarations */
	B_MONOCHROME_1_BIT = 	B_GRAY1,
	B_GRAYSCALE_8_BIT =		B_GRAY8,
	B_COLOR_8_BIT =			B_CMAP8,
	B_RGB_32_BIT =			B_RGB32,
	B_RGB_16_BIT =			B_RGB15,
	B_BIG_RGB_32_BIT =		B_RGB32_BIG,
	B_BIG_RGB_16_BIT =		B_RGB15_BIG
} color_space;

#endif /* _APPSERVER2_LEGACY_H_ */
