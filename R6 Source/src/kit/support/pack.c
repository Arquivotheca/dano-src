#include <SupportDefs.h>

/******************************************************************************/ 
/*                         Start of LZRW1.C                                   */ 
/******************************************************************************/ 
/*
THE LZRW1 ALGORITHM 
=================== 
Author : Ross N. Williams. 
Date   : 31-Mar-1991. 

1. I typed the following code in from my paper "An Extremely Fast Data 
Compression Algorithm", Data Compression Conference, Utah, 7-11 April, 
1991. The  fact that this  code works indicates  that the code  in the 
paper is OK. 

2. This file has been copied into a test harness and works. 

3. Some users running old C compilers may wish to insert blanks around 
the "="  symbols of  assignments so  as to  avoid expressions  such as 
"a=*b;" being interpreted as "a=a*b;" 

4. This code is public domain. 

5. Warning:  This code  is non-deterministic insofar  as it  may yield 
different  compressed representations  of the  same file  on different 
runs. (However, it will always decompress correctly to the original). 

6. If you use this code in anger (e.g. in a product) drop me a note at 
ross@spam.ua.oz.au and I will put you  on a mailing list which will be 
invoked if anyone finds a bug in this code. 

7.   The  internet   newsgroup  comp.compression   might  also   carry 
information on this algorithm from time to time. 
*/
/******************************************************************************/ 

#define FLAG_BYTES    4     /* Number of bytes used by copy flag. */ 
#define FLAG_COMPRESS 0     /* Signals that compression occurred. */ 
#define FLAG_COPY     1     /* Signals that a copyover occurred.  */ 

/******************************************************************************/ 

static
void fast_copy(const uchar *p_src,uchar *p_dst, int len)
{
	while (len--)
		*p_dst++=*p_src++;
} 

/******************************************************************************/ 

void compress(const uchar *p_src_first,ulong src_len,uchar *p_dst_first,ulong *p_dst_len) 

/* Input  : Specify input block using p_src_first and src_len.          */ 
/* Input  : Point p_dst_first to the start of the output zone (OZ).     */ 
/* Input  : Point p_dst_len to a ulong to receive the output length.    */ 
/* Input  : Input block and output zone must not overlap.               */ 
/* Output : Length of output block written to *p_dst_len.               */ 
/* Output : Output block in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */ 
/* Output : May write in OZ=Mem[p_dst_first..p_dst_first+src_len+256-1].*/ 
/* Output : Upon completion guaranteed *p_dst_len<=src_len+FLAG_BYTES.  */ 

#define PS *p++!=*s++  /* Body of inner unrolled matching loop.         */ 
#define ITEMMAX 16     /* Maximum number of bytes in an expanded item.  */ 

{
 	const uchar	*p_src=p_src_first;
	uchar	*p_dst=p_dst_first; 

 	const uchar	*p_src_post=p_src_first+src_len;
	uchar	*p_dst_post=p_dst_first+src_len; 

 	const uchar 	*p_src_max1=p_src_post-ITEMMAX;
	const uchar	*p_src_max16=p_src_post-16*ITEMMAX; 

 	const uchar 	*hash[4096];
	uchar	*p_control;

	ushort	control=0,
			control_bits=0; 

 	*p_dst=FLAG_COMPRESS;
 	p_dst[1] = B_PACK_MAGIC_1;		/* hplus */
 	p_dst[2] = B_PACK_MAGIC_2;
	p_dst+=FLAG_BYTES;
	p_control=p_dst;
	p_dst+=2; 

 	while (1) {
   		const uchar	*p,
   					*s;
	 	ushort	unroll=16,
				len,
				index;
		ulong	offset; 

    
		if (p_dst>p_dst_post)
			goto overrun; 

    	if (p_src>p_src_max16) {
      		unroll=1; 
       		if (p_src>p_src_max1) {
         		if (p_src==p_src_post)
					break;
				goto literal;
			}
		} 

begin_unrolled_loop: 

       	index=((40543*((((p_src[0]<<4)^p_src[1])<<4)^p_src[2]))>>4) & 0xFFF; 

       	p=hash[index];
		hash[index]=s=p_src;
		offset=s-p; 

       	if (offset>4095 || p<p_src_first || offset==0 || PS || PS || PS) {
        	literal:
			*p_dst++=*p_src++;
			control>>=1;
			control_bits++;
		} 
       	else { 
          	PS || PS || PS || PS || PS || PS || PS || 
          	PS || PS || PS || PS || PS || PS || s++;
		
		  	len=s-p_src-1; 
          	*p_dst++=((offset&0xF00)>>4)+(len-1);
			*p_dst++=offset&0xFF; 
          	p_src+=len;
			control=(control>>1)|0x8000;
			control_bits++;
		} 
end_unrolled_loop:
		if (--unroll)
			goto begin_unrolled_loop; 

    	if (control_bits==16)  {
      		*p_control=control&0xFF;
			*(p_control+1)=control>>8; 
       		p_control=p_dst;
			p_dst+=2;
			control=control_bits=0;
		} 
	} 
 	control>>=16-control_bits; 
 	*p_control++=control&0xFF;
	*p_control++=control>>8; 
 	if (p_control==p_dst)
		p_dst-=2; 

 	*p_dst_len=p_dst-p_dst_first; 
 	return; 
 
overrun:
	fast_copy(p_src_first,p_dst_first+FLAG_BYTES,src_len); 
	*p_dst_first=FLAG_COPY;
	*p_dst_len=src_len+FLAG_BYTES; 
} 

