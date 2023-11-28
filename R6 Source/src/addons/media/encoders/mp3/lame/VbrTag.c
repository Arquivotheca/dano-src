/*
 *	Xing VBR tagging for LAME.
 *
 *	Copyright (c) 1999 A.L. Faber
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "machine.h"
#if defined(__riscos__) && defined(FPA10)
#include	"ymath.h"
#else
#include	<math.h>
#endif
#include "VbrTag.h"
#include "version.h"
#include "bitstream.h"

#ifdef _DEBUG
/*  #define DEBUG_VBRTAG */
#endif


const static char	VBRTag[]={"Xing"};
const int SizeOfEmptyFrame[2][2]=
{
	{17,9},
	{32,17},
};



/****************************************************************************
 * AddVbrFrame: Add VBR entry, used to fill the VBR the TOC entries
 * Paramters:
 *	nStreamPos: how many bytes did we write to the bitstream so far
 *				(in Bytes NOT Bits)
 ****************************************************************************
*/
void AddVbrFrame(lame_global_flags *gfp)
{
  int nStreamPos;
  lame_internal_flags *gfc=gfp->internal_flags;
  nStreamPos = (gfc->bs.totbit/8);

        /* Simple exponential growing buffer */
	if (gfp->pVbrFrames==NULL || gfp->nVbrFrameBufferSize==0)
	{
                /* Start with 100 frames */
		gfp->nVbrFrameBufferSize=100;

		/* Allocate them */
		gfp->pVbrFrames=(int*)malloc((size_t)(gfp->nVbrFrameBufferSize*sizeof(int)));
	}

	/* Is buffer big enough to store this new frame */
	if (gfp->nVbrNumFrames==gfp->nVbrFrameBufferSize)
	{
                /* Guess not, double th e buffer size */
		gfp->nVbrFrameBufferSize*=2;

		/* Allocate new buffer */
		gfp->pVbrFrames=(int*)realloc(gfp->pVbrFrames,(size_t)(gfp->nVbrFrameBufferSize*sizeof(int)));
	}

	/* Store values */
	gfp->pVbrFrames[gfp->nVbrNumFrames++]=nStreamPos;
}


/*-------------------------------------------------------------*/
static int ExtractI4(unsigned char *buf)
{
	int x;
	/* big endian extract */
	x = buf[0];
	x <<= 8;
	x |= buf[1];
	x <<= 8;
	x |= buf[2];
	x <<= 8;
	x |= buf[3];
	return x;
}

void CreateI4(unsigned char *buf, int nValue)
{
        /* big endian create */
	buf[0]=(nValue>>24)&0xff;
	buf[1]=(nValue>>16)&0xff;
	buf[2]=(nValue>> 8)&0xff;
	buf[3]=(nValue    )&0xff;
}


/*-------------------------------------------------------------*/
/* Same as GetVbrTag below, but only checks for the Xing tag.
   requires buf to contain only 40 bytes */
/*-------------------------------------------------------------*/
int CheckVbrTag(unsigned char *buf)
{
	int			h_id, h_mode, h_sr_index;

	/* get selected MPEG header data */
	h_id       = (buf[1] >> 3) & 1;
	h_sr_index = (buf[2] >> 2) & 3;
	h_mode     = (buf[3] >> 6) & 3;

	/*  determine offset of header */
	if( h_id )
	{
                /* mpeg1 */
		if( h_mode != 3 )	buf+=(32+4);
		else				buf+=(17+4);
	}
	else
	{
                /* mpeg2 */
		if( h_mode != 3 ) buf+=(17+4);
		else              buf+=(9+4);
	}

	if( buf[0] != VBRTag[0] ) return 0;    /* fail */
	if( buf[1] != VBRTag[1] ) return 0;    /* header not found*/
	if( buf[2] != VBRTag[2] ) return 0;
	if( buf[3] != VBRTag[3] ) return 0;
	return 1;
}

