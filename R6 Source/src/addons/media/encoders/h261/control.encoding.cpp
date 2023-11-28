//=============================================================================
#include "control.encoding.h"
#include "control.dct.h"
//=============================================================================
#include <stdio.h>
#include <memory.h>
//=============================================================================
#include <OS.h>
//=============================================================================
encoding_control::encoding_control()
{
	dct_init();
	previously_sent.set_black();
}
//=============================================================================
encoding_control::~encoding_control()
{
	stream.save2();
}
//=============================================================================
void encoding_control::new_frame_352_288_422(unsigned char *buffer)
{
	static int frame_counter = 0; 
	bigtime_t t1,t2;
	
	int decision_t[22*18];			//type (nothing, inter, intra)
	int decision_q_inter[22*18];	//q used for inter
	int decision_q_intra[22*18];	//q used for intra
	int q_controlled = 3;
	
	//make DCT of new frame
	t1 = real_time_clock_usecs();
	actual.set_422(buffer);
	t2 = real_time_clock_usecs();
	printf("%40s = %7d usecs\n", "making DCT, 422 conversion", (int)(t2-t1));
			
	//calculate delta
	t1 = real_time_clock_usecs();
	delta.set_diff(&actual, &previously_sent);
	t2 = real_time_clock_usecs();
	printf("%40s = %7d usecs\n", "calculate delta", (int)(t2-t1));


	//perform quantization
	t1 = real_time_clock_usecs();
	 delta.quantify(q_controlled, 1, 2);
	actual.quantify(q_controlled, 1, 2);
	t2 = real_time_clock_usecs();
	printf("%40s = %7d usecs\n", "quantify (delta and actual)", (int)(t2-t1));

	if (frame_counter == -1)
	{
		int i;
		printf("Y1\n");
		for (i=0;i<64;i++)
		{
			int idx = (22*11+17)*64+i;
			printf("(%5d %5d %5d) ", actual.Y1[idx], previously_sent.Y1[idx]/(2*q_controlled), delta.Y1[idx]);
			if (i%8 == 7) printf("\n");
		}
		printf("Y2\n");
		for (i=0;i<64;i++)
		{
			int idx = (22*11+17)*64+i;
			printf("(%5d %5d %5d) ", actual.Y2[idx], previously_sent.Y2[idx]/(2*q_controlled), delta.Y2[idx]);
			if (i%8 == 7) printf("\n");
		}
		printf("Y3\n");
		for (i=0;i<64;i++)
		{
			int idx = (22*11+17)*64+i;
			printf("(%5d %5d %5d) ", actual.Y3[idx], previously_sent.Y3[idx]/(2*q_controlled), delta.Y3[idx]);
			if (i%8 == 7) printf("\n");
		}
		printf("Y4\n");
		for (i=0;i<64;i++)
		{
			int idx = (22*11+17)*64+i;
			printf("(%5d %5d %5d) ", actual.Y4[idx], previously_sent.Y4[idx]/(2*q_controlled), delta.Y4[idx]);
			if (i%8 == 7) printf("\n");
		}
	}
	
	//take decision
	t1 = real_time_clock_usecs();
	if (frame_counter == 0)
	{
		//send a full intra frame
		int i;
		for (i=0;i<22*18;i++)
		{
			decision_t[i]=0;
		}
	}
	else
	{
		int i,j;
		for (i=0;i<22*18;i++)
		{
			int sum_actual = 0;
			int sum_delta  = 0;
			
			for (j=0;j<64;j++)
			{
				//printf("%d %d\n", sum_delta, sum_actual);
				int idx = i*64+j;
				
				sum_actual += actual.Y1[idx] * actual.Y1[idx];
				sum_actual += actual.Y2[idx] * actual.Y2[idx];
				sum_actual += actual.Y3[idx] * actual.Y3[idx];
				sum_actual += actual.Y4[idx] * actual.Y4[idx];
				sum_actual += actual.U [idx] * actual.U [idx];
				sum_actual += actual.V [idx] * actual.V [idx];
				
				sum_delta  += delta.Y1[idx] * delta.Y1[idx];
				sum_delta  += delta.Y2[idx] * delta.Y2[idx];
				sum_delta  += delta.Y3[idx] * delta.Y3[idx];
				sum_delta  += delta.Y4[idx] * delta.Y4[idx];
				sum_delta  += delta.U [idx] * delta.U [idx];
				sum_delta  += delta.V [idx] * delta.V [idx];
				
				//get more importance to constant variation
				if (j==0)
				{
					sum_delta  *= 4;
					sum_actual *= 4; //to make fair comparison
				}
			}
			//printf("* %d %d\n", sum_delta, sum_actual);
			//printf("===\n");
			if (sum_delta < 35) //== 0)
			{
				decision_t[i] = 2; //send nothing
			}
			else
			{
				if (actual.q_used[i]<delta.q_used[i])
				{
					decision_t[i] = 0; //send full
				}
				if (sum_actual <= sum_delta)
				{
					decision_t[i] = 0; //send full
				}
				else
				{
					decision_t[i] = 1; //send delta
				}
			}
		}
	}
	//decision[22*11+17] = 2;
	
	
	t2 = real_time_clock_usecs();
	char message[150];
	{
		int delta = 0;
		int intra = 0;
		int nothing = 0;
		int i;
		for (i=0;i<22*18;i++)
		{
			switch (decision_t[i])
			{
				case 0:
					intra++;
					break;
				case 1:
					delta++;
					break;
				case 2:
					nothing++;
					break;
			}
		}
		sprintf(message, "delta:%4d intra:%4d nothing:%4d", delta, intra, nothing);
	}
	printf("%40s = %7d usecs %s\n", "delta/intra/nothing decision", (int)(t2-t1), message);

	//encode bitstream
	t1 = real_time_clock_usecs();
	stream.encode_picture
		(
		&actual,
		&delta,
		decision_t,
		q_controlled
		);
	t2 = real_time_clock_usecs();
	printf("%40s = %7d usecs\n", "generating bitstream", (int)(t2-t1));
	
	//update reference picture
	t1 = real_time_clock_usecs();
	{
		int i,j;
		for (i=0;i<22*18;i++)
		{
			if (decision_t[i] == 0) //intra
			{
				previously_sent.mb_copy_ref(&actual, i, q_controlled);
			}
			else if (decision_t[i] == 1) //delta
			{
				previously_sent.mb_add_delta(&delta, i, q_controlled);
			}
			else //(decision[i] == 2) //nothing
			{
			}
		}
	}
	t2 = real_time_clock_usecs();
	printf("%40s = %7d usecs\n", "update of ref picture", (int)(t2-t1));
	
	frame_counter++;
}
//=============================================================================
