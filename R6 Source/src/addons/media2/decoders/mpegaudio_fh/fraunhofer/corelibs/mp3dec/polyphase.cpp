/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: polyphase.cpp
 *   project : ISO/MPEG-Decoder
 *   author  : Stefan Gewinner
 *   date    : 1998-05-26
 *   contents/description: polyphase class
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/06/24 10:31:15 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/polyphase.cpp,v 1.8 1999/06/24 10:31:15 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "polyphase.h"

#include <stdio.h>
#include <math.h>

/*-------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------
 *
 * function to convert a Polyphase Window according to ISO/MPEG into a
 * polyphase window used in _this_ decoder
 *
 *
 *  int    i, j, index = 0;
 *  double tmp;
 *  float  window[512];    // ISO/MPEG Window
 *  float  my_window[512]; // window used in _this_ decoder
 *
 *  for ( i=0; i<512; i+=32 )
 *    for ( j=1; j<8; j++ )
 *      {
 *      tmp            = window[i+j+16];
 *      window[i+j+16] = window[i+32-j];
 *      window[i+32-j] = tmp;
 *      }
 *
 *  for ( j=0; j<16; j++ )
 *     for ( i=0; i<512; i+=64 )
 *       {
 *       window[i+j+16] *= -1.0;
 *       window[i+j+32] *= -1.0;
 *       window[i+j+48] *= -1.0;
 *       }
 *
 *  for ( j=0; j<16; j++ )
 *    for ( i=0; i<512; i+=64 )
 *      {
 *      my_window[index]    = window[i+j];
 *      my_window[index+1]  = window[i+j+16];
 *      my_window[index+2]  = window[i+j+32];
 *      my_window[index+3]  = window[i+j+48];
 *      index += 4;
 *      }
 * ------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------*/

#if _USE_X86_ASM

extern "C" {
	void CPolyphaseASM___window_band_m(
		long buf_offset,
		long short_window,
		float* syn_buf,
		long qual,
		long resl,
		short* out_samples);
}
extern "C" {
	void CPolyphaseASM___window_band_s(
		long buf_offset,
		long short_window,
		float* syn_buf,
		long qual,
		long resl,
		short* out_samples);
};

#if _USE_3DNOW
extern "C" void CPolyphaseASM___cost32(const float *vec,float *f_vec);
#endif
#endif 

static const float syn_f_window[HAN_SIZE] =
{
  0.00000000f, 0.00007629f, 0.00044250f, 0.00158691f,
  0.00325012f,-0.00222778f, 0.00700378f, 0.02391052f,
  0.03108215f, 0.00068665f, 0.07862854f, 0.14842224f,
  0.10031128f, 0.15220642f, 0.57203674f, 0.97685242f,
  1.14498901f,-0.97685242f,-0.57203674f,-0.15220642f,
  0.10031128f,-0.14842224f,-0.07862854f,-0.00068665f,
  0.03108215f,-0.02391052f,-0.00700378f, 0.00222778f,
  0.00325012f,-0.00158691f,-0.00044250f,-0.00007629f,

 -0.00001526f, 0.00039673f, 0.00047302f, 0.00317383f,
  0.00332642f, 0.00611877f, 0.00791931f, 0.03147888f,
  0.03051758f, 0.07305908f, 0.08418274f, 0.10885620f,
  0.09092712f, 0.54382324f, 0.60021973f, 1.14428711f,
  1.14428711f,-0.60021973f,-0.54382324f, 0.09092712f,
  0.10885620f,-0.08418274f,-0.07305908f, 0.03051758f,
  0.03147888f,-0.00791931f,-0.00611877f, 0.00332642f,
  0.00317383f,-0.00047302f,-0.00039673f,-0.00001526f,

 -0.00001526f, 0.00036621f, 0.00053406f, 0.00308227f,
  0.00338745f, 0.00529480f, 0.00886536f, 0.03173828f,
  0.02978516f, 0.06752014f, 0.08970642f, 0.11657715f,
  0.08068848f, 0.51560974f, 0.62829590f, 1.14221191f,
  1.14221191f,-0.62829590f,-0.51560974f, 0.08068848f,
  0.11657715f,-0.08970642f,-0.06752014f, 0.02978516f,
  0.03173828f,-0.00886536f,-0.00529480f, 0.00338745f,
  0.00308227f,-0.00053406f,-0.00036621f,-0.00001526f,

 -0.00001526f, 0.00032044f, 0.00057983f, 0.00299072f,
  0.00343323f, 0.00448608f, 0.00984192f, 0.03184509f,
  0.02888489f, 0.06199646f, 0.09516907f, 0.12347412f,
  0.06959534f, 0.48747253f, 0.65621948f, 1.13876343f,
  1.13876343f,-0.65621948f,-0.48747253f, 0.06959534f,
  0.12347412f,-0.09516907f,-0.06199646f, 0.02888489f,
  0.03184509f,-0.00984192f,-0.00448608f, 0.00343323f,
  0.00299072f,-0.00057983f,-0.00032044f,-0.00001526f,

 -0.00001526f, 0.00028992f, 0.00062561f, 0.00289917f,
  0.00346374f, 0.00372314f, 0.01084900f, 0.03181458f,
  0.02780151f, 0.05653381f, 0.10054016f, 0.12957764f,
  0.05761719f, 0.45947266f, 0.68391418f, 1.13392639f,
  1.13392639f,-0.68391418f,-0.45947266f, 0.05761719f,
  0.12957764f,-0.10054016f,-0.05653381f, 0.02780151f,
  0.03181458f,-0.01084900f,-0.00372314f, 0.00346374f,
  0.00289917f,-0.00062561f,-0.00028992f,-0.00001526f,

 -0.00001526f, 0.00025940f, 0.00068665f, 0.00279236f,
  0.00347900f, 0.00300598f, 0.01188660f, 0.03166199f,
  0.02653503f, 0.05113220f, 0.10581970f, 0.13488770f,
  0.04478455f, 0.43165588f, 0.71131897f, 1.12774658f,
  1.12774658f,-0.71131897f,-0.43165588f, 0.04478455f,
  0.13488770f,-0.10581970f,-0.05113220f, 0.02653503f,
  0.03166199f,-0.01188660f,-0.00300598f, 0.00347900f,
  0.00279236f,-0.00068665f,-0.00025940f,-0.00001526f,

 -0.00001526f, 0.00024414f, 0.00074768f, 0.00268555f,
  0.00347900f, 0.00233459f, 0.01293945f, 0.03138733f,
  0.02508545f, 0.04583740f, 0.11094666f, 0.13945007f,
  0.03108215f, 0.40408325f, 0.73837280f, 1.12022400f,
  1.12022400f,-0.73837280f,-0.40408325f, 0.03108215f,
  0.13945007f,-0.11094666f,-0.04583740f, 0.02508545f,
  0.03138733f,-0.01293945f,-0.00233459f, 0.00347900f,
  0.00268555f,-0.00074768f,-0.00024414f,-0.00001526f,

 -0.00003052f, 0.00021362f, 0.00080872f, 0.00257873f,
  0.00346374f, 0.00169373f, 0.01402283f, 0.03100586f,
  0.02342224f, 0.04063416f, 0.11592102f, 0.14326477f,
  0.01651001f, 0.37680054f, 0.76502991f, 1.11137390f,
  1.11137390f,-0.76502991f,-0.37680054f, 0.01651001f,
  0.14326477f,-0.11592102f,-0.04063416f, 0.02342224f,
  0.03100586f,-0.01402283f,-0.00169373f, 0.00346374f,
  0.00257873f,-0.00080872f,-0.00021362f,-0.00003052f,

 -0.00003052f, 0.00019836f, 0.00088501f, 0.00245667f,
  0.00341797f, 0.00109863f, 0.01512146f, 0.03053284f,
  0.02157593f, 0.03555298f, 0.12069702f, 0.14636230f,
  0.00106812f, 0.34986877f, 0.79121399f, 1.10121155f,
  1.10121155f,-0.79121399f,-0.34986877f, 0.00106812f,
  0.14636230f,-0.12069702f,-0.03555298f, 0.02157593f,
  0.03053284f,-0.01512146f,-0.00109863f, 0.00341797f,
  0.00245667f,-0.00088501f,-0.00019836f,-0.00003052f,

 -0.00003052f, 0.00016785f, 0.00096130f, 0.00234985f,
  0.00337219f, 0.00054932f, 0.01623535f, 0.02993774f,
  0.01953125f, 0.03060913f, 0.12525940f, 0.14877319f,
 -0.01522827f, 0.32331848f, 0.81686401f, 1.08978271f,
  1.08978271f,-0.81686401f,-0.32331848f,-0.01522827f,
  0.14877319f,-0.12525940f,-0.03060913f, 0.01953125f,
  0.02993774f,-0.01623535f,-0.00054932f, 0.00337219f,
  0.00234985f,-0.00096130f,-0.00016785f,-0.00003052f,

 -0.00003052f, 0.00015259f, 0.00103760f, 0.00224304f,
  0.00328064f, 0.00003052f, 0.01734924f, 0.02928162f,
  0.01725769f, 0.02581787f, 0.12956238f, 0.15049744f,
 -0.03237915f, 0.29721069f, 0.84194946f, 1.07711792f,
  1.07711792f,-0.84194946f,-0.29721069f,-0.03237915f,
  0.15049744f,-0.12956238f,-0.02581787f, 0.01725769f,
  0.02928162f,-0.01734924f,-0.00003052f, 0.00328064f,
  0.00224304f,-0.00103760f,-0.00015259f,-0.00003052f,

 -0.00004578f, 0.00013733f, 0.00111389f, 0.00212097f,
  0.00317383f,-0.00044250f, 0.01846313f, 0.02853394f,
  0.01480103f, 0.02117920f, 0.13359070f, 0.15159607f,
 -0.05035400f, 0.27159119f, 0.86636353f, 1.06321716f,
  1.06321716f,-0.86636353f,-0.27159119f,-0.05035400f,
  0.15159607f,-0.13359070f,-0.02117920f, 0.01480103f,
  0.02853394f,-0.01846313f, 0.00044250f, 0.00317383f,
  0.00212097f,-0.00111389f,-0.00013733f,-0.00004578f,

 -0.00004578f, 0.00012207f, 0.00120544f, 0.00201416f,
  0.00305176f,-0.00086975f, 0.01957703f, 0.02772522f,
  0.01211548f, 0.01670837f, 0.13729858f, 0.15206909f,
 -0.06916809f, 0.24650574f, 0.89009094f, 1.04815674f,
  1.04815674f,-0.89009094f,-0.24650574f,-0.06916809f,
  0.15206909f,-0.13729858f,-0.01670837f, 0.01211548f,
  0.02772522f,-0.01957703f, 0.00086975f, 0.00305176f,
  0.00201416f,-0.00120544f,-0.00012207f,-0.00004578f,

 -0.00006103f, 0.00010681f, 0.00129700f, 0.00190735f,
  0.00288391f,-0.00126648f, 0.02069092f, 0.02684021f,
  0.00923157f, 0.01242065f, 0.14067078f, 0.15196228f,
 -0.08877563f, 0.22198486f, 0.91305542f, 1.03193665f,
  1.03193665f,-0.91305542f,-0.22198486f,-0.08877563f,
  0.15196228f,-0.14067078f,-0.01242065f, 0.00923157f,
  0.02684021f,-0.02069092f, 0.00126648f, 0.00288391f,
  0.00190735f,-0.00129700f,-0.00010681f,-0.00006103f,

 -0.00006103f, 0.00010681f, 0.00138855f, 0.00178528f,
  0.00270081f,-0.00161743f, 0.02178955f, 0.02590942f,
  0.00613403f, 0.00831604f, 0.14367676f, 0.15130615f,
 -0.10916138f, 0.19805908f, 0.93519592f, 1.01461792f,
  1.01461792f,-0.93519592f,-0.19805908f,-0.10916138f,
  0.15130615f,-0.14367676f,-0.00831604f, 0.00613403f,
  0.02590942f,-0.02178955f, 0.00161743f, 0.00270081f,
  0.00178528f,-0.00138855f,-0.00010681f,-0.00006103f,

 -0.00007629f, 0.00009155f, 0.00148010f, 0.00169373f,
  0.00248718f,-0.00193787f, 0.02285767f, 0.02493286f,
  0.00282288f, 0.00439453f, 0.14625549f, 0.15011597f,
 -0.13031006f, 0.17478943f, 0.95648193f, 0.99624634f,
  0.99624634f,-0.95648193f,-0.17478943f,-0.13031006f,
  0.15011597f,-0.14625549f,-0.00439453f, 0.00282288f,
  0.02493286f,-0.02285767f, 0.00193787f, 0.00248718f,
  0.00169373f,-0.00148010f,-0.00009155f,-0.00007629f
};

