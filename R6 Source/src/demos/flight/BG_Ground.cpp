#include "BG_Erreur.h"
#include "BG_Types3D.h"
#include "BG_Memory.h"
#include "BG_Graphic.h"
#include "BG_Matrice.h"
#include "BG_Math.h"
#include "BG_Ground.h"
#include "BG_Main.h"
#include "BG_Fractal.h"
#include "BG_Goureaud.h"
#include <string.h>

static BG_Pt3D	Soleil =
	{
	-3.0,	-2.0,	 6.0
	};

// table of color interaction
static char BG_ColorInterAction[8*8] =
	{
	1,	1,	0,	0,	0,	0,	0,	0,
	1,	1,	1,	1,	1,	0,	0,	0,
	0,	1,	1,	1,	1,	0,	0,	0,
	0,	1,	1,	1,	1,	0,	0,	0,
	0,	1,	1,	1,	1,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0
	};

// size of the working ground
float			*BG_Altimetric;
BG_VGCase		*BG_VirtualGround;
char			*BG_TableBonus;		// table d'etat des bonus
unsigned short	*BG_OffsetBonus;	// offset radar bonus
unsigned char	*BG_Lumieres[8];

int		BG_GROUNDDH = 128;
int		BG_GROUNDDV = 128;
int		BG_GROUNDMASK = 127;
int		BG_GROUNDSHIFT = 7;
float	BG_AltFact;

/***********************************************************
* What :	PreInit Ground model	
*			
* State : 	In progress
* Last Modif :	16/10/95 (Pierre)
*					
***********************************************************/
void BG_InitGroundModel()
	{
	BG_Altimetric = (float*)BG_GetMemLow(sizeof(float)*BG_GROUNDDH*BG_GROUNDDV);
	BG_VirtualGround = (BG_VGCase*)BG_GetMemLow(sizeof(BG_VGCase)*BG_GROUNDDH*BG_GROUNDDV);
	BG_TableBonus = (char*)BG_GetMemLow(sizeof(char)*(BG_NBBONUSMAX+1));
	BG_OffsetBonus = (unsigned short*)BG_GetMemLow(sizeof(short)*BG_NBBONUSMAX);
	BG_TableBonus[BG_NBBONUSMAX] = 0;
	}

/***********************************************************
* What :	Dispose Ground model	
*			
* State : 	In progress
* Last Modif :	17/10/95 (Pierre)
*					
***********************************************************/
void BG_DisposeGroundModel()
	{
	BG_FreeMemLow((char*)BG_Altimetric);
	}

/***********************************************************
* What :	Add Normales Triangle component from a VGCase 	
*			depending of real color of the point.
* State : 	In progress
* Last Modif :	16/10/95 (Pierre)
*					
***********************************************************/
void BG_AddCaseNormales(BG_Pt3D		*NCase1,
						BG_Pt3D		*NCase2,
						char		*CaseColor,
						char		orient,
						BG_Pt3D		*norm,
						int			pt,
						char		color)
	{
	int			mode;
	
// add normales of concerned triangles
	switch (pt)
		{
		case 0 :
			if (orient == 0)
				mode = 2;
			else
				mode = 0;
			break;
		case 1 :
			if (orient == 0)
				mode = 0;
			else
				mode = 2;
			break;
		case 2 :
			if (orient == 0)
				mode = 2;
			else
				mode = 1;
			break;
		case 3 :
			if (orient == 0)
				mode = 1;
			else
				mode = 2;
			break;
		}
	switch (mode)
		{
		case 0 :
			if (BG_ColorInterAction[CaseColor[0]*8+color] != 0)
				{
				norm->X += NCase1->X;
				norm->Y += NCase1->Y;
				norm->Z += NCase1->Z;
				}
			break;
		case 1 :
			if (BG_ColorInterAction[CaseColor[1]*8+color] != 0)
				{
				norm->X += NCase2->X;
				norm->Y += NCase2->Y;
				norm->Z += NCase2->Z;
				}
			break;
		case 2 :
			if (BG_ColorInterAction[CaseColor[0]*8+color] != 0)
				{
				norm->X += NCase1->X;
				norm->Y += NCase1->Y;
				norm->Z += NCase1->Z;
				}
			if (BG_ColorInterAction[CaseColor[1]*8+color] != 0)
				{
				norm->X += NCase2->X;
				norm->Y += NCase2->Y;
				norm->Z += NCase2->Z;
				}
			break;
		}
	}

