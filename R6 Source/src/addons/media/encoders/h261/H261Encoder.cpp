//=============================================================================
#include "H261Encoder.h"
//=============================================================================
#include <memory.h>
//=============================================================================
H261EncoderStream2::H261EncoderStream2()
{
	//clear the bit buffer
	nbb_ = 0;
	bb_  = 0;
	bs_  = 0;
	bc_  = 0;
	
	//allocate and 'initialize' the destination buffer
	bs_ = new unsigned char[33000000];
	{
		for (int i=0;i<3300000;i++)
			bs_[i] = 0xff;
	}
	bc_ = bs_;	//buffer current = buffer start

	//we encode CIF...
	pt = &cif;
	//...with full quality
	q[0] = 12;
	q[1] = 8;
	q[2] = 5; 
	q[3] = 3; 
	q[4] = 2; 
	q[5] = 1;
	

//	previous_data = new u_char[352*288 + 176*144 +176*144];
//	memset(previous_data, 0, 352*288 + 176*144 +176*144);

//	previous_code = new u_char[22*18];
//	memset(previous_code, 1, 22*18);
//	actual_code   = new u_char[22*18];
//	acc_error     = new u_int [22*18];

}
//=============================================================================
void H261EncoderStream2::save2()
{
	int size;

	//calculate size used
	size = ((bc_ - bs_) << 3) + nbb_;
	size += 7;
	size /= 8;

	
	printf("%d bits\n", nbb_ + ((bc_-bs_) << 3));

	if (nbb_ != 0)
	{
		PUT_BITS(0,NBIT,nbb_, bb_, bc_);
	}
	/*
	printf("%d bytes\n", bc_-bs_);
	printf("%d bits in buffer\n", nbb_);
	*/

	printf("saving %d bytes\n", size);
	FILE *p64 = fopen("test.p64", "wb");
	fwrite(bs_, size, 1, p64);
	fclose(p64);
}
//=============================================================================
