/* 
 * smbdes.c 
 *
 * This program implements the des authentication protocol
 * as used in NT4 and Win98 to authenticate SMB clients.
 * It can only be used to encrypt and can only do a single 
 * forward pass. The algorithms are the same as used in 
 * the Samba client software, they were changed here to get
 * around the GPL license agreement. I believe this is code 
 * is allowed under ITAR Section 121 Category XIIIb terms
 * but I'm not a lawyer, so use this code appropriately.
 *
 * Russ McMahon russ@be.com
 *
 * 11/7/98
 */

#define EN0   0      /* MODE == encrypt */
#define DE1   1      /* MODE == decrypt */

typedef struct {
        unsigned long ek[32];
        unsigned long dk[32];
} des_ctx;

static void make_des_key(unsigned char *key,unsigned char *deskey)
{
	int i;

	deskey[0] = key[0]>>1;
	deskey[1] = ((key[0]&0x01)<<6) | (key[1]>>2);
	deskey[2] = ((key[1]&0x03)<<5) | (key[2]>>3);
	deskey[3] = ((key[2]&0x07)<<4) | (key[3]>>4);
	deskey[4] = ((key[3]&0x0F)<<3) | (key[4]>>5);
	deskey[5] = ((key[4]&0x1F)<<2) | (key[5]>>6);
	deskey[6] = ((key[5]&0x3F)<<1) | (key[6]>>7);
	deskey[7] = key[6]&0x7F;
	for (i=0;i<8;i++) {
		deskey[i] = (deskey[i]<<1);
	}
}

static void deskey(unsigned char *, short);
/*                  hexkey[8]     MODE
 * Sets the internal key register according to the hexadecimal
 * key contained in the 8 bytes of hexkey, according to the DES,
 * for encryption or decryption according to MODE.
 */

static void usekey(unsigned long *);
/*                cookedkey[32]
 * Loads the internal key register with the data in cookedkey.
 */

static void cpkey(unsigned long *);
/*               cookedkey[32]
 * Copies the contents of the internal key register into the storage
 * located at &cookedkey[0].
 */

static void des(unsigned char *, unsigned char *);
/*                from[8]         to[8]
 * Encrypts/Decrypts (according to the key currently loaded in the
 * internal key register) one block of eight bytes at address 'from'
 * into the block at address 'to'.  They can be the same.
 */

static void scrunch(unsigned char *, unsigned long *);
static void unscrun(unsigned long *, unsigned char *);
static void desfunc(unsigned long *, unsigned long *);
static void cookey(unsigned long *);

static unsigned long KnL[32] = { 0L };
static unsigned long KnR[32] = { 0L };
static unsigned long Kn3[32] = { 0L };
static unsigned char Df_Key[24] = {
       0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
       0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10,
       0x89,0xab,0xcd,0xef,0x01,0x23,0x45,0x67 };

static unsigned short bytebit[8]    = {
       0200, 0100, 040, 020, 010, 04, 02, 01 };

static unsigned long bigbyte[24] = {
       0x800000L,    0x400000L,     0x200000L,    0x100000L,
       0x80000L,     0x40000L,      0x20000L,     0x10000L,
       0x8000L,      0x4000L,       0x2000L,      0x1000L,
       0x800L,              0x400L,               0x200L,              0x100L,
       0x80L,               0x40L,                0x20L,               0x10L,
       0x8L,         0x4L,          0x2L,         0x1L   };


static unsigned char pc1[56] = {
       56, 48, 40, 32, 24, 16,  8,   0, 57, 49, 41, 33, 25, 17,
        9,  1, 58, 50, 42, 34, 26,  18, 10,  2, 59, 51, 43, 35,
       62, 54, 46, 38, 30, 22, 14,   6, 61, 53, 45, 37, 29, 21,
       13,  5, 60, 52, 44, 36, 28,  20, 12,  4, 27, 19, 11,  3 };

static unsigned char totrot[16] = {
       1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28 };

static unsigned char pc2[48] = {
       13, 16, 10, 23,  0,  4,       2, 27, 14,  5, 20,  9,
       22, 18, 11,  3, 25,  7,      15,  6, 26, 19, 12,  1,
       40, 51, 30, 36, 46, 54,      29, 39, 50, 44, 32, 47,
       43, 48, 38, 55, 33, 52,      45, 41, 49, 35, 28, 31 };