/***********************************************************
* What :	Choose the color of a triangle	
*			
* State : 	In progress
* Last Modif :	17/10/95 (Pierre)
*					
***********************************************************/
char BG_ChooseColor(int i0,int i1,int i2)
	{
	float		alt0,alt1,alt2,sum;
	
	alt0 = BG_Altimetric[i0]*BG_AltFact;
	alt1 = BG_Altimetric[i1]*BG_AltFact;
	alt2 = BG_Altimetric[i2]*BG_AltFact;
	sum = alt0+alt1+alt2;
// case of water
	if (sum == 0.0) return 0;
// case of sand
	if ((alt0*alt1*alt2) == 0.0)
		return 1;
	if (((alt0 > 1.7) && (alt1 > 1.7) && (alt2 > 1.7)) || (sum > 4.3))
		{
		if ((sum > 7.8) || ((alt0 > 3.0) && (alt1 >  3.0) && (alt2 >  3.0)))
			return 4;
		else
			return 3;
		}
	return 2;
	}
	
/***********************************************************
* What :	Get the color of a border of a square	
*			
* State : 	In progress
* Last Modif :	20/10/95 (Pierre)
*					
***********************************************************/
char BG_GetBorderColor(int i,int j,int border)
	{
	int		i0;
	
	i0 = INDEX0(i,j);
	switch (border)
		{
		case 0 :
			return BG_VirtualGround[i0].Color[0];
			break;
		case 1 :
			if (BG_VirtualGround[i0].Orientation == 0)
				return BG_VirtualGround[i0].Color[0];
			else
				return BG_VirtualGround[i0].Color[1];
			break;
		case 2 :
			return BG_VirtualGround[i0].Color[1];
			break;
		case 3 :
			if (BG_VirtualGround[i0].Orientation == 0)
				return BG_VirtualGround[i0].Color[1];
			else
				return BG_VirtualGround[i0].Color[0];
			break;
		}
	return 0L;
	}

/***********************************************************
* What :	Select the best color depending the environment	
*			
* State : 	In progress
* Last Modif :	20/10/95 (Pierre)
*					
***********************************************************/
char BG_GetSmoothColor(char theColor,char colorH,char color0,char color1)
	{
	if ((color0 == colorH) || (color1 == colorH))
		return colorH;
	if (color0 == color1)
		return color0;
	return theColor;
	}
		
/***********************************************************
* What :	Eliminate bad color triangle	
*			
* State : 	In progress
* Last Modif :	20/10/95 (Pierre)
*					
***********************************************************/
void BG_SmoothColor(int i,int j)
	{
	int		i0;
	char 	color0,color1,colorH,theColor;
	
	i0 = INDEX0(i,j);
	switch (BG_VirtualGround[i0].Orientation)
		{
		case 0 :
			theColor = BG_VirtualGround[i0].Color[0];
			colorH = BG_VirtualGround[i0].Color[1];
			color0 = BG_GetBorderColor(i,j-1,2);
			color1 = BG_GetBorderColor(i+1,j,3);
			BG_VirtualGround[i0].Color[0] = BG_GetSmoothColor(theColor,colorH,color0,color1);
			theColor = BG_VirtualGround[i0].Color[1];
			colorH = BG_VirtualGround[i0].Color[0];
			color0 = BG_GetBorderColor(i,j+1,0);
			color1 = BG_GetBorderColor(i-1,j,1);
			BG_VirtualGround[i0].Color[1] = BG_GetSmoothColor(theColor,colorH,color0,color1);
			break;
		case 1 :
			theColor = BG_VirtualGround[i0].Color[0];
			colorH = BG_VirtualGround[i0].Color[1];
			color0 = BG_GetBorderColor(i,j-1,2);
			color1 = BG_GetBorderColor(i-1,j,1);
			BG_VirtualGround[i0].Color[0] = BG_GetSmoothColor(theColor,colorH,color0,color1);
			theColor = BG_VirtualGround[i0].Color[1];
			colorH = BG_VirtualGround[i0].Color[0];
			color0 = BG_GetBorderColor(i,j+1,0);
			color1 = BG_GetBorderColor(i+1,j,3);
			BG_VirtualGround[i0].Color[1] = BG_GetSmoothColor(theColor,colorH,color0,color1);
			break;
		}
	}
	
