
#include "polyphase_int.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

/* ------------------------------------------------------------------------*/

static const int32 syn_f_window_int[HAN_SIZE] =
{
	0x00000000, 0x00000005, 0x0000001d, 0x00000068,
	0x000000d5, 0xffffff6f, 0x000001cb, 0x0000061f,
	0x000007f5, 0x0000002d, 0x00001421, 0x000025ff,
	0x000019ae, 0x000026f7, 0x00009271, 0x0000fa13,
	0x0001251e, 0xffff05ee, 0xffff6d90, 0xffffd90a,
	0x000019ae, 0xffffda02, 0xffffebe0, 0xffffffd4,
	0x000007f5, 0xfffff9e2, 0xfffffe36, 0x00000092,
	0x000000d5, 0xffffff99, 0xffffffe4, 0xfffffffc,
	0x00000000, 0x0000001a, 0x0000001f, 0x000000d0,
	0x000000da, 0x00000191, 0x00000207, 0x0000080f,
	0x000007d0, 0x000012b4, 0x0000158d, 0x00001bde,
	0x00001747, 0x00008b38, 0x000099a8, 0x000124f0,
	0x000124f0, 0xffff6659, 0xffff74c9, 0x00001747,
	0x00001bde, 0xffffea74, 0xffffed4d, 0x000007d0,
	0x0000080f, 0xfffffdfa, 0xfffffe70, 0x000000da,
	0x000000d0, 0xffffffe2, 0xffffffe7, 0x00000000,
	0x00000000, 0x00000018, 0x00000023, 0x000000ca,
	0x000000de, 0x0000015b, 0x00000245, 0x00000820,
	0x000007a0, 0x00001149, 0x000016f7, 0x00001dd8,
	0x000014a8, 0x000083ff, 0x0000a0d8, 0x00012468,
	0x00012468, 0xffff5f29, 0xffff7c02, 0x000014a8,
	0x00001dd8, 0xffffe90a, 0xffffeeb8, 0x000007a0,
	0x00000820, 0xfffffdbc, 0xfffffea6, 0x000000de,
	0x000000ca, 0xffffffde, 0xffffffe9, 0x00000000,
	0x00000000, 0x00000015, 0x00000026, 0x000000c4,
	0x000000e1, 0x00000126, 0x00000285, 0x00000827,
	0x00000765, 0x00000fdf, 0x0000185d, 0x00001f9c,
	0x000011d1, 0x00007ccb, 0x0000a7fe, 0x00012386,
	0x00012386, 0xffff5803, 0xffff8336, 0x000011d1,
	0x00001f9c, 0xffffe7a4, 0xfffff022, 0x00000765,
	0x00000827, 0xfffffd7c, 0xfffffedb, 0x000000e1,
	0x000000c4, 0xffffffdb, 0xffffffec, 0x00000000,
	0x00000000, 0x00000013, 0x00000029, 0x000000be,
	0x000000e3, 0x000000f4, 0x000002c7, 0x00000825,
	0x0000071e, 0x00000e79, 0x000019bd, 0x0000212c,
	0x00000ec0, 0x000075a0, 0x0000af15, 0x00012249,
	0x00012249, 0xffff50ec, 0xffff8a61, 0x00000ec0,
	0x0000212c, 0xffffe644, 0xfffff188, 0x0000071e,
	0x00000825, 0xfffffd3a, 0xffffff0d, 0x000000e3,
	0x000000be, 0xffffffd8, 0xffffffee, 0x00000000,
	0x00000000, 0x00000011, 0x0000002d, 0x000000b7,
	0x000000e4, 0x000000c5, 0x0000030b, 0x0000081b,
	0x000006cb, 0x00000d17, 0x00001b17, 0x00002288,
	0x00000b77, 0x00006e81, 0x0000b619, 0x000120b4,
	0x000120b4, 0xffff49e8, 0xffff9180, 0x00000b77,
	0x00002288, 0xffffe4ea, 0xfffff2ea, 0x000006cb,
	0x0000081b, 0xfffffcf6, 0xffffff3c, 0x000000e4,
	0x000000b7, 0xffffffd4, 0xfffffff0, 0x00000000,
	0x00000000, 0x00000010, 0x00000031, 0x000000b0,
	0x000000e4, 0x00000099, 0x00000350, 0x00000809,
	0x0000066c, 0x00000bbc, 0x00001c67, 0x000023b3,
	0x000007f5, 0x00006772, 0x0000bd06, 0x00011ec7,
	0x00011ec7, 0xffff42fb, 0xffff988f, 0x000007f5,
	0x000023b3, 0xffffe39a, 0xfffff445, 0x0000066c,
	0x00000809, 0xfffffcb1, 0xffffff68, 0x000000e4,
	0x000000b0, 0xffffffd0, 0xfffffff1, 0x00000000,
	0xffffffff, 0x0000000e, 0x00000035, 0x000000a9,
	0x000000e3, 0x0000006f, 0x00000397, 0x000007f0,
	0x000005ff, 0x00000a67, 0x00001dad, 0x000024ad,
	0x0000043a, 0x00006076, 0x0000c3d9, 0x00011c83,
	0x00011c83, 0xffff3c28, 0xffff9f8b, 0x0000043a,
	0x000024ad, 0xffffe254, 0xfffff59a, 0x000005ff,
	0x000007f0, 0xfffffc6a, 0xffffff92, 0x000000e3,
	0x000000a9, 0xffffffcc, 0xfffffff3, 0xffffffff,
	0xffffffff, 0x0000000d, 0x0000003a, 0x000000a1,
	0x000000e0, 0x00000048, 0x000003df, 0x000007d1,
	0x00000586, 0x0000091a, 0x00001ee6, 0x00002578,
	0x00000046, 0x00005991, 0x0000ca8d, 0x000119e9,
	0x000119e9, 0xffff3574, 0xffffa670, 0x00000046,
	0x00002578, 0xffffe11b, 0xfffff6e7, 0x00000586,
	0x000007d1, 0xfffffc22, 0xffffffb9, 0x000000e0,
	0x000000a1, 0xffffffc7, 0xfffffff4, 0xffffffff,
	0xffffffff, 0x0000000b, 0x0000003f, 0x0000009a,
	0x000000dd, 0x00000024, 0x00000428, 0x000007aa,
	0x00000500, 0x000007d6, 0x00002011, 0x00002616,
	0xfffffc1b, 0x000052c5, 0x0000d11e, 0x000116fc,
	0x000116fc, 0xffff2ee3, 0xffffad3c, 0xfffffc1b,
	0x00002616, 0xffffdff0, 0xfffff82b, 0x00000500,
	0x000007aa, 0xfffffbd9, 0xffffffdd, 0x000000dd,
	0x0000009a, 0xffffffc2, 0xfffffff6, 0xffffffff,
	0xffffffff, 0x0000000a, 0x00000044, 0x00000093,
	0x000000d7, 0x00000002, 0x00000471, 0x0000077f,
	0x0000046b, 0x0000069c, 0x0000212b, 0x00002687,
	0xfffff7b7, 0x00004c16, 0x0000d78a, 0x000113be,
	0x000113be, 0xffff2877, 0xffffb3eb, 0xfffff7b7,
	0x00002687, 0xffffded6, 0xfffff965, 0x0000046b,
	0x0000077f, 0xfffffb90, 0xffffffff, 0x000000d7,
	0x00000093, 0xffffffbd, 0xfffffff7, 0xffffffff,
	0xfffffffe, 0x00000009, 0x00000049, 0x0000008b,
	0x000000d0, 0xffffffe4, 0x000004ba, 0x0000074e,
	0x000003ca, 0x0000056c, 0x00002233, 0x000026cf,
	0xfffff31d, 0x00004587, 0x0000ddca, 0x0001102f,
	0x0001102f, 0xffff2237, 0xffffba7a, 0xfffff31d,
	0x000026cf, 0xffffddce, 0xfffffa95, 0x000003ca,
	0x0000074e, 0xfffffb47, 0x0000001d, 0x000000d0,
	0x0000008b, 0xffffffb8, 0xfffffff8, 0xfffffffe,
	0xfffffffe, 0x00000008, 0x0000004f, 0x00000084,
	0x000000c8, 0xffffffc8, 0x00000503, 0x00000719,
	0x0000031a, 0x00000447, 0x00002326, 0x000026ee,
	0xffffee4c, 0x00003f1b, 0x0000e3dd, 0x00010c54,
	0x00010c54, 0xffff1c24, 0xffffc0e6, 0xffffee4c,
	0x000026ee, 0xffffdcdb, 0xfffffbba, 0x0000031a,
	0x00000719, 0xfffffafe, 0x00000039, 0x000000c8,
	0x00000084, 0xffffffb2, 0xfffffff9, 0xfffffffe,
	0xfffffffd, 0x00000007, 0x00000055, 0x0000007d,
	0x000000bd, 0xffffffae, 0x0000054c, 0x000006df,
	0x0000025d, 0x0000032e, 0x00002403, 0x000026e7,
	0xffffe947, 0x000038d4, 0x0000e9be, 0x0001082d,
	0x0001082d, 0xffff1643, 0xffffc72d, 0xffffe947,
	0x000026e7, 0xffffdbfe, 0xfffffcd3, 0x0000025d,
	0x000006df, 0xfffffab5, 0x00000053, 0x000000bd,
	0x0000007d, 0xffffffac, 0xfffffffa, 0xfffffffd,
	0xfffffffd, 0x00000007, 0x0000005b, 0x00000075,
	0x000000b1, 0xffffff97, 0x00000594, 0x000006a2,
	0x00000192, 0x00000221, 0x000024c8, 0x000026bc,
	0xffffe40f, 0x000032b4, 0x0000ef69, 0x000103be,
	0x000103be, 0xffff1098, 0xffffcd4d, 0xffffe40f,
	0x000026bc, 0xffffdb39, 0xfffffde0, 0x00000192,
	0x000006a2, 0xfffffa6d, 0x0000006a, 0x000000b1,
	0x00000075, 0xffffffa6, 0xfffffffa, 0xfffffffd,
	0xfffffffc, 0x00000006, 0x00000061, 0x0000006f,
	0x000000a3, 0xffffff82, 0x000005da, 0x00000662,
	0x000000b9, 0x00000120, 0x00002571, 0x0000266e,
	0xffffdea5, 0x00002cbf, 0x0000f4dc, 0x0000ff0a,
	0x0000ff0a, 0xffff0b25, 0xffffd342, 0xffffdea5,
	0x0000266e, 0xffffda90, 0xfffffee1, 0x000000b9,
	0x00000662, 0xfffffa27, 0x0000007f, 0x000000a3,
	0x0000006f, 0xffffffa0, 0xfffffffb, 0xfffffffc
};