void deskey(unsigned char* key, short edf)       
{
       register int i, j, l, m, n;
       unsigned char pc1m[56], pcr[56];
       unsigned long kn[32];

       for ( j = 0; j < 56; j++ ) {
              l = pc1[j];
              m = l & 07;
              pc1m[j] = (key[l >> 3] & bytebit[m]) ? 1 : 0;
              }
       for( i = 0; i < 16; i++ ) {
              if( edf == DE1 ) m = (15 - i) << 1;
              else m = i << 1;
              n = m + 1;
              kn[m] = kn[n] = 0L;
              for( j = 0; j < 28; j++ ) {
                     l = j + totrot[i];
                     if( l < 28 ) pcr[j] = pc1m[l];
                     else pcr[j] = pc1m[l - 28];
                     }
              for( j = 28; j < 56; j++ ) {
                  l = j + totrot[i];
                  if( l < 56 ) pcr[j] = pc1m[l];
                  else pcr[j] = pc1m[l - 28];
                  }
              for( j = 0; j < 24; j++ ) {
                     if( pcr[pc2[j]] ) kn[m] |= bigbyte[j];
                     if( pcr[pc2[j+24]] ) kn[n] |= bigbyte[j];
                     }
              }
       cookey(kn);
       return;
}

static void cookey(register unsigned long *raw1)
{
       register unsigned long *cook, *raw0;
       unsigned long dough[32];
       register int i;

       cook = dough;
       for( i = 0; i < 16; i++, raw1++ ) {
              raw0 = raw1++;
              *cook   = (*raw0 & 0x00fc0000L) << 6;
              *cook  |= (*raw0 & 0x00000fc0L) << 10;
              *cook  |= (*raw1 & 0x00fc0000L) >> 10;
              *cook++       |= (*raw1 & 0x00000fc0L) >> 6;
              *cook   = (*raw0 & 0x0003f000L) << 12;
              *cook  |= (*raw0 & 0x0000003fL) << 16;
              *cook  |= (*raw1 & 0x0003f000L) >> 4;
              *cook++       |= (*raw1 & 0x0000003fL);
              }
       usekey(dough);
       return;
}

void cpkey(register unsigned long *into)
{
       register unsigned long *from, *endp;

       from = KnL, endp = &KnL[32];
       while( from < endp ) *into++ = *from++;
       return;
}

void usekey(register unsigned long *from)
{
       register unsigned long *to, *endp;

       to = KnL, endp = &KnL[32];
       while( to < endp ) *to++ = *from++;
       return;
}

void des(unsigned char *inblock, unsigned char *outblock)
{
       unsigned long work[2];

       scrunch(inblock, work);
       desfunc(work, KnL);
       unscrun(work, outblock);
       return;
}

static void scrunch(register unsigned char *outof, register unsigned long *into)
{
       *into   = (*outof++ & 0xffL) << 24;
       *into  |= (*outof++ & 0xffL) << 16;
       *into  |= (*outof++ & 0xffL) << 8;
       *into++ |= (*outof++ & 0xffL);
       *into   = (*outof++ & 0xffL) << 24;
       *into  |= (*outof++ & 0xffL) << 16;
       *into  |= (*outof++ & 0xffL) << 8;
       *into  |= (*outof   & 0xffL);
       return;
}

static void unscrun(register unsigned long *outof, register unsigned char *into)
{
       *into++ = (*outof >> 24) & 0xffL;
       *into++ = (*outof >> 16) & 0xffL;
       *into++ = (*outof >>  8) & 0xffL;
       *into++ =  *outof++           & 0xffL;
       *into++ = (*outof >> 24) & 0xffL;
       *into++ = (*outof >> 16) & 0xffL;
       *into++ = (*outof >>  8) & 0xffL;
       *into   =  *outof     & 0xffL;
       return;
}

