/*
	
	ClipList.cpp
	
	Copyright 1998 Be Incorporated, All Rights Reserved.
	
*/

#include <stdio.h>
#include "ClipList.h"

//------------------------------------------------------------------------------

	void				sort_trans(int32 *lptr1, int32 count);
	void				sort_trans_spec(int32 *lptr1, int32 count);
	void				do_new(	BRegion *r, int32 vp,
								BRect frame, bt848_cliplist clip_list,
								int32 *trans, int16 *cur,
								int32 *cur_cnt, int32 *r_cnt,
								int32 *first_active);
	void				do_same(int32 vp, BRect frame, bt848_cliplist clip_list,
								int16* cur, int32 *cur_cnt);
	
//------------------------------------------------------------------------------

status_t
ClipListFromRegion(BRegion *clip_region, BRect frame, bt848_cliplist clip_list)
{
	BRect	r;
	int32	cnt;
	int32	i;
	int32	*vtrans;	
	int32	vtc;
	int32	vtp;
	int32	vp;
	int32	last;

	int32	trans[B_VIDEO_V_MAX];
	int16	cur[B_VIDEO_V_MAX];
	int32	cur_cnt, r_cnt;
	int32	first_active;

	/* if empty region, then clip entire frame */			
	if ((cnt = clip_region->CountRects()) == 0)
	{
		for ( i = 0; i < (frame.bottom - frame.top + 1); i++)
		{
			clip_list[i][0] = (int16) ( - ((frame.right - frame.left) + 1));
			clip_list[i][1] = 0;
		}
		return B_NO_ERROR;
	}
		
	vtrans = (int32 *)malloc(16 + sizeof(int32) * cnt * 2);
	vtc = 0;		
	vtrans[vtc] = 0;
	vtc++;
	
	for (i = 0; i < cnt; i++) {
		r = clip_region->RectAt(i);
		if (!r.IsValid())
		{
			free((char *)vtrans);
			return B_ERROR;
		}
		vtrans[vtc] = (int32) r.top;
		vtc++;
		vtrans[vtc] = (int32) r.bottom + 1;
		vtc++;
	}
		
	sort_trans(vtrans, vtc);

	vtp = 0;
	vp = 0;
	first_active = 0;
	
	last = (int32) frame.bottom+1;		
	while(vp < last) {
		if (vp == vtrans[vtp])
		{
			do_new(clip_region, vp, frame, clip_list, trans, cur, &cur_cnt, &r_cnt, &first_active);
			vtp++;
			while(vp == vtrans[vtp])
			{
				vtp++;
			}
		}
		else
		{
			do_same(vp, frame, clip_list, cur, &cur_cnt);
		}
		vp++;
	}

	free((char *)vtrans);

/*
	int j;
	int16 *p;
	for(i = 0; i < last; i++)
	{
		printf("Line %d:  ",i);
		j = 0;
		p = clip_list[i];
		while (p[j] != 0)
			printf("%03d ",p[j++]);
		printf("\n  ");	
	}
*/
	return B_NO_ERROR;
}

/* private functions */

//------------------------------------------------------------------------------

void
sort_trans(int32 *lptr1, int32 count)
{
	char	again;
	int32	i, k;
	int32	v1, v2;
	int32	step, last;

	step = count;
	while (step != 1)
	{
		step = (step / 3) - 1;
		if (step <= 0)
			step = 1;

		again = TRUE;
		last = count;
		
		while (again)
		{
			again = 0;
			k = 0;
			do
			{
				i = k + step;
				do
				{
					v1 = lptr1[i];
					v2 = lptr1[i - step];				
					if (v1 < v2)
					{
						lptr1[i] = v2;
						lptr1[i - step] = v1;
						again = TRUE;
					}
					i += step;
				} while (i < count);				
				k++;
			} while (k < step);
		}
	}
}

//------------------------------------------------------------------------------

void
sort_trans_spec(int32 *lptr1, int32 count)
{
	char	again;
	int32	i, k;
	int32	v1, v2;
	int32	p1, p2;
	int32	step, last;

	step = count;

	while (step != 1)
	{
		step = (step / 3) - 1;
		if (step <= 0)
			step = 1;

		again = TRUE;
		last = count;
		
		while (again)
		{
			again = 0;
			k = 0;
			do
			{
				i = k + step;
				do
				{
					v1 = lptr1[i];
					v2 = lptr1[i - step];
					
					p1 = v1;
					p2 = v2;
					
					if (p1 >= 32768)
						p1 -= 32768;
					if (p2 >= 32768)
						p2 -= 32768;
				
					if (p1 < p2)
					{
						lptr1[i] = v2;
						lptr1[i - step] = v1;
						again = TRUE;
					}
					i += step;
				} while (i < count);				
				k++;
			} while (k < step);
		}
	}
}

//------------------------------------------------------------------------------

void
do_new(BRegion *r, int32 vp, BRect frame, bt848_cliplist clip_list, int32 *trans, int16 * cur
, int32 *cur_cnt, int32 *r_cnt, int32 *first_active)
{
	int32	cnt;
	BRect	tmp_rect;
	int32	i;
	int32	last;
	int32	clip_array_pos;
	int32	cv;
	int32	copy_active;
	
	*cur_cnt = 0;
	*r_cnt = 0;
	copy_active = *first_active;	
	*first_active = 32000;
	
	cnt = r->CountRects();
	
	for (i = copy_active; i < cnt; i++)
	{
		tmp_rect = r->RectAt(i);
		if (tmp_rect.bottom >=vp)
		{
			if (*first_active > cnt)
			{
				*first_active = i;
			}
			if ((tmp_rect.top <= vp))
			{
				trans[*r_cnt] = (int32) tmp_rect.left;
				(*r_cnt)++;
				trans[*r_cnt] = (int32) tmp_rect.right + 32768 + 1;
				(*r_cnt)++;
			}
		}
		if (tmp_rect.top > vp)
			break;			
	}

	trans[*r_cnt] = (int32)frame.right + 1;
	(*r_cnt)++;
	
	sort_trans_spec(trans, *r_cnt);
	
	i = 0;	
	last = 0;
	clip_array_pos = 0;

	if (trans[0] == 0)
	{
		i = 1;
		(clip_list[vp])[clip_array_pos] = (trans[i] - 32768);		
		cur[clip_array_pos] =  (trans[i] - 32768);
		clip_array_pos++;
		last = trans[i] - 32768;
		i++;
	}

	while(i < *r_cnt)
	{
		cv = trans[i];
		if (cv >= 32768)
		{
			(clip_list[vp])[clip_array_pos] = (cv - 32768) - last;
			cur[clip_array_pos] =  (cv - 32768) - last;
			clip_array_pos++;
			last = cv - 32768;
		}
		else
		{
			(clip_list[vp])[clip_array_pos] = -(cv - last);
			cur[clip_array_pos] =  -(cv - last);
			clip_array_pos++;
			last = cv;
		}
		i++;
		
		while(trans[i] == trans[i - 1])
		{
			i++;
		}
	
		(clip_list[vp])[clip_array_pos] = 0;
		cur[clip_array_pos] =  0;
		*cur_cnt = clip_array_pos;
	}
}

//------------------------------------------------------------------------------

void
do_same(int32 vp, BRect , bt848_cliplist clip_list, int16 *cur, int32 *cur_cnt)
{
	memcpy(&clip_list[vp][0],  cur, (*cur_cnt) * sizeof(int));
}