/*-------------------------------------------------------------------------*/

static const int32 syn_f_window_short[HAN_SIZE-128] =
{
	0x00000000, 0xffffff6f, 0x000001cb, 0x0000061f,
	0x000007f5, 0x0000002d, 0x00001421, 0x000025ff,
	0x000019ae, 0x000026f7, 0x00009271, 0x0000fa13,
	0x0001251e, 0xffff05ee, 0xffff6d90, 0xffffd90a,
	0x000019ae, 0xffffda02, 0xffffebe0, 0xffffffd4,
	0x000007f5, 0xfffff9e2, 0xfffffe36, 0x00000092,
	0x000000da, 0x00000191, 0x00000207, 0x0000080f,
	0x000007d0, 0x000012b4, 0x0000158d, 0x00001bde,
	0x00001747, 0x00008b38, 0x000099a8, 0x000124f0,
	0x000124f0, 0xffff6659, 0xffff74c9, 0x00001747,
	0x00001bde, 0xffffea74, 0xffffed4d, 0x000007d0,
	0x0000080f, 0xfffffdfa, 0xfffffe70, 0x000000da,
	0x000000de, 0x0000015b, 0x00000245, 0x00000820,
	0x000007a0, 0x00001149, 0x000016f7, 0x00001dd8,
	0x000014a8, 0x000083ff, 0x0000a0d8, 0x00012468,
	0x00012468, 0xffff5f29, 0xffff7c02, 0x000014a8,
	0x00001dd8, 0xffffe90a, 0xffffeeb8, 0x000007a0,
	0x00000820, 0xfffffdbc, 0xfffffea6, 0x000000de,
	0x000000e1, 0x00000126, 0x00000285, 0x00000827,
	0x00000765, 0x00000fdf, 0x0000185d, 0x00001f9c,
	0x000011d1, 0x00007ccb, 0x0000a7fe, 0x00012386,
	0x00012386, 0xffff5803, 0xffff8336, 0x000011d1,
	0x00001f9c, 0xffffe7a4, 0xfffff022, 0x00000765,
	0x00000827, 0xfffffd7c, 0xfffffedb, 0x000000e1,
	0x000000e3, 0x000000f4, 0x000002c7, 0x00000825,
	0x0000071e, 0x00000e79, 0x000019bd, 0x0000212c,
	0x00000ec0, 0x000075a0, 0x0000af15, 0x00012249,
	0x00012249, 0xffff50ec, 0xffff8a61, 0x00000ec0,
	0x0000212c, 0xffffe644, 0xfffff188, 0x0000071e,
	0x00000825, 0xfffffd3a, 0xffffff0d, 0x000000e3,
	0x000000e4, 0x000000c5, 0x0000030b, 0x0000081b,
	0x000006cb, 0x00000d17, 0x00001b17, 0x00002288,
	0x00000b77, 0x00006e81, 0x0000b619, 0x000120b4,
	0x000120b4, 0xffff49e8, 0xffff9180, 0x00000b77,
	0x00002288, 0xffffe4ea, 0xfffff2ea, 0x000006cb,
	0x0000081b, 0xfffffcf6, 0xffffff3c, 0x000000e4,
	0x000000e4, 0x00000099, 0x00000350, 0x00000809,
	0x0000066c, 0x00000bbc, 0x00001c67, 0x000023b3,
	0x000007f5, 0x00006772, 0x0000bd06, 0x00011ec7,
	0x00011ec7, 0xffff42fb, 0xffff988f, 0x000007f5,
	0x000023b3, 0xffffe39a, 0xfffff445, 0x0000066c,
	0x00000809, 0xfffffcb1, 0xffffff68, 0x000000e4,
	0x000000e3, 0x0000006f, 0x00000397, 0x000007f0,
	0x00000600, 0x00000a67, 0x00001dad, 0x000024ad,
	0x0000043a, 0x00006076, 0x0000c3d9, 0x00011c83,
	0x00011c83, 0xffff3c28, 0xffff9f8b, 0x0000043a,
	0x000024ad, 0xffffe254, 0xfffff59a, 0x00000600,
	0x000007f0, 0xfffffc6a, 0xffffff92, 0x000000e3,
	0x000000e0, 0x00000048, 0x000003df, 0x000007d1,
	0x00000586, 0x0000091a, 0x00001ee6, 0x00002578,
	0x00000046, 0x00005991, 0x0000ca8d, 0x000119e9,
	0x000119e9, 0xffff3574, 0xffffa670, 0x00000046,
	0x00002578, 0xffffe11b, 0xfffff6e7, 0x00000586,
	0x000007d1, 0xfffffc22, 0xffffffb9, 0x000000e0,
	0x000000dd, 0x00000024, 0x00000428, 0x000007aa,
	0x00000500, 0x000007d6, 0x00002011, 0x00002616,
	0xfffffc1b, 0x000052c5, 0x0000d11e, 0x000116fc,
	0x000116fc, 0xffff2ee3, 0xffffad3c, 0xfffffc1b,
	0x00002616, 0xffffdff0, 0xfffff82b, 0x00000500,
	0x000007aa, 0xfffffbd9, 0xffffffdd, 0x000000dd,
	0x000000d7, 0x00000002, 0x00000471, 0x0000077f,
	0x0000046b, 0x0000069c, 0x0000212b, 0x00002687,
	0xfffff7b7, 0x00004c16, 0x0000d78a, 0x000113be,
	0x000113be, 0xffff2877, 0xffffb3eb, 0xfffff7b7,
	0x00002687, 0xffffded6, 0xfffff965, 0x0000046b,
	0x0000077f, 0xfffffb90, 0xffffffff, 0x000000d7,
	0x000000d0, 0xffffffe4, 0x000004ba, 0x0000074e,
	0x000003ca, 0x0000056c, 0x00002233, 0x000026cf,
	0xfffff31d, 0x00004587, 0x0000ddca, 0x0001102f,
	0x0001102f, 0xffff2237, 0xffffba7a, 0xfffff31d,
	0x000026cf, 0xffffddce, 0xfffffa95, 0x000003ca,
	0x0000074e, 0xfffffb47, 0x0000001d, 0x000000d0,
	0x000000c8, 0xffffffc8, 0x00000503, 0x00000719,
	0x0000031a, 0x00000447, 0x00002326, 0x000026ee,
	0xffffee4c, 0x00003f1b, 0x0000e3dd, 0x00010c54,
	0x00010c54, 0xffff1c24, 0xffffc0e6, 0xffffee4c,
	0x000026ee, 0xffffdcdb, 0xfffffbba, 0x0000031a,
	0x00000719, 0xfffffafe, 0x00000039, 0x000000c8,
	0x000000bd, 0xffffffae, 0x0000054c, 0x000006df,
	0x0000025d, 0x0000032e, 0x00002403, 0x000026e7,
	0xffffe947, 0x000038d4, 0x0000e9be, 0x0001082d,
	0x0001082d, 0xffff1643, 0xffffc72d, 0xffffe947,
	0x000026e7, 0xffffdbfe, 0xfffffcd3, 0x0000025d,
	0x000006df, 0xfffffab5, 0x00000053, 0x000000bd,
	0x000000b1, 0xffffff97, 0x00000594, 0x000006a2,
	0x00000192, 0x00000221, 0x000024c8, 0x000026bc,
	0xffffe40f, 0x000032b4, 0x0000ef69, 0x000103be,
	0x000103be, 0xffff1098, 0xffffcd4d, 0xffffe40f,
	0x000026bc, 0xffffdb39, 0xfffffde0, 0x00000192,
	0x000006a2, 0xfffffa6d, 0x0000006a, 0x000000b1,
	0x000000a3, 0xffffff82, 0x000005da, 0x00000662,
	0x000000b9, 0x00000120, 0x00002571, 0x0000266e,
	0xffffdea5, 0x00002cbf, 0x0000f4dc, 0x0000ff0a,
	0x0000ff0a, 0xffff0b25, 0xffffd342, 0xffffdea5,
	0x0000266e, 0xffffda90, 0xfffffee1, 0x000000b9,
	0x00000662, 0xfffffa27, 0x0000007f, 0x000000a3
};