static unsigned long SP1[64] = {
       0x01010400L, 0x00000000L, 0x00010000L, 0x01010404L,
       0x01010004L, 0x00010404L, 0x00000004L, 0x00010000L,
       0x00000400L, 0x01010400L, 0x01010404L, 0x00000400L,
       0x01000404L, 0x01010004L, 0x01000000L, 0x00000004L,
       0x00000404L, 0x01000400L, 0x01000400L, 0x00010400L,
       0x00010400L, 0x01010000L, 0x01010000L, 0x01000404L,
       0x00010004L, 0x01000004L, 0x01000004L, 0x00010004L,
       0x00000000L, 0x00000404L, 0x00010404L, 0x01000000L,
       0x00010000L, 0x01010404L, 0x00000004L, 0x01010000L,
       0x01010400L, 0x01000000L, 0x01000000L, 0x00000400L,
       0x01010004L, 0x00010000L, 0x00010400L, 0x01000004L,
       0x00000400L, 0x00000004L, 0x01000404L, 0x00010404L,
       0x01010404L, 0x00010004L, 0x01010000L, 0x01000404L,
       0x01000004L, 0x00000404L, 0x00010404L, 0x01010400L,
       0x00000404L, 0x01000400L, 0x01000400L, 0x00000000L,
       0x00010004L, 0x00010400L, 0x00000000L, 0x01010004L };

static unsigned long SP2[64] = {
       0x80108020L, 0x80008000L, 0x00008000L, 0x00108020L,
       0x00100000L, 0x00000020L, 0x80100020L, 0x80008020L,
       0x80000020L, 0x80108020L, 0x80108000L, 0x80000000L,
       0x80008000L, 0x00100000L, 0x00000020L, 0x80100020L,
       0x00108000L, 0x00100020L, 0x80008020L, 0x00000000L,
       0x80000000L, 0x00008000L, 0x00108020L, 0x80100000L,
       0x00100020L, 0x80000020L, 0x00000000L, 0x00108000L,
       0x00008020L, 0x80108000L, 0x80100000L, 0x00008020L,
       0x00000000L, 0x00108020L, 0x80100020L, 0x00100000L,
       0x80008020L, 0x80100000L, 0x80108000L, 0x00008000L,
       0x80100000L, 0x80008000L, 0x00000020L, 0x80108020L,
       0x00108020L, 0x00000020L, 0x00008000L, 0x80000000L,
       0x00008020L, 0x80108000L, 0x00100000L, 0x80000020L,
       0x00100020L, 0x80008020L, 0x80000020L, 0x00100020L,
       0x00108000L, 0x00000000L, 0x80008000L, 0x00008020L,
       0x80000000L, 0x80100020L, 0x80108020L, 0x00108000L };

static unsigned long SP3[64] = {
       0x00000208L, 0x08020200L, 0x00000000L, 0x08020008L,
       0x08000200L, 0x00000000L, 0x00020208L, 0x08000200L,
       0x00020008L, 0x08000008L, 0x08000008L, 0x00020000L,
       0x08020208L, 0x00020008L, 0x08020000L, 0x00000208L,
       0x08000000L, 0x00000008L, 0x08020200L, 0x00000200L,
       0x00020200L, 0x08020000L, 0x08020008L, 0x00020208L,
       0x08000208L, 0x00020200L, 0x00020000L, 0x08000208L,
       0x00000008L, 0x08020208L, 0x00000200L, 0x08000000L,
       0x08020200L, 0x08000000L, 0x00020008L, 0x00000208L,
       0x00020000L, 0x08020200L, 0x08000200L, 0x00000000L,
       0x00000200L, 0x00020008L, 0x08020208L, 0x08000200L,
       0x08000008L, 0x00000200L, 0x00000000L, 0x08020008L,
       0x08000208L, 0x00020000L, 0x08000000L, 0x08020208L,
       0x00000008L, 0x00020208L, 0x00020200L, 0x08000008L,
       0x08020000L, 0x08000208L, 0x00000208L, 0x08020000L,
       0x00020208L, 0x00000008L, 0x08020008L, 0x00020200L };