/*-------------------------------------------------------------------------*/

static const float syn_f_window_short[HAN_SIZE-128] =
{
 -0.0000000000f, -0.0022294761f,  0.0070019178f,  0.0239159092f,
  0.0310762096f,  0.0006843590f,  0.0786295608f,  0.1484193951f,
  0.1003170982f,  0.1522008926f,  0.5720440745f,  0.9768452048f,
  1.1449840069f, -0.9768452048f, -0.5720440745f, -0.1522008926f,
  0.1003170982f, -0.1484193951f, -0.0786295608f, -0.0006843590f,
  0.0310762096f, -0.0239159092f, -0.0070019178f,  0.0022294761f,

  0.0033227950f,  0.0061257849f,  0.0079142749f,  0.0314808004f,
  0.0305130407f,  0.0730657727f,  0.0841832235f,  0.1088624969f,
  0.0909292325f,  0.5438203216f,  0.6002249122f,  1.1442910433f,
  1.1442910433f, -0.6002249122f, -0.5438203216f,  0.0909292325f,
  0.1088624969f, -0.0841832235f, -0.0730657727f,  0.0305130407f,
  0.0314808004f, -0.0079142749f, -0.0061257849f,  0.0033227950f,

  0.0033810670f,  0.0052873888f,  0.0088609839f,  0.0317342617f,
  0.0297841094f,  0.0675140470f,  0.0897035599f,  0.1165779009f,
  0.0806888789f,  0.5156124234f,  0.6283028126f,  1.1422129869f,
  1.1422129869f, -0.6283028126f, -0.5156124234f,  0.0806888789f,
  0.1165779009f, -0.0897035599f, -0.0675140470f,  0.0297841094f,
  0.0317342617f, -0.0088609839f, -0.0052873888f,  0.0033810670f,

  0.0034272340f,  0.0044880132f,  0.0098399213f,  0.0318443291f,
  0.0288826302f,  0.0619953983f,  0.0951662809f,  0.1234773025f, 
  0.0695880502f,  0.4874784052f,  0.6562176943f,  1.1387569904f,
  1.1387569904f, -0.6562176943f, -0.4874784052f,  0.0695880502f,
  0.1234773025f, -0.0951662809f, -0.0619953983f,  0.0288826302f,
  0.0318443291f, -0.0098399213f, -0.0044880132f,  0.0034272340f,

  0.0034598880f,  0.0037286030f,  0.0108485902f,  0.0318188891f,
  0.0278021507f,  0.0565297604f,  0.1005462036f,  0.1295771003f,
  0.0576211400f,  0.4594751894f,  0.6839085817f,  1.1339299679f,
  1.1339299679f, -0.6839085817f, -0.4594751894f,  0.0576211400f,
  0.1295771003f, -0.1005462036f, -0.0565297604f,  0.0278021507f,
  0.0318188891f, -0.0108485902f, -0.0037286030f,  0.0034598880f,

  0.0034775150f,  0.0030099021f,  0.0118842097f,  0.0316660590f,
  0.0265367609f,  0.0511358418f,  0.1058171019f,  0.1348952055f,
  0.0447848216f,  0.4316585064f,  0.7113146782f,  1.1277459860f,
  1.1277459860f, -0.7113146782f, -0.4316585064f,  0.0447848216f,
  0.1348952055f, -0.1058171019f, -0.0511358418f,  0.0265367609f,
  0.0316660590f, -0.0118842097f, -0.0030099021f,  0.0034775150f,

  0.0034786200f,  0.0023323391f,  0.0129436301f,  0.0313939899f,
  0.0250809509f,  0.0458312109f,  0.1109521016f,  0.1394512951f,
  0.0310782604f,  0.4040825069f,  0.7383748293f,  1.1202180386f,
  1.1202180386f, -0.7383748293f, -0.4040825069f,  0.0310782604f,
  0.1394512951f, -0.1109521016f, -0.0458312109f,  0.0250809509f,
  0.0313939899f, -0.0129436301f, -0.0023323391f,  0.0034786200f,

  0.0034616119f,  0.0016961680f,  0.0140234102f,  0.0310109705f,
  0.0234298706f,  0.0406321697f,  0.1159232035f,  0.1432666034f,
  0.0165030695f,  0.3767997921f,  0.7650279999f,  1.1113669872f,
  1.1113669872f, -0.7650279999f, -0.3767997921f,  0.0165030695f,
  0.1432666034f, -0.1159232035f, -0.0406321697f,  0.0234298706f,
  0.0310109705f, -0.0140234102f, -0.0016961680f,  0.0034616119f,

  0.0034249341f,  0.0011013590f,  0.0151197100f,  0.0305252392f,
  0.0215791892f,  0.0355538614f,  0.1207021028f,  0.1463640034f,
  0.0010634609f,  0.3498612940f,  0.7912135720f,  1.1012150049f,
  1.1012150049f, -0.7912135720f, -0.3498612940f,  0.0010634609f,
  0.1463640034f, -0.1207021028f, -0.0355538614f,  0.0215791892f,
  0.0305252392f, -0.0151197100f, -0.0011013590f,  0.0034249341f,

  0.0033669460f,  0.0005477320f,  0.0162284095f,  0.0299451109f,
  0.0195253193f,  0.0306101404f,  0.1252595931f,  0.1487675011f,
 -0.0152338203f,  0.3233161867f,  0.8168715835f,  1.0897879601f,
  1.0897879601f, -0.8168715835f, -0.3233161867f, -0.0152338203f,
  0.1487675011f, -0.1252595931f, -0.0306101404f,  0.0195253193f,
  0.0299451109f, -0.0162284095f, -0.0005477320f,  0.0033669460f,

  0.0032860560f,  0.0000348550f,  0.0173449796f,  0.0292787701f,
  0.0172653105f,  0.0258137006f,  0.1295658946f,  0.1505026072f,
 -0.0323793218f,  0.2972115874f,  0.8419424891f,  1.0771130323f,
  1.0771130323f, -0.8419424891f, -0.2972115874f, -0.0323793218f,
  0.1505026072f, -0.1295658946f, -0.0258137006f,  0.0172653105f,
  0.0292787701f, -0.0173449796f, -0.0000348550f,  0.0032860560f,

  0.0031806100f, -0.0004378230f,  0.0184646100f,  0.0285343695f,
  0.0147970403f,  0.0211759601f,  0.1335908026f,  0.1515956074f,
 -0.0503610000f,  0.2715924978f,  0.8663678169f,  1.0632220507f,
  1.0632220507f, -0.8663678169f, -0.2715924978f, -0.0503610000f,
  0.1515956074f, -0.1335908026f, -0.0211759601f,  0.0147970403f,
  0.0285343695f, -0.0184646100f,  0.0004378230f,  0.0031806100f,

  0.0030490190f, -0.0008710770f,  0.0195821002f,  0.0277198907f,
  0.0121191498f,  0.0167071596f,  0.1373039037f,  0.1520740986f,
 -0.0691640675f,  0.2465019971f,  0.8900899291f,  1.0481510162f,
  1.0481510162f, -0.8900899291f, -0.2465019971f, -0.0691640675f,
  0.1520740986f, -0.1373039037f, -0.0167071596f,  0.0121191498f,
  0.0277198907f, -0.0195821002f,  0.0008710770f,  0.0030490190f,

  0.0028896530f, -0.0012657720f,  0.0206919704f,  0.0268432405f,
  0.0092311660f,  0.0124163199f,  0.1406743973f,  0.1519663036f,
 -0.0887710974f,  0.2219804972f,  0.9130526781f,  1.0319360495f,
  1.0319360495f, -0.9130526781f, -0.2219804972f, -0.0887710974f,
  0.1519663036f, -0.1406743973f, -0.0124163199f,  0.0092311660f,
  0.0268432405f, -0.0206919704f,  0.0012657720f,  0.0028896530f,

  0.0027009640f, -0.0016229640f,  0.0217883606f,  0.0259120706f,
  0.0061334972f,  0.0083113024f,  0.1436710954f,  0.1513013989f,
 -0.1091618985f,  0.1980662942f,  0.9352014065f,  1.0146180391f,
  1.0146180391f, -0.9352014065f, -0.1980662942f, -0.1091618985f,
  0.1513013989f, -0.1436710954f, -0.0083113024f,  0.0061334972f,
  0.0259120706f, -0.0217883606f,  0.0016229640f,  0.0027009640f,

  0.0024813900f, -0.0019437710f,  0.0228651706f,  0.0249338895f,
  0.0028275431f,  0.0043987860f,  0.1462630928f,  0.1501089931f,
 -0.1303136945f,  0.1747952998f,  0.9564827085f,  0.9962394834f,
  0.9962394834f, -0.9564827085f, -0.1747952998f, -0.1303136945f,
  0.1501089931f, -0.1462630928f, -0.0043987860f,  0.0028275431f,
  0.0249338895f, -0.0228651706f,  0.0019437710f,  0.0024813900f
};