/*-------------------------------------------------------------------------*/

static const int32 cost32_c0_int[] = 
{ 
	0x00008028, 0x00008167, 0x000083f4, 0x000087f2,
	0x00008d98, 0x0000953b, 0x00009f5c, 0x0000acc0,
	0x0000be9a, 0x0000d6e0, 0x0000f8fa, 0x00012b60,
	0x00017bf2, 0x00020ecb, 0x00036859, 0x000a30a4
};

/* ------------------------------------------------------------------------*/

static const int32 cost32_c1_int[] =
{
	0x0000809f, 0x000085c2, 0x00009123, 0x0000a596,
	0x0000c9c5, 0x00010f89, 0x0001b8f2, 0x000519e5
};

/* ------------------------------------------------------------------------*/

static const int32 cost32_c2_int[] =
{
	0x00008282, 0x000099f2, 0x0000e665, 0x0002901b
};

/* ------------------------------------------------------------------------*/

static const int32 cost32_c3_int[] =
{
	0x00008a8c, 0x00014e7b
};

/* ------------------------------------------------------------------------*/

static const int32 cost32_c4[] =
{
	0x0000b505
};

/*-------------------------------------------------------------------------*/

void cost32(const int32 *vec,int32 *f_vec);

/*-------------------------------------------------------------------------*/