static unsigned long SP4[64] = {
       0x00802001L, 0x00002081L, 0x00002081L, 0x00000080L,
       0x00802080L, 0x00800081L, 0x00800001L, 0x00002001L,
       0x00000000L, 0x00802000L, 0x00802000L, 0x00802081L,
       0x00000081L, 0x00000000L, 0x00800080L, 0x00800001L,
       0x00000001L, 0x00002000L, 0x00800000L, 0x00802001L,
       0x00000080L, 0x00800000L, 0x00002001L, 0x00002080L,
       0x00800081L, 0x00000001L, 0x00002080L, 0x00800080L,
       0x00002000L, 0x00802080L, 0x00802081L, 0x00000081L,
       0x00800080L, 0x00800001L, 0x00802000L, 0x00802081L,
       0x00000081L, 0x00000000L, 0x00000000L, 0x00802000L,
       0x00002080L, 0x00800080L, 0x00800081L, 0x00000001L,
       0x00802001L, 0x00002081L, 0x00002081L, 0x00000080L,
       0x00802081L, 0x00000081L, 0x00000001L, 0x00002000L,
       0x00800001L, 0x00002001L, 0x00802080L, 0x00800081L,
       0x00002001L, 0x00002080L, 0x00800000L, 0x00802001L,
       0x00000080L, 0x00800000L, 0x00002000L, 0x00802080L };

static unsigned long SP5[64] = {
       0x00000100L, 0x02080100L, 0x02080000L, 0x42000100L,
       0x00080000L, 0x00000100L, 0x40000000L, 0x02080000L,
       0x40080100L, 0x00080000L, 0x02000100L, 0x40080100L,
       0x42000100L, 0x42080000L, 0x00080100L, 0x40000000L,
       0x02000000L, 0x40080000L, 0x40080000L, 0x00000000L,
       0x40000100L, 0x42080100L, 0x42080100L, 0x02000100L,
       0x42080000L, 0x40000100L, 0x00000000L, 0x42000000L,
       0x02080100L, 0x02000000L, 0x42000000L, 0x00080100L,
       0x00080000L, 0x42000100L, 0x00000100L, 0x02000000L,
       0x40000000L, 0x02080000L, 0x42000100L, 0x40080100L,
       0x02000100L, 0x40000000L, 0x42080000L, 0x02080100L,
       0x40080100L, 0x00000100L, 0x02000000L, 0x42080000L,
       0x42080100L, 0x00080100L, 0x42000000L, 0x42080100L,
       0x02080000L, 0x00000000L, 0x40080000L, 0x42000000L,
       0x00080100L, 0x02000100L, 0x40000100L, 0x00080000L,
       0x00000000L, 0x40080000L, 0x02080100L, 0x40000100L };

static unsigned long SP6[64] = {
       0x20000010L, 0x20400000L, 0x00004000L, 0x20404010L,
       0x20400000L, 0x00000010L, 0x20404010L, 0x00400000L,
       0x20004000L, 0x00404010L, 0x00400000L, 0x20000010L,
       0x00400010L, 0x20004000L, 0x20000000L, 0x00004010L,
       0x00000000L, 0x00400010L, 0x20004010L, 0x00004000L,
       0x00404000L, 0x20004010L, 0x00000010L, 0x20400010L,
       0x20400010L, 0x00000000L, 0x00404010L, 0x20404000L,
       0x00004010L, 0x00404000L, 0x20404000L, 0x20000000L,
       0x20004000L, 0x00000010L, 0x20400010L, 0x00404000L,
       0x20404010L, 0x00400000L, 0x00004010L, 0x20000010L,
       0x00400000L, 0x20004000L, 0x20000000L, 0x00004010L,
       0x20000010L, 0x20404010L, 0x00404000L, 0x20400000L,
       0x00404010L, 0x20404000L, 0x00000000L, 0x20400010L,
       0x00000010L, 0x00004000L, 0x20400000L, 0x00404010L,
       0x00004000L, 0x00400010L, 0x20004010L, 0x00000000L,
       0x20404000L, 0x20000000L, 0x00400010L, 0x20004010L };