int GetVbrTag(VBRTAGDATA *pTagData,  unsigned char *buf)
{
	int			i, head_flags;
	int			h_id, h_mode, h_sr_index;
	static int	sr_table[4] = { 44100, 48000, 32000, 99999 };

	/* get Vbr header data */
	pTagData->flags = 0;

	/* get selected MPEG header data */
	h_id       = (buf[1] >> 3) & 1;
	h_sr_index = (buf[2] >> 2) & 3;
	h_mode     = (buf[3] >> 6) & 3;

	/*  determine offset of header */
	if( h_id )
	{
                /* mpeg1 */
		if( h_mode != 3 )	buf+=(32+4);
		else				buf+=(17+4);
	}
	else
	{
                /* mpeg2 */
		if( h_mode != 3 ) buf+=(17+4);
		else              buf+=(9+4);
	}

	if( buf[0] != VBRTag[0] ) return 0;    /* fail */
	if( buf[1] != VBRTag[1] ) return 0;    /* header not found*/
	if( buf[2] != VBRTag[2] ) return 0;
	if( buf[3] != VBRTag[3] ) return 0;

	buf+=4;

	pTagData->h_id = h_id;

	pTagData->samprate = sr_table[h_sr_index];

	if( h_id == 0 )
		pTagData->samprate >>= 1;

	head_flags = pTagData->flags = ExtractI4(buf); buf+=4;      /* get flags */

	if( head_flags & FRAMES_FLAG )
	{
		pTagData->frames   = ExtractI4(buf); buf+=4;
	}

	if( head_flags & BYTES_FLAG )
	{
		pTagData->bytes = ExtractI4(buf); buf+=4;
	}

	if( head_flags & TOC_FLAG )
	{
		if( pTagData->toc != NULL )
		{
			for(i=0;i<NUMTOCENTRIES;i++)
				pTagData->toc[i] = buf[i];
		}
		buf+=NUMTOCENTRIES;
	}

	pTagData->vbr_scale = -1;

	if( head_flags & VBR_SCALE_FLAG )
	{
		pTagData->vbr_scale = ExtractI4(buf); buf+=4;
	}

#ifdef DEBUG_VBRTAG
	DEBUGF("\n\n********************* VBR TAG INFO *****************\n");
	DEBUGF("tag         :%s\n",VBRTag);
	DEBUGF("head_flags  :%d\n",head_flags);
	DEBUGF("bytes       :%d\n",pTagData->bytes);
	DEBUGF("frames      :%d\n",pTagData->frames);
	DEBUGF("VBR Scale   :%d\n",pTagData->vbr_scale);
	DEBUGF("toc:\n");
	if( pTagData->toc != NULL )
	{
		for(i=0;i<NUMTOCENTRIES;i++)
		{
			if( (i%10) == 0 ) DEBUGF("\n");
			DEBUGF(" %3d", (int)(pTagData->toc[i]));
		}
	}
	DEBUGF("\n***************** END OF VBR TAG INFO ***************\n");
#endif
	return 1;       /* success */
}