void cost16(const int32 *vec,int32 *f_vec)
{
  int32 tmp1_0,tmp1_1,tmp1_2,tmp1_3,tmp1_4,tmp1_5,tmp1_6,tmp1_7;
  int32 res1_0,res1_1,res1_2,res1_3,res1_4,res1_5,res1_6,res1_7;

  int32 tmp2_0,tmp2_1,tmp2_2,tmp2_3;
  int32 res2_0,res2_1,res2_2,res2_3;

  int32 tmp3_0,tmp3_1;
  int32 res3_0,res3_1;

  tmp1_0 = vec[0]+vec[30];
  tmp1_1 = vec[2]+vec[28];
  tmp1_2 = vec[4]+vec[26];
  tmp1_3 = vec[6]+vec[24];
  tmp1_4 = vec[8]+vec[22];
  tmp1_5 = vec[10]+vec[20];
  tmp1_6 = vec[12]+vec[18];
  tmp1_7 = vec[14]+vec[16];

  tmp2_0 = tmp1_0+tmp1_7;
  tmp2_1 = tmp1_1+tmp1_6;
  tmp2_2 = tmp1_2+tmp1_5;
  tmp2_3 = tmp1_3+tmp1_4;

  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  f_vec[0]  = tmp3_0+tmp3_1;
  f_vec[32] = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
  tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);
    
  f_vec[16] = res3_0+res3_1;
  f_vec[48] = res3_1;
    
  tmp2_0 = fixed32_mul(tmp1_0-tmp1_7, cost32_c2_int[0]);
  tmp2_1 = fixed32_mul(tmp1_1-tmp1_6, cost32_c2_int[1]);
  tmp2_2 = fixed32_mul(tmp1_2-tmp1_5, cost32_c2_int[2]);
  tmp2_3 = fixed32_mul(tmp1_3-tmp1_4, cost32_c2_int[3]);
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res2_0 = tmp3_0+tmp3_1;
  res2_2 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
  tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);
  res3_0 = tmp3_0+tmp3_1;
  res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  res2_1 = res3_0+res3_1;
  res2_3 = res3_1;

  res1_1 = res2_0+res2_1;
  res1_3 = res2_1+res2_2;
  res1_5 = res2_2+res2_3;
  res1_7 = res2_3;
    
  f_vec[24] = res1_1;
  f_vec[8]  = res1_3;
  f_vec[40] = res1_5;
  f_vec[56] = res1_7;

  tmp1_0 = fixed32_mul(vec[0]-vec[30],  cost32_c1_int[0]);
  tmp1_1 = fixed32_mul(vec[2]-vec[28],  cost32_c1_int[1]);
  tmp1_2 = fixed32_mul(vec[4]-vec[26],  cost32_c1_int[2]);
  tmp1_3 = fixed32_mul(vec[6]-vec[24],  cost32_c1_int[3]);
  tmp1_4 = fixed32_mul(vec[8]-vec[22],  cost32_c1_int[4]);
  tmp1_5 = fixed32_mul(vec[10]-vec[20], cost32_c1_int[5]);
  tmp1_6 = fixed32_mul(vec[12]-vec[18], cost32_c1_int[6]);
  tmp1_7 = fixed32_mul(vec[14]-vec[16], cost32_c1_int[7]);

  tmp2_0 = tmp1_0+tmp1_7;
  tmp2_1 = tmp1_1+tmp1_6;
  tmp2_2 = tmp1_2+tmp1_5;
  tmp2_3 = tmp1_3+tmp1_4;
  
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res1_0 = tmp3_0+tmp3_1;
  res1_4 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
  tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  res1_2 = res3_0+res3_1;
  res1_6 = res3_1;

  tmp2_0 = fixed32_mul(tmp1_0-tmp1_7, cost32_c2_int[0]);
  tmp2_1 = fixed32_mul(tmp1_1-tmp1_6, cost32_c2_int[1]);
  tmp2_2 = fixed32_mul(tmp1_2-tmp1_5, cost32_c2_int[2]);
  tmp2_3 = fixed32_mul(tmp1_3-tmp1_4, cost32_c2_int[3]);
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res2_0 = tmp3_0+tmp3_1;
  res2_2 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
  tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  res2_1 = res3_0+res3_1;
  res2_3 = res3_1;
  res1_1 = res2_0+res2_1;
  res1_3 = res2_1+res2_2;
  res1_5 = res2_2+res2_3;
  res1_7 = res2_3;

  f_vec[28] = res1_0+res1_1;
  f_vec[20] = res1_1+res1_2;
  f_vec[12] = res1_2+res1_3;
  f_vec[4]  = res1_3+res1_4;
  f_vec[36] = res1_4+res1_5;
  f_vec[44] = res1_5+res1_6;
  f_vec[52] = res1_6+res1_7;
  f_vec[60] = res1_7;
}

/*-------------------------------------------------------------------------*/

void cost8(const int32 *vec,int32 *f_vec)
{
  int32 res1_1,res1_3,res1_5,res1_7;

  int32 tmp2_0,tmp2_1,tmp2_2,tmp2_3;
  int32 res2_0,res2_1,res2_2,res2_3;

  int32 tmp3_0,tmp3_1;
  int32 res3_0,res3_1;
  
  tmp2_0 = vec[0]+vec[14];
  tmp2_1 = vec[2]+vec[12];
  tmp2_2 = vec[4]+vec[10];
  tmp2_3 = vec[6]+vec[8];

  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  f_vec[0]  = tmp3_0+tmp3_1;
  f_vec[32] = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
  tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);
    
  f_vec[16]  = res3_0+res3_1;
  f_vec[48] = res3_1;
        
  tmp2_0 = fixed32_mul(vec[0]-vec[7], cost32_c2_int[0]);
  tmp2_1 = fixed32_mul(vec[1]-vec[6], cost32_c2_int[1]);
  tmp2_2 = fixed32_mul(vec[2]-vec[5], cost32_c2_int[2]);
  tmp2_3 = fixed32_mul(vec[3]-vec[4], cost32_c2_int[3]);
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res2_0 = tmp3_0+tmp3_1;
  res2_2 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
  tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);
  res3_0 = tmp3_0+tmp3_1;
  res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  res2_1 = res3_0+res3_1;
  res2_3 = res3_1;

  res1_1 = res2_0+res2_1;
  res1_3 = res2_1+res2_2;
  res1_5 = res2_2+res2_3;
  res1_7 = res2_3;
    
  f_vec[24] = res1_1;
  f_vec[8]  = res1_3;
  f_vec[40] = res1_5;
  f_vec[56] = res1_7;
}

/*-------------------------------------------------------------------------*/

//-------------------------------------------------------------------------*
//
//                   C P o l y p h a s e
//
//-------------------------------------------------------------------------*

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*

CPolyphaseInt::CPolyphaseInt (const MPEG_INFO &_info,
                        int              _qual,
                        int              _resl,
                        int              _downMix) : 
  info (_info),
  qual (_qual),
  resl (_resl),
  downMix (_downMix)
{
  Init() ;
}

CPolyphaseInt::~CPolyphaseInt()
{
}

//-------------------------------------------------------------------------*
//   Init
//-------------------------------------------------------------------------*

void CPolyphaseInt::Init(void)
{
  int i,j;

//  for ( j=0; j<2; j++ )
//    for ( i=0; i<HAN_SIZE; i++ )
//      syn_buf[j][i] = 0.0f;
  memset(syn_buf, 0, sizeof(syn_buf));
  bufOffset = 64;
}

//-------------------------------------------------------------------------*
//   Apply (short)
//
// sample is ordered like so: [SSLIMIT][SBLIMIT][2]
//-------------------------------------------------------------------------*

