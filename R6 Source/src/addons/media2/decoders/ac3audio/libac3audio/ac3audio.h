#ifndef C_AC3_AUDIO_H

#define C_AC3_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mpeg2buffer.h"
#include "mpeg2bitstream.h"

typedef status_t (*acquire_buffer_func) (void *cookie, buffer_t **buffer);
typedef status_t (*send_buffer_func) (void *cookie, buffer_t *buffer);

#define CPL_ARRAY_SIZE (72*4+217)
#define FBW_ARRAY_SIZE (88*4+1)

typedef enum output_audio_coding_mode_
{
	C_PASS_THROUGH = 0,
	C_CONVENTIONAL_STEREO,
	C_MATRIX_SURROUND_STEREO,
	C_MONO

} output_audio_coding_mode_t;

typedef enum audio_coding_mode_
{
	C_INDEPENDENT_MONO = 0,
	C_CENTER,
	C_LEFT_RIGHT,
	C_LEFT_CENTER_RIGHT,
	C_LEFT_RIGHT_SURROUND,
	C_LEFT_CENTER_RIGHT_SURROUND,
	C_LEFT_RIGHT_LSURROUND_RSURROUND,
	C_LEFT_CENTER_RIGHT_LSURROUND_RSURROUND

} audio_coding_mode_t;

typedef struct ac3audio_syncinfo_
{
	uint16 crc1;
	uint8 fscod;
	uint8 frmsizecod;

} ac3audio_syncinfo_t;

typedef struct ac3audio_bsi_param_
{
	uint8 dialnorm;
	bool compre;
	uint8 compr;
	bool langcode;
	uint8 langcod;
	bool audprodie;
	uint8 mixlevel;
	uint8 roomtyp;				

} ac3audio_bsi_param_t;

typedef struct ac3audio_bsi_
{
	uint16 bsid;
	uint8 bsmod;
	uint8 acmod;
	uint8 cmixlev;
	uint8 surmixlev;
	uint8 dsurmod;
	bool lfeon;
	
	ac3audio_bsi_param_t param[2];
	
	bool copyrightb;
	bool origbs;
	
	bool timecod1e;
	uint16 timecod1;
	
	bool timecod2e;
	uint16 timecod2;
	
	// derived
	
	uint8 nfchans;

} ac3audio_bsi_t;

typedef struct ac3audio_audblk_
{
	bool blksw[5];
	bool dithflag[5];
	
	bool dynrnge;
	uint8 dynrng;

	bool dynrng2e;
	uint8 dynrng2;
	
	bool cplstre;
	
	uint8 ncplsubnd;
	uint8 ncplbnd;
	
	bool cplinu;
	bool chincpl[5];
	bool phsflginu;
	uint8 cplbegf;
	uint8 cplendf;
	
	bool cplbndstrc[18];	// this maps to [cplbegf,..,cplendf+2]
	
	bool cplcoe[5];
	uint8 mstrcplco[5];
	
	uint8 cplcoexp[5][18];
	uint8 cplcomant[5][18];
	
	bool phsflg[18];
	
	bool rematstr;
	bool rematflg[4];
	
	uint8 cplexpstr;
	uint8 chexpstr[5];
	bool lfeexpstr;
	
	uint8 chbwcod[5];
	
	uint8 cplabsexp;
	uint8 cplstrtmant;
	uint8 cplendmant;
	uint8 ncplgrps;
	uint8 cplexps[72];
	
	uint16 endmant[5];
	uint8 nchgrps[5];
	
	uint8 exps[5][88];
	
	uint8 gainrng[5];
	
	uint8 lfeexps[3];
	
	bool baie;
	uint8 sdcycod;
	uint8 fdcycod;
	uint8 sgaincod;
	uint8 dbpbcod;
	uint8 floorcod;
	
	bool snroffste;
	uint8 csnroffst;
	
	uint8 cplfsnroffst;
	uint8 cplfgaincod;
	
	uint8 fsnroffst[5];
	uint8 fgaincod[5];
	
	uint8 lfefsnroffst;
	uint8 lfefgaincod;
	
	bool cplleake;
	uint8 cplfleak;
	uint8 cplsleak;
	
	bool deltbaie;
	uint8 cpldeltbae;
	uint8 deltbae[5];
	uint8 cpldeltnseg;
	
	uint8 cpldeltoffst[8];
	uint8 cpldeltlen[8];
	uint8 cpldeltba[8];
	
	uint8 deltnseg[5];
	uint8 deltoffst[5][8];
	uint8 deltlen[5][8];
	uint8 deltba[5][8];

} ac3audio_audblk_t;