/*-------------------------------------------------------------------------*/

static const float cost32_c0[] = 
  { 
  0.50060299823520f,  0.50547095989754f,  0.51544730992262f,  0.53104259108978f,
  0.55310389603444f,  0.58293496820613f,  0.62250412303566f,  0.67480834145501f,
  0.74453627100230f,  0.83934964541553f,  0.97256823786196f,  1.16943993343288f,
  1.48416461631417f,  2.05778100995341f,  3.40760841846872f, 10.19000812354803f,
  };

/* ------------------------------------------------------------------------*/

static const float cost32_c1[] =
  {
  0.50241928618816f,  0.52249861493969f,  0.56694403481636f,  0.64682178335999f,
  0.78815462345125f,  1.06067768599035f,  1.72244709823833f,  5.10114861868916f,
  };

/* ------------------------------------------------------------------------*/

static const float cost32_c2[] =
  {
  0.50979557910416f,  0.60134488693505f,  0.89997622313642f,  2.56291544774151f
  };

/* ------------------------------------------------------------------------*/

static const float cost32_c3[] =
  {
  0.54119610014620f,  1.30656296487638f,
  };

/* ------------------------------------------------------------------------*/

static const float cost32_c4[] =
  {
  0.70710678118655f,
  };

/*-------------------------------------------------------------------------*/

void cost32(const float *vec,float *f_vec);

/*-------------------------------------------------------------------------*/

void cost16(const float *vec,float *f_vec)
{
  float tmp1_0,tmp1_1,tmp1_2,tmp1_3,tmp1_4,tmp1_5,tmp1_6,tmp1_7;
  float res1_0,res1_1,res1_2,res1_3,res1_4,res1_5,res1_6,res1_7;

  float tmp2_0,tmp2_1,tmp2_2,tmp2_3;
  float res2_0,res2_1,res2_2,res2_3;

  float tmp3_0,tmp3_1;
  float res3_0,res3_1;

  tmp1_0 = vec[0]+vec[15];
  tmp1_1 = vec[1]+vec[14];
  tmp1_2 = vec[2]+vec[13];
  tmp1_3 = vec[3]+vec[12];
  tmp1_4 = vec[4]+vec[11];
  tmp1_5 = vec[5]+vec[10];
  tmp1_6 = vec[6]+vec[9];
  tmp1_7 = vec[7]+vec[8];

  tmp2_0 = tmp1_0+tmp1_7;
  tmp2_1 = tmp1_1+tmp1_6;
  tmp2_2 = tmp1_2+tmp1_5;
  tmp2_3 = tmp1_3+tmp1_4;

  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  f_vec[0]  = tmp3_0+tmp3_1;
  f_vec[16] = (tmp3_0-tmp3_1)*cost32_c4[0];

  tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
  tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];
    
  f_vec[8]  = res3_0+res3_1;
  f_vec[24] = res3_1;
    
  tmp2_0 = (tmp1_0-tmp1_7)*cost32_c2[0];
  tmp2_1 = (tmp1_1-tmp1_6)*cost32_c2[1];
  tmp2_2 = (tmp1_2-tmp1_5)*cost32_c2[2];
  tmp2_3 = (tmp1_3-tmp1_4)*cost32_c2[3];
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res2_0 = tmp3_0+tmp3_1;
  res2_2 = (tmp3_0-tmp3_1)*cost32_c4[0];

  tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
  tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];
  res3_0 = tmp3_0+tmp3_1;
  res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];

  res2_1 = res3_0+res3_1;
  res2_3 = res3_1;

  res1_1 = res2_0+res2_1;
  res1_3 = res2_1+res2_2;
  res1_5 = res2_2+res2_3;
  res1_7 = res2_3;
    
  f_vec[12] = res1_1;
  f_vec[4]  = res1_3;
  f_vec[20] = res1_5;
  f_vec[28] = res1_7;

  tmp1_0 = (vec[0]-vec[15])*cost32_c1[0];
  tmp1_1 = (vec[1]-vec[14])*cost32_c1[1];
  tmp1_2 = (vec[2]-vec[13])*cost32_c1[2];
  tmp1_3 = (vec[3]-vec[12])*cost32_c1[3];
  tmp1_4 = (vec[4]-vec[11])*cost32_c1[4];
  tmp1_5 = (vec[5]-vec[10])*cost32_c1[5];
  tmp1_6 = (vec[6]-vec[9])*cost32_c1[6];
  tmp1_7 = (vec[7]-vec[8])*cost32_c1[7];

  tmp2_0 = tmp1_0+tmp1_7;
  tmp2_1 = tmp1_1+tmp1_6;
  tmp2_2 = tmp1_2+tmp1_5;
  tmp2_3 = tmp1_3+tmp1_4;
  
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res1_0 = tmp3_0+tmp3_1;
  res1_4 = (tmp3_0-tmp3_1)*cost32_c4[0];

  tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
  tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];

  res1_2 = res3_0+res3_1;
  res1_6 = res3_1;

  tmp2_0 = (tmp1_0-tmp1_7)*cost32_c2[0];
  tmp2_1 = (tmp1_1-tmp1_6)*cost32_c2[1];
  tmp2_2 = (tmp1_2-tmp1_5)*cost32_c2[2];
  tmp2_3 = (tmp1_3-tmp1_4)*cost32_c2[3];
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res2_0 = tmp3_0+tmp3_1;
  res2_2 = (tmp3_0-tmp3_1)*cost32_c4[0];

  tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
  tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];

  res2_1 = res3_0+res3_1;
  res2_3 = res3_1;
  res1_1 = res2_0+res2_1;
  res1_3 = res2_1+res2_2;
  res1_5 = res2_2+res2_3;
  res1_7 = res2_3;

  f_vec[14] = res1_0+res1_1;
  f_vec[10] = res1_1+res1_2;
  f_vec[6] = res1_2+res1_3;
  f_vec[2] = res1_3+res1_4;
  f_vec[18] = res1_4+res1_5;
  f_vec[22] = res1_5+res1_6;
  f_vec[26] = res1_6+res1_7;
  f_vec[30] = res1_7;
}

/*-------------------------------------------------------------------------*/

void cost8(const float *vec,float *f_vec)
{
  float res1_1,res1_3,res1_5,res1_7;

  float tmp2_0,tmp2_1,tmp2_2,tmp2_3;
  float res2_0,res2_1,res2_2,res2_3;

  float tmp3_0,tmp3_1;
  float res3_0,res3_1;
  
  tmp2_0 = vec[0]+vec[7];
  tmp2_1 = vec[1]+vec[6];
  tmp2_2 = vec[2]+vec[5];
  tmp2_3 = vec[3]+vec[4];

  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  f_vec[0]  = tmp3_0+tmp3_1;
  f_vec[16] = (tmp3_0-tmp3_1)*cost32_c4[0];

  tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
  tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];
    
  f_vec[8]  = res3_0+res3_1;
  f_vec[24] = res3_1;
        
  tmp2_0 = (vec[0]-vec[7])*cost32_c2[0];
  tmp2_1 = (vec[1]-vec[6])*cost32_c2[1];
  tmp2_2 = (vec[2]-vec[5])*cost32_c2[2];
  tmp2_3 = (vec[3]-vec[4])*cost32_c2[3];
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res2_0 = tmp3_0+tmp3_1;
  res2_2 = (tmp3_0-tmp3_1)*cost32_c4[0];

  tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
  tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];
  res3_0 = tmp3_0+tmp3_1;
  res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];

  res2_1 = res3_0+res3_1;
  res2_3 = res3_1;

  res1_1 = res2_0+res2_1;
  res1_3 = res2_1+res2_2;
  res1_5 = res2_2+res2_3;
  res1_7 = res2_3;
    
  f_vec[12] = res1_1;
  f_vec[4]  = res1_3;
  f_vec[20] = res1_5;
  f_vec[28] = res1_7;
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