short *CPolyphaseInt::Apply(INT_SPECTRUM &sample, short *pPcm, int frames)
{
	int nChannels    = (downMix ? 1:info.stereo);
	int nIncrement   = (16<<nChannels)>>(qual+resl);
	int fShortWindow = (downMix && (info.stereo==2)) ? 1 : 0;
	
	int j,k;
	
	for ( k=0; k<frames; k++ )
	{
		bufOffset = (bufOffset-64)&((HAN_SIZE-1)*2);
		
		if ( nChannels == 1 )
		{
			switch ( qual )
			{
			case 0:
				cost32(&sample[k*SBLIMIT*2], &(syn_buf[bufOffset]));
				break;
			
			case 1:
				cost16(&sample[k*SBLIMIT*2], &(syn_buf[bufOffset]));
				break;
			
			case 2:
				cost8(&sample[k*SBLIMIT*2], &(syn_buf[bufOffset]));
				break;
			}
		
			window_band_m(bufOffset, pPcm, fShortWindow);
		}
		else
		{
			switch ( qual )
			{
			case 0:
				cost32(&sample[k*SBLIMIT*2],   &(syn_buf[bufOffset]));
				cost32(&sample[k*SBLIMIT*2+1], &(syn_buf[bufOffset+1]));
				break;
			
			case 1:
				cost16(&sample[k*SBLIMIT*2],   &(syn_buf[bufOffset]));
				cost16(&sample[k*SBLIMIT*2+1], &(syn_buf[bufOffset+1]));
				break;
			
			case 2:
				cost8(&sample[k*SBLIMIT*2],   &(syn_buf[bufOffset]));
				cost8(&sample[k*SBLIMIT*2+1], &(syn_buf[bufOffset+1]));
				break;
			}
		
			window_band_s(bufOffset, pPcm, fShortWindow);
		}
	
		pPcm += nIncrement;
	}
	
	return pPcm;    
}

//
// functions for sample clipping
// 

static inline short CLIP16(int32 fSample)
{
  const int32 min = int32_to_fixed32(-fixed32_guard_domain);
  const int32 max = int32_to_fixed32( fixed32_guard_domain-1);
  if(fSample < min) fSample = min;
  else if(fSample > max) fSample = max;
  return short(fixed32_to_int32_round(fSample << fixed32_guard_bits));
}

static inline unsigned char CLIP8(int32 fSample)
{
  return (unsigned char) ( (CLIP16(fSample) >> 8) + 0x80 );
}


/*-------------------------------------------------------------------------*/

void CPolyphaseInt::window_band_m(int bufOffset,short *out_samples, int /* short_window */)
{
  const int32 *winPtr = syn_f_window_int;
  int32        sum1,sum2;
  int          i,j;

  /* sum 0 and sum 16, 8, 4 (full, half, quarter spectrum) */
  sum1 = sum2 = 0;

  for ( i=0; i<512; i+=64 )
    {
    sum1 += fixed32_mul(syn_buf[(bufOffset+i*2+32) & ((HAN_SIZE-1)*2)], winPtr[0]);
    sum2 += fixed32_mul(syn_buf[(bufOffset+i*2+64) & ((HAN_SIZE-1)*2)], winPtr[3]);
    sum1 += fixed32_mul(syn_buf[(bufOffset+i*2+96) & ((HAN_SIZE-1)*2)], winPtr[2]);
    winPtr += 4;
    }

  if ( 0 == resl )
    {
    // 16bit PCM
    out_samples[0]          = CLIP16(sum1);
    out_samples[16 >> qual] = CLIP16(sum2);
    }
  else
    {
    // 8bit PCM
    ((unsigned char*)out_samples)[0]          = CLIP8(sum1);
    ((unsigned char*)out_samples)[16 >> qual] = CLIP8(sum2);
    }

  /* sum 1-15, 1-7, 1-3 and 17-31, 9-15, 5-7 (full, half, quarter spectrum) */

  if ( 0 == resl )
    {
    // 16bit PCM
    for ( j=1; j<(16>>qual); j++ )
      {
      sum1 = sum2 = 0;
      
      winPtr += (1<<qual)*32 - 32;
      
      for ( i=0;i<512;i+=64 )
        {
        sum1 += fixed32_mul(syn_buf[(bufOffset+i*2+j*(2<<qual)+32) & ((HAN_SIZE-1)*2)], winPtr[0]);
        sum2 += fixed32_mul(syn_buf[(bufOffset+i*2+j*(2<<qual)+32) & ((HAN_SIZE-1)*2)], winPtr[1]);
        sum1 += fixed32_mul(syn_buf[(bufOffset+i*2+j*(2<<qual)+64) & ((HAN_SIZE-1)*2)], winPtr[2]);
        sum2 += fixed32_mul(syn_buf[(bufOffset+i*2+j*(2<<qual)+64) & ((HAN_SIZE-1)*2)], winPtr[3]);
        winPtr += 4;
        }
      
      out_samples[j]            = CLIP16(sum1);
      out_samples[(32>>qual)-j] = CLIP16(sum2);
      }
    }
  else
    {
    // 8 bit PCM
    for ( j=1; j<(16>>qual); j++ )
      {
      sum1 = sum2 = 0;
      
      winPtr += (1<<qual)*32 - 32;
      
      for ( i=0;i<512;i+=64 )
        {
        sum1 += fixed32_mul(syn_buf[(bufOffset+i*2+j*(2<<qual)+32) & ((HAN_SIZE-1)*2)], winPtr[0]);
        sum2 += fixed32_mul(syn_buf[(bufOffset+i*2+j*(2<<qual)+32) & ((HAN_SIZE-1)*2)], winPtr[1]);
        sum1 += fixed32_mul(syn_buf[(bufOffset+i*2+j*(2<<qual)+64) & ((HAN_SIZE-1)*2)], winPtr[2]);
        sum2 += fixed32_mul(syn_buf[(bufOffset+i*2+j*(2<<qual)+64) & ((HAN_SIZE-1)*2)], winPtr[3]);
        winPtr += 4;
        }
      
      ((unsigned char*)out_samples)[j]            = CLIP8(sum1);
      ((unsigned char*)out_samples)[(32>>qual)-j] = CLIP8(sum2);
      }
    }
}

extern "C" void CPolyphaseInt__window_band_s_asm16( int bufOffset, int32 *syn_buf, int32 qual, int16 *out_samples );
extern "C" void CPolyphaseInt__window_band_s_asm8( int bufOffset, int32 *syn_buf, int32 qual, uint8 *out_samples );

extern "C" uint32 junk[256];