/****************************************************************************
 * InitVbrTag: Initializes the header, and write empty frame to stream
 * Paramters:
 *				fpStream: pointer to output file stream
 *				nMode	: Channel Mode: 0=STEREO 1=JS 2=DS 3=MONO
 ****************************************************************************
*/
int InitVbrTag(lame_global_flags *gfp)
{
	int nMode,SampIndex;
	lame_internal_flags *gfc = gfp->internal_flags;
	u_char pbtStreamBuffer[216];
	nMode = gfp->mode;
	SampIndex = gfc->samplerate_index;


	/* Clear Frame position array variables */
	gfp->pVbrFrames=NULL;
	gfp->nVbrNumFrames=0;
	gfp->nVbrFrameBufferSize=0;


	/* Clear stream buffer */
	memset(pbtStreamBuffer,0x00,sizeof(pbtStreamBuffer));



	/* Reserve the proper amount of bytes */
	if (nMode==3)
	{
		gfp->nZeroStreamSize=SizeOfEmptyFrame[gfp->version][1]+4;
	}
	else
	{
		gfp->nZeroStreamSize=SizeOfEmptyFrame[gfp->version][0]+4;
	}

	/*
	// Xing VBR pretends to be a 48kbs layer III frame.  (at 44.1kHz).
        // (at 48kHz they use 56kbs since 48kbs frame not big enough for
        // table of contents)
	// let's always embed Xing header inside a 64kbs layer III frame.
	// this gives us enough room for a LAME version string too.
	// size determined by sampling frequency (MPEG1)
	// 32kHz:    216 bytes@48kbs    288bytes@ 64kbs
	// 44.1kHz:  156 bytes          208bytes@64kbs     (+1 if padding = 1)
	// 48kHz:    144 bytes          192
	//
	// MPEG 2 values are the since the framesize and samplerate
        // are each reduced by a factor of 2.
	*/
	{
	int tot;
	static const int framesize[3]={208,192,288};  /* 64kbs MPEG1 or MPEG2  framesize */
	/* static int framesize[3]={156,144,216}; */ /* 48kbs framesize */

	if (SampIndex>2) {
	  ERRORF("illegal sampling frequency index\n");
	  LAME_ERROR_EXIT();
	}
	gfp->TotalFrameSize= framesize[SampIndex];
	tot = (gfp->nZeroStreamSize+VBRHEADERSIZE);
	tot += 20;  /* extra 20 bytes for LAME & version string */

	if (gfp->TotalFrameSize < tot ) {
	  ERRORF("Xing VBR header problem...use -t\n");
	  LAME_ERROR_EXIT();
	}
	}

	add_dummy_vbrframe(gfp,8*gfp->TotalFrameSize);

	/* Success */
	return 0;
}