/***********************************************************
* What :	Process altimetric data into full defined ground	
*			
* State : 	In progress
* Last Modif :	17/10/95 (Pierre)
*					
***********************************************************/
void BG_ProcessVirtualGround()
	{
	int				i,j,i0,i1,i2,i3,a,b;
	float			sum0,sum1,scal;
	BRect			Draw(0.0,0.0,1000.0,1000.0);
	BG_Pt3D			v1,v2;
	BG_GoureaudPt	tab[4];

	BG_NormaliserVect(&Soleil);
// for each square
	for (i=0;i<BG_GROUNDDH;i++)
		for (j=0;j<BG_GROUNDDV;j++)
			{
		// preprocess the 4 working index
			i0 = INDEX0(i,j);
			i1 = INDEX1(i,j);
			i2 = INDEX2(i,j);
			i3 = INDEX3(i,j);
		// memorize point altitude
			BG_VirtualGround[i0].Altitude = BG_Altimetric[i0];
		// choose triangle orientation
			sum0 = BG_Altimetric[i0]+BG_Altimetric[i2];
			sum1 = BG_Altimetric[i1]+BG_Altimetric[i3];
			if (sum0 < sum1)
				BG_VirtualGround[i0].Orientation = 0;
			else
				BG_VirtualGround[i0].Orientation = 1;
		// initialise bonus and link
			BG_VirtualGround[i0].lien = 0L;
		// choose color
			switch (BG_VirtualGround[i0].Orientation)
				{
				case 0 :
					BG_VirtualGround[i0].Color[0] = BG_ChooseColor(i0,i1,i2);
					BG_VirtualGround[i0].Color[1] = BG_ChooseColor(i0,i3,i2);
					break;
				case 1 :
					BG_VirtualGround[i0].Color[0] = BG_ChooseColor(i3,i0,i1);
					BG_VirtualGround[i0].Color[1] = BG_ChooseColor(i3,i2,i1);
					break;
				}
		// calculate triangle normales
			switch (BG_VirtualGround[i0].Orientation)
				{
				case 0 :
				// first normale
					v1.X = 1.0;
					v1.Y = 0.0;
					v1.Z = BG_Altimetric[i1]-BG_Altimetric[i0];
					v2.X = 0.0;
					v2.Y = 1.0;
					v2.Z = BG_Altimetric[i2]-BG_Altimetric[i1];
					BG_ProduitVectoriel(&v1,&v2,BG_VirtualGround[i0].FNorm+0);
					BG_NormaliserVect(BG_VirtualGround[i0].FNorm+0);
				// second normale
					v1.X = 1.0;
					v1.Y = 0.0;
					v1.Z = BG_Altimetric[i2]-BG_Altimetric[i3];
					v2.X = 0.0;
					v2.Y = 1.0;
					v2.Z = BG_Altimetric[i3]-BG_Altimetric[i0];
					BG_ProduitVectoriel(&v1,&v2,BG_VirtualGround[i0].FNorm+1);
					BG_NormaliserVect(BG_VirtualGround[i0].FNorm+1);
					break;
				case 1 :
				// first normale
					v1.X = 1.0;
					v1.Y = 0.0;
					v1.Z = BG_Altimetric[i1]-BG_Altimetric[i0];
					v2.X = 0.0;
					v2.Y = 1.0;
					v2.Z = BG_Altimetric[i3]-BG_Altimetric[i0];
					BG_ProduitVectoriel(&v1,&v2,BG_VirtualGround[i0].FNorm+0);
					BG_NormaliserVect(BG_VirtualGround[i0].FNorm+0);
				// second normale
					v1.X = 1.0;
					v1.Y = 0.0;
					v1.Z = BG_Altimetric[i2]-BG_Altimetric[i3];
					v2.X = 0.0;
					v2.Y = 1.0;
					v2.Z = BG_Altimetric[i2]-BG_Altimetric[i1];
					BG_ProduitVectoriel(&v1,&v2,BG_VirtualGround[i0].FNorm+1);
					BG_NormaliserVect(BG_VirtualGround[i0].FNorm+1);
					break;
				}
			}
// smooth the color
	for (i=0;i<BG_GROUNDDH;i++)
		for (j=0;j<BG_GROUNDDV;j++)
			BG_SmoothColor(i,j);
// calculate point normales
	for (i=0;i<BG_GROUNDDH;i++)
		for (j=0;j<BG_GROUNDDV;j++)
			{
			i0 = INDEX0(i,j);
			for (a=0;a<=1;a++)
				for (b=0;b<=1;b++)
					{
					v1.X = v1.Y = v1.Z = 0.0;
					i1 = INDEX0(i+a-1,j+b-1);
					BG_AddCaseNormales(BG_VirtualGround[i1].FNorm+0,
									   BG_VirtualGround[i1].FNorm+1,
									   BG_VirtualGround[i1].Color,
									   BG_VirtualGround[i1].Orientation,
									   &v1,2,BG_VirtualGround[i0].Color[0]);
					i1 = INDEX1(i+a-1,j+b-1);
					BG_AddCaseNormales(BG_VirtualGround[i1].FNorm+0,
									   BG_VirtualGround[i1].FNorm+1,
									   BG_VirtualGround[i1].Color,
									   BG_VirtualGround[i1].Orientation,
									   &v1,3,BG_VirtualGround[i0].Color[0]);
					i1 = INDEX3(i+a-1,j+b-1);
					BG_AddCaseNormales(BG_VirtualGround[i1].FNorm+0,
									   BG_VirtualGround[i1].FNorm+1,
									   BG_VirtualGround[i1].Color,
									   BG_VirtualGround[i1].Orientation,
									   &v1,1,BG_VirtualGround[i0].Color[1]);
					i1 = INDEX2(i+a-1,j+b-1);
					BG_AddCaseNormales(BG_VirtualGround[i1].FNorm+0,
									   BG_VirtualGround[i1].FNorm+1,
									   BG_VirtualGround[i1].Color,
									   BG_VirtualGround[i1].Orientation,
									   &v1,0,BG_VirtualGround[i0].Color[1]);
					BG_NormaliserVect(&v1);
				// reindex normales point in global table
					scal = v1.X*Soleil.X+v1.Y*Soleil.Y+v1.Z*Soleil.Z;
					if (scal < 0.0) scal = 0.0;
					BG_VirtualGround[i0].Normales[(3*b)^a] =
						BG_LOWBORDER+(int)((256-BG_LOWBORDER-BG_HIGHBORDER)*scal);
					}
			}
// draw ground map
	OffScreen_baseAddr = (long)BG_OffscreenMap;
	if (BG_GROUNDDH == 32) a = 3;
	else if (BG_GROUNDDH == 64) a = 2;
	else a = 1;
	if (BG_Screen24)
	{
		OffScreen_largeur = 1024;
		for (i=0;i<BG_GROUNDDH;i++)
			for (j=0;j<BG_GROUNDDV;j++)
			{
				i0 = INDEX0(i,j);
				if (BG_VirtualGround[i0].Orientation == 0)
				{
					tab[0].H = i<<a;
					tab[0].V = 256-(j<<a);
					tab[0].Level = BG_VirtualGround[i0].Normales[0];
					tab[2].H = (i+1)<<a;
					tab[2].V = 256-(j<<a);
					tab[2].Level = BG_VirtualGround[i0].Normales[1];
					tab[1].H = (i+1)<<a;
					tab[1].V = 256-((j+1)<<a);
					tab[1].Level = BG_VirtualGround[i0].Normales[2];
					BG_Goureaud24(tab,BG_Lumieres[BG_VirtualGround[i0].Color[0]]);
					//				tab[2].H = (i+1)<<a;
					tab[2].V = 256-((j+1)<<a);
					tab[2].Level = BG_VirtualGround[i0].Normales[2];
					tab[1].H = i<<a;
					//				tab[1].V = 256-((j+1)<<a);
					tab[1].Level = BG_VirtualGround[i0].Normales[3];
					BG_Goureaud24(tab,BG_Lumieres[BG_VirtualGround[i0].Color[1]]);
				}
				else
				{
					tab[0].H = i<<a;
					tab[0].V = 256-(j<<a);
					tab[0].Level = BG_VirtualGround[i0].Normales[0];
					tab[2].H = (i+1)<<a;
					tab[2].V = 256-(j<<a);
					tab[2].Level = BG_VirtualGround[i0].Normales[1];
					tab[1].H = i<<a;
					tab[1].V = 256-((j+1)<<a);
					tab[1].Level = BG_VirtualGround[i0].Normales[3];
					BG_Goureaud24(tab,BG_Lumieres[BG_VirtualGround[i0].Color[0]]);
					tab[0].H = (i+1)<<a;
					//				tab[0].V = 256-(j<<a);
					tab[0].Level = BG_VirtualGround[i0].Normales[1];
					//				tab[2].H = (i+1)<<a;
					tab[2].V = 256-((j+1)<<a);
					tab[2].Level = BG_VirtualGround[i0].Normales[2];
					BG_Goureaud24(tab,BG_Lumieres[BG_VirtualGround[i0].Color[1]]);
				}
			}
	}
	else
	{
		OffScreen_largeur = 256;
		for (i=0;i<BG_GROUNDDH;i++)
			for (j=0;j<BG_GROUNDDV;j++)
			{
				i0 = INDEX0(i,j);
				if (BG_VirtualGround[i0].Orientation == 0)
				{
					tab[0].H = i<<a;
					tab[0].V = 256-(j<<a);
					tab[0].Level = BG_VirtualGround[i0].Normales[0];
					tab[2].H = (i+1)<<a;
					tab[2].V = 256-(j<<a);
					tab[2].Level = BG_VirtualGround[i0].Normales[1];
					tab[1].H = (i+1)<<a;
					tab[1].V = 256-((j+1)<<a);
					tab[1].Level = BG_VirtualGround[i0].Normales[2];
					BG_Goureaud(tab,BG_Lumieres[BG_VirtualGround[i0].Color[0]]);
					//				tab[2].H = (i+1)<<a;
					tab[2].V = 256-((j+1)<<a);
					tab[2].Level = BG_VirtualGround[i0].Normales[2];
					tab[1].H = i<<a;
					//				tab[1].V = 256-((j+1)<<a);
					tab[1].Level = BG_VirtualGround[i0].Normales[3];
					BG_Goureaud(tab,BG_Lumieres[BG_VirtualGround[i0].Color[1]]);
				}
				else
				{
					tab[0].H = i<<a;
					tab[0].V = 256-(j<<a);
					tab[0].Level = BG_VirtualGround[i0].Normales[0];
					tab[2].H = (i+1)<<a;
					tab[2].V = 256-(j<<a);
					tab[2].Level = BG_VirtualGround[i0].Normales[1];
					tab[1].H = i<<a;
					tab[1].V = 256-((j+1)<<a);
					tab[1].Level = BG_VirtualGround[i0].Normales[3];
					BG_Goureaud(tab,BG_Lumieres[BG_VirtualGround[i0].Color[0]]);
					tab[0].H = (i+1)<<a;
					//				tab[0].V = 256-(j<<a);
					tab[0].Level = BG_VirtualGround[i0].Normales[1];
					//				tab[2].H = (i+1)<<a;
					tab[2].V = 256-((j+1)<<a);
					tab[2].Level = BG_VirtualGround[i0].Normales[2];
					BG_Goureaud(tab,BG_Lumieres[BG_VirtualGround[i0].Color[1]]);
				}
			}
	}
}
	