CPolyphase::CPolyphase (const MPEG_INFO &_info,
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

//-------------------------------------------------------------------------*
//   Init
//-------------------------------------------------------------------------*

void CPolyphase::Init(void)
{
  int i,j;

  for ( j=0; j<2; j++ )
    for ( i=0; i<HAN_SIZE; i++ )
      syn_buf[j][i] = 0.0f;

  bufOffset = 32;
}

//-------------------------------------------------------------------------*
//   Apply (short)
//-------------------------------------------------------------------------*
  
short *CPolyphase::Apply(POLYSPECTRUM &sample, short *pPcm, int frames)
{
	int nChannels    = (downMix ? 1:info.stereo);
	int nIncrement   = (16<<nChannels)>>(qual+resl);
	int fShortWindow = (downMix && (info.stereo==2)) ? 1 : 0;
	
	int j,k;
	
	for ( k=0; k<frames; k++ )
	{
		bufOffset = (bufOffset-32)&(HAN_SIZE-1);
		
		for ( j=0; j<nChannels; j++ )
		{ 
			switch ( qual )
			{
			case 0:
#if _USE_3DNOW
			CPolyphaseASM___cost32(sample[j][k], &(syn_buf[j][bufOffset]));
#else
			cost32(sample[j][k], &(syn_buf[j][bufOffset]));
#endif
			break;
			
			case 1:
			cost16(sample[j][k], &(syn_buf[j][bufOffset]));
			break;
			
			case 2:
			cost8(sample[j][k], &(syn_buf[j][bufOffset]));
			break;
			}
		}
		
		if ( nChannels == 1 )
		window_band_m(bufOffset, pPcm, fShortWindow);
		else
		{
#if _USE_X86_ASM
			CPolyphaseASM___window_band_s(
				bufOffset,
				fShortWindow,
				&CPolyphase::syn_buf[0][0],
				CPolyphase::qual,
				CPolyphase::resl,
				pPcm );
#else
			window_band_s(bufOffset, pPcm, fShortWindow);
#endif
		}
	
	pPcm += nIncrement;
	}
	
	return pPcm;    
}

//-------------------------------------------------------------------------*
//   Apply (float)
//-------------------------------------------------------------------------*
  
float *CPolyphase::Apply(POLYSPECTRUM &sample, float *pPcm, int frames)
{
	int nChannels    = (downMix ? 1:info.stereo);
	int nIncrement   = (16<<nChannels)>>(qual);
	int fShortWindow = (downMix && (info.stereo==2)) ? 1 : 0;
	
	int j,k;
	
	for ( k=0; k<frames; k++ )
	{
		bufOffset = (bufOffset-32)&(HAN_SIZE-1);
		
		for ( j=0; j<nChannels; j++ )
		{ 
			switch ( qual )
			{
			case 0:
#if _USE_3DNOW
			CPolyphaseASM___cost32(sample[j][k], &(syn_buf[j][bufOffset]));
#else
			cost32(sample[j][k], &(syn_buf[j][bufOffset]));
#endif
			break;
			
			case 1:
			cost16(sample[j][k], &(syn_buf[j][bufOffset]));
			break;
			
			case 2:
			cost8(sample[j][k], &(syn_buf[j][bufOffset]));
			break;
			}
		}
		
		if ( nChannels == 1 )
		window_band_m(bufOffset, pPcm, fShortWindow);
		else
		{
			window_band_s(bufOffset, pPcm, fShortWindow);
		}
		
		pPcm += nIncrement;
	}
	
	return pPcm;    
}

/*-------------------------------------------------------------------------*/

// [em] window_band_? float versions (C++ implementations only)

void CPolyphase::window_band_m(int bufOffset,float *out_samples, int /* short_window */)
{
  const float *winPtr = syn_f_window;
  double       sum1,sum2;
  int          i,j;

  /* sum 0 and sum 16, 8, 4 (full, half, quarter spectrum) */
  sum1 = sum2 = 0;

  for ( i=0; i<512; i+=64 )
    {
    sum1 += syn_buf[0][(bufOffset+i+16)    & (HAN_SIZE-1)]*winPtr[0];
    sum2 += syn_buf[0][(bufOffset+i+32)    & (HAN_SIZE-1)]*winPtr[3];
    sum1 += syn_buf[0][(bufOffset+i+32+16) & (HAN_SIZE-1)]*winPtr[2];
    winPtr += 4;
    }

  out_samples[0]          = float(sum1 / 32768.0);
  out_samples[16 >> qual] = float(sum2 / 32768.0);

  /* sum 1-15, 1-7, 1-3 and 17-31, 9-15, 5-7 (full, half, quarter spectrum) */

  for ( j=1; j<(16>>qual); j++ )
    {
    sum1 = sum2 = 0;
    
    winPtr += (1<<qual)*32 - 32;
    
    for ( i=0;i<512;i+=64 )
      {
      sum1 += syn_buf[0][(bufOffset+i+j*(1<<qual)+16) & (HAN_SIZE-1)]*winPtr[0];
      sum2 += syn_buf[0][(bufOffset+i+j*(1<<qual)+16) & (HAN_SIZE-1)]*winPtr[1];
      sum1 += syn_buf[0][(bufOffset+i+j*(1<<qual)+32) & (HAN_SIZE-1)]*winPtr[2];
      sum2 += syn_buf[0][(bufOffset+i+j*(1<<qual)+32) & (HAN_SIZE-1)]*winPtr[3];
      winPtr += 4;
      }
    
    out_samples[j]            = float(sum1 / 32768.0);
    out_samples[(32>>qual)-j] = float(sum2 / 32768.0);
    }
}

/*-------------------------------------------------------------------------*/

void CPolyphase::window_band_s(int bufOffset, float *out_samples, int /* short_window */)
{
  const float *winPtr = syn_f_window;
  double       sum1l,sum2l,sum1r,sum2r;
  int          i,j,bufPtr;

  /* sum 0 and sum 16, 8, 4 (full, half, quarter spectrum) */
  sum1l = sum2l = sum1r = sum2r = 0;

  bufPtr = bufOffset;

  for ( i=0; i<512; i+=64 )
    {
    sum1l += syn_buf[0][bufPtr+16] * winPtr[0];
    sum1r += syn_buf[1][bufPtr+16] * winPtr[0];

    bufPtr = (bufPtr+32)&(HAN_SIZE-1);
    
    sum1l += syn_buf[0][bufPtr+16] * winPtr[2];
    sum1r += syn_buf[1][bufPtr+16] * winPtr[2];
    sum2l += syn_buf[0][bufPtr   ] * winPtr[3];
    sum2r += syn_buf[1][bufPtr   ] * winPtr[3];

    bufPtr = (bufPtr+32)&(HAN_SIZE-1);

    winPtr+=4;
    }

  out_samples[0]            = float(sum1l / 32768.0);
  out_samples[32>>qual]     = float(sum2l / 32768.0);
  out_samples[1]            = float(sum1r / 32768.0);
  out_samples[(32>>qual)+1] = float(sum2r / 32768.0);

  /* sum 1-15, 1-7, 1-3 and 17-31, 9-15, 5-7 (full, half, quarter spectrum) */

  for ( j=1; j<(16>>qual); j++ )
    {
    sum1l = sum2l = sum1r = sum2r = 0;
    
    bufPtr  = bufOffset+j*(1<<qual);
    winPtr += (1<<qual)*32 - 32;
    
    for ( i=0; i<512; i+=64 )
      {
      sum1l += syn_buf[0][bufPtr+16]*winPtr[0];
      sum1r += syn_buf[1][bufPtr+16]*winPtr[0];
      sum2l += syn_buf[0][bufPtr+16]*winPtr[1];
      sum2r += syn_buf[1][bufPtr+16]*winPtr[1];
      
      bufPtr = (bufPtr+32)&(HAN_SIZE-1);
      
      sum1l += syn_buf[0][bufPtr]*winPtr[2];
      sum1r += syn_buf[1][bufPtr]*winPtr[2];
      sum2l += syn_buf[0][bufPtr]*winPtr[3];
      sum2r += syn_buf[1][bufPtr]*winPtr[3];
      
      bufPtr = (bufPtr+32)&(HAN_SIZE-1);
      
      winPtr += 4;
      }
    
    out_samples[j*2]                = float(sum1l / 32768.0);
    out_samples[((32>>qual)-j)*2]   = float(sum2l / 32768.0);
    out_samples[j*2+1]              = float(sum1r / 32768.0);
    out_samples[((32>>qual)-j)*2+1] = float(sum2r / 32768.0);
    }
}

/*-------------------------------------------------------------------------*/

// [em] window_band_? short versions

#ifndef USE_ASM

/*-------------------------------------------------------------------------*/

//
// functions for sample clipping
//

/*-------------------------------------------------------------------------*/

static inline short CLIP16(double fSample)
{
  return (short) ( (fSample < 32767.0) ? ( (fSample > -32767.0) ? fSample : -32767.0) : 32767.0);
}

/*-------------------------------------------------------------------------*/

static inline unsigned char CLIP8(double fSample)
{
  return (unsigned char) ( (CLIP16(fSample) >> 8) + 0x80 );
}


/*-------------------------------------------------------------------------*/

#if _USE_X86_ASM

void CPolyphase::window_band_m(int bufOffset,short *out_samples, int short_window) {

	CPolyphaseASM___window_band_m(
		bufOffset,
		short_window,
		&CPolyphase::syn_buf[0][0],
		CPolyphase::qual,
		CPolyphase::resl,
		out_samples
	);
}

void CPolyphase::window_band_s(int bufOffset,short *out_samples, int short_window) {

	CPolyphaseASM___window_band_s(
		bufOffset,
		short_window,
		&CPolyphase::syn_buf[0][0],
		CPolyphase::qual,
		CPolyphase::resl,
		out_samples
	);
}

#else //_USE_X86_ASM

/*-------------------------------------------------------------------------*/

void CPolyphase::window_band_m(int bufOffset,short *out_samples, int /* short_window */)
{
  const float *winPtr = syn_f_window;
  double       sum1,sum2;
  int          i,j;

  /* sum 0 and sum 16, 8, 4 (full, half, quarter spectrum) */
  sum1 = sum2 = 0;

  for ( i=0; i<512; i+=64 )
    {
    sum1 += syn_buf[0][(bufOffset+i+16)    & (HAN_SIZE-1)]*winPtr[0];
    sum2 += syn_buf[0][(bufOffset+i+32)    & (HAN_SIZE-1)]*winPtr[3];
    sum1 += syn_buf[0][(bufOffset+i+32+16) & (HAN_SIZE-1)]*winPtr[2];
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
        sum1 += syn_buf[0][(bufOffset+i+j*(1<<qual)+16) & (HAN_SIZE-1)]*winPtr[0];
        sum2 += syn_buf[0][(bufOffset+i+j*(1<<qual)+16) & (HAN_SIZE-1)]*winPtr[1];
        sum1 += syn_buf[0][(bufOffset+i+j*(1<<qual)+32) & (HAN_SIZE-1)]*winPtr[2];
        sum2 += syn_buf[0][(bufOffset+i+j*(1<<qual)+32) & (HAN_SIZE-1)]*winPtr[3];
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
        sum1 += syn_buf[0][(bufOffset+i+j*(1<<qual)+16) & (HAN_SIZE-1)]*winPtr[0];
        sum2 += syn_buf[0][(bufOffset+i+j*(1<<qual)+16) & (HAN_SIZE-1)]*winPtr[1];
        sum1 += syn_buf[0][(bufOffset+i+j*(1<<qual)+32) & (HAN_SIZE-1)]*winPtr[2];
        sum2 += syn_buf[0][(bufOffset+i+j*(1<<qual)+32) & (HAN_SIZE-1)]*winPtr[3];
        winPtr += 4;
        }
      
      ((unsigned char*)out_samples)[j]            = CLIP8(sum1);
      ((unsigned char*)out_samples)[(32>>qual)-j] = CLIP8(sum2);
      }
    }
}