/****************************************************************************
 * PutVbrTag: Write final VBR tag to the file
 * Paramters:
 *				lpszFileName: filename of MP3 bit stream
 *				nVbrScale	: encoder quality indicator (0..100)
 ****************************************************************************
*/
int PutVbrTag(lame_global_flags *gfp,char* lpszFileName,int nVbrScale)
{
	int			i;
	long lFileSize;
	int nStreamIndex;
	char abyte;
	u_char		btToc[NUMTOCENTRIES];
	FILE *fpStream;
        u_char pbtStreamBuffer[216];
	char str1[80];
        unsigned char id3v2Header[10];
        size_t id3v2TagSize;

	if (gfp->nVbrNumFrames==0 || gfp->pVbrFrames==NULL)
		return -1;

	/* Open the bitstream again */
	fpStream=fopen(lpszFileName,"rb+");

	/* Assert stream is valid */
	if (fpStream==NULL)
		return -1;

	/* Clear stream buffer */
	memset(pbtStreamBuffer,0x00,sizeof(pbtStreamBuffer));

	/* Seek to end of file*/
	fseek(fpStream,0,SEEK_END);

	/* Get file size */
	lFileSize=ftell(fpStream);

	/* Abort if file has zero length. Yes, it can happen :) */
	if (lFileSize==0)
		return -1;

        /*
         * The VBR tag may NOT be located at the beginning of the stream.
         * If an ID3 version 2 tag was added, then it must be skipped to write
         * the VBR tag data.
         */

        /* seek to the beginning of the stream */
	fseek(fpStream,0,SEEK_SET);
        /* read 10 bytes in case there's an ID3 version 2 header here */
        fread(id3v2Header,1,sizeof id3v2Header,fpStream);
        /* does the stream begin with the ID3 version 2 file identifier? */
        if (!strncmp((char *)id3v2Header,"ID3",3)) {
          /* the tag size (minus the 10-byte header) is encoded into four
           * bytes where the most significant bit is clear in each byte */
          id3v2TagSize=(((id3v2Header[6] & 0x7f)<<21)
            | ((id3v2Header[7] & 0x7f)<<14)
            | ((id3v2Header[8] & 0x7f)<<7)
            | (id3v2Header[9] & 0x7f))
            + sizeof id3v2Header;
        } else {
          /* no ID3 version 2 tag in this stream */
          id3v2TagSize=0;
        }

	/* Seek to first real frame */
	fseek(fpStream,id3v2TagSize+(long)gfp->TotalFrameSize,SEEK_SET);

	/* Read the header (first valid frame) */
	fread(pbtStreamBuffer,4,1,fpStream);

	/* the default VBR header.  48kbs layer III, no padding, no crc */
	/* but sampling freq, mode andy copyright/copy protection taken */
	/* from first valid frame */
	pbtStreamBuffer[0]=(u_char) 0xff;
	abyte = (pbtStreamBuffer[1] & (char) 0xf1);
	if (gfp->version==1) {
          /* changed behaviour as suggested by Mathew Hendry:
           * The Xing VBR header is a valid mpeg.  But because some
	   * decoders require CRC on for all frames, or off for all frames,
           * keep crc from the first valid frame without calculating it
           * for this additional Xing frame
           */ 
	  pbtStreamBuffer[1]=abyte | (char) 0x0a;     /* was 0x0b; */
	  abyte = pbtStreamBuffer[2] & (char) 0x0d;   /* AF keep also private bit */
	  pbtStreamBuffer[2]=(char) 0x50 | abyte;     /* 64kbs MPEG1 frame */
	}else{
	  pbtStreamBuffer[1]=abyte | (char) 0x02;     /* was 0x03; */
	  abyte = pbtStreamBuffer[2] & (char) 0x0d;   /* AF keep also private bit */
	  pbtStreamBuffer[2]=(char) 0x80 | abyte;     /* 64kbs MPEG2 frame */
	}


	/*Seek to the beginning of the stream */
	fseek(fpStream,id3v2TagSize,SEEK_SET);

	/* Clear all TOC entries */
	memset(btToc,0,sizeof(btToc));

        for (i=1;i<NUMTOCENTRIES;i++) /* Don't touch zero point... */
        {
                /* Calculate frame from given percentage */
                int frameNum=(int)(floor(0.01*i*gfp->nVbrNumFrames));

                /*  Calculate relative file postion, normalized to 0..256!(?) */
                float fRelStreamPos=(float)256.0*(float)gfp->pVbrFrames[frameNum]/(float)lFileSize;

                /* Just to be safe */
                if (fRelStreamPos>255) fRelStreamPos=255;

                /* Assign toc entry value */
                btToc[i]=(u_char) fRelStreamPos;
        }



	/* Start writing the tag after the zero frame */
	nStreamIndex=gfp->nZeroStreamSize;

	/* Put Vbr tag */
	pbtStreamBuffer[nStreamIndex++]=VBRTag[0];
	pbtStreamBuffer[nStreamIndex++]=VBRTag[1];
	pbtStreamBuffer[nStreamIndex++]=VBRTag[2];
	pbtStreamBuffer[nStreamIndex++]=VBRTag[3];

	/* Put header flags */
	CreateI4(&pbtStreamBuffer[nStreamIndex],FRAMES_FLAG+BYTES_FLAG+TOC_FLAG+VBR_SCALE_FLAG);
	nStreamIndex+=4;

	/* Put Total Number of frames */
	CreateI4(&pbtStreamBuffer[nStreamIndex],gfp->nVbrNumFrames);
	nStreamIndex+=4;

	/* Put Total file size */
	CreateI4(&pbtStreamBuffer[nStreamIndex],(int)lFileSize);
	nStreamIndex+=4;

	/* Put TOC */
	memcpy(&pbtStreamBuffer[nStreamIndex],btToc,sizeof(btToc));
	nStreamIndex+=sizeof(btToc);

	/* Put VBR SCALE */
	CreateI4(&pbtStreamBuffer[nStreamIndex],nVbrScale);
	nStreamIndex+=4;

	/* Put LAME id */
	sprintf(str1,"LAME%s",get_lame_version());
	strncpy((char *)&pbtStreamBuffer[nStreamIndex],str1,(size_t) 20);
	nStreamIndex+=20;


#ifdef DEBUG_VBRTAG
{
	VBRTAGDATA TestHeader;
	GetVbrTag(&TestHeader,pbtStreamBuffer);
}
#endif

        /* Put it all to disk again */
	if (fwrite(pbtStreamBuffer,(unsigned int)gfp->TotalFrameSize,1,fpStream)!=1)
	{
		return -1;
	}
	fclose(fpStream);

	/* Save to delete the frame buffer */
	free(gfp->pVbrFrames);
	gfp->pVbrFrames=NULL;

	return 0;       /* success */
}

