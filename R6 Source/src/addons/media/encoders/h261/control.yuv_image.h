//=============================================================================
#ifndef __INCLUDE_control_yuv_image
#define __INCLUDE_control_yuv_image
//=============================================================================
class yuv_image
{
public:
	short Y1[22*18*64];
	short Y2[22*18*64];
	short Y3[22*18*64];
	short Y4[22*18*64];

	short U[22*18*64];
	short V[22*18*64];
	
	int q_used[22*18];
	
public:
	void set_black();
	void set_422(unsigned char *buffer);
	void set_diff(yuv_image *n, yuv_image *n_1);
	void mb_add_delta(yuv_image *delta, int mb, int q);
	void mb_copy_ref (yuv_image *ref,   int mb, int q);
	void quantify(int q, int thre_y, int thre_c);
};
//=============================================================================
#endif
//=============================================================================