void CPolyphase::window_band_s(int bufOffset,short *out_samples, int /* short_window */)
{
  const float *winPtr = syn_f_window;
  double       sum1l,sum2l,sum1r,sum2r;
  int          i,j,bufPtr;

  /* sum 0 and sum 16, 8, 4 (full, half, quarter spectrum) */
  sum1l = sum2l = sum1r = sum2r = 0;

  bufPtr = bufOffset;

  for ( i=0; i<512; i+=64 )
    {
    sum1l += syn_buf[0][bufPtr+16] * winPtr[0];
    sum1r += syn_buf[1][bufPtr+16] * winPtr[0];

    bufPtr = (bufPtr+32)&(HAN_SIZE-1);
    
    sum1l += syn_buf[0][bufPtr+16] * winPtr[2];
    sum1r += syn_buf[1][bufPtr+16] * winPtr[2];
    sum2l += syn_buf[0][bufPtr   ] * winPtr[3];
    sum2r += syn_buf[1][bufPtr   ] * winPtr[3];

    bufPtr = (bufPtr+32)&(HAN_SIZE-1);

    winPtr+=4;
    }

  if ( 0 == resl )
    {
    // 16bit PCM
    out_samples[0]            = CLIP16(sum1l);
    out_samples[32>>qual]     = CLIP16(sum2l);
    out_samples[1]            = CLIP16(sum1r);
    out_samples[(32>>qual)+1] = CLIP16(sum2r);
    }
  else
    {
    // 8bit PCM
    ((unsigned char*)out_samples)[0]            = CLIP8(sum1l);
    ((unsigned char*)out_samples)[32>>qual]     = CLIP8(sum2l);
    ((unsigned char*)out_samples)[1]            = CLIP8(sum1r);
    ((unsigned char*)out_samples)[(32>>qual)+1] = CLIP8(sum2r);
    }

  /* sum 1-15, 1-7, 1-3 and 17-31, 9-15, 5-7 (full, half, quarter spectrum) */

  if ( 0 == resl )
    {
    // 16 bit PCM
    for ( j=1; j<(16>>qual); j++ )
      {
      sum1l = sum2l = sum1r = sum2r = 0;
      
      bufPtr  = bufOffset+j*(1<<qual);
      winPtr += (1<<qual)*32 - 32;
      
      for ( i=0; i<512; i+=64 )
        {
        sum1l += syn_buf[0][bufPtr+16]*winPtr[0];
        sum1r += syn_buf[1][bufPtr+16]*winPtr[0];
        sum2l += syn_buf[0][bufPtr+16]*winPtr[1];
        sum2r += syn_buf[1][bufPtr+16]*winPtr[1];
        
        bufPtr = (bufPtr+32)&(HAN_SIZE-1);
        
        sum1l += syn_buf[0][bufPtr]*winPtr[2];
        sum1r += syn_buf[1][bufPtr]*winPtr[2];
        sum2l += syn_buf[0][bufPtr]*winPtr[3];
        sum2r += syn_buf[1][bufPtr]*winPtr[3];
        
        bufPtr = (bufPtr+32)&(HAN_SIZE-1);
        
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
    for ( j=1; j<(16>>qual); j++ )
      {
      sum1l = sum2l = sum1r = sum2r = 0;
      
      bufPtr  = bufOffset+j*(1<<qual);
      winPtr += (1<<qual)*32 - 32;
      
      for ( i=0; i<512; i+=64 )
        {
        sum1l += syn_buf[0][bufPtr+16]*winPtr[0];
        sum1r += syn_buf[1][bufPtr+16]*winPtr[0];
        sum2l += syn_buf[0][bufPtr+16]*winPtr[1];
        sum2r += syn_buf[1][bufPtr+16]*winPtr[1];
        
        bufPtr = (bufPtr+32)&(HAN_SIZE-1);
        
        sum1l += syn_buf[0][bufPtr]*winPtr[2];
        sum1r += syn_buf[1][bufPtr]*winPtr[2];
        sum2l += syn_buf[0][bufPtr]*winPtr[3];
        sum2r += syn_buf[1][bufPtr]*winPtr[3];
        
        bufPtr = (bufPtr+32)&(HAN_SIZE-1);
        
        winPtr += 4;
        }
      
      ((unsigned char*)out_samples)[j*2]                = CLIP8(sum1l);
      ((unsigned char*)out_samples)[((32>>qual)-j)*2]   = CLIP8(sum2l);
      ((unsigned char*)out_samples)[j*2+1]              = CLIP8(sum1r);
      ((unsigned char*)out_samples)[((32>>qual)-j)*2+1] = CLIP8(sum2r);
      }
    }
}
#endif //_USE_X86_ASM

/*-------------------------------------------------------------------------*/

#if !_USE_3DNOW

void cost32(const float *vec,float *f_vec)
{
  float tmp0_0,tmp0_1,tmp0_2,tmp0_3,tmp0_4,tmp0_5,tmp0_6,tmp0_7;
  float tmp0_8,tmp0_9,tmp0_10,tmp0_11,tmp0_12,tmp0_13,tmp0_14,tmp0_15;
  float res0_0,res0_1,res0_2,res0_3,res0_4,res0_5,res0_6,res0_7;
  float res0_8,res0_9,res0_10,res0_11,res0_12,res0_13,res0_14,res0_15;

  float tmp1_0,tmp1_1,tmp1_2,tmp1_3,tmp1_4,tmp1_5,tmp1_6,tmp1_7;
  float res1_0,res1_1,res1_2,res1_3,res1_4,res1_5,res1_6,res1_7;

  float tmp2_0,tmp2_1,tmp2_2,tmp2_3;
  float res2_0,res2_1,res2_2,res2_3;

  float tmp3_0,tmp3_1;
  float res3_0,res3_1;

  tmp0_0 = vec[0]+vec[31];
  tmp0_1 = vec[1]+vec[30];
  tmp0_2 = vec[2]+vec[29];
  tmp0_3 = vec[3]+vec[28];
  tmp0_4 = vec[4]+vec[27];
  tmp0_5 = vec[5]+vec[26];
  tmp0_6 = vec[6]+vec[25];
  tmp0_7 = vec[7]+vec[24];
  tmp0_8 = vec[8]+vec[23];
  tmp0_9 = vec[9]+vec[22];
  tmp0_10 = vec[10]+vec[21];
  tmp0_11 = vec[11]+vec[20];
  tmp0_12 = vec[12]+vec[19];
  tmp0_13 = vec[13]+vec[18];
  tmp0_14 = vec[14]+vec[17];
  tmp0_15 = vec[15]+vec[16];

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
  f_vec[16] = (tmp3_0-tmp3_1)*cost32_c4[0];

  tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
  tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];

  f_vec[8]  = res3_0+res3_1;
  f_vec[24] = res3_1;

  tmp2_0 = (tmp1_0-tmp1_7)*cost32_c2[0];
  tmp2_1 = (tmp1_1-tmp1_6)*cost32_c2[1];
  tmp2_2 = (tmp1_2-tmp1_5)*cost32_c2[2];
  tmp2_3 = (tmp1_3-tmp1_4)*cost32_c2[3];
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res2_0 = tmp3_0+tmp3_1;
  res2_2 = (tmp3_0-tmp3_1)*cost32_c4[0];

  tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
  tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];
  res3_0 = tmp3_0+tmp3_1;
  res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];

  res2_1 = res3_0+res3_1;

  f_vec[12] = res2_0+res2_1;
  f_vec[4] = res2_1+res2_2;
  f_vec[20] = res2_2+res3_1;
  f_vec[28] = res3_1;

  tmp1_0 = (tmp0_0-tmp0_15)*cost32_c1[0];
  tmp1_1 = (tmp0_1-tmp0_14)*cost32_c1[1];
  tmp1_2 = (tmp0_2-tmp0_13)*cost32_c1[2];
  tmp1_3 = (tmp0_3-tmp0_12)*cost32_c1[3];
  tmp1_4 = (tmp0_4-tmp0_11)*cost32_c1[4];
  tmp1_5 = (tmp0_5-tmp0_10)*cost32_c1[5];
  tmp1_6 = (tmp0_6-tmp0_9)*cost32_c1[6];
  tmp1_7 = (tmp0_7-tmp0_8)*cost32_c1[7];
  tmp2_0 = tmp1_0+tmp1_7;
  tmp2_1 = tmp1_1+tmp1_6;
  tmp2_2 = tmp1_2+tmp1_5;
  tmp2_3 = tmp1_3+tmp1_4;
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res1_0 = tmp3_0+tmp3_1;
  res1_4 = (tmp3_0-tmp3_1)*cost32_c4[0];

  tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
  tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];

  res1_2 = res3_0+res3_1;
  res1_6 = res3_1;

  tmp2_0 = (tmp1_0-tmp1_7)*cost32_c2[0];
  tmp2_1 = (tmp1_1-tmp1_6)*cost32_c2[1];
  tmp2_2 = (tmp1_2-tmp1_5)*cost32_c2[2];
  tmp2_3 = (tmp1_3-tmp1_4)*cost32_c2[3];
  tmp3_0 = tmp2_0+tmp2_3;
  tmp3_1 = tmp2_1+tmp2_2;

  res2_0 = tmp3_0+tmp3_1;
  res2_2 = (tmp3_0-tmp3_1)*cost32_c4[0];

  tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
  tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];

  res3_0 = tmp3_0+tmp3_1;
  res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];

  res2_1 = res3_0+res3_1;
  res2_3 = res3_1;
  
  res1_1 = res2_0+res2_1;
  res1_3 = res2_1+res2_2;
  res1_5 = res2_2+res2_3;
  res1_7 = res2_3;

  f_vec[14] = res1_0+res1_1;
  f_vec[10] = res1_1+res1_2;
  f_vec[6] = res1_2+res1_3;
  f_vec[2] = res1_3+res1_4;
  f_vec[18] = res1_4+res1_5;
  f_vec[22] = res1_5+res1_6;
  f_vec[26] = res1_6+res1_7;
  f_vec[30] = res1_7;

  /*  Odd Terms */
  tmp0_0 = (vec[0]-vec[31])*cost32_c0[0];
  tmp0_1 = (vec[1]-vec[30])*cost32_c0[1];
  tmp0_2 = (vec[2]-vec[29])*cost32_c0[2];
  tmp0_3 = (vec[3]-vec[28])*cost32_c0[3];
  tmp0_4 = (vec[4]-vec[27])*cost32_c0[4];
  tmp0_5 = (vec[5]-vec[26])*cost32_c0[5];
  tmp0_6 = (vec[6]-vec[25])*cost32_c0[6];
  tmp0_7 = (vec[7]-vec[24])*cost32_c0[7];
  tmp0_8 = (vec[8]-vec[23])*cost32_c0[8];
  tmp0_9 = (vec[9]-vec[22])*cost32_c0[9];
  tmp0_10 = (vec[10]-vec[21])*cost32_c0[10];
  tmp0_11 = (vec[11]-vec[20])*cost32_c0[11];
  tmp0_12 = (vec[12]-vec[19])*cost32_c0[12];
  tmp0_13 = (vec[13]-vec[18])*cost32_c0[13];
  tmp0_14 = (vec[14]-vec[17])*cost32_c0[14];
  tmp0_15 = (vec[15]-vec[16])*cost32_c0[15];

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
	res0_8 = (tmp3_0-tmp3_1)*cost32_c4[0];
//printf( "r0_0 %f   r0_8 %f \n", res0_0, res0_8 );	
	   
	tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
	tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];
	res3_0 = tmp3_0+tmp3_1;
	res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];
	res0_4 = res3_0+res3_1;
	res0_12 = res3_1;
