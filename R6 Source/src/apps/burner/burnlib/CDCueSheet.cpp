
#include <stdio.h>
#include <stdlib.h>
#include "CDCueSheet.h"
#include "CDTrack.h"
 
static int msf2lba(int min, int sec, int frame)
{
	return (((min * 60) + sec) * 75) + frame - 150;
}

static void adj_msf_time(int *min, int *sec, int *frame, int adjframes)
{
	*frame += adjframes;
	
	if(*frame >= 75){
		*sec += (*frame / 75);
		*frame = (*frame % 75);
		if(*sec >= 60){
			*min += (*sec / 60);
			*sec = (*sec % 60);
		}
	}
}

static void SetForm(uchar &_data_form0, uchar &_data_form1, uchar &_ctl_adr, CDTrack *t)
{
	if(t && t->IsData()){
		_data_form0 = 0x14;
		_data_form1 = 0x10;
		_ctl_adr = 0x41;
	} else {
		_data_form0 = 0x01;
		_data_form1 = 0x00;
		_ctl_adr = 0x01;
	}
}

CDCueLine *build_cuesheet(CDTrack *t, uint *linecount)
{
	int lines = 1;
	int cur_min = 0;
	int cur_sec = 0;
	int cur_frame = 0;
	int cur_track = 1;
	int tmp;
	
	CDCueLine *cue;
	
	int i;
	CDTrack *t0 = t;
	i = 2; /* lead-in / lead-out */
	while(t0){
		i+=2;
		t0 = t0->Next();
	}
	cue = (CDCueLine *) malloc(sizeof(CDCueLine) * i);
	
	uchar _data_form0;
	uchar _data_form1;
	uchar _ctl_adr;
	SetForm(_data_form0, _data_form1, _ctl_adr, t);	
	
	/* lead-in */
	cue[0].ctl_adr = _ctl_adr;
	cue[0].trackno = 0;
	cue[0].index = 0;
	cue[0].data_form = _data_form0;
	cue[0].scms = 0;
	cue[0].min = 0;
	cue[0].sec = 0;
	cue[0].frame = 0;
	fprintf(stderr,"lead-in          @ %02d:%02d:%02d  LBA = %8d\n",
			cur_min, cur_sec, cur_frame, -150);

	while(t){
		t->SetLBA(msf2lba(cur_min, cur_sec, cur_frame));
		SetForm(_data_form0, _data_form1, _ctl_adr, t);	
		
		/* pre-gap */
		if(t->PreGap()){
			cue[lines].ctl_adr = _ctl_adr;
			cue[lines].trackno = cur_track;
			cue[lines].index = 0;
			cue[lines].data_form = _data_form1;
			cue[lines].scms = 0;
			cue[lines].min = cur_min;
			cue[lines].sec = cur_sec;
			cue[lines].frame = cur_frame;
			lines++;			
			fprintf(stderr,"track %02d (%02x %02x) pre-gap @ %02d:%02d:%02d  LBA = %8ld  len = %8ld\n",
				   cur_track, _ctl_adr, _data_form1, cur_min, cur_sec, cur_frame, t->LBA(), t->PreGap());
			adj_msf_time(&cur_min, &cur_sec, &cur_frame, t->PreGap());
		
		}
		
		/* data */
		cue[lines].ctl_adr = _ctl_adr;
		cue[lines].trackno = cur_track;
		cue[lines].index = 1;
		cue[lines].data_form = _data_form1;
		cue[lines].scms = 0;
		cue[lines].min = cur_min;
		cue[lines].sec = cur_sec;
		cue[lines].frame = cur_frame;
		lines++;			

		tmp = msf2lba(cur_min, cur_sec, cur_frame);
		fprintf(stderr,"track %02d (%02x %02x) data    @ %02d:%02d:%02d  LBA = %8d  len = %8ld\n",
				cur_track, _ctl_adr, _data_form1, cur_min, cur_sec, cur_frame, tmp, t->Length() - t->PreGap());
		adj_msf_time(&cur_min, &cur_sec, &cur_frame, t->Length() - t->PreGap());
		
		cur_track++;
		t = t->Next();
	}
	
	/* lead-out */
	cue[lines].ctl_adr = _ctl_adr;
	cue[lines].trackno = 0xaa;
	cue[lines].index = 0x01;
	cue[lines].data_form = _data_form0;
	cue[lines].scms = 0;
	cue[lines].min = cur_min;
	cue[lines].sec = cur_sec;
	cue[lines].frame = cur_frame;
	lines++;	

	fprintf(stderr,"lead-out         @ %02d:%02d:%02d  LBA = %8d\n",
			cur_min, cur_sec, cur_frame, msf2lba(cur_min,cur_sec,cur_frame));

//	for(int i=0;i<lines;i++){
//		fprintf(stderr,"%02d: %02x %02x %02x %02x %02x %02x %02x %02x\n",i,
//				cue[i].ctl_adr, cue[i].trackno, cue[i].index, cue[i].data_form,
//				cue[i].scms, cue[i].min, cue[i].sec, cue[i].frame);
//	}
	
	*linecount = lines;
	return cue;
}


CDCueSheet::CDCueSheet(CDTrack *tracks)
{
	lines = build_cuesheet(tracks, &linecount);
}


CDCueSheet::~CDCueSheet(void)
{
	free(lines);
}

void 
CDCueSheet::SetTracks(CDTrack *tracks)
{
	free(lines);
	lines = build_cuesheet(tracks, &linecount);
}

CDCueLine *
CDCueSheet::Lines(void)
{
	return lines;
}

uint 
CDCueSheet::LineCount(void)
{
	return linecount;
}

