#include <GraphicsDefs.h>

bool
bitmaps_support_space(
	color_space space,
	uint32 * out_support_flags)
{
	static color_space supported_spaces[] = {
	    B_RGB32,
	    B_RGBA32,
	    B_RGB16,
	    B_RGB15,
	    B_RGBA15,
	    B_CMAP8,
	    B_RGB32_BIG,
	    B_RGBA32_BIG,
	    B_RGB16_BIG,
	    B_RGB15_BIG,
	    B_RGBA15_BIG,
	};

	for (color_space * sptr = supported_spaces; 
		sptr<&supported_spaces[sizeof(supported_spaces)/sizeof(supported_spaces[0])];
		sptr++)
		if (*sptr == space) {
			*out_support_flags = 0x3;
			return true;
		}

	*out_support_flags = 0;
	return false;
}


static struct {
	color_space space;
	size_t	chunk;
	size_t	pixels;
} s_pixel_info[] = {
	{ B_RGB32, 4, 1 },	/* B[7:0]  G[7:0]  R[7:0]  -[7:0]					*/
	{ B_RGBA32, 4, 1 },	/* B[7:0]  G[7:0]  R[7:0]  A[7:0]					*/
	{ B_RGB24, 3, 1 },	/* B[7:0]  G[7:0]  R[7:0]							*/
	{ B_RGB16, 2, 1 },	/* G[2:0],B[4:0]  R[4:0],G[5:3]						*/
	{ B_RGB15, 2, 1 },	/* G[2:0],B[4:0]  	   -[0],R[4:0],G[4:3]			*/
	{ B_RGBA15, 2, 1 },	/* G[2:0],B[4:0]  	   A[0],R[4:0],G[4:3]			*/
	{ B_CMAP8, 1, 1 },	/* D[7:0]  											*/
	{ B_GRAY8, 1, 1 },	/* Y[7:0]											*/
	{ B_GRAY1, 1, 8 },	/* Y0[0],Y1[0],Y2[0],Y3[0],Y4[0],Y5[0],Y6[0],Y7[0]	*/

	/* big endian version, when the encoding is not endianess independent */
	{ B_RGB32_BIG, 4, 1 },	/* -[7:0]  R[7:0]  G[7:0]  B[7:0]					*/
	{ B_RGBA32_BIG, 4, 1 },	/* A[7:0]  R[7:0]  G[7:0]  B[7:0]					*/
	{ B_RGB24_BIG, 3, 1 },	/* R[7:0]  G[7:0]  B[7:0]							*/
	{ B_RGB16_BIG, 2, 1 },	/* R[4:0],G[5:3]  G[2:0],B[4:0]						*/
	{ B_RGB15_BIG, 2, 1 },	/* -[0],R[4:0],G[4:3]  G[2:0],B[4:0]				*/
	{ B_RGBA15_BIG, 2, 1 },	/* A[0],R[4:0],G[4:3]  G[2:0],B[4:0]				*/

	/* little-endian declarations, for completness */
	{ B_RGB32_LITTLE, 4, 1 },
	{ B_RGBA32_LITTLE, 4, 1 },
	{ B_RGB24_LITTLE, 3, 1 },
	{ B_RGB16_LITTLE, 2, 1 },
	{ B_RGB15_LITTLE, 2, 1 },
	{ B_RGBA15_LITTLE, 2, 1 },

	/* non linear color space -- note that these are here for exchange purposes;	*/
	/* a BBitmap or BView may not necessarily support all these color spaces.	*/

	/* Loss/Saturation points are Y 16-235 (absoulte); Cb/Cr 16-240 (center 128) */

	{ B_YCbCr422, 4, 2 },	/* Y0[7:0]  Cb0[7:0]  Y1[7:0]  Cr0[7:0]  Y2[7:0]...	*/
								/* Cb2[7:0]  Y3[7:0]  Cr2[7:0]						*/
	{ B_YCbCr411, 12, 8 },	/* Cb0[7:0]  Y0[7:0]  Cr0[7:0]  Y1[7:0]  Cb4[7:0]...*/
								/* Y2[7:0]  Cr4[7:0]  Y3[7:0]  Y4[7:0]  Y5[7:0]...	*/
								/* Y6[7:0]  Y7[7:0]	 								*/
	{ B_YCbCr444, 3, 1 },	/* Y0[7:0]  Cb0[7:0]  Cr0[7:0]		*/
	{ B_YCbCr420, 3, 2 },	/* Non-interlaced only, Cb0  Y0  Y1  Cb2 Y2  Y3  on even scan lines ... */
								/* Cr0  Y0  Y1  Cr2 Y2  Y3  on odd scan lines */

	/* Extrema points are Y 0 - 207 (absolute) U -91 - 91 (offset 128) V -127 - 127 (offset 128) */
	/* note that YUV byte order is different from YCbCr */
	/* USE YCbCr, not YUV, when that's what you mean (which is most video!) */
	{ B_YUV422, 4, 2 }, /* U0[7:0]  Y0[7:0]   V0[7:0]  Y1[7:0] ... */
								/* U2[7:0]  Y2[7:0]   V2[7:0]  Y3[7:0]  */
	{ B_YUV411, 12, 8 }, /* U0[7:0]  Y0[7:0]  Y1[7:0]  V0[7:0]  Y2[7:0]  Y3[7:0]  */
								/* U4[7:0]  Y4[7:0]  Y5[7:0]  V4[7:0]  Y6[7:0]  Y7[7:0]  */
	{ B_YUV444, 3, 1 },	/* U0[7:0]  Y0[7:0]  V0[7:0]  U1[7:0]  Y1[7:0]  V1[7:0] */
	{ B_YUV420, 6, 4 },	/* Non-interlaced only, U0  Y0  Y1  U2 Y2  Y3  on even scan lines ... */
								/* V0  Y0  Y1  V2 Y2  Y3  on odd scan lines */
	{ B_YUV9, 5, 4 },	/* planar?	410?								*/
	{ B_YUV12, 6, 4 },	/* planar?	420?								*/

	{ B_UVL24, 3, 1 },	/* U0[7:0] V0[7:0] L0[7:0] ... */
	{ B_UVL32, 4, 1 },	/* U0[7:0] V0[7:0] L0[7:0] X0[7:0]... */
	{ B_UVLA32, 4, 1 },	/* U0[7:0] V0[7:0] L0[7:0] A0[7:0]... */

	{ B_LAB24, 3, 1 },	/* L0[7:0] a0[7:0] b0[7:0] ...  (a is not alpha!) */
	{ B_LAB32, 4, 1 },	/* L0[7:0] a0[7:0] b0[7:0] X0[7:0] ... (b is not alpha!) */
	{ B_LABA32, 4, 1 },	/* L0[7:0] a0[7:0] b0[7:0] A0[7:0] ... (A is alpha) */

	/* red is at hue = 0 */

	{ B_HSI24, 3, 1 },	/* H[7:0]  S[7:0]  I[7:0]							*/
	{ B_HSI32, 4, 1 },	/* H[7:0]  S[7:0]  I[7:0]  X[7:0]					*/
	{ B_HSIA32, 4, 1 },	/* H[7:0]  S[7:0]  I[7:0]  A[7:0]					*/

	{ B_HSV24, 3, 1 },	/* H[7:0]  S[7:0]  V[7:0]							*/
	{ B_HSV32, 4, 1 },	/* H[7:0]  S[7:0]  V[7:0]  X[7:0]					*/
	{ B_HSVA32, 4, 1 },	/* H[7:0]  S[7:0]  V[7:0]  A[7:0]					*/

	{ B_HLS24, 3, 1 },	/* H[7:0]  L[7:0]  S[7:0]							*/
	{ B_HLS32, 4, 1 },	/* H[7:0]  L[7:0]  S[7:0]  X[7:0]					*/
	{ B_HLSA32, 4, 1 },	/* H[7:0]  L[7:0]  S[7:0]  A[7:0]					*/

	{ B_CMY24, 3, 1 },	/* C[7:0]  M[7:0]  Y[7:0]  			No gray removal done		*/
	{ B_CMY32, 4, 1 },	/* C[7:0]  M[7:0]  Y[7:0]  X[7:0]	No gray removal done		*/
	{ B_CMYA32, 4, 1 },	/* C[7:0]  M[7:0]  Y[7:0]  A[7:0]	No gray removal done		*/
	{ B_CMYK32, 4, 1 },	/* C[7:0]  M[7:0]  Y[7:0]  K[7:0]					*/
	{ B_NO_COLOR_SPACE, 0, 0 }
};

status_t
get_pixel_size_for(
	color_space space,
	size_t * pixel_chunk,
	size_t * row_alignment,
	size_t * pixels_per_chunk)
{
	for (int ix=0; s_pixel_info[ix].space; ix++) {
		if (s_pixel_info[ix].space == space) {
			if (pixel_chunk) *pixel_chunk = s_pixel_info[ix].chunk;
			if (row_alignment) {
				uint align = 4;
				while (align < s_pixel_info[ix].chunk) {
					align <<= 1;
				}
				*row_alignment = align;
			}
			if (pixels_per_chunk) *pixels_per_chunk = s_pixel_info[ix].pixels;
			return B_OK;
		}
	}
	return B_BAD_VALUE;
}
