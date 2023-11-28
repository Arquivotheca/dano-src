#ifndef __BITFLINGER_H__
#define __BITFLINGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <OS.h>
#include <GL/gl.h>


#define B_BIT_IN		0x1000
#define B_BIT_OUT		0x1001

#define B_BIT_CACHE_L1			1
#define B_BIT_CACHE_L2			2
#define B_BIT_CACHE_L3			3
#define B_BIT_CACHE_RAM			4
#define B_BIT_CACHE_WC			5
#define B_BIT_CACHE_NC			6
#define B_BIT_CACHE_PCI			7
#define B_BIT_CACHE_PCI_WC		8


/*
#define B_BIT_MAP_I_TO_I               0x0C70
#define B_BIT_MAP_S_TO_S               0x0C71
#define B_BIT_MAP_I_TO_R               0x0C72
#define B_BIT_MAP_I_TO_G               0x0C73
#define B_BIT_MAP_I_TO_B               0x0C74
#define B_BIT_MAP_I_TO_A               0x0C75
#define B_BIT_MAP_R_TO_R               0x0C76
#define B_BIT_MAP_G_TO_G               0x0C77
#define B_BIT_MAP_B_TO_B               0x0C78
#define B_BIT_MAP_A_TO_A               0x0C79
*/

typedef void (*cv_extractor)( void *context, int32 x, int32 y, void *pixel, const void *srcData );
extern void * cvCreateContext();
extern void cvDestroyContext( void *context );

/**********************************************************************************
* cvSetType
* 	context :		The context from cvCreateContext()
*
* 	dir : 	 		The direction to set.  Allowed values are
*						B_BIT_IN, B_BIT_OUT
*
*	type : 			The type of data to be processed.  Allowed values are
*						GL_UNSIGNED_BYTE_3_3_2, GL_UNSIGNED_BYTE_2_3_3_REV,
*						GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_5_6_5_REV,
*						GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_4_4_4_4_REV,
*						GL_UNSIGNED_SHORT_5_5_5_1, GL_UNSIGNED_SHORT_1_5_5_5_REV,
*						GL_UNSIGNED_INT_8_8_8_8, GL_UNSIGNED_INT_8_8_8_8_REV,
*						GL_UNSIGNED_INT_10_10_10_2, GL_UNSIGNED_INT_2_10_10_10_REV,
*						GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT,
*						GL_INT, GL_UNSIGNED_INT, GL_FLOAT, GL_DOUBLE
*
*	format : 		The format of the data to be processed.  Allowed values are
*						GL_COLOR_INDEX, GL_STENCIL_INDEX, GL_DEPTH_COMPONENT,
*						GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA, GL_RGB, GL_RGBA,
*						GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_BGR, GL_BGRA, GL_ABGR_EXT
*
*	swap_endian :	0= no swap,  1= swap
*
*	lsb_first :		0= no change,  1= reverse bit ordering
*
*******************************************************************************/
extern status_t cvSetType( void *context, int32 dir, int32 type, int32 format, uint8 swap_endian, uint8 lsb_first );


/**********************************************************************************
* cvSetTransferParam
* 	context :		The context from cvCreateContext()
*
*	name : 			The name of the value to set.  Allowed values are:
*						GL_RED_SCALE, GL_GREEN_SCALE, GL_BLUE_SCALE, GL_ALPHA_SCALE, GL_DEPTH_SCALE
*						GL_RED_BIAS, GL_GREEN_BIAS, GL_BLUE_BIAS, GL_ALPHA_BIAS, GL_DEPTH_BIAS
*						GL_INDEX_SHIFT, GL_INDEX_OFFSET, GL_MAP_COLOR, GL_MAP_STENCIL
*						
*	value :			The value of name to set
*
*******************************************************************************/
extern status_t cvSetTransferParam( void *context, int32 name, float value);