static unsigned long SP7[64] = {
       0x00200000L, 0x04200002L, 0x04000802L, 0x00000000L,
       0x00000800L, 0x04000802L, 0x00200802L, 0x04200800L,
       0x04200802L, 0x00200000L, 0x00000000L, 0x04000002L,
       0x00000002L, 0x04000000L, 0x04200002L, 0x00000802L,
       0x04000800L, 0x00200802L, 0x00200002L, 0x04000800L,
       0x04000002L, 0x04200000L, 0x04200800L, 0x00200002L,
       0x04200000L, 0x00000800L, 0x00000802L, 0x04200802L,
       0x00200800L, 0x00000002L, 0x04000000L, 0x00200800L,
       0x04000000L, 0x00200800L, 0x00200000L, 0x04000802L,
       0x04000802L, 0x04200002L, 0x04200002L, 0x00000002L,
       0x00200002L, 0x04000000L, 0x04000800L, 0x00200000L,
       0x04200800L, 0x00000802L, 0x00200802L, 0x04200800L,
       0x00000802L, 0x04000002L, 0x04200802L, 0x04200000L,
       0x00200800L, 0x00000000L, 0x00000002L, 0x04200802L,
       0x00000000L, 0x00200802L, 0x04200000L, 0x00000800L,
       0x04000002L, 0x04000800L, 0x00000800L, 0x00200002L };

static unsigned long SP8[64] = {
       0x10001040L, 0x00001000L, 0x00040000L, 0x10041040L,
       0x10000000L, 0x10001040L, 0x00000040L, 0x10000000L,
       0x00040040L, 0x10040000L, 0x10041040L, 0x00041000L,
       0x10041000L, 0x00041040L, 0x00001000L, 0x00000040L,
       0x10040000L, 0x10000040L, 0x10001000L, 0x00001040L,
       0x00041000L, 0x00040040L, 0x10040040L, 0x10041000L,
       0x00001040L, 0x00000000L, 0x00000000L, 0x10040040L,
       0x10000040L, 0x10001000L, 0x00041040L, 0x00040000L,
       0x00041040L, 0x00040000L, 0x10041000L, 0x00001000L,
       0x00000040L, 0x10040040L, 0x00001000L, 0x00041040L,
       0x10001000L, 0x00000040L, 0x10000040L, 0x10040000L,
       0x10040040L, 0x10000000L, 0x00040000L, 0x10001040L,
       0x00000000L, 0x10041040L, 0x00040040L, 0x10000040L,
       0x10040000L, 0x10001000L, 0x10001040L, 0x00000000L,
       0x10041040L, 0x00041000L, 0x00041000L, 0x00001040L,
       0x00001040L, 0x00040040L, 0x10000000L, 0x10041000L };

static void desfunc(register unsigned long *block, register unsigned long *keys)
{
       register unsigned long fval, work, right, leftt;
       register int round;

       leftt = block[0];
       right = block[1];
       work = ((leftt >> 4) ^ right) & 0x0f0f0f0fL;
       right ^= work;
       leftt ^= (work << 4);
       work = ((leftt >> 16) ^ right) & 0x0000ffffL;
       right ^= work;
       leftt ^= (work << 16);
       work = ((right >> 2) ^ leftt) & 0x33333333L;
       leftt ^= work;
       right ^= (work << 2);
       work = ((right >> 8) ^ leftt) & 0x00ff00ffL;
       leftt ^= work;
       right ^= (work << 8);
       right = ((right << 1) | ((right >> 31) & 1L)) & 0xffffffffL;
       work = (leftt ^ right) & 0xaaaaaaaaL;
       leftt ^= work;
       right ^= work;
       leftt = ((leftt << 1) | ((leftt >> 31) & 1L)) & 0xffffffffL;

       for( round = 0; round < 8; round++ ) {
              work  = (right << 28) | (right >> 4);
              work ^= *keys++;
              fval  = SP7[ work             & 0x3fL];
              fval |= SP5[(work >>  8) & 0x3fL];
              fval |= SP3[(work >> 16) & 0x3fL];
              fval |= SP1[(work >> 24) & 0x3fL];
              work  = right ^ *keys++;
              fval |= SP8[ work             & 0x3fL];
              fval |= SP6[(work >>  8) & 0x3fL];
              fval |= SP4[(work >> 16) & 0x3fL];
              fval |= SP2[(work >> 24) & 0x3fL];
              leftt ^= fval;
              work  = (leftt << 28) | (leftt >> 4);
              work ^= *keys++;
              fval  = SP7[ work             & 0x3fL];
              fval |= SP5[(work >>  8) & 0x3fL];
              fval |= SP3[(work >> 16) & 0x3fL];
              fval |= SP1[(work >> 24) & 0x3fL];
              work  = leftt ^ *keys++;
              fval |= SP8[ work             & 0x3fL];
              fval |= SP6[(work >>  8) & 0x3fL];
              fval |= SP4[(work >> 16) & 0x3fL];
              fval |= SP2[(work >> 24) & 0x3fL];
              right ^= fval;
              }

       right = (right << 31) | (right >> 1);
       work = (leftt ^ right) & 0xaaaaaaaaL;
       leftt ^= work;
       right ^= work;
       leftt = (leftt << 31) | (leftt >> 1);
       work = ((leftt >> 8) ^ right) & 0x00ff00ffL;
       right ^= work;
       leftt ^= (work << 8);
       work = ((leftt >> 2) ^ right) & 0x33333333L;
       right ^= work;
       leftt ^= (work << 2);
       work = ((right >> 16) ^ leftt) & 0x0000ffffL;
       leftt ^= work;
       right ^= (work << 16);
       work = ((right >> 4) ^ leftt) & 0x0f0f0f0fL;
       leftt ^= work;
       right ^= (work << 4);
       *block++ = right;
       *block = leftt;
       return;
}