/***********************************************************
* What :	Put bonus in the low part of the ground	
*			
* State : 	In progress
* Last Modif :	06/11/95 (Pierre)
*					
***********************************************************/
void BG_AddBonus(long RamdomKey)
	{
	int				i,NbBonus,piste,refus;
	unsigned long	rand;

// reset bonus marker
	for (i=0;i<BG_GROUNDDH*BG_GROUNDDV;i++)
		BG_VirtualGround[i].bonus = BG_NBBONUSMAX;
// create bonus	
	NbBonus = BG_GROUNDDH-20;
	piste = (BG_GROUNDDH+80)/13;
	rand = RamdomKey|1;
	refus = 0L;
	while (NbBonus>0)
		{
		rand<<=1;
		if (rand & 0x8000000) rand ^= 0x05EF037B;
		if (BG_PutBonus(rand & BG_GROUNDMASK, (rand>>12) & BG_GROUNDMASK,piste,NbBonus-1))
			{
			NbBonus--;
			refus = 0;
			}
		else
			{
			refus++;
			if (refus > 32)
				{
				refus = 0;
				piste--;
				if (piste == 0) break;
				}
			}
		}
	}

/***********************************************************
* What :	Try to put a bonus from the case h,v. 
*			
* State : 	In progress
* Last Modif :	06/11/95 (Pierre)
*					
***********************************************************/
Boolean BG_PutBonus(int h,int v,int piste,int iBonus)
	{
	int			i,h0,v0;
	float		min,val,pas;
	
	for (i=0;i<piste;i++)
		{
	// look for the min of the 8 neighboors
		min = BG_Altimetric[INDEX0(h-1,v-1)];
		h0 = h-1;
		v0 = v-1;
		val = BG_Altimetric[INDEX0(h-1,v)];
		if (val < min) {min = val;h0 = h-1;v0 = v;}
		val = BG_Altimetric[INDEX0(h-1,v+1)];
		if (val < min) {min = val;h0 = h-1;v0 = v+1;}
		val = BG_Altimetric[INDEX0(h,v+1)];
		if (val < min) {min = val;h0 = h;v0 = v+1;}
		val = BG_Altimetric[INDEX0(h+1,v+1)];
		if (val < min) {min = val;h0 = h+1;v0 = v+1;}
		val = BG_Altimetric[INDEX0(h+1,v)];
		if (val < min) {min = val;h0 = h+1;v0 = v;}
		val = BG_Altimetric[INDEX0(h+1,v-1)];
		if (val < min) {min = val;h0 = h+1;v0 = v-1;}
		val = BG_Altimetric[INDEX0(h,v-1)];
		if (val < min) {min = val;h0 = h;v0 = v-1;}
	// move
		h = h0;
		v = v0;
	// abort if touch water
		if (BG_Altimetric[INDEX0(h,v)] == 0.0) return FALSE;
		}
// abort if end on a bonus
	if (BG_VirtualGround[INDEX0(h0,v0)].bonus != BG_NBBONUSMAX) return FALSE;
// abort if ground too irregular
	min = BG_Altimetric[INDEX0(h0,v0)]+0.15;
	if ((BG_Altimetric[INDEX0(h0+1,v0+1)] > min) ||
		(BG_Altimetric[INDEX0(h0,v0+1)] > min) ||
		(BG_Altimetric[INDEX0(h0+1,v0)] > min)) return FALSE;
// create a bonus
	BG_VirtualGround[INDEX0(h0,v0)].bonus = iBonus;
	BG_TableBonus[iBonus] = 1;
	if (BG_GROUNDDH == 32) pas = 8.0;
	else if (BG_GROUNDDH == 64) pas = 4.0;
	else pas = 2.0;
	h = (int)((h0+0.5)*pas)&255;
	v = (256-(int)((v0+0.5)*pas))&255;
	BG_OffsetBonus[iBonus] = h+256*v;
	BG_NbBonus++;
	return TRUE;
	}
	
