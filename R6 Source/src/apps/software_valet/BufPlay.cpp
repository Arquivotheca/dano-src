#include "BufPlay.h"

#if 0
BufPlay::BufPlay()
	:	bufP(NULL),
		length(0)
{
	dacSubscriber = new BDACStream();
	dacSubscriber->Subscribe (B_DAC_STREAM,
			B_SHARED_SUBSCRIBER_ID,
			TRUE);
	
}

BufPlay::~BufPlay()
{
	dacSubscriber->ExitStream(TRUE);
	
	delete dacSubscriber;
}
	
void BufPlay::Play(char *buffer, long size)
{
	bufP = buffer;
	length = size;
	
	dacSubscriber->ExitStream(TRUE);
	dacSubscriber->SetSamplingRate(22050);
	
	dacSubscriber->EnterStream(NULL,FALSE,this,dac_writer,NULL,TRUE);
}

/*
void BufPlay::FadeOut()
{

}
*/

bool BufPlay::dac_writer(void *arg, char *dac_buf, long count)
{
	BufPlay	*obj = (BufPlay *)arg;

	short	*out = (short *)dac_buf;
	short	*in = (short *)obj->bufP;
	
	long len = min_c(obj->length,count);
	long slen = len/2;

	// assumes 16-bit samples!!
	for (long i = 0; i < slen; i++) {
		*out++ += *in++;
	}
	
	obj->bufP += len;
	obj->length -= len;
	if (len < count) {
		return FALSE;
	}
	return TRUE;
}
#endif