void des_key(des_ctx *dc, unsigned char *key){
        deskey(key,EN0);
        cpkey(dc->ek);
}


/* Encrypt 1 blocks in ECB mode.  */
void des_enc(des_ctx *dc, unsigned char *data){
        unsigned long work[2];
        int i;
        unsigned char *cp;

        cp = data;
        scrunch(cp,work);
        desfunc(work,dc->ek);
        unscrun(work,cp);
        cp+=8;
}


/*
 * The following SMB encryption techniques are specified
 * in CAE Specification, Protocols for X\Open PC interworking:
 * SMB, Version 2. Appendix D. The only difference is the 
 * Microsoft well known string (sp8 below) the specs say
 * "????????" which is wrong.
 */
void E_P16(unsigned char *key,unsigned char *p16)
{
	/* sp8 is a magic value available from Microsoft. */
	unsigned char s8[8] = {0x4b,0x47,0x53,0x21,0x40,0x23,0x24,0x25}; 
	
	/* Comment out s8 above and use s8 below to do validation tests */
	/* unsigned char s8[8] = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xe7}; */
	
	des_ctx dc;
	unsigned char x[8]; // Keep the data through all passes
	unsigned char key2[8];


	/* Get the des encoded first 8 bytes 
	 * Key strings come in as 7 byte chucks, Microsoft passwords
	 * are always upper case and 14 bytes. str_to_key coverts to
	 * 8 byte key lenghts (2 Des key lengths ). That is why the
	 * des_enc is called twice.
	 */
    make_des_key(key, key2); 
	des_key(&dc, key2);
	memcpy(x, s8, 8);
	des_enc(&dc, x);
	memcpy(p16, x, 8);
	
	/* Get the des encoded second 8 bytes */
	make_des_key(key+7, key2); 
	des_key(&dc, key2);
	memcpy(x, s8, 8);
	des_enc(&dc, x);
	memcpy(p16+8, x, 8);
}

void E_P24(unsigned char *key, unsigned char *c8, unsigned char *p24)
{
	des_ctx dc;
	unsigned char x[8];
	unsigned char key2[8];
	
	/*
	 * Need to go through 3 des key lengths this time
	 */
	make_des_key(key, key2); 
	des_key(&dc, key2);
	memcpy(x, c8, 8);
	des_enc(&dc, x);
	memcpy(p24, x, 8);
	
	/* Second key length */
	make_des_key(key+7, key2); 
	des_key(&dc, key2);
	memcpy(x, c8, 8);
	des_enc(&dc, x);
	memcpy(p24+8, x, 8);
	
	/* Third key length */
	make_des_key(key+14, key2); 
	des_key(&dc, key2);
	memcpy(x, c8, 8);
	des_enc(&dc, x);
	memcpy(p24+16, x, 8);
}