void CPolyphaseInt::window_band_s(int bufOffset,short *out_samples, int /* short_window */)
{
	const int32 *winPtr = syn_f_window_int;
	int32        sum1l,sum2l,sum1r,sum2r;
	int          i,j,bufPtr;

	CPolyphaseInt__window_band_s_asm16( bufOffset, &syn_buf[0], qual, out_samples );
	return;
	
	/* sum 0 and sum 16, 8, 4 (full, half, quarter spectrum) */
	sum1l = sum2l = sum1r = sum2r = 0;
	
	bufPtr = bufOffset;

	for ( i=0; i<512; i+=64 )
	{
		int32 t1, t2;
		sum1l += t1 = fixed32_mul(syn_buf[bufPtr+32], winPtr[0]);
		sum1r += t2 = fixed32_mul(syn_buf[bufPtr+33], winPtr[0]);
		
		bufPtr = (bufPtr+64)&((HAN_SIZE-1)*2);
		
		sum1l += t1 = fixed32_mul(syn_buf[bufPtr+32], winPtr[2]);
		sum1r += t2 = fixed32_mul(syn_buf[bufPtr+33], winPtr[2]);
		sum2l += fixed32_mul(syn_buf[bufPtr   ], winPtr[3]);
		sum2r += fixed32_mul(syn_buf[bufPtr+1 ], winPtr[3]);
		
		bufPtr = (bufPtr+64)&((HAN_SIZE-1)*2);
		
		winPtr+=4;
	}

	if ( 0 == resl )
	{
		// 16bit PCM
		out_samples[0]            = CLIP16(sum1l);
		out_samples[32>>qual]     = CLIP16(sum2l);
		out_samples[1]            = CLIP16(sum1r);
		out_samples[(32>>qual)+1] = CLIP16(sum2r);

		/* sum 1-15, 1-7, 1-3 and 17-31, 9-15, 5-7 (full, half, quarter spectrum) */
		// 16 bit PCM
		for ( j=1; j<(16>>qual); j++ )
		{
			sum1l = sum2l = sum1r = sum2r = 0;
			
			bufPtr  = bufOffset+j*(2<<qual);
			winPtr += (1<<qual)*32 - 32;

			for ( i=0; i<512; i+=64 )
			{
				sum1l += fixed32_mul(syn_buf[bufPtr+32], winPtr[0]);
				sum1r += fixed32_mul(syn_buf[bufPtr+33], winPtr[0]);
				sum2l += fixed32_mul(syn_buf[bufPtr+32], winPtr[1]);
				sum2r += fixed32_mul(syn_buf[bufPtr+33], winPtr[1]);
				
				bufPtr = (bufPtr+64)&((HAN_SIZE-1)*2);
				
				sum1l += fixed32_mul(syn_buf[bufPtr   ], winPtr[2]);
				sum1r += fixed32_mul(syn_buf[bufPtr+1 ], winPtr[2]);
				sum2l += fixed32_mul(syn_buf[bufPtr   ], winPtr[3]);
				sum2r += fixed32_mul(syn_buf[bufPtr+1 ], winPtr[3]);
				
				bufPtr = (bufPtr+64)&((HAN_SIZE-1)*2);
				
				winPtr += 4;
			}
			
			out_samples[j*2]                = CLIP16(sum1l);
			out_samples[((32>>qual)-j)*2]   = CLIP16(sum2l);
			out_samples[j*2+1]              = CLIP16(sum1r);
			out_samples[((32>>qual)-j)*2+1] = CLIP16(sum2r);
		}
	}
	else
	{
		// 8bit PCM
		((unsigned char*)out_samples)[0]            = CLIP8(sum1l);
		((unsigned char*)out_samples)[32>>qual]     = CLIP8(sum2l);
		((unsigned char*)out_samples)[1]            = CLIP8(sum1r);
		((unsigned char*)out_samples)[(32>>qual)+1] = CLIP8(sum2r);
		
		// 8bit PCM
		for ( j=1; j<(16>>qual); j++ )
		{
			sum1l = sum2l = sum1r = sum2r = 0;
			
			bufPtr  = bufOffset+j*(2<<qual);
			winPtr += (1<<qual)*32 - 32;
			
			for ( i=0; i<512; i+=64 )
			{
				sum1l += fixed32_mul(syn_buf[bufPtr+32], winPtr[0]);
				sum1r += fixed32_mul(syn_buf[bufPtr+33], winPtr[0]);
				sum2l += fixed32_mul(syn_buf[bufPtr+32], winPtr[1]);
				sum2r += fixed32_mul(syn_buf[bufPtr+33], winPtr[1]);
				
				bufPtr = (bufPtr+64)&((HAN_SIZE-1)*2);
				
				sum1l += fixed32_mul(syn_buf[bufPtr   ], winPtr[2]);
				sum1r += fixed32_mul(syn_buf[bufPtr+1 ], winPtr[2]);
				sum2l += fixed32_mul(syn_buf[bufPtr   ], winPtr[3]);
				sum2r += fixed32_mul(syn_buf[bufPtr+1 ], winPtr[3]);
				
				bufPtr = (bufPtr+64)&((HAN_SIZE-1)*2);
				
				winPtr += 4;
			}
			
			((unsigned char*)out_samples)[j*2]                = CLIP8(sum1l);
			((unsigned char*)out_samples)[((32>>qual)-j)*2]   = CLIP8(sum2l);
			((unsigned char*)out_samples)[j*2+1]              = CLIP8(sum1r);
			((unsigned char*)out_samples)[((32>>qual)-j)*2+1] = CLIP8(sum2r);
		}
	}
}

/*-------------------------------------------------------------------------*/

