#ifndef _T1ASM_H_
#define _T1ASM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <SupportDefs.h>

#define ASM_LINESIZE 256

#define ASM_MAXBLOCKLEN ((1L<<17)-6)
#define ASM_MINBLOCKLEN ((1L<<8)-6)

#define ASM_MARKER   128
#define ASM_ASCII    1
#define ASM_BINARY   2
#define ASM_DONE     3

typedef unsigned char byte;

class t1asm
{
 public:
					t1asm();
					~t1asm();
					
	int32 assemble_t1(int pfbflag, const char *inPath, const char *outPath);

 private:

	byte eencrypt(byte plain);
	byte cencrypt(byte plain);
	void output_block();
	void output_byte(byte b);
	void eexec_byte(byte b);
	void eexec_string(char *string);
	void eexec_start();
	void eexec_end();
	void getline();
	int is_integer(char *string);
	void charstring_start();
	void charstring_byte(int v);
	void charstring_end();
	void charstring_int(int num);
	void parse_charstring();
	void usage();
	void print_banner();


		FILE *ifp;
		FILE *ofp;

 int pfb;
 int active;
 int start_charstring;
 int in_eexec;

char line[ASM_LINESIZE];

/* lenIV and charstring start command */
 int lenIV;
 char cs_start[10];

/* for charstring buffering */
 byte charstring_buf[65535];
 byte *charstring_bp;

/* for PFB block buffering */
 byte blockbuf[ASM_MAXBLOCKLEN];
 int32 blocklen;
 int32 blockpos;
 int blocktyp;

/* decryption stuff */
 uint16 er, cr;
 uint16 c1, c2;


	
};

#endif


