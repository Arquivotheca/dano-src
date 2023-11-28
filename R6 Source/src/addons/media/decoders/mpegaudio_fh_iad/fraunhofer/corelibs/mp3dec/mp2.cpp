// mp2.cpp

#include "mp2.h"
#include "l2table.h"
#include <cmath>
#include <cstdio>
#include <cstring>

#include <Debug.h>

#define SCALE_RANGE                     64
#define SCALE                           32768
#define HAN_SIZE                        512
#define         PI4                     PI/4
#define         PI64                    PI/64

// integer scale factor
static const float pre_scale_factor = 0.125f;

// -------------------------------------------------------------------------- //

int mpgaPickBitAllocTable(
	const CMpegHeader& header) {

	int version = header.GetMpegVersion();
	int br_per_ch = header.GetBitrate() / header.GetChannels();
	int sfrq = header.GetSampleRate();
	
	if (!version) { // MPEG 1
		if ((sfrq == 48000 && br_per_ch >= 56000) ||
			(br_per_ch >= 56000 && br_per_ch <= 80000))
			return 0;
		else if (sfrq != 48000 && br_per_ch >= 96000)
			return 1;
		else if (sfrq != 32000 && br_per_ch <= 48000)
			return 2;
		else
			return 3;
	}
	else // MPEG 2 or 2.5
		return 4;
}

int mpgaCalcJSBound(
	const CMpegHeader& header) {

	static int jsb_table[3][4] =  { { 4, 8, 12, 16 }, { 4, 8, 12, 16},
                                { 0, 4, 8, 16} };  /* lay+m_e -> jsbound */

	int lay = header.GetLayer();
	int m_ext = header.GetModeExt();
	if(lay<1 || lay >3 || m_ext<0 || m_ext>3) {
#if DEBUG
		fprintf(stderr, "mp2CalcJSBound() bad layer/modext (%d/%d)\n", lay, m_ext);
#endif
		return -1;
	}
	return(jsb_table[lay-1][m_ext]);		
}

// ------------------------------------------------------------------------ //