/**********************************************************************************
* cvSetMapXX
* 	context :		The context from cvCreateContext()
*
*	mapName : 		The name of the value to set.  Allowed values are:
*						GL_PIXEL_MAP_I_TO_I, GL_PIXEL_MAP_S_TO_S, GL_PIXEL_MAP_I_TO_R,
*						GL_PIXEL_MAP_I_TO_G, GL_PIXEL_MAP_I_TO_B, GL_PIXEL_MAP_I_TO_A,
*						GL_PIXEL_MAP_R_TO_R, GL_PIXEL_MAP_G_TO_G, GL_PIXEL_MAP_B_TO_B,
*						GL_PIXEL_MAP_A_TO_A
*						
*	size :			The size of the map.  This should be the number of entries.
*
*	map :			Pointer to the map data.  
*
*******************************************************************************/
extern status_t cvSetMapUIV( void *context, uint32 mapName, uint32 size, uint32 *map );
extern status_t cvSetMapUSV( void *context, uint32 mapName, uint32 size, uint16 *map );
extern status_t cvSetMapFV( void *context, uint32 mapName, uint32 size, float *map );


/**********************************************************************************
* cvSetCache
* 	context :		The context from cvCreateContext()
*
* 	dir : 	 		The direction to set.  Allowed values are
*						B_BIT_IN, B_BIT_OUT
*
*	type :			The type of memory for direction.  Allowed values are
*						B_BIT_CACHE_L1 : Data expected to be in the L1 cache
*						B_BIT_CACHE_L2 : Data expected to be in the L2 cache
*						B_BIT_CACHE_L3 : Data expected to be in the L3 cache
*						B_BIT_CACHE_RAM : Data expected to be in cached system ram
*						B_BIT_CACHE_WC : Data expected to be in non-cached system ram with write combining enabled
*						B_BIT_CACHE_NC : Data expected to be in non-cached system ram
*						B_BIT_CACHE_PCI : Data expected to be accessed across a PCI / AGP bus.
*						B_BIT_CACHE_PCI_WC : Data expected to be accessed across a PCI / AGP bus with write combining enabled.
*
*******************************************************************************/
extern status_t cvSetCache( void *context, uint32 dir, uint32 type );


/**********************************************************************************
* cvSetBlendFunction
* 	context :		The context from cvCreateContext()
*
*	srcFactor :		The operation to be applied to the source when
*					combining the extraced pixel with the destination.
*						GL_ZERO, GL_ONE, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA,
*						GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA,
*						GL_SRC_ALPHA_SATURATE, GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR
*						GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA
*
*	dstFactor :		The operation to be applied to the destination when
*					combining the extraced pixel with the destination.
*						GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA,
*						GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA,
*						GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR
*						GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA
*
*******************************************************************************/
extern status_t cvSetBlendFunction( void *context, uint32 srcFactor, uint32 dstFactor );


/**********************************************************************************
* cvSetBlendEquation
* 	context :		The context from cvCreateContext()
*
*	equ :			The equation to be used to combine the two blending functions.
*						GL_FUNC_ADD : 				D = (src * fs) + (dst * fd)
*						GL_FUNC_SUBTRACT : 			D = (src * fs) - (dst * fd)
*						GL_FUNC_REVERSE_SUBTRACT : 	D = (dst * fd) - (src * fs)
*						GL_MIN : 					D = MIN( (src * fs), (dst * fd) )
*						GL_MAX : 					D = MAX( (src * fs), (dst * fd) )
*
*******************************************************************************/
extern status_t cvSetBlendEquation( void *context, uint32 equ );

/**********************************************************************************
* cvSetBlendColor
* 	context :		The context from cvCreateContext()
*
*	r :				The red component of the blending color.
*
*	g :				The green component of the blending color.
*
*	b :				The blue component of the blending color.
*
*	a :				The alpha component of the blending color.
*
*******************************************************************************/
extern status_t cvSetBlendColor( void *context, float r, float g, float b, float a );

/**********************************************************************************
* cvSetWriteMask
* 	context :		The context from cvCreateContext()
*
*	r :				Enable flag for red component.  1=write, 0=don't write.
*
*	g :				Enable flag for green component.  1=write, 0=don't write.
*
*	b :				Enable flag for blue component.  1=write, 0=don't write.
*
*	a :				Enable flag for alpha component.  1=write, 0=don't write.
*
*******************************************************************************/
extern status_t cvSetWriteMask( void *context, int8 r, int8 g, int8 b, int8 a );


