
#include <unistd.h> 
#include <scsi.h> 
#include <stdio.h>
#include "cdda_io.h"

typedef struct { 
  uint64  offset;    /* in device blocks */ 
  uint64  blocks;    /* number of blocks in session */ 
  bool    data;      /* true: data session, session */ 
} session_data;


extern "C" _EXPORT  status_t ds_get_nth_session(int32 dev, int32 index, int32 block_size, session_data *session);


status_t ds_get_nth_session(int32 dev, int32 index, int32 block_size, session_data *session)
{
	t_CDtoc toc;
	status_t result;
	uint64 bias;

	result = ioctl(dev, B_SCSI_GET_TOC, &toc);
	
	
	if(result==B_OK)
	{
		//printf("session add-on loaded in thread %d for CD: index %d, blocksize=%d\n",find_thread(NULL),index,block_size);
		int32 numtracks=toc.maxtrack-toc.mintrack+1;
		int32 numdatatracks=0;
		int32 numaudiotracks=0;
		for(int i=0;i<numtracks;i++)
			if(toc.tocentry[i].flags&4)
				numdatatracks++;
			else
				numaudiotracks++;

		t_CDaddress ba=toc.tocentry[0].address;
		bias=(((ba.minute*60)+ba.second)*75)+ba.frame;

		// this addons returns "virtual" sessions, as if all the audio tracks are a single session
		// at the end of the disk.
//		printf("_numtracks(t/d/a):%d/%d/%d\n",numtracks,numdatatracks,numaudiotracks);

		if(index<numdatatracks)
		{
			// return the Nth data session
			for(int i=0;i<numtracks;i++)
			{
				if(toc.tocentry[i].flags&4)
				{
					index--;
					if(index<0)
					{
						// found Nth datasession
						t_CDaddress ta=toc.tocentry[i].address;
						session->offset=(((ta.minute*60)+ta.second)*75)+ta.frame;
						ta=toc.tocentry[i+1].address;
						session->blocks=(((ta.minute*60)+ta.second)*75)+ta.frame-session->offset;
						session->offset-=bias;
						session->data=true;
						break;
					}
				}
			}
		
		}
		else if((index==numdatatracks)&&(numaudiotracks>0))
		{
			// return *all* audio sessions as one big session
			session->offset=0x7fffffff;
			session->blocks=0;
			for(int i=0;i<numtracks;i++)
			{
				if(!(toc.tocentry[i].flags&4))
				{
					// found Nth audiosession
					t_CDaddress ta1=toc.tocentry[i].address;
					t_CDaddress ta2=toc.tocentry[i+1].address;
					if(session->offset==0x7fffffff)
						session->offset=(((ta1.minute*60)+ta1.second)*75)+ta1.frame;
					session->blocks+= (((ta2.minute*60)+ta2.second)*75)+ta2.frame-(((ta1.minute*60)+ta1.second)*75)+ta1.frame;
					session->data=true; // make DriveSetup try partition and session add-ons on this
				}
			}
//			session->offset-=bias;
			session->offset=0x7fffffff; // use fake offset instead of real one
		}
		else
			result=B_ERROR;
	}
//	else
//		printf("session add-on loaded for non-CD");

	
//	if(result==B_OK)
//		printf("session at %Lx\n",session->offset);
	
	return result;
}


