/*
 * Copyright (c) 1993-1994 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Network Research
 *	Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) $Header: p64.h,v 1.44 96/03/07 04:50:35 van Exp $ (LBL)
 */

#ifndef lib_p64_h
#define lib_p64_h

#include <sys/types.h>

struct huffcode;

#define MBPERGOB 33

class P64Decoder {
    public:
	virtual ~P64Decoder();
	const u_char* frame() const { return (back_); }
	inline int ndblk() const { return (ndblk_); }
	inline void resetndblk() { ndblk_ = 0; }
	inline int width() const { return (width_); }
	inline int height() const { return (height_); }
	virtual int decode(const u_char* bp, int cc,
			   int sbit, int ebit, int mba, int gob,
			   int quant, int mvdh, int mvdv);
	virtual void sync();
	inline void bb(int& x, int& y, int& w, int& h) {
		x = bbx_; y = bby_; w = bbw_; h = bbh_;
	};
	inline u_int bad_psc() const { return (bad_psc_); }
	inline u_int bad_bits() const { return (bad_bits_); }
	inline u_int bad_GOBno() const { return (bad_GOBno_); }
	inline u_int bad_fmt() const { return (bad_fmt_); }
	inline int seenMaxGob() const { return (maxgob_ >= ngob_ - 1); }
	inline u_int gobquant() const { return (gobquant_); }

	inline void marks(u_char* p) { marks_ = p; }
	inline void mark(int v) { mark_ = v; }
    protected:
	P64Decoder();
	void init();
	virtual void allocate() = 0;
	void inithuff();
	void initquant();
	virtual void err(const char* msg ...) const;
	int quantize(int v, int q);
#ifdef INT_64
	int parse_block(short* blk, INT_64* mask);
#else
	int parse_block(short* blk, u_int* mask);
#endif
	void decode_block(u_int tc, u_int x, u_int y, u_int stride,
			  u_char* front, u_char* back, int sf);
	void filter(u_char* in, u_char* out, u_int stride);
	void mvblk(u_char* in, u_char* out, u_int stride);
	void mvblka(u_char*, u_char*, u_int stride);

	int parse_picture_hdr();
	int parse_sc();
	int parse_gob_hdr(int);
	int parse_mb_hdr(u_int& cbp);
	int decode_gob(u_int gob);
	int decode_mb();

	u_int size_;
	u_char* fs_;
	u_char* front_;
	u_char* back_;

	struct hufftab {
		int maxlen;
		const short* prefix;
	};
	hufftab ht_mba_;
	hufftab ht_mvd_;
	hufftab ht_cbp_;
	hufftab ht_tcoeff_;
	hufftab ht_mtype_;

	u_int bb_;		/* 32-bit bit buffer */
	int nbb_;		/* number bits in bit buffer */
	const u_short* bs_;	/* input bit stream (less bits in bb_) */
	const u_short* es_;	/* pointer to end if input stream */
	const u_short* ps_;	/* packet start (for error reporting) */
	int pebit_;		/* packet end bit (for error reporting) */

#define MBST_FRESH	0
#define MBST_OLD	1
#define MBST_NEW	2
	u_char* mbst_;
	short* qt_;
	u_short* coord_;

	u_int width_;		/* width of Y component */
	u_int height_;		/* height of Y component */
	
#define IT_QCIF	0
#define IT_CIF	1
	u_int fmt_;		/* image format (CIF/QCIF) */
	u_int ngob_;		/* number of gobs (12 for CIF, 3 for QCIF) */
	u_int maxgob_;		/* max gob seen this frame */
	int ndblk_;		/* # of decoded macroblocks */
	u_int gobquant_;	/* last gob quantizer (for info funtion) */

	u_int mt_;		/* macroblock type (flags in p64-huff.h) */
	u_int gob_;		/* current gob index */
	int mba_;		/* macroblock address (predictor) */
	int mvdh_;		/* motion vector (predictor) */
	int mvdv_;		/* motion vector (predictor) */

	/* bounding box */
	u_int minx_;
	u_int miny_;
	u_int maxx_;
	u_int maxy_;
	u_int bbx_;
	u_int bby_;
	u_int bbw_;
	u_int bbh_;

	/*
	 * Table to indicate which blocks have changed.
	 */
	u_char* marks_;
	int mark_;

	/* error counters */
	int bad_psc_;
	int bad_bits_;
	int bad_GOBno_;
	int bad_fmt_;		/* # times RTP fmt != H.261 fmt */

	u_char mb_state_[16 * 64];
	/* inverse quantization via table lookup */
	short quant_[32 * 256];
	/* gob/mba to coordinate mappings */
	u_short base_[16 * 64];
};

class FullP64Decoder : public P64Decoder {
    public:
	FullP64Decoder();
    //protected:
	virtual void allocate();
	void mbcopy(u_int mba);
	void swap();
	virtual void sync();
};

class IntraP64Decoder : public P64Decoder {
    public:
	IntraP64Decoder();
    protected:
	virtual void allocate();
};

#endif