void cost32(const int32 *vec,int32 *f_vec)
{
  int32 tmp0_0,tmp0_1,tmp0_2,tmp0_3,tmp0_4,tmp0_5,tmp0_6,tmp0_7;
  int32 tmp0_8,tmp0_9,tmp0_10,tmp0_11,tmp0_12,tmp0_13,tmp0_14,tmp0_15;
  int32 res0_0,res0_1,res0_2,res0_3,res0_4,res0_5,res0_6,res0_7;
  int32 res0_8,res0_9,res0_10,res0_11,res0_12,res0_13,res0_14,res0_15;

  int32 tmp1_0,tmp1_1,tmp1_2,tmp1_3,tmp1_4,tmp1_5,tmp1_6,tmp1_7;
  int32 res1_0,res1_1,res1_2,res1_3,res1_4,res1_5,res1_6,res1_7;

  int32 tmp2_0,tmp2_1,tmp2_2,tmp2_3;
  int32 res2_0,res2_1,res2_2,res2_3;

  int32 tmp3_0,tmp3_1;
  int32 res3_0,res3_1;

  tmp0_0 =  vec[0] +vec[62];
  tmp0_1 =  vec[2] +vec[60];
  tmp0_2 =  vec[4] +vec[58];
  tmp0_3 =  vec[6] +vec[56];
  tmp0_4 =  vec[8] +vec[54];
  tmp0_5 =  vec[10]+vec[52];
  tmp0_6 =  vec[12]+vec[50];
  tmp0_7 =  vec[14]+vec[48];
  tmp0_8 =  vec[16]+vec[46];
  tmp0_9 =  vec[18]+vec[44];
  tmp0_10 = vec[20]+vec[42];
  tmp0_11 = vec[22]+vec[40];
  tmp0_12 = vec[24]+vec[38];
  tmp0_13 = vec[26]+vec[36];
  tmp0_14 = vec[28]+vec[34];
  tmp0_15 = vec[30]+vec[32];

  tmp1_0 = tmp0_0+tmp0_15;
  tmp1_1 = tmp0_1+tmp0_14;
  tmp1_2 = tmp0_2+tmp0_13;
  tmp1_3 = tmp0_3+tmp0_12;
  tmp1_4 = tmp0_4+tmp0_11;
  tmp1_5 = tmp0_5+tmp0_10;
  tmp1_6 = tmp0_6+tmp0_9;
  tmp1_7 = tmp0_7+tmp0_8;

  tmp2_0 = tmp1_0+tmp1_7;
  tmp2_1 = tmp1_1+tmp1_6;
  tmp2_2 = tmp1_2+tmp1_5;
  tmp2_3 = tmp1_3+tmp1_4;

  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  f_vec[0]  = tmp3_0+tmp3_1;
  f_vec[32] = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
  tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  f_vec[16] = res3_0+res3_1;
  f_vec[48] = res3_1;

  tmp2_0 = fixed32_mul(tmp1_0-tmp1_7, cost32_c2_int[0]);
  tmp2_1 = fixed32_mul(tmp1_1-tmp1_6, cost32_c2_int[1]);
  tmp2_2 = fixed32_mul(tmp1_2-tmp1_5, cost32_c2_int[2]);
  tmp2_3 = fixed32_mul(tmp1_3-tmp1_4, cost32_c2_int[3]);
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res2_0 = tmp3_0+tmp3_1;
  res2_2 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
  tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);
  res3_0 = tmp3_0+tmp3_1;
  res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  res2_1 = res3_0+res3_1;

  f_vec[24] = res2_0+res2_1;
  f_vec[8]  = res2_1+res2_2;
  f_vec[40] = res2_2+res3_1;
  f_vec[56] = res3_1;

  tmp1_0 = fixed32_mul(tmp0_0-tmp0_15, cost32_c1_int[0]);
  tmp1_1 = fixed32_mul(tmp0_1-tmp0_14, cost32_c1_int[1]);
  tmp1_2 = fixed32_mul(tmp0_2-tmp0_13, cost32_c1_int[2]);
  tmp1_3 = fixed32_mul(tmp0_3-tmp0_12, cost32_c1_int[3]);
  tmp1_4 = fixed32_mul(tmp0_4-tmp0_11, cost32_c1_int[4]);
  tmp1_5 = fixed32_mul(tmp0_5-tmp0_10, cost32_c1_int[5]);
  tmp1_6 = fixed32_mul(tmp0_6-tmp0_9, cost32_c1_int[6]);
  tmp1_7 = fixed32_mul(tmp0_7-tmp0_8, cost32_c1_int[7]);
  tmp2_0 = tmp1_0+tmp1_7;
  tmp2_1 = tmp1_1+tmp1_6;
  tmp2_2 = tmp1_2+tmp1_5;
  tmp2_3 = tmp1_3+tmp1_4;
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res1_0 = tmp3_0+tmp3_1;
  res1_4 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
  tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  res1_2 = res3_0+res3_1;
  res1_6 = res3_1;

  tmp2_0 = fixed32_mul(tmp1_0-tmp1_7, cost32_c2_int[0]);
  tmp2_1 = fixed32_mul(tmp1_1-tmp1_6, cost32_c2_int[1]);
  tmp2_2 = fixed32_mul(tmp1_2-tmp1_5, cost32_c2_int[2]);
  tmp2_3 = fixed32_mul(tmp1_3-tmp1_4, cost32_c2_int[3]);
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res2_0 = tmp3_0+tmp3_1;
  res2_2 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
  tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

  res2_1 = res3_0+res3_1;
  res2_3 = res3_1;
  
  res1_1 = res2_0+res2_1;
  res1_3 = res2_1+res2_2;
  res1_5 = res2_2+res2_3;
  res1_7 = res2_3;

  f_vec[28] = res1_0+res1_1;
  f_vec[20] = res1_1+res1_2;
  f_vec[12] = res1_2+res1_3;
  f_vec[4] = res1_3+res1_4;
  f_vec[36] = res1_4+res1_5;
  f_vec[44] = res1_5+res1_6;
  f_vec[52] = res1_6+res1_7;
  f_vec[60] = res1_7;

  /*  Odd Terms */
  tmp0_0 =  fixed32_mul(vec[0]-vec[62], cost32_c0_int[0]);
  tmp0_1 =  fixed32_mul(vec[2]-vec[60], cost32_c0_int[1]);
  tmp0_2 =  fixed32_mul(vec[4]-vec[58], cost32_c0_int[2]);
  tmp0_3 =  fixed32_mul(vec[6]-vec[56], cost32_c0_int[3]);
  tmp0_4 =  fixed32_mul(vec[8]-vec[54], cost32_c0_int[4]);
  tmp0_5 =  fixed32_mul(vec[10]-vec[52], cost32_c0_int[5]);
  tmp0_6 =  fixed32_mul(vec[12]-vec[50], cost32_c0_int[6]);
  tmp0_7 =  fixed32_mul(vec[14]-vec[48], cost32_c0_int[7]);
  tmp0_8 =  fixed32_mul(vec[16]-vec[46], cost32_c0_int[8]);
  tmp0_9 =  fixed32_mul(vec[18]-vec[44], cost32_c0_int[9]);
  tmp0_10 = fixed32_mul(vec[20]-vec[42], cost32_c0_int[10]);
  tmp0_11 = fixed32_mul(vec[22]-vec[40], cost32_c0_int[11]);
  tmp0_12 = fixed32_mul(vec[24]-vec[38], cost32_c0_int[12]);
  tmp0_13 = fixed32_mul(vec[26]-vec[36], cost32_c0_int[13]);
  tmp0_14 = fixed32_mul(vec[28]-vec[34], cost32_c0_int[14]);
  tmp0_15 = fixed32_mul(vec[30]-vec[32], cost32_c0_int[15]);

  tmp1_0 = tmp0_0+tmp0_15;
  tmp1_1 = tmp0_1+tmp0_14;
  tmp1_2 = tmp0_2+tmp0_13;
  tmp1_3 = tmp0_3+tmp0_12;
  tmp1_4 = tmp0_4+tmp0_11;
  tmp1_5 = tmp0_5+tmp0_10;
  tmp1_6 = tmp0_6+tmp0_9;
  tmp1_7 = tmp0_7+tmp0_8;
  
  tmp2_0 = tmp1_0+tmp1_7;
  tmp2_1 = tmp1_1+tmp1_6;
  tmp2_2 = tmp1_2+tmp1_5;
  tmp2_3 = tmp1_3+tmp1_4;
  
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;


	res0_0 = tmp3_0+tmp3_1;
	res0_8 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);
//printf( "r0_0 %f   r0_8 %f \n", res0_0, res0_8 );	
	   
	tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
	tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);
	res3_0 = tmp3_0+tmp3_1;
	res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);
	res0_4 = res3_0+res3_1;
	res0_12 = res3_1;
//printf( "r0_4 %f   r0_12 %f \n", res0_4, res0_12 );	
	
	
	tmp2_0 = fixed32_mul(tmp1_0-tmp1_7, cost32_c2_int[0]);
	tmp2_1 = fixed32_mul(tmp1_1-tmp1_6, cost32_c2_int[1]);
	tmp2_2 = fixed32_mul(tmp1_2-tmp1_5, cost32_c2_int[2]);
	tmp2_3 = fixed32_mul(tmp1_3-tmp1_4, cost32_c2_int[3]);
	tmp3_0 = tmp2_0+tmp2_3;
	tmp3_1 = tmp2_1+tmp2_2;
	res2_0 = tmp3_0+tmp3_1;
	res2_2 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);
	
	tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
	tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);
	res3_0 = tmp3_0+tmp3_1;
	res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);
	res2_1 = res3_0+res3_1;
	res2_3 = res3_1;
	res0_2 = res2_0+res2_1;
	res0_6 = res2_1+res2_2;
	res0_10 = res2_2+res2_3;
	res0_14 = res2_3;