//printf( "r0_4 %f   r0_12 %f \n", res0_4, res0_12 );	
	
	
	tmp2_0 = (tmp1_0-tmp1_7)*cost32_c2[0];
	tmp2_1 = (tmp1_1-tmp1_6)*cost32_c2[1];
	tmp2_2 = (tmp1_2-tmp1_5)*cost32_c2[2];
	tmp2_3 = (tmp1_3-tmp1_4)*cost32_c2[3];
	tmp3_0 = tmp2_0+tmp2_3;
	tmp3_1 = tmp2_1+tmp2_2;
	res2_0 = tmp3_0+tmp3_1;
	res2_2 = (tmp3_0-tmp3_1)*cost32_c4[0];
	
	tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
	tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];
	res3_0 = tmp3_0+tmp3_1;
	res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];
	res2_1 = res3_0+res3_1;
	res2_3 = res3_1;
	res0_2 = res2_0+res2_1;
	res0_6 = res2_1+res2_2;
	res0_10 = res2_2+res2_3;
	res0_14 = res2_3;
//printf( "r0_2  %f   r0_6  %f \n", res0_2, res0_6 );	
//printf( "r0_10 %f   r0_14 %f \n", res0_10, res0_14 );	
	
	tmp1_0 = (tmp0_0-tmp0_15)*cost32_c1[0];
	tmp1_1 = (tmp0_1-tmp0_14)*cost32_c1[1];
	tmp1_2 = (tmp0_2-tmp0_13)*cost32_c1[2];
	tmp1_3 = (tmp0_3-tmp0_12)*cost32_c1[3];
	tmp1_4 = (tmp0_4-tmp0_11)*cost32_c1[4];
	tmp1_5 = (tmp0_5-tmp0_10)*cost32_c1[5];
	tmp1_6 = (tmp0_6-tmp0_9)*cost32_c1[6];
	tmp1_7 = (tmp0_7-tmp0_8)*cost32_c1[7];
	tmp2_0 = tmp1_0+tmp1_7;
	tmp2_1 = tmp1_1+tmp1_6;
	tmp2_2 = tmp1_2+tmp1_5;
	tmp2_3 = tmp1_3+tmp1_4;
	tmp3_0 = tmp2_0+tmp2_3;
	tmp3_1 = tmp2_1+tmp2_2;

	res1_0 = tmp3_0+tmp3_1;
	res1_4 = (tmp3_0-tmp3_1)*cost32_c4[0];
//printf( "r1_0  %f     r1_4  %f \n", res1_0, res1_4 );	
	
	tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
	tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];
	res3_0 = tmp3_0+tmp3_1;
	res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];
	res1_2 = res3_0+res3_1;
	res1_6 = res3_1;
//printf( "r1_2  %f     r1_6  %f \n", res1_2, res1_6 );	
	
	tmp2_0 = (tmp1_0-tmp1_7)*cost32_c2[0];
	tmp2_1 = (tmp1_1-tmp1_6)*cost32_c2[1];
	tmp2_2 = (tmp1_2-tmp1_5)*cost32_c2[2];
	tmp2_3 = (tmp1_3-tmp1_4)*cost32_c2[3];
	tmp3_0 = tmp2_0+tmp2_3;
	tmp3_1 = tmp2_1+tmp2_2;
	res2_0 = tmp3_0+tmp3_1;
	res2_2 = (tmp3_0-tmp3_1)*cost32_c4[0];

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
	
	tmp3_0 = (tmp2_0-tmp2_3)*cost32_c3[0];
	tmp3_1 = (tmp2_1-tmp2_2)*cost32_c3[1];
	res3_0 = tmp3_0+tmp3_1;
	res3_1 = (tmp3_0-tmp3_1)*cost32_c4[0];
	
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
	
	f_vec[15] = res0_0+res0_1;
	f_vec[13] = res0_1+res0_2;
	f_vec[11] = res0_2+res0_3;
	f_vec[9] = res0_3+res0_4;
	f_vec[7] = res0_4+res0_5;
	f_vec[5] = res0_5+res0_6;
	f_vec[3] = res0_6+res0_7;
	f_vec[1] = res0_7+res0_8;
	f_vec[17] = res0_8+res0_9;
	f_vec[19] = res0_9+res0_10;
	f_vec[21] = res0_10+res0_11;
	f_vec[23] = res0_11+res0_12;
	f_vec[25] = res0_12+res0_13;
	f_vec[27] = res0_13+res0_14;
	f_vec[29] = res0_14+res0_15;
	f_vec[31] = res0_15;

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
#endif

/*-------------------------------------------------------------------------*/

//
// [em] NOT SUPPORTED (uses VC++ inline assembly)
//


#else /* ifndef USE_ASM */
#pragma message (__FILE__": using Intel X86 inline assembler") 

/*-------------------------------------------------------------------------*/

void CPolyphase::window_band_s(int bufOffset,short *out_samples,int short_window)
{  
  unsigned long isam [32*2] ;
  unsigned long win_inc ; 
  unsigned long buf_inc ;
  unsigned long win_start ;

  int qual = CPolyphase::qual ;
  int resl = CPolyphase::resl ;

  float *syn_buf = &CPolyphase::syn_buf [0][0] ;

  unsigned long tmp_edi ;

  _asm
  {
    mov   edi, dword ptr [syn_buf] // gew

    mov   ecx,qual
    mov   edx,16*4
    shr   edx,cl  
    
    mov   eax,1
    shl   eax,cl
    mov   buf_inc,eax

    cmp   short_window,0
    jne   l0
    
    mov   eax,32*4
    shl   eax,cl
    mov   win_inc,eax
    mov   win_start,-32
    lea   ecx,[syn_f_window+32*4]
    jmp l00

l0: mov   eax,24*4
    shl   eax,cl
    mov   win_inc,eax
    mov   win_start,-24
    add   bufOffset,64
    and   bufOffset,(HAN_SIZE-1)
    lea   ecx,[syn_f_window_short+24*4]

l00:
    // Sums 0 and 16
    fldz
    fldz
    fldz
    fldz
    
    mov   esi,win_start
    mov   ebx,bufOffset

l1: fld   dword ptr [edi+ebx*4+16*4]
    fmul  dword ptr [ecx+esi*4+0*4]
    fld   dword ptr [edi+ebx*4+16*4+512*4]
    fmul  dword ptr [ecx+esi*4+0*4]
    fxch  st(1)
    faddp st(2),st(0)
    faddp st(2),st(0)

    add   ebx,32
    and   ebx,(HAN_SIZE-1)

    fld   dword ptr [edi+ebx*4]
    fmul  dword ptr [ecx+esi*4+3*4]
    fld   dword ptr [edi+ebx*4+512*4]
    fmul  dword ptr [ecx+esi*4+3*4]

    fld   dword ptr [edi+ebx*4+16*4]
    fmul  dword ptr [ecx+esi*4+2*4]
    fld   dword ptr [edi+ebx*4+16*4+512*4]
    fmul  dword ptr [ecx+esi*4+2*4]

    fxch  st(3)
    faddp st(6),st(0)
    faddp st(3),st(0)
    faddp st(5),st(0)
    faddp st(2),st(0)

    add   ebx,32
    and   ebx,(HAN_SIZE-1)
    add   esi,4
    js    l1

    lea   edi,isam // gew

    fistp dword ptr [edi+0]
    fistp dword ptr [edi+4]
    fistp dword ptr [edi+edx*2]
    fistp dword ptr [edi+edx*2+4]

    sub   edx,4

l2: fldz
    fldz
    fldz
    fldz

    mov   esi,win_start
    mov   ebx,bufOffset   
    add   ebx,buf_inc
    mov   bufOffset,ebx
    add   ecx,win_inc

    mov   tmp_edi, edi // gew
    mov   edi, dword ptr [syn_buf] // gew

l3: fld   dword ptr [edi+ebx*4+16*4]
    fmul  dword ptr [ecx+esi*4+0*4]
    fld   dword ptr [edi+ebx*4+16*4+512*4]
    fmul  dword ptr [ecx+esi*4+0*4]
    
    fld   dword ptr [edi+ebx*4+16*4]
    fmul  dword ptr [ecx+esi*4+1*4]
    fld   dword ptr [edi+ebx*4+16*4+512*4]
    fmul  dword ptr [ecx+esi*4+1*4]

    fxch  st(3)

    faddp st(4),st(0)      
    faddp st(5),st(0)      
    faddp st(3),st(0)      
    faddp st(4),st(0)      

    add   ebx,32
    and   ebx,(HAN_SIZE-1)

    fld   dword ptr [edi+ebx*4]
    fmul  dword ptr [ecx+esi*4+2*4]
    fld   dword ptr [edi+ebx*4+512*4]
    fmul  dword ptr [ecx+esi*4+2*4]
    
    fld   dword ptr [edi+ebx*4]
    fmul  dword ptr [ecx+esi*4+3*4]
    fld   dword ptr [edi+ebx*4+512*4]
    fmul  dword ptr [ecx+esi*4+3*4]

    fxch  st(3)
    faddp st(4),st(0)
    faddp st(5),st(0)
    faddp st(3),st(0)
    faddp st(4),st(0)

    add   ebx,32
    and   ebx,(HAN_SIZE-1)
    add   esi,4
    js    l3

    mov   edi, tmp_edi // gew

    lea   edi,[edi+8]

    fistp dword ptr [edi]
    fistp dword ptr [edi+4]
    fistp dword ptr [edi+edx*4]
    fistp dword ptr [edi+edx*4+4]

    sub   edx,4
    jnz   l2

    mov   edx,64
    mov   ecx,qual
    shr   edx,cl
    lea   esi,[isam+edx*4]
    mov   edi,out_samples

    cmp   resl,0
    je    c_16bit

    /*   8 bit clipping   */
    lea   edi,[edi+edx]
    neg   edx

l4_08:  
    mov   eax,[esi+edx*4]
    cmp   eax,0x00007fff
    jle   l5_08
    mov   eax,0x00007fff
l5_08:  
    cmp   eax,0xffff8000
    jge   l6_08
    mov   eax,0xffff8000
l6_08:  
    add   ax,0x8000
    mov   [edi+edx],ah
    inc   edx
    js    l4_08
    jmp   l7

    /*   16 bit clipping    */
c_16bit:
    lea   edi,[edi+edx*2]
    neg   edx

l4_16:  
    mov   eax,[esi+edx*4]
    cmp   eax,0x00007fff
    jle   l5_16
    mov   eax,0x00007fff
l5_16: 
    cmp   eax,0xffff8000
    jge   l6_16
    mov   eax,0xffff8000
l6_16:
    mov   [edi+edx*2],ax
    inc   edx
    js    l4_16
      
l7:   
  }
}

/*-------------------------------------------------------------------------*/
  