typedef struct ac3audio_exponent_
{
	uint8 cpl[CPL_ARRAY_SIZE];
	uint8 fbw[5][FBW_ARRAY_SIZE];
	uint8 lfe[2*4+1];

} ac3audio_exponent_t;

typedef struct ac3audio_bap_
{
	uint8 sdecay;					// +0
	uint8 fdecay;
	uint16 sgain;
	uint16 dbknee;
	int16 floor;
		
	int32 fastleak;
	int32 slowleak;
	
	uint16 psd[262];				// +16
	uint16 bndpsd[50];				// +0x21c
	
	int16 excite[50];				// +0x280
	int16 mask[50];					// +0x2e4
	
	uint8 cpl[CPL_ARRAY_SIZE];		// +0x348
	uint8 fbw[5][FBW_ARRAY_SIZE];	// +0xa2d
	uint8 lfe[2*4+1];

} ac3audio_bap_t;

typedef struct ac3audio_coeff_
{
	float fbw[5][256];
	float cpl[CPL_ARRAY_SIZE];
	float lfe[256];

} ac3audio_coeff_t;

typedef struct ac3audio_sample_
{
	float channel[6][512];

} ac3audio_sample_t;

typedef struct ac3audio_get_float_state_
{
	uint8 queue_count;
	float *queue;

} ac3audio_get_float_state_t;

typedef struct complex_
{
	float re,im;
	
} complex_t;

typedef struct ac3audio_imdct_
{
	complex_t *unit_circle_512;
	complex_t unit_circle_256[512/8];

	complex_t w[7][128];
	
	complex_t *Z;
				
	float delay[6][512/2];

} ac3audio_imdct_t;

typedef struct ac3audio_decoder_
{
	acquire_buffer_func acquire_output_buffer;
	send_buffer_func send_output_buffer;
	void *cookie;
	
	output_audio_coding_mode_t output_coding_mode;
	bool first_mono_channel_selected;
	
	mpeg2video_bitstream_t *bitstream;	
	
	thread_id tid;
	
	ac3audio_syncinfo_t syncinfo;
	ac3audio_bsi_t bsi;
	ac3audio_audblk_t audblk;
	
	ac3audio_exponent_t exponents;
	ac3audio_bap_t bap;
	ac3audio_coeff_t coeffs;
	ac3audio_sample_t *samples;
	
	float fast_table_1[32][3];
	float fast_table_2[128][3];
	float fast_table_4[128][2];
	
	ac3audio_imdct_t imdct;
	float *window_coeffs;
	
} ac3audio_decoder_t;

typedef enum ac3audio_special_code_
{
	AC3AUDIO_RESERVED = 0,
	AC3AUDIO_SHUTDOWN = 1,
	AC3AUDIO_FLUSH
	
} ac3audio_special_code_t;

ac3audio_decoder_t *ac3audio_create (output_audio_coding_mode_t output_coding_mode,
									acquire_buffer_func acquire_output_buffer,
									send_buffer_func send_output_buffer,
									void *cookie);

void ac3audio_destroy (ac3audio_decoder_t *decoder);
void ac3audio_decode (ac3audio_decoder_t *decoder, buffer_t *buffer);
void ac3audio_flush(ac3audio_decoder_t *decoder);

int32 ac3audio_threadfunc (void *param);

#ifdef __cplusplus
}
#endif

#endif