void mp1ReadBitAlloc(
	CBitStream& bs,
	unsigned int outBitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound) {

	int i,j,b;
	int stereo = header.GetChannels();
	
	for (i=0;i<jsbound;i++)
		for (j=0;j<stereo;j++)
			outBitAlloc[j][i] = bs.GetBits(4);

	for (i=jsbound;i<SBLIMIT;i++) {
		b = bs.GetBits(4);
		for (j=0;j<stereo;j++)
			outBitAlloc[j][i] = b;
	}

}

	
void mp1ReadScale(
	CBitStream& bs,
	unsigned int outScaleIndex[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound) {
	
	int i,j;
	int stereo = header.GetChannels();

	for (i=0;i<SBLIMIT;i++)
		for (j=0;j<stereo;j++)
			if (!bitAlloc[j][i])
				outScaleIndex[j][0][i] = SCALE_RANGE-1;
			else /* 6 bit per scale factor */
				outScaleIndex[j][0][i] = bs.GetBits(6);
}
	
void mp1BufferSample(
	CBitStream& bs,
	unsigned int outSample[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound) {

	int i,j,k,s;
	int stereo = header.GetChannels();

	for (i=0;i<jsbound;i++)
		for (j=0;j<stereo;j++)
			if ((k = bitAlloc[j][i]) == 0)
				outSample[j][0][i] = 0;
			else
				outSample[j][0][i] = (unsigned int)bs.GetBits(k+1);

	for (i=jsbound;i<SBLIMIT;i++) {
		if ((k = bitAlloc[0][i]) == 0)
			s = 0;
		else 
			s = (unsigned int)bs.GetBits(k+1);
	
		for (j=0;j<stereo;j++)
			outSample[j][0][i] = s;
	}
}

#if defined(ENABLE_FLOATING_POINT)

void mp1Dequantize(
	float outSpectrum[2][SSLIMIT][SBLIMIT],
	unsigned int sample[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound) {

	int i, j, k, nb;
	int stereo = header.GetChannels();

	for (i=0;i<SBLIMIT;i++)
		for (k=0;k<stereo;k++)
			if (bitAlloc[k][i]) {
				nb = bitAlloc[k][i] + 1;
				if (((sample[k][0][i] >> nb-1) & 1) == 1)
					outSpectrum[k][0][i] = 0.0;
				else
					outSpectrum[k][0][i] = -1.0;
				
				outSpectrum[k][0][i] +=
					(double)(sample[k][0][i] & ((1<<nb-1)-1)) /
					(double)(1L<<nb-1);
	
				outSpectrum[k][0][i] =
					(double)(outSpectrum[k][0][i]+1.0/(double)(1L<<nb-1)) *
					(double)(1L<<nb) / (double) ((1L<<nb)-1);
			}
			else
				outSpectrum[k][0][i] = 0.0;
}

void mp1Denormalize(
	float ioSpectrum[2][SSLIMIT][SBLIMIT],
	unsigned int scaleIndex[2][3][SBLIMIT],
	const CMpegHeader& header,
	int jsbound) {

	int i,j,k;
	int stereo = header.GetChannels();
	
	for (i=0;i<SBLIMIT;i++)
		for (j=0;j<stereo;j++)
			ioSpectrum[j][0][i] *= sDenormalizeScale[scaleIndex[j][0][i]];

}

#endif // ENABLE_FLOATING_POINT

#if defined(ENABLE_FIXED_POINT)

// dequant/denormalize

void 
mp1Dequantize(
	int32 outSpectrum[SSLIMIT*SBLIMIT*2],
	unsigned int sample[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	unsigned int scaleIndex[2][3][SBLIMIT],
	const CMpegHeader &header,
	int jsbound)
{

	int i, j, k, nb;
	int stereo = header.GetChannels();
	int32* outp = outSpectrum;

	for (i=0;i<SBLIMIT;i++, outp += 2)
		for (k=0;k<stereo;k++)
			if (bitAlloc[k][i])
			{
				nb = bitAlloc[k][i] + 1;
				double v;
				if (((sample[k][0][i] >> nb-1) & 1) == 1)
					v = 0.0;
				else
					v = -1.0;
				
				v +=
					(double)(sample[k][0][i] & ((1<<nb-1)-1)) /
					(double)(1L<<nb-1);
	
				v =
					(double)(v+1.0/(double)(1L<<nb-1)) *
					(double)(1L<<nb) / (double) ((1L<<nb)-1);
					
				v *= sDenormalizeScale[scaleIndex[k][0][i]];
				v *= pre_scale_factor; // +++++ scale table when this settles down
				
				if(v > 32767.)
				{
					v = 32767.;
				}
				else if(v < -32768.)
				{
					v = -32768.;
				}
				outp[k] = double_to_fixed32(v);
			}
			else
				outp[k] = 0;
}
#endif

// ------------------------------------------------------------------------ //

void mp2ReadBitAlloc(
	CBitStream& bs,
	unsigned int outBitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound,
	const sb_alloc_table& table) {

	int i,j;
	int stereo = header.GetChannels();
	int sblimit = table.sblimit;
	
//	printf("* mp2ReadBitAlloc()\n");
	for (i=0;i<jsbound;i++)
		for (j=0;j<stereo;j++) {
			int bits = table.data[i][0].bits;
//			printf("  %ld\n", bits);
			outBitAlloc[j][i] = (char)bs.GetBits(bits);
		}
	
	for (i=jsbound;i<sblimit;i++)
		outBitAlloc[0][i] = outBitAlloc[1][i] =
			(char)bs.GetBits(table.data[i][0].bits);
	
	for (i=sblimit;i<SBLIMIT;i++)
		for (j=0;j<stereo;j++)
			outBitAlloc[j][i] = 0;
}

	
void mp2ReadScale(
	CBitStream& bs,
	unsigned int outScaleIndex[2][3][SBLIMIT],
	unsigned int outScaleSel[2][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound,
	const sb_alloc_table& table) {
	
	int i,j;
	int stereo = header.GetChannels();
	int sblimit = table.sblimit;

	for (i=0;i<sblimit;i++)
		for (j=0;j<stereo;j++) /* 2 bit scfsi */
			if (bitAlloc[j][i])
				outScaleSel[j][i] = (char)bs.GetBits(2);

	for (i=sblimit;i<SBLIMIT;i++)
		for (j=0;j<stereo;j++)   
			outScaleSel[j][i] = 0;

	for (i=0;i<sblimit;i++)
		for (j=0;j<stereo;j++) {
			if (bitAlloc[j][i])   
				switch(outScaleSel[j][i]) {
				/* all three scale factors transmitted */
				case 0:
					outScaleIndex[j][0][i] = bs.GetBits(6);
					outScaleIndex[j][1][i] = bs.GetBits(6);
					outScaleIndex[j][2][i] = bs.GetBits(6);
					break;
				/* scale factor 1 & 3 transmitted */
				case 1:
					outScaleIndex[j][0][i] =
						outScaleIndex[j][1][i] = bs.GetBits(6);
						outScaleIndex[j][2][i] = bs.GetBits(6);
					break;
				/* scale factor 1 & 2 transmitted */
				case 3:
					outScaleIndex[j][0][i] = bs.GetBits(6);
					outScaleIndex[j][1][i] =
						outScaleIndex[j][2][i] = bs.GetBits(6);
					break;
				/* only one scale factor transmitted */
				case 2:
					outScaleIndex[j][0][i] =
						outScaleIndex[j][1][i] =
						outScaleIndex[j][2][i] = bs.GetBits(6);
					break;
				default:
					break;
				}
			else {
				outScaleIndex[j][0][i] = outScaleIndex[j][1][i] =
					outScaleIndex[j][2][i] = SCALE_RANGE-1;
			}
		}
	
	for (i=sblimit;i<SBLIMIT;i++)
		for (j=0;j<stereo;j++) {
			outScaleIndex[j][0][i] = outScaleIndex[j][1][i] =
				outScaleIndex[j][2][i] = SCALE_RANGE-1;
		}
}
	
void mp2BufferSample(
	CBitStream& bs,
	unsigned int outSample[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound,
	const sb_alloc_table& table) {

	int i,j,k,m;
	int stereo = header.GetChannels();
	int sblimit = table.sblimit;

	for (i=0;i<sblimit;i++)
		for (j=0;j<((i<jsbound)?stereo:1);j++) {
			if (bitAlloc[j][i]) {
			/* check for grouping in subband */
				if (table.data[i][bitAlloc[j][i]].group==3)
					for (m=0;m<3;m++) {
						k = table.data[i][bitAlloc[j][i]].bits;
						outSample[j][m][i] = (unsigned int)bs.GetBits(k);
					}         
					else {              /* bit_alloc = 3, 5, 9 */
						unsigned int nlevels, c=0;

						nlevels = table.data[i][bitAlloc[j][i]].steps;
						k=table.data[i][bitAlloc[j][i]].bits;
						c = (unsigned int)bs.GetBits(k);
						for (k=0;k<3;k++) {
							outSample[j][k][i] = c % nlevels;
							c /= nlevels;
						}
					}
			}
			else {                  /* for no sample transmitted */
				for (k=0;k<3;k++)
					outSample[j][k][i] = 0;
			}
			if(stereo == 2 && i>= jsbound) /* joint stereo : copy L to R */
				for (k=0;k<3;k++)
					outSample[1][k][i] = outSample[0][k][i];
		}

	for (i=sblimit;i<SBLIMIT;i++)
		for (j=0;j<stereo;j++)
			for (k=0;k<3;k++)
				outSample[j][k][i] = 0;
}

#if defined(ENABLE_FLOATING_POINT)

void mp2Dequantize(
	float outSpectrum[2][SSLIMIT][SBLIMIT],
	unsigned int sample[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	const CMpegHeader& header,
	int jsbound,
	const sb_alloc_table& table) {

	int i, j, k, x;
	int stereo = header.GetChannels();
	int sblimit = table.sblimit;

	for (i=0;i<sblimit;i++)
		for (j=0;j<3;j++)
			for (k=0;k<stereo;k++)
				if (bitAlloc[k][i]) {

					/* locate MSB in the sample */
					x = 0;
					while ((1L<<x) < table.data[i][bitAlloc[k][i]].steps)
						x++;
					/* MSB inversion */
					if (((sample[k][j][i] >> x-1) & 1) == 1)
						outSpectrum[k][j][i] = 0.0;
					else
						outSpectrum[k][j][i] = -1.0;
					
					/* Form a 2's complement sample */
					outSpectrum[k][j][i] += (float) (sample[k][j][i] & ((1<<x-1)-1)) /
						(float) (1L<<x-1);
					
					/* Dequantize the sample */
					outSpectrum[k][j][i] += sDequantOffset[table.data[i][bitAlloc[k][i]].quant];
					outSpectrum[k][j][i] *= sDequantScale[table.data[i][bitAlloc[k][i]].quant];
				}
				else
					outSpectrum[k][j][i] = 0.0;   

	for (i=sblimit;i<SBLIMIT;i++)
		for (j=0;j<3;j++)
			for(k=0;k<stereo;k++)
				outSpectrum[k][j][i] = 0.0;
}
void mp2Denormalize(
	float ioSpectrum[2][SSLIMIT][SBLIMIT],
	unsigned int scaleIndex[2][3][SBLIMIT],
	unsigned int seg,
	const CMpegHeader& header,
	int jsbound,
	const sb_alloc_table& table) {

	int i,j,k;
	int stereo = header.GetChannels();
	int sblimit = table.sblimit;
	
	for (i=0;i<sblimit;i++)
		for (j=0;j<stereo;j++) {
			ioSpectrum[j][0][i] *= (sDenormalizeScale[scaleIndex[j][seg][i]] * 0.125); // +++++
			ioSpectrum[j][1][i] *= (sDenormalizeScale[scaleIndex[j][seg][i]] * 0.125); // +++++
			ioSpectrum[j][2][i] *= (sDenormalizeScale[scaleIndex[j][seg][i]] * 0.125); // +++++
		}	
}

#endif // ENABLE_FLOATING_POINT

#if defined(ENABLE_FIXED_POINT)

void 
mp2Dequantize(
	int32 outSpectrum[SSLIMIT*SBLIMIT*2],
	unsigned int sample[2][3][SBLIMIT],
	unsigned int bitAlloc[2][SBLIMIT],
	unsigned int scaleIndex[2][3][SBLIMIT],
	unsigned int seg,
	const CMpegHeader &header,
	int jsbound,
	const sb_alloc_table &table)
{
	int stereo = header.GetChannels();
	int sblimit = table.sblimit;

	for (int i=0; i<sblimit; i++)
		for (int j=0; j<3; j++)
			for (int k=0; k<stereo; k++)
				if (bitAlloc[k][i])
				{
					/* locate MSB in the sample */
					int x = 0;
					while ((1L<<x) < table.data[i][bitAlloc[k][i]].steps)
						x++;
					/* MSB inversion */
					double v;
					if (((sample[k][j][i] >> x-1) & 1) == 1)
						v = 0.0;
					else
						v = -1.0;
					
					/* Form a 2's complement sample */
					v += (double)(sample[k][j][i] & ((1<<x-1)-1)) / (double)(1L<<x-1);
					
					/* Dequantize the sample */
					v += sDequantOffset[table.data[i][bitAlloc[k][i]].quant];
					v *= sDequantScale[table.data[i][bitAlloc[k][i]].quant];
					
					/* Denormalize */
					v *= sDenormalizeScale[scaleIndex[k][seg][i]];
					v *= pre_scale_factor; // +++++ scale table when this settles down
				
					if(v > 32767.)
					{
						v = 32767.;
					}
					else if(v < -32768.)
					{
						v = -32768.;
					}
					outSpectrum[((j*SBLIMIT)+i)*2+k] = double_to_fixed32(v);
				}
				else
					outSpectrum[((j*SBLIMIT)+i)*2+k] = 0;   

	int32* outp = outSpectrum + sblimit*2;
	for (int j=0; j<3; j++, outp += (SBLIMIT*2))
		for (int i=0; i<SBLIMIT-sblimit; i++)
			for(int k=0; k<stereo; k++)
				outp[i*2+k] = 0;
}
#endif // ENABLE_FIXED_POINT

// ------------------------------------------------------------------------ //

#if 0

// +++++ PRECALCULATE ME +++++
void _mpga_create_syn_filter(
	double filter[64][SBLIMIT]) {
	register int i,k;
	
	memset(filter, 0, sizeof(float)*64*SBLIMIT);
	
	for (i=0; i<64; i++)
		for (k=0; k<32; k++) {
			if ((filter[i][k] = 1e9*cos((float)((PI64*i+PI4)*(2*k+1)))) >= 0)
				modf(filter[i][k]+0.5, &filter[i][k]);
			else
				modf(filter[i][k]-0.5, &filter[i][k]);
				filter[i][k] *= 1e-9;
		}
}

void mpgaSubBandInit(
	double filter[64][SBLIMIT],
	float buf[2][2*HAN_SIZE],
	int bufOffset[2]) {

	_mpga_create_syn_filter(filter);
	bufOffset[0] = 64;
	bufOffset[1] = 64;
	memset(buf, 0, sizeof(float)*2*2*HAN_SIZE);
}


void mpgaSubBandSynthesis(
	float* spectrum,
	int channel,
	short* outPCM,
	double filter[64][SBLIMIT],
	float buf[2][2*HAN_SIZE],
	int bufOffset[2]) {
	
	int i,j,k;
	float* bufOffsetPtr, sum;
	long foo;
	
	bufOffset[channel] = (bufOffset[channel] - 64) & 0x3ff;
	bufOffsetPtr = &(buf[channel][bufOffset[channel]]);
	
	for (i=0; i<64; i++) {
		sum = 0;
		for (k=0; k<32; k++)
			sum += spectrum[k] * filter[i][k];
		bufOffsetPtr[i] = sum;
	}

	for (j=0; j<32; j++) {
		sum = 0;
		for (i=0; i<16; i++) {
			k = j + (i<<5);
			sum +=
				sLayer2DecodeWindow[k] *
				buf[channel][
					( (k + ( ((i+1)>>1) <<6) ) + bufOffset[channel]) & 0x3ff
				];
		}

		if (sum >= (long) SCALE)
			outPCM[j] = (short)(SCALE-1);
		else if (foo < (long) -SCALE)
			outPCM[j] = (short)(-SCALE);
		else
			outPCM[j] = (short)sum;
	}
}

#endif // 0

// END -- mp2.cpp