void CPolyphase::window_band_m(int bufOffset,short *out_samples,int short_window)
{
  unsigned long isam [32*2] ;
  unsigned long win_inc ; 
  unsigned long buf_inc ;
  unsigned long win_start ;

  int qual = CPolyphase::qual ;
  int resl = CPolyphase::resl ;

  float *syn_buf = &CPolyphase::syn_buf [0][0] ;

  unsigned long tmp_edi ;

  _asm
  {
    // lea   edi,isam
    mov   edi, dword ptr [syn_buf] // gew
    
    mov   ecx,qual
    mov   edx,16*4
    shr   edx,cl  
    
    mov   eax,1
    shl   eax,cl
    mov   buf_inc,eax
    
    cmp   short_window,0
    jne   l0
    
    mov   eax,32*4
    shl   eax,cl
    mov   win_inc,eax
    mov   win_start,-32
    lea   ecx,[syn_f_window+32*4]
    jmp l00

l0: mov   eax,24*4
    shl   eax,cl
    mov   win_inc,eax
    mov   win_start,-24
    add   bufOffset,64
    and   bufOffset,(HAN_SIZE-1)
    lea   ecx,[syn_f_window_short+24*4]
l00:
   
    // Sums 0 and 16
    fldz
    fldz
    
    mov   esi,win_start
    mov   ebx,bufOffset


l1: fld   dword ptr [edi+ebx*4+16*4]
    fmul  dword ptr [ecx+esi*4+0*4]

    add   ebx,32
    and   ebx,(HAN_SIZE-1)

    fld   dword ptr [edi+ebx*4]
    fmul  dword ptr [ecx+esi*4+3*4]
  
    fld   dword ptr [edi+ebx*4+16*4]
    fmul  dword ptr [ecx+esi*4+2*4]

    fxch  st(2)
    faddp st(3),st(0)
    faddp st(3),st(0)
    faddp st(1),st(0)
  
    add   ebx,32
    and   ebx,(HAN_SIZE-1)
    add   esi,4
    js    l1

    lea   edi, isam // gew

    fistp dword ptr [edi+0]
    fistp dword ptr [edi+edx]

    sub   edx,4

l2: fldz
    fldz

    mov   esi,win_start
    mov   ebx,bufOffset
    add   ebx,buf_inc
    mov   bufOffset,ebx
    add   ecx,win_inc

    mov   tmp_edi, edi // gew
    mov   edi, dword ptr [syn_buf] // gew

l3: fld   dword ptr [edi+ebx*4+16*4]
    fmul  dword ptr [ecx+esi*4+0*4]
    
    fld   dword ptr [edi+ebx*4+16*4]
    fmul  dword ptr [ecx+esi*4+1*4]

    fxch  st(1)

    faddp st(2),st(0)      
    faddp st(2),st(0)      

    add   ebx,32
    and   ebx,(HAN_SIZE-1)

    fld   dword ptr [edi+ebx*4]
    fmul  dword ptr [ecx+esi*4+2*4]
    
    fld   dword ptr [edi+ebx*4]
    fmul  dword ptr [ecx+esi*4+3*4]

    fxch  st(1)
    faddp st(2),st(0)
    faddp st(2),st(0)

    add   ebx,32
    and   ebx,(HAN_SIZE-1)
    add   esi,4
    js    l3

    mov   edi, tmp_edi // gew

    lea   edi,[edi+4]
    
    fistp dword ptr [edi]
    fistp dword ptr [edi+edx*2]
    
    sub   edx,4
    jnz   l2
    
    mov   edx,32
    mov   ecx,qual
    shr   edx,cl
    lea   esi,[isam+edx*4]
    mov   edi,out_samples


    cmp   resl,0
    je    c_16bit

    /*   8 bit clipping   */
    lea   edi,[edi+edx]
    neg   edx

l4_08:  
    mov   eax,[esi+edx*4]
    cmp   eax,0x00007fff
    jle     l5_08
    mov   eax,0x00007fff
l5_08:  
    cmp   eax,0xffff8000
    jge   l6_08
    mov   eax,0xffff8000
l6_08:  
    add   ax,0x8000
    mov   [edi+edx],ah
    inc     edx
    js    l4_08
    jmp   l7
    
    /*   16 bit clipping    */
c_16bit:
    lea   edi,[edi+edx*2]
    neg   edx

l4_16:  
    mov   eax,[esi+edx*4]
    cmp   eax,0x00007fff
    jle     l5_16
    mov   eax,0x00007fff
l5_16:
    cmp   eax,0xffff8000
    jge   l6_16
    mov   eax,0xffff8000
l6_16:
    mov   [edi+edx*2],ax
    inc     edx
    js    l4_16
l7:   
  }
}

/*-------------------------------------------------------------------------*/

#define C0_OFF 0*4
#define C1_OFF 16*4
#define C2_OFF 24*4
#define C3_OFF 28*4
#define C4_OFF 30*4 

/*-------------------------------------------------------------------------*/

/* Even DCT-4 Transform */
#define COST4E(Src,Res0,Res1,Res2,Res3) _asm \
                                             \
{                                            \
  _asm fld  dword ptr Src                    \
  _asm fadd dword ptr Src+7*4                \
  _asm fld  dword ptr Src+1*4                \
  _asm fadd dword ptr Src+6*4                \
  _asm fld  dword ptr Src+3*4                \
  _asm fadd dword ptr Src+4*4                \
  _asm fld  dword ptr Src+2*4                \
  _asm fadd dword ptr Src+5*4                \
  _asm fld    st(3)                          \
  _asm fadd   st(0),st(2)                    \
  _asm fxch   st(4)                          \
  _asm fsubrp st(2),st(0)                    \
  _asm fld    st(2)                          \
  _asm fadd   st(0),st(1)                    \
  _asm fxch   st(3)                          \
  _asm fsubrp st(1),st(0)                    \
  _asm fld    st(3)                          \
  _asm fadd   st(0),st(3)                    \
  _asm fxch   st(4)                          \
  _asm fsubrp st(3),st(0)                    \
  _asm fmul   dword ptr [ecx+C3_OFF+1*4]     \
  _asm fxch   st(1)                          \
  _asm fmul   dword ptr [ecx+C3_OFF+0*4]     \
  _asm fxch   st(2)                          \
  _asm fmul   dword ptr [ecx+C4_OFF+0*4]     \
  _asm fld    st(2)                          \
  _asm fadd   st(0),st(2)                    \
  _asm fxch   st(3)                          \
  _asm fsubrp st(2),st(0)                    \
  _asm fstp   dword ptr Res1                 \
  _asm fmul   dword ptr [ecx+C4_OFF+0*4]     \
  _asm fxch   st(2)                          \
  _asm fstp   dword ptr Res0                 \
  _asm fadd   st(0),st(1)                    \
  _asm fxch   st(1)                          \
  _asm fstp   dword ptr Res3                 \
  _asm fstp   dword ptr Res2                 \
}

/*-------------------------------------------------------------------------*/

/* ODD DCT-4 Transform */
#define COST4O(Src,Res0,Res1,Res2,Res3) _asm \
                                             \
{                                            \
  _asm fld  dword ptr Src                    \
  _asm fsub dword ptr Src+7*4                \
  _asm fmul dword ptr [ecx+C2_OFF+0*4]       \
  _asm fld  dword ptr Src+1*4                \
  _asm fsub dword ptr Src+6*4                \
  _asm fmul dword ptr [ecx+C2_OFF+1*4]       \
  _asm fld  dword ptr Src+3*4                \
  _asm fsub dword ptr Src+4*4                \
  _asm fmul dword ptr [ecx+C2_OFF+3*4]       \
  _asm fld  dword ptr Src+2*4                \
  _asm fsub dword ptr Src+5*4                \
  _asm fmul dword ptr [ecx+C2_OFF+2*4]       \
  _asm fld    st(3)                          \
  _asm fadd   st(0),st(2)                    \
  _asm fxch   st(4)                          \
  _asm fsubrp st(2),st(0)                    \
  _asm fld    st(2)                          \
  _asm fadd   st(0),st(1)                    \
  _asm fxch   st(3)                          \
  _asm fsubrp st(1),st(0)                    \
  _asm fld    st(3)                          \
  _asm fadd   st(0),st(3)                    \
  _asm fxch   st(4)                          \
  _asm fsubrp st(3),st(0)                    \
  _asm fmul   dword ptr [ecx+C3_OFF+1*4]     \
  _asm fxch   st(1)                          \
  _asm fmul   dword ptr [ecx+C3_OFF+0*4]     \
  _asm fxch   st(2)                          \
  _asm fmul   dword ptr [ecx+C4_OFF+0*4]     \
  _asm fld    st(2)                          \
  _asm fadd   st(0),st(2)                    \
  _asm fxch   st(3)                          \
  _asm fsubrp st(2),st(0)                    \
  _asm fxch   st(1)                          \
  _asm fmul   dword ptr [ecx+C4_OFF+0*4]     \
  _asm fadd   st(2),st(0)                    \
  _asm fxch   st(2)                          \
  _asm fadd   st(3),st(0)                    \
  _asm fadd   st(0),st(1)                    \
  _asm fxch   st(2)                          \
  _asm fadd   st(1),st(0)                    \
  _asm fstp   dword ptr Res3                 \
  _asm fstp   dword ptr Res1                 \
  _asm fstp   dword ptr Res2                 \
  _asm fstp   dword ptr Res0                 \
}

/*-------------------------------------------------------------------------*/

