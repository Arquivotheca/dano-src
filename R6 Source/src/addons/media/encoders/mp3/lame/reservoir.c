#include <assert.h>
#include "util.h"



/*
  ResvFrameBegin:
  Called (repeatedly) at the beginning of a frame. Updates the maximum
  size of the reservoir, and checks to make sure main_data_begin
  was set properly by the formatter
*/
int
ResvFrameBegin(lame_global_flags *gfp,III_side_info_t *l3_side, int mean_bits, int frameLength )
{
    lame_internal_flags *gfc=gfp->internal_flags;
    int fullFrameBits;
    int resvLimit;
    int maxmp3buf;


    /* main_data_begin has 9 bits in MPEG 1, 8 bits MPEG2 */
    resvLimit = (gfp->version==1) ? 4088 : 2040 ;


    /* maximum allowed frame size */
    if (gfp->strict_ISO)
      maxmp3buf = 8*960;
    else
      maxmp3buf = 8*2047;

    if ( frameLength > maxmp3buf )
	gfc->ResvMax = 0;
    else
	gfc->ResvMax = maxmp3buf - frameLength;
    if (gfp->disable_reservoir) gfc->ResvMax=0;
    if ( gfc->ResvMax > resvLimit ) gfc->ResvMax = resvLimit;
    assert(0==(gfc->ResvMax % 8));

    l3_side->resvDrain_pre = 0;

    if (gfc->pinfo != NULL){
      gfc->pinfo->mean_bits=mean_bits/2;  /* expected bits per channel per granule */
      gfc->pinfo->resvsize=gfc->ResvSize;
    }

    fullFrameBits = mean_bits * gfc->mode_gr + Min(gfc->ResvSize,gfc->ResvMax);
    if (gfp->strict_ISO) {
      if (fullFrameBits>maxmp3buf) fullFrameBits=maxmp3buf;
    }
    return fullFrameBits;
}


/*
  ResvMaxBits
  returns targ_bits:  target number of bits to use for 1 granule
         extra_bits:  amount extra available from reservoir
  Mark Taylor 4/99
*/
void ResvMaxBits(lame_global_flags *gfp, int mean_bits, int *targ_bits, int *extra_bits)
{
  lame_internal_flags *gfc=gfp->internal_flags;
  int add_bits,full_fac;
  *targ_bits = mean_bits ;


  /* extra bits if the reservoir is almost full */
  full_fac=9;
  if (gfc->ResvSize > ((gfc->ResvMax * full_fac) / 10)) {
    add_bits= gfc->ResvSize-((gfc->ResvMax * full_fac) / 10);
    *targ_bits += add_bits;
  }else {
    add_bits =0 ;
    /* build up reservoir.  this builds the reservoir a little slower
     * than FhG.  It could simple be mean_bits/15, but this was rigged
     * to always produce 100 (the old value) at 128kbs */
    /*    *targ_bits -= (int) (mean_bits/15.2);*/
    if (!gfp->disable_reservoir) 
      *targ_bits -= .1*mean_bits;
  }


  /* amount from the reservoir we are allowed to use. ISO says 6/10 */
  *extra_bits =
    (gfc->ResvSize  < (gfc->ResvMax*6)/10  ? gfc->ResvSize : (gfc->ResvMax*6)/10);
  *extra_bits -= add_bits;

  if (*extra_bits < 0) *extra_bits=0;


}

/*
  ResvAdjust:
  Called after a granule's bit allocation. Readjusts the size of
  the reservoir to reflect the granule's usage.
*/
void
ResvAdjust(lame_global_flags *gfp,gr_info *gi, III_side_info_t *l3_side, int mean_bits )
{
  lame_internal_flags *gfc=gfp->internal_flags;
  gfc->ResvSize += (mean_bits / gfc->stereo) - gi->part2_3_length;
#if 0
  printf("part2_3_length:  %i  avg=%i  incres: %i\n",gi->part2_3_length,
	 mean_bits/gfc->stereo,
mean_bits/gfc->stereo-gi->part2_3_length);
#endif
}


/*
  ResvFrameEnd:
  Called after all granules in a frame have been allocated. Makes sure
  that the reservoir size is within limits, possibly by adding stuffing
  bits.
*/
void
ResvFrameEnd(lame_global_flags *gfp,III_side_info_t *l3_side, int mean_bits)
{
    int stuffingBits;
    int over_bits;
    lame_internal_flags *gfc=gfp->internal_flags;


    /* just in case mean_bits is odd, this is necessary... */
    if ( gfc->stereo == 2 && mean_bits & 1)
	gfc->ResvSize += 1;

    stuffingBits=0;
    l3_side->resvDrain_post = 0;
    l3_side->resvDrain_pre = 0;

    /* we must be byte aligned */
    if ( (over_bits = (gfc->ResvSize % 8)) )
	stuffingBits += over_bits;


    over_bits = (gfc->ResvSize - stuffingBits) - gfc->ResvMax;
    if (over_bits > 0) {
      assert(0==(over_bits % 8 ));
      stuffingBits += over_bits;
    }


#define NEW_DRAINXX
#ifdef NEW_DRAIN
    /* drain as many bits as possible into previous frame ancillary data
     * In particular, in VBR mode ResvMax may have changed, and we have
     * to make sure main_data_begin does not create a reservoir bigger
     * than ResvMax  mt 4/00*/
  {
    int mdb_bytes = Min(l3_side->main_data_begin*8,stuffingBits)/8;
    l3_side->resvDrain_pre += 8*mdb_bytes;
    stuffingBits -= 8*mdb_bytes;
    gfc->ResvSize -= 8*mdb_bytes;
    l3_side->main_data_begin -= mdb_bytes;


    /* drain just enough to be byte aligned.  The remaining bits will
     * be added to the reservoir, and we will deal with them next frame.
     * If the next frame is at a lower bitrate, it may have a larger ResvMax,
     * and we will not have to waste these bits!  mt 4/00 */
    l3_side->resvDrain_post += (stuffingBits % 8);
    gfc->ResvSize -= stuffingBits % 8;
  }
#else
    /* drain the rest into this frames ancillary data*/
    l3_side->resvDrain_post += stuffingBits;
    gfc->ResvSize -= stuffingBits;
#endif

    return;
}