/**********************************************************************************
* cvSetLogicOp
* 	context :		The context from cvCreateContext()
*
*	operation :		The operation to be applied when combining the extraced pixel with the destination.
*						GL_CLEAR : D = 0
*						GL_SET : D = 1
*						GL_COPY : D = S
*						GL_COPY_INVERTEX : D = ~S
*						GL_NOOP : D = D
*						GL_INVERT : D = ~D
*						GL_AND : D = S & D
*						GL_NAND : D = ~(S & D)
*						GL_OR : D = S | D
*						GL_NOR : D = (~S | D)
*						GL_XOR : D = S ^ D
*						GL_EQUIV : D = ~(S ^ D)
*						GL_AND_REVERSE : D = S & (~D)
*						GL_AND_INVERTED : D = (~S) & D
*						GL_OR_REVERSE : D = S | (~D)
*						GL_OR_INVERTED : D = (~S) | D
*
* Note:	Logic ops will be ignored if the destination type is double.
*******************************************************************************/
extern status_t cvSetLogicOp( void *context, uint32 operation );

/**********************************************************************************
* cvSetEnvMode
* 	context :		The context from cvCreateContext()
*
*	mode : 			The enviroment mode.  Allowed values are:
*						GL_REPLACE, GL_MODULATE, GL_DECAL, GL_BLEND
*
*******************************************************************************/
extern status_t cvSetEnvMode( void *context, uint32 mode );


/**********************************************************************************
* cvSetEnvColor
* 	context :		The context from cvCreateContext()
*
*	r,g,b,a :		The enviroment color used for GL_MODULATE and GL_BLEND
*
*******************************************************************************/
extern status_t cvSetEnvColor( void *context, float r, float g, float b, float a );

/**********************************************************************************
* cvSetEnvConstColor
* 	context :		The context from cvCreateContext()
*
*	r,g,b,a :		The constant enviroment color used for GL_BLEND
*
*******************************************************************************/
extern status_t cvSetEnvConstColor( void *context, float r, float g, float b, float a );

/**********************************************************************************
* cvPickExtractor
* 	context :		The context from cvCreateContext()
*
*	width : 		The width of the source data in pixels
*
*	height : 		The height of the source data in pixels
*
*	skipPixels :	The number of pixels to skip at the beginning of each line.
*
*	skipRows :		The number of rows to skip at the top of the image.
*
*	rowLength :		The stride of the image in bytes.  If 0 the width is used.
*
*	This function builds and returns a pixel extractor.  It is only valid until any
*	other cv function is called.
*
*******************************************************************************/
extern cv_extractor cvPickExtractor( void *context, int32 width, int32 height,
					int32 skipPixels, int32 skipRows, int32 rowLength );


/**********************************************************************************
* cvConvert
* 	context :		The context from cvCreateContext()
*
*	width : 		The width of the source data in pixels
*
*	height : 		The height of the source data in pixels
*
*	srcSkipPixels :	The number of pixels to skip at the beginning of each source line.
*
*	srcSkipRows :	The number of rows to skip at the top of the source image.
*
*	srcRowLength :	The stride of the source image in bytes.  If 0 the width is used.
*
*	srcData : 		Pointer to the source data
*
*	dstSkipPixels :	The number of pixels to skip at the beginning of each destination line.
*
*	dstSkipRows :	The number of rows to skip at the top of the destination image.
*
*	dstRowLength :	The stride of the destination image in bytes.  If 0 the width is used.
*
*	dstData : 		Pointer to the destination data
*
*
*	This function converts the pixel data from the source format to the destination format.
*
*******************************************************************************/
extern status_t cvConvert( void *context, int32 width, int32 height,
					int32 srcSkipPixels, int32 srcSkipRows, int32 srcRowLength, const void *srcData, 
					int32 dstSkipPixels, int32 dstSkipRows, int32 dstRowLength, void *dstData );


#ifdef __cplusplus
}
#endif

#endif

