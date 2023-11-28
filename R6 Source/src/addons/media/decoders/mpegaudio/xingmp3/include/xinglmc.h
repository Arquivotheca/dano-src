/*____________________________________________________________________________
   
   FreeAmp - The Free MP3 Player
   Portions Copyright (C) 1998 GoodNoise

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
   
   $Id: xinglmc.h,v 1.32 1999/07/21 19:24:50 ijr Exp $

____________________________________________________________________________*/

#ifndef _XINGLMC_H_
#define _XINGLMC_H_

/* system headers */
#include <stdlib.h>
#include <time.h>

/* project headers */
#include "config.h"

#include "pmi.h"
#include "pmo.h"
#include "mutex.h"
#include "event.h"
#include "lmc.h"
#include "thread.h"
#include "mutex.h"
#include "queue.h"
#include "semaphore.h"

extern    "C"
{
#include "mhead.h"
#include "port.h"
}

#define BS_BUFBYTES 60000U
#define PCM_BUFBYTES 60000U

typedef struct
{
   int       (*decode_init) (MPEG_HEAD * h, int framebytes_arg,
              int reduction_code, int transform_code,
              int convert_code, int freq_limit);
   void      (*decode_info) (DEC_INFO * info);
             IN_OUT(*decode) (unsigned char *bs, short *pcm);
}
AUDIO;

enum
{
   lmcError_MinimumError = 1000,
   lmcError_DecodeFailed,
   lmcError_AudioDecodeInitFailed,
   lmcError_DecoderThreadFailed,
   lmcError_PMIError,
   lmcError_PMOError,
   lmcError_MaximumError
};

class     XingLMC:public LogicalMediaConverter
{

   public:
            XingLMC(FAContext *context);
   virtual ~XingLMC();

   virtual Error ChangePosition(int32 position);

   virtual Error CanDecode();
   virtual void  Clear();
   virtual Error ExtractMediaInfo();

   virtual void  SetPMI(PhysicalMediaInput *pmi) { m_pPmi = pmi; };
   virtual void  SetPMO(PhysicalMediaOutput *pmo) { m_pPmo = pmo; };
   virtual Error Prepare(PullBuffer *pInputBuffer, PullBuffer *&pOutBuffer);
   virtual Error InitDecoder();

   virtual Error SetEQData(float *);
   virtual Error SetEQData(bool);

   virtual bool CanHandleExt(char *ext);
 private:

   static void          DecodeWorkerThreadFunc(void *);
   void                 DecodeWork();
	Error                BeginRead(void *&pBuffer, unsigned int iBytesNeeded,
                             bool bBufferUp = true);
	Error                BlockingBeginRead(void *&pBuffer, 
                                          unsigned int iBytesNeeded);
	Error                AdvanceBufferToNextFrame();
	Error                GetHeadInfo();

   PhysicalMediaInput  *m_pPmi;
   PhysicalMediaOutput *m_pPmo;

   int                  m_iMaxWriteSize;
   int                  m_frameBytes, m_iBufferUpInterval, m_iBufferSize;
	MPEG_HEAD            m_sMpegHead;
	int32                m_iBitRate;
   bool                 m_bBufferingUp;
   Thread              *m_decoderThread;

   int32                m_frameCounter;
	time_t               m_iBufferUpdate;
   char                *m_szUrl;
   const char          *m_szError;
   AUDIO                m_audioMethods; 
};

#endif /* _XINGLMC_H */




