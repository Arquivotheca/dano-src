//=============================================================================
#include "H261Encoder.h"
//=============================================================================
#include "control.dct.h"
#include "h261_table.h"
#include <memory.h>
//=============================================================================
void H261EncoderStream2::encode_macro_block_intra
(
	u_int mba,		//number of this macro block within the gob
	const yuv_image *picture,
	u_int x,
	u_int y
)
{
	//start with the desired quantizer
	int q_to_use = picture->q_used[x+22*y];

	//write macro block header
	/* MBA */
	{
		//delta between last macro block transmitted
		u_int macro_block_adress = mba-mba_;
	
		huffent* he = &hte_mba[macro_block_adress - 1];
		PUT_BITS(he->val, he->nb, nbb_, bb_, bc_);

		//update mba_
		mba_ = mba;
	}

	/* MTYPE */
	if (q_to_use != mquant_) 
	{
		PUT_BITS(1,        7, nbb_, bb_, bc_);	// MTYPE = (INTRA) + MQUANT + TCOEFF
		PUT_BITS(q_to_use, 5, nbb_, bb_, bc_);	// MQUANT
		//update mquant_ (last quant transmitted to the other side)
		mquant_ = q_to_use;
	} 
	else 
	{		
		PUT_BITS(1, 4, nbb_, bb_, bc_);			// MTYPE = INTRA + TCOEFF (no quantizer)
	}

	//write the blocks (TCOEFF)
	int base_idx = 64*(x+22*y);
	encode_block_intra(&picture->Y1[base_idx], q_to_use);	// Y 1
	encode_block_intra(&picture->Y2[base_idx], q_to_use);	// Y 2		
	encode_block_intra(&picture->Y3[base_idx], q_to_use);	// Y 3
	encode_block_intra(&picture->Y4[base_idx], q_to_use);	// Y 4
	encode_block_intra(&picture-> U[base_idx], q_to_use);	// U
	encode_block_intra(&picture-> V[base_idx], q_to_use);	// V
}
//=============================================================================
void H261EncoderStream2::encode_macro_block_inter
(
	u_int mba,		//number of this macro block within the gob
	const yuv_image *picture,
	u_int x,
	u_int y
)
{
	//start with the desired quantizer
	int q_to_use;// = gquant;//q[actual_code[x+22*y]-1];
	q_to_use = picture->q_used[x+22*y];

	//encode the 6 blocks
	const short *blk_ptr[6];
	int base_idx = 64*(x+22*y);
	
	blk_ptr[0] = &picture->Y1[base_idx];
	blk_ptr[1] = &picture->Y2[base_idx];
	blk_ptr[2] = &picture->Y3[base_idx];
	blk_ptr[3] = &picture->Y4[base_idx];
	blk_ptr[4] = &picture->U [base_idx];
	blk_ptr[5] = &picture->V [base_idx];
	

	//calculate cbp
	int cbp = 0;

	{
		int sum;
		int i;
		
		sum = 0;
		for (i=0;i<64;i++)
		{
			sum += blk_ptr[0][i]*blk_ptr[0][i];
		}
		if (sum != 0)
		{
			cbp += 32;
		}
		
		sum = 0;
		for (i=0;i<64;i++)
		{
			sum += blk_ptr[1][i]*blk_ptr[1][i];
		}
		if (sum != 0)
		{
			cbp += 16;
		}

		sum = 0;
		for (i=0;i<64;i++)
		{
			sum += blk_ptr[2][i]*blk_ptr[2][i];
		}
		if (sum != 0)
		{
			cbp += 8;
		}

		sum = 0;
		for (i=0;i<64;i++)
		{
			sum += blk_ptr[3][i]*blk_ptr[3][i];
		}
		if (sum != 0)
		{
			cbp += 4;
		}
		
		sum = 0;
		for (i=0;i<64;i++)
		{
			sum += blk_ptr[4][i]*blk_ptr[4][i];
		}
		if (sum != 0)
		{
			cbp += 2;
		}

		sum = 0;
		for (i=0;i<64;i++)
		{
			sum += blk_ptr[5][i]*blk_ptr[5][i];
		}
		if (sum != 0)
		{
			cbp += 1;
		}
	}


	if (cbp == 0)
	{
		printf("SHOULD NEVER HAPPEN 7\n");
		return;
	}

	static int cbp_stat[64];
	{
		static bool first = true;
		if (first)
		{
			first=false;
			int i;
			for (i=0;i<64;i++)
			{
				cbp_stat[i] = 0;
			}
		}
	}
	cbp_stat[cbp]++;

	//write macro block header
	/* MBA */
	{
		//delta between last macro block transmitted
		u_int macro_block_adress = mba-mba_;
	
		huffent* he = &hte_mba[macro_block_adress - 1];
		PUT_BITS(he->val, he->nb, nbb_, bb_, bc_);

		//update mba_
		mba_ = mba;
	}

	/* MTYPE */
	if (q_to_use != mquant_) 
	{
		PUT_BITS(1,        5, nbb_, bb_, bc_);	// MTYPE = (INTER) + MQUANT + CBP + TCOEFF
		PUT_BITS(q_to_use, 5, nbb_, bb_, bc_);	// MQUANT
		//update mquant_ (last quant transmitted to the other side)
		mquant_ = q_to_use;
	} 
	else 
	{
		PUT_BITS(1, 1, nbb_, bb_, bc_);			// MTYPE = (INTER) + CBP + TCOEFF (no quantizer)		
	}
	
	//CBP
	{
		//table 4/H.261
		huffent cbp_vlc[] =
		{
			{ 0,  0}, // 0 <<== should never happen
			{11,  5}, // 1 = 0101 1
			{ 9,  5}, // 2 = 0100 1
			{13,  6}, // 3 = 0011 01
			{13,  4}, // 4 = 1101
			{23,  7}, // 5 = 0010 111
			{19,  7}, // 6 = 0010 011
			{31,  8}, // 7 = 0001 1111
			{12,  4}, // 8 = 1100
			{22,  7}, // 9 = 0010 110
			{18,  7}, //10 = 0010 010
			{30,  8}, //11 = 0001 1110
			{19,  5}, //12 = 1001 1
			{27,  8}, //13 = 0001 1011
			{23,  8}, //14 = 0001 0111 
			{19,  8}, //15 = 0001 0011
			{11,  4}, //16 = 1011
			{21,  7}, //17 = 0010 101 
			{17,  7}, //18 = 0010 001
			{29,  8}, //19 = 0001 1101
			{17,  5}, //20 = 1000 1
			{25,  8}, //21 = 0001 1001
			{21,  8}, //22 = 0001 0101 
			{17,  8}, //23 = 0001 0001
			{15,  6}, //24 = 0011 11
			{15,  8}, //25 = 0000 1111
			{13,  8}, //26 = 0000 1101
			{ 3,  9}, //27 = 0000 0001 1
			{15,  5}, //28 = 0111 1
			{11,  8}, //29 = 0000 1011 
			{ 7,  8}, //30 = 0000 0111
			{ 7,  9}, //31 = 0000 0011 1
			{10,  4}, //32 = 1010
			{20,  7}, //33 = 0010 100 
			{16,  7}, //34 = 0010 000
			{28,  8}, //35 = 0001 1100
			{14,  6}, //36 = 0011 10
			{14,  8}, //37 = 0000 1110
			{12,  8}, //38 = 0000 1100
			{ 2,  9}, //39 = 0000 0001 0
			{16,  5}, //40 = 1000 0
			{24,  8}, //41 = 0001 1000
			{20,  8}, //42 = 0001 0100
			{16,  8}, //43 = 0001 0000
			{14,  5}, //44 = 0111 0
			{10,  8}, //45 = 0000 1010 
			{ 6,  8}, //46 = 0000 0110
			{ 6,  9}, //47 = 0000 0011 0
			{18,  5}, //48 = 1001 0
			{26,  8}, //49 = 0001 1010 
			{22,  8}, //50 = 0001 0110
			{18,  8}, //51 = 0001 0010
			{13,  5}, //52 = 0110 1
			{ 9,  8}, //53 = 0000 1001
			{ 5,  8}, //54 = 0000 0101
			{ 5,  9}, //55 = 0000 0010 1
			{12,  5}, //56 = 0110 0
			{ 8,  8}, //57 = 0000 1000
			{ 4,  8}, //58 = 0000 0100
			{ 4,  9}, //59 = 0000 0010 0
			{ 7,  3}, //60 = 111
			{10,  5}, //61 = 0101 0
			{ 8,  5}, //62 = 0100 0
			{12,  6}, //63 = 0011 00
		};
		
		huffent he = cbp_vlc[cbp];
		if (he.nb == 0)
		{
			printf("SHOULD NEVER HAPPEN 9\n");
		}
		PUT_BITS(he.val, he.nb, nbb_, bb_, bc_);

		if ((cbp & 32) != 0)
		{
			encode_block_inter(blk_ptr[0]);//, q_to_use, 1);	// Y 1
		}
		if ((cbp & 16) != 0)
		{
			encode_block_inter(blk_ptr[1]);//, q_to_use, 1);	// Y 2
		}
		if ((cbp & 8) != 0)
		{
			encode_block_inter(blk_ptr[2]);//, q_to_use, 1);	// Y 3
		}
		if ((cbp & 4) != 0)
		{
			encode_block_inter(blk_ptr[3]);//, q_to_use, 1);	// Y 4
		}
		if ((cbp & 2) != 0)
		{
			encode_block_inter(blk_ptr[4]);//, q_to_use, 2);	// U
		}
		if ((cbp & 1) != 0)
		{
			encode_block_inter(blk_ptr[5]);//, q_to_use, 2);	// V
		}
	}
	
	if (false)
	{
		static int pass_count;
		pass_count++;
		if (pass_count%1000 == 0)
		{
			int i;
			for (i=0;i<64;i++)
			{
				printf("%3d => %6d\n", i, cbp_stat[i]);
			}
		}
	}
}
//=============================================================================