//printf( "r0_2  %f   r0_6  %f \n", res0_2, res0_6 );	
//printf( "r0_10 %f   r0_14 %f \n", res0_10, res0_14 );	
	
	tmp1_0 = fixed32_mul(tmp0_0-tmp0_15, cost32_c1_int[0]);
	tmp1_1 = fixed32_mul(tmp0_1-tmp0_14, cost32_c1_int[1]);
	tmp1_2 = fixed32_mul(tmp0_2-tmp0_13, cost32_c1_int[2]);
	tmp1_3 = fixed32_mul(tmp0_3-tmp0_12, cost32_c1_int[3]);
	tmp1_4 = fixed32_mul(tmp0_4-tmp0_11, cost32_c1_int[4]);
	tmp1_5 = fixed32_mul(tmp0_5-tmp0_10, cost32_c1_int[5]);
	tmp1_6 = fixed32_mul(tmp0_6-tmp0_9, cost32_c1_int[6]);
	tmp1_7 = fixed32_mul(tmp0_7-tmp0_8, cost32_c1_int[7]);
	tmp2_0 = tmp1_0+tmp1_7;
	tmp2_1 = tmp1_1+tmp1_6;
	tmp2_2 = tmp1_2+tmp1_5;
	tmp2_3 = tmp1_3+tmp1_4;
	tmp3_0 = tmp2_0+tmp2_3;
	tmp3_1 = tmp2_1+tmp2_2;

	res1_0 = tmp3_0+tmp3_1;
	res1_4 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);
//printf( "r1_0  %f     r1_4  %f \n", res1_0, res1_4 );	
	
	tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
	tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);
	res3_0 = tmp3_0+tmp3_1;
	res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);
	res1_2 = res3_0+res3_1;
	res1_6 = res3_1;
//printf( "r1_2  %f     r1_6  %f \n", res1_2, res1_6 );	
	
	tmp2_0 = fixed32_mul(tmp1_0-tmp1_7, cost32_c2_int[0]);
	tmp2_1 = fixed32_mul(tmp1_1-tmp1_6, cost32_c2_int[1]);
	tmp2_2 = fixed32_mul(tmp1_2-tmp1_5, cost32_c2_int[2]);
	tmp2_3 = fixed32_mul(tmp1_3-tmp1_4, cost32_c2_int[3]);
	tmp3_0 = tmp2_0+tmp2_3;
	tmp3_1 = tmp2_1+tmp2_2;
	res2_0 = tmp3_0+tmp3_1;
	res2_2 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);

#if 0
printf( "\nt0_0  %f   t0_1  %f \n", tmp0_0, tmp0_1 );
printf( "t0_2  %f   t0_3  %f \n", tmp0_2, tmp0_3 );
printf( "t0_4  %f   t0_5  %f \n", tmp0_4, tmp0_5 );
printf( "t0_6  %f   t0_7  %f \n", tmp0_6, tmp0_7 );
printf( "t0_8  %f   t0_9  %f \n", tmp0_8, tmp0_9 );
printf( "t0_10 %f   t0_11  %f \n", tmp0_10, tmp0_11 );
printf( "t0_12 %f   t0_13  %f \n", tmp0_12, tmp0_13 );
printf( "t0_14 %f   t0_15  %f \n", tmp0_14, tmp0_15 );

printf( "t1_0 %f   t1_1 %f \n", tmp1_0, tmp1_1 );
printf( "t1_2 %f   t1_3 %f \n", tmp1_2, tmp1_3 );
printf( "t1_4 %f   t1_5 %f \n", tmp1_4, tmp1_5 );
printf( "t1_6 %f   t1_7 %f \n", tmp1_6, tmp1_7 );

printf( "t2_0 %f   t2_1 %f \n", tmp2_0, tmp2_1 );
printf( "t2_2 %f   t2_3 %f \n", tmp2_2, tmp2_3 );

printf( "t3_0 %f   t3_1 %f \n", tmp3_0, tmp3_1 );
#endif
//printf( "r2_0  %f     r2_2  %f \n", res2_0, res2_2 );	
	
	tmp3_0 = fixed32_mul(tmp2_0-tmp2_3, cost32_c3_int[0]);
	tmp3_1 = fixed32_mul(tmp2_1-tmp2_2, cost32_c3_int[1]);
	res3_0 = tmp3_0+tmp3_1;
	res3_1 = fixed32_mul(tmp3_0-tmp3_1, cost32_c4[0]);
	
	res2_1 = res3_0+res3_1;
	res2_3 = res3_1;
	res1_1 = res2_0+res2_1;
	res1_3 = res2_1+res2_2;
	res1_5 = res2_2+res2_3;
	res1_7 = res2_3;
	
	res0_1 = res1_0+res1_1;
	res0_3 = res1_1+res1_2;
	res0_5 = res1_2+res1_3;
	res0_7 = res1_3+res1_4;
	res0_9 = res1_4+res1_5;
	res0_11 = res1_5+res1_6;
	res0_13 = res1_6+res1_7;
	res0_15 = res1_7;
	
	f_vec[30] = res0_0+res0_1;
	f_vec[26] = res0_1+res0_2;
	f_vec[22] = res0_2+res0_3;
	f_vec[18] = res0_3+res0_4;
	f_vec[14] = res0_4+res0_5;
	f_vec[10] = res0_5+res0_6;
	f_vec[6]  = res0_6+res0_7;
	f_vec[2]  = res0_7+res0_8;
	f_vec[34] = res0_8+res0_9;
	f_vec[38] = res0_9+res0_10;
	f_vec[42] = res0_10+res0_11;
	f_vec[46] = res0_11+res0_12;
	f_vec[50] = res0_12+res0_13;
	f_vec[54] = res0_13+res0_14;
	f_vec[58] = res0_14+res0_15;
	f_vec[62] = res0_15;
	
#if 0  //debugging helper code
	{
	  	float tmp[32];
	  	int ct;
	  	for( ct=0; ct<32; ct++ )
	  		tmp[ct]=0;

#if 0
	printf( "in  00 %7.3f    01 %7.3f    02 %7.3f    03 %7.3f \n", vec[0], vec[1], vec[2], vec[3] );
	printf( "in  04 %7.3f    05 %7.3f    06 %7.3f    07 %7.3f \n", vec[4], vec[5], vec[6], vec[7] );
	printf( "in  08 %7.3f    09 %7.3f    10 %7.3f    11 %7.3f \n", vec[8], vec[9], vec[10], vec[11] );
	printf( "in  12 %7.3f    13 %7.3f    14 %7.3f    15 %7.3f \n", vec[12], vec[13], vec[14], vec[15] );
	printf( "in  16 %7.3f    17 %7.3f    18 %7.3f    19 %7.3f \n", vec[16], vec[17], vec[18], vec[19] );
	printf( "in  20 %7.3f    21 %7.3f    22 %7.3f    23 %7.3f \n", vec[20], vec[21], vec[22], vec[23] );
	printf( "in  24 %7.3f    25 %7.3f    26 %7.3f    27 %7.3f \n", vec[24], vec[25], vec[26], vec[27] );
	printf( "in  28 %7.3f    29 %7.3f    30 %7.3f    31 %7.3f \n", vec[28], vec[29], vec[30], vec[31] );
#endif

	  	CPolyphaseASM___cost32( vec, tmp );
	  	for( ct=0; ct<32; ct++ )
	  	{
	  		if( fabs( (f_vec[ct] / tmp[ct]) - 1.0 ) > .01 )
		  		printf( "ERROR index %2i c %7.3f   asm %7.3f \n", ct, f_vec[ct], tmp[ct] );
	  	}
  	
  	}
#endif
}