void cost32 (const float *vec,float *f_vec)
{
  static const float cost32_const[31] =
  {
    /* c0 */
    0.50060299823520f,0.50547095989754f,0.51544730992262f,0.53104259108978f,
    0.55310389603444f,0.58293496820613f,0.62250412303566f,0.67480834145501f,
    0.74453627100230f,0.83934964541553f,0.97256823786196f,1.16943993343288f,
    1.48416461631417f,2.05778100995341f,3.40760841846872f,10.19000812354803f,
  
    /* c1 */
    0.50241928618816f,0.52249861493969f,0.56694403481636f,0.64682178335999f,
    0.78815462345125f,1.06067768599035f,1.72244709823833f,5.10114861868916f,

    /* c2 */
    0.50979557910416f,0.60134488693505f,0.89997622313642f,2.56291544774151f,

    /* c3 */
    0.54119610014620f,1.30656296487638f,

    /* c4 */
    0.70710678118655f
  };

  float tmp[16]; 
  float res[16];

  _asm
    {
    mov ebx,vec
    mov esi,f_vec
  
    lea eax,tmp          /* to produce a smaller code size */
    lea edx,res          /*           ""                   */
    lea ecx,cost32_const /*           ""                   */
  
    /* even Part */
    
    fld   dword ptr [ebx]
    fadd  dword ptr [ebx+124]
  
    fld   dword ptr [ebx+60]
    fadd  dword ptr [ebx+64]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+0*4]
    fxch  st(1)
    fstp  dword ptr [eax+0*4]
    fstp  dword ptr [eax+8*4+0*4]
  
  
    fld   dword ptr [ebx+4]
    fadd  dword ptr [ebx+120]
  
    fld   dword ptr [ebx+56]
    fadd  dword ptr [ebx+68]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+1*4]
    fxch  st(1)
    fstp  dword ptr [eax+1*4]
    fstp  dword ptr [eax+8*4+1*4]
  
    fld   dword ptr [ebx+8]
    fadd  dword ptr [ebx+116]
  
    fld   dword ptr [ebx+52]
    fadd  dword ptr [ebx+72]
  
    fld st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+2*4]
    fxch  st(1)
    fstp  dword ptr [eax+2*4]
    fstp  dword ptr [eax+8*4+2*4]
  
    fld   dword ptr [ebx+12]
    fadd  dword ptr [ebx+112]
  
    fld   dword ptr [ebx+48]
    fadd  dword ptr [ebx+76]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+3*4]
    fxch  st(1)
    fstp  dword ptr [eax+3*4]
    fstp  dword ptr [eax+8*4+3*4]
  
    fld   dword ptr [ebx+16]
    fadd  dword ptr [ebx+108]
  
    fld   dword ptr [ebx+44]
    fadd  dword ptr [ebx+80]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+4*4]
    fxch  st(1)
    fstp  dword ptr [eax+4*4]
    fstp  dword ptr [eax+8*4+4*4]
  
    fld   dword ptr [ebx+20]
    fadd  dword ptr [ebx+104]
  
    fld   dword ptr [ebx+40]
    fadd  dword ptr [ebx+84]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+5*4]
    fxch  st(1)
    fstp  dword ptr [eax+5*4]
    fstp  dword ptr [eax+8*4+5*4]
  
    fld   dword ptr [ebx+24]
    fadd  dword ptr [ebx+100]
  
    fld   dword ptr [ebx+36]
    fadd  dword ptr [ebx+88]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+6*4]
    fxch  st(1)
    fstp  dword ptr [eax+6*4]
    fstp  dword ptr [eax+8*4+6*4]
  
    fld   dword ptr [ebx+28]
    fadd  dword ptr [ebx+96]
  
    fld   dword ptr [ebx+32]
    fadd  dword ptr [ebx+92]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+7*4]
    fxch  st(1)
    fstp  dword ptr [eax+7*4]
    fstp  dword ptr [eax+8*4+7*4]
      
    COST4E([eax],[esi],[esi+64],[esi+32],[esi+96])
    COST4O([eax],[esi+48],[esi+80],[esi+16],[esi+112])
  
    COST4E([eax+8*4],[edx+0*4],[edx+4*4],[edx+2*4],[edx+6*4])
    COST4O([eax+8*4],[edx+1*4],[edx+5*4],[edx+3*4],[esi+120])
  
    fld   dword ptr [edx+0*4]
    fld   dword ptr [edx+1*4]
    fadd  st(1),st(0)
    fld   dword ptr [edx+2*4]
    fadd  st(1),st(0)
    fld   dword ptr [edx+3*4]
    fadd  st(1),st(0)
    fld   dword ptr [edx+4*4]
    fadd  st(1),st(0)
    fld   dword ptr [edx+5*4]
    fadd  st(1),st(0)
    fld   dword ptr [edx+6*4]
    fadd  st(1),st(0)
    fadd  dword ptr [esi+120]
    fxch  st(6)
    fstp  dword ptr [esi+56]
    fstp  dword ptr [esi+88]
    fstp  dword ptr [esi+72]
    fstp  dword ptr [esi+8]
    fstp  dword ptr [esi+24]
    fstp  dword ptr [esi+40]
    fstp  dword ptr [esi+104]
  
  
    /* odd part */
      
    fld   dword ptr [ebx]
    fsub  dword ptr [ebx+124]
    fmul  dword ptr [ecx+C0_OFF+0*4]
  
    fld   dword ptr [ebx+60]
    fsub  dword ptr [ebx+64]
    fmul  dword ptr [ecx+C0_OFF+15*4]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+0*4]
    fxch  st(1)
    fstp  dword ptr [eax+0*4]
    fstp  dword ptr [eax+8*4+0*4]
  
  
    fld   dword ptr [ebx+4]
    fsub  dword ptr [ebx+120]
    fmul  dword ptr [ecx+C0_OFF+1*4]
  
    fld   dword ptr [ebx+56]
    fsub  dword ptr [ebx+68]
    fmul  dword ptr [ecx+C0_OFF+14*4]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+1*4]
    fxch  st(1)
    fstp  dword ptr [eax+1*4]
    fstp  dword ptr [eax+8*4+1*4]
  
    fld   dword ptr [ebx+8]
    fsub  dword ptr [ebx+116]
    fmul  dword ptr [ecx+C0_OFF+2*4]
  
    fld   dword ptr [ebx+52]
    fsub  dword ptr [ebx+72]
    fmul  dword ptr [ecx+C0_OFF+13*4]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+2*4]
    fxch  st(1)
    fstp  dword ptr [eax+2*4]
    fstp  dword ptr [eax+8*4+2*4]
  
    fld   dword ptr [ebx+12]
    fsub  dword ptr [ebx+112]
    fmul  dword ptr [ecx+C0_OFF+3*4]
  
    fld   dword ptr [ebx+48]
    fsub  dword ptr [ebx+76]
    fmul  dword ptr [ecx+C0_OFF+12*4]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+3*4]
    fxch  st(1)
    fstp  dword ptr [eax+3*4]
    fstp  dword ptr [eax+8*4+3*4]
  
    fld   dword ptr [ebx+16]
    fsub  dword ptr [ebx+108]
    fmul  dword ptr [ecx+C0_OFF+4*4]
  
    fld   dword ptr [ebx+44]
    fsub  dword ptr [ebx+80]
    fmul  dword ptr [ecx+C0_OFF+11*4]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+4*4]
    fxch  st(1)
    fstp  dword ptr [eax+4*4]
    fstp  dword ptr [eax+8*4+4*4]
  
    fld   dword ptr [ebx+20]
    fsub  dword ptr [ebx+104]
    fmul  dword ptr [ecx+C0_OFF+5*4]
  
    fld   dword ptr [ebx+40]
    fsub  dword ptr [ebx+84]
    fmul  dword ptr [ecx+C0_OFF+10*4]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+5*4]
    fxch  st(1)
    fstp  dword ptr [eax+5*4]
    fstp  dword ptr [eax+8*4+5*4]
  
    fld   dword ptr [ebx+24]
    fsub  dword ptr [ebx+100]
    fmul  dword ptr [ecx+C0_OFF+6*4]
  
    fld   dword ptr [ebx+36]
    fsub  dword ptr [ebx+88]
    fmul  dword ptr [ecx+C0_OFF+9*4]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+6*4]
    fxch  st(1)
    fstp  dword ptr [eax+6*4]
    fstp  dword ptr [eax+8*4+6*4]
  
    fld   dword ptr [ebx+28]
    fsub  dword ptr [ebx+96]
    fmul  dword ptr [ecx+C0_OFF+7*4]
  
    fld   dword ptr [ebx+32]
    fsub  dword ptr [ebx+92]
    fmul  dword ptr [ecx+C0_OFF+8*4]
  
    fld   st(1)
    fsub  st(0),st(1)
    fxch  st(1)
    faddp st(2),st(0)
    fmul  dword ptr [ecx+C1_OFF+7*4]
    fxch  st(1)
    fstp  dword ptr [eax+7*4]
    fstp  dword ptr [eax+8*4+7*4]
  
    COST4E([eax],[edx+8*4+0*4],[edx+8*4+4*4],[edx+8*4+2*4],[edx+8*4+6*4])
    COST4O([eax],[edx+8*4+1*4],[edx+8*4+5*4],[edx+8*4+3*4],[edx+8*4+7*4])
  
    COST4E([eax+8*4],[edx+0*4],[edx+4*4],[edx+2*4],[edx+6*4])
    COST4O([eax+8*4],[edx+1*4],[edx+5*4],[edx+3*4],[esi+124])
  
  
    fld   dword ptr [esi+124]
    fld   dword ptr [edx+6*4]      
    fadd  st(1),st(0)        
    fld   dword ptr [edx+5*4]      
    fadd  st(1),st(0)        
    fld   dword ptr [edx+4*4]      
    fadd  st(1),st(0)        
    fld   dword ptr [edx+3*4]      
    fadd  st(1),st(0)
    fld   dword ptr [edx+2*4]
    fadd  st(1),st(0)
    fld   dword ptr [edx+1*4]
    fadd  st(1),st(0)
    fadd  dword ptr [edx+0*4]
  
  
    fld   dword ptr [edx+8*4+0*4]      
    fadd  st(0),st(1)
    fstp  dword ptr [esi+60]
  
  
    fld   dword ptr [edx+8*4+1*4]      
    fadd  st(1),st(0)
    fadd  st(0),st(2)
    fxch  st(1)
    fstp  dword ptr [esi+52]
    fstp  dword ptr [esi+44]
  
  
    fld   dword ptr [edx+8*4+2*4]      
    fadd  st(1),st(0)
    fadd  st(0),st(2)
  
    fld   dword ptr [edx+8*4+3*4]
    fadd  st(3),st(0)
    fadd  st(0),st(4)
  
    fxch  st(2)
    fstp  dword ptr [esi+36]
    fstp  dword ptr [esi+28]
    fstp  dword ptr [esi+12]
    fstp  dword ptr [esi+20]
  
  
    fld   dword ptr [edx+8*4+4*4]      
    fadd  st(1),st(0)
    fadd  st(0),st(2)
  
    fld   dword ptr [edx+8*4+5*4]
    fadd  st(3),st(0)
    fadd  st(0),st(4)
  
    fxch  st(2)
    fstp  dword ptr [esi+4]
    fstp  dword ptr [esi+68]
    fstp  dword ptr [esi+84]
    fstp  dword ptr [esi+76]
  
  
    fld   dword ptr [edx+8*4+6*4]       
    fadd  st(1),st(0)
    fadd  st(0),st(2)
  
    fld   dword ptr [edx+8*4+7*4]
    fadd  st(3),st(0)
    fadd  dword ptr [esi+124]
  
    fxch  st(2)
    fstp  dword ptr [esi+92]
    fstp  dword ptr [esi+100]
    fstp  dword ptr [esi+116]
    fstp  dword ptr [esi+108]
    }
}

/*-------------------------------------------------------------------------*/

#endif /* ifndef USE_ASM */

/*-------------------------------------------------------------------------*/