/******************************************************************************/ 

void decompress(const uchar *p_src_first,long src_len,uchar *p_dst_first,ulong *p_dst_len) 

/* Input  : Specify input block using p_src_first and src_len.          */ 
/* Input  : Point p_dst_first to the start of the output zone.          */ 
/* Input  : Point p_dst_len to a ULONG to receive the output length.    */ 
/* Input  : Input block and output zone must not overlap. User knows    */ 
/* Input  : upperbound on output block length from earlier compression. */ 
/* Input  : In any case, maximum expansion possible is eight times.     */ 
/* Output : Length of output block written to *p_dst_len.               */ 
/* Output : Output block in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */ 
/* Output : Writes only  in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */ 

{
	ushort	controlbits=0,
			control; 

 	const uchar	*p_src=p_src_first+FLAG_BYTES;
	uchar	*p_dst=p_dst_first;
    const uchar	*p_src_post=p_src_first+src_len; 

 	if (*p_src_first==FLAG_COPY) {
   		fast_copy(p_src_first+FLAG_BYTES,p_dst_first,src_len-FLAG_BYTES); 
    	*p_dst_len=src_len-FLAG_BYTES;
		return;
	} 

 	while (p_src!=p_src_post) {
   		if (controlbits==0) {
      		control=*p_src++;
			control|=(*p_src++)<<8;
			controlbits=16;
		} 
    	if (control&1) {
      		ushort	offset,len;
			uchar	*p; 

       		offset=(*p_src&0xF0)<<4;
			len=1+(*p_src++&0xF); 
       		offset+=*p_src++&0xFF;
			p=p_dst-offset; 
       		while (len--)
				*p_dst++=*p++;
		} 
    	else 
       		*p_dst++=*p_src++; 

    	control>>=1; controlbits--; 
	} 
 	*p_dst_len=p_dst-p_dst_first; 
} 

/******************************************************************************/ 
/*                          End of LZRW1.C                                    */ 
/******************************************************************************/

#ifdef	TESTS
// benoit test

#include <OS.h>
#include <math.h>
#include <stdio.h>

#define	TEST_SIZE	102400

void	main()
{
	char	*src_buffer;
	char	*dst_buffer;
	char	*decomp_buffer;
	long	dst_len;
	long	decomp_len;
	long	i;
	double	start, end;

	src_buffer = (char *)malloc(TEST_SIZE);
	dst_buffer = (char *)malloc(TEST_SIZE);
	decomp_buffer = (char *)malloc(TEST_SIZE);


	for (i = 0; i < TEST_SIZE; i++)
		*(src_buffer + i) = i;

	start = system_time();
	lzrw1_compress(src_buffer, TEST_SIZE, dst_buffer, &dst_len);
	end = system_time();

	printf("compress took %f msec\n", (end-start)/1000.0);	

	printf("dst len=%ld\n", dst_len);

	start = system_time();
	lzrw1_decompress(dst_buffer,dst_len,decomp_buffer,&decomp_len);
	end = system_time();
	printf("decompress took %f msec\n", (end-start)/1000.0);	

	printf("decomp len=%ld\n", decomp_len); 

	for (i = 0; i < TEST_SIZE; i++) {
		if (*(src_buffer + i) != *(decomp_buffer + i))
			printf("mismatch\n");
	}
}

#endif
