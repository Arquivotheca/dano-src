#include "BG_Erreur.h"
#include "BG_Types3D.h"
#include "BG_Memory.h"
#include "BG_Graphic.h"
#include "BG_Matrice.h"
#include "BG_Projection.h"
#include "BG_PaletteManager.h"
#include "BG_Math.h"
#include "BG_Ground.h"
#include "BG_AirCraft.h"
#include "BG_Main.h"

// helicoptere
BG_Pt3D BG_ListPt0[] =
	{
		{	8,	0,	-3	},
		{	4,	-2,	1	},
		{	4,	2,	1	},
		{	2,	0,	-3	},
		{	-8,	0,	1	},
		{	-5,	0,	1	},
		{	-10,0,	5	},
		{	2,	0,	2	},
		{	9,	-1,	2	},
		{	9,	1,	2	},
		{	3,	-7,	2	},
		{	1,	-7,	2	},
		{	-5,	-1,	2	},
		{	-5,	1,	2	},
		{	3,	7,	2	},
		{	1,	7,	2	}
	};
	
BG_Pt3D BG_ListVect0[] =
	{
		{	0,	0,	1	},
		{	1,	0,	0	}
	};
	
unsigned char BG_ListFace0[] =
	{
	0,	1,	2,	190,140,100,	// 0
	0,	3,	1,	190,80,	140,
	0,	2,	3,	190,100,80,
	1,	4,	2,	140,130,100,
	2,	4,	3,	100,130,80,
	3,	4,	1,	80,	130,140,
	0xff,
	7,	10,	11,	150,190,190,	// 37
	7,	11,	10,	150,190,190,
	7,	12,	13,	150,190,190,
	7,	13,	12,	150,190,190,
	7,	14,	15,	150,190,190,
	7,	15,	14,	150,190,190,
	7,	8,	9,	150,190,190,
	7,	9,	8,	150,190,190,
	0xff,
	4,	5,	6,	130,90,150,	// 86
	4,	6,	5,	130,170,190,
	0xff
	};

BG_BSP BG_ListBSP0[] =
	{
		{	5,	0,	-1,	0	},
		{	5,	1,	86,	37	}
	};
	
// airplane
BG_Pt3D BG_ListPt1[] =
	{
		{	10,	0,	-2	},
		{	4,	-2,	-2	},
		{	4,	0,	1	},
		{	4,	2,	-2	},
		{	4,	-1,	0	},
		{	4,	1,	0	},
		{	-2,	-1,	0	},
		{	-3,	-8,	0	},
		{	-2,	1,	0	},
		{	-3,	8,	0	},
		{	-8,	0,	0	},
		{	-4,	0,	0.334	},
		{	-8, 0,	0	},
		{	-10,0,	4	},
		{	-9,-3,	2	},
		{	-9,0,	2	},
		{	-9,3,	2	},
		{	-6,	0,	2	}
	};
	
BG_Pt3D BG_ListVect1[] =
	{
		{	-1,	0,	12	},
		{	0,	-1,	1	},
		{	0,	1,	1	},
		{	0,	-1,	0	},
		{	0,	1,	0	}
	};
	
unsigned char BG_ListFace1[] =
	{
	0,	1,	2,	150,190,140,	// 0
	0,	2,	3,	150,140,110,
	0,	3,	1,	150,110,190,
	1,	10,	2,	190,80,	140,
	2,	10,	3,	140,80,	110,
	3,	10,	1,	110,80,	190,
	0xff,
	4,	7,	6,	160,190,120,	// 37
	4,	6,	7,	120,150,80,
	0xff,
	5,	8,	9,	160,190,120,	// 50
	5,	9,	8,	120,150,80,
	0xff,
	11,	12,	13,	170,110,150,	// 63
	11,	13,	12,	140,120,80,
	0xff,
	14,	15,	17,	160,190,120,	// 76
	14,	17,	15,	120,150,80,
	0xff,
	17,	15,	16,	160,190,120,	// 89
	17,	16,	15,	120,150,80,
	0xff
	};

BG_BSP BG_ListBSP1[] =
	{
		{	10,	0,	-3,	-1	},
		{	4,	1,	37,	-2	},
		{	5,	2,	50,	0	},
		{	12,	3,	76,	-4	},
		{	12,	4,	89,	63	}
	};

// missile
BG_Pt3D BG_ListPt2[] =
	{
		{	-2,	0.5, -0.2	},
		{	-2, 0, 	-0.6	},
		{	-2, -0.5, -0.2	},
		{	3,	0,	0	}
	};
	
BG_Pt3D BG_ListVect2[] =
	{
		{	0,	0,	1	}
	};
	
unsigned char BG_ListFace2[] =
	{
	0,	1,	2,	230,190,160,	// 0
	3,	1,	0,	130,190,230,
	0xff,
	3,	2,	1,	130,160,190,	// 13
	3,	0,	2,	130,230,160,
	0xff
	};

BG_BSP BG_ListBSP2[] =
	{
		{	4,	0,	0,	13	}
	};

// explosion
BG_Pt3D BG_ListPt3[] =
	{
		{	0,	0,	15	},
		{	0,	15,	0	},
		{	15,	0,	0	},
		{	0,	-15,0	},
		{	-15,0,	0	},
		{	0,	0,	-15	},
	};
	
BG_Pt3D BG_ListVect3[] =
	{
		{	0,	0,	1	}
	};
	
unsigned char BG_ListFace3[] =
	{
	0,	1,	2,	230,210,190,	// 0
	0,	2,	3,	230,190,170,
	0,	3,	4,	230,170,150,
	0,	4,	1,	230,150,210,
	0xff,
	5,	2,	1,	130,190,210,	// 25
	5,	3,	2,	130,170,190,
	5,	4,	3,	130,150,170,
	5,	1,	4,	130,210,150,
	0xff
	};

BG_BSP BG_ListBSP3[] =
	{
		{	4,	0,	0,	25	}
	};

BG_Matrice3_3 Rot0 = {1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0};

// globals
static float		curRayon;
static char			GroundColor;

//***********************************************************
// Descripteur des ? appareils
BG_Aircraft		BG_ListAppareil[BG_NBAIRCRAFTMAX];

/***********************************************************
* What :	Create an aircraft	
*			
* State : 	In progress
* Last Modif :	20/10/95 (Pierre)
*					
***********************************************************/
static float TypeToRayon[4] = {0.1,0.1,0.02,0.0};
static float TypeToDuree[4] = {1e6,1e6,10.0,1.2};
int BG_NewAircraft(int Type,int Color,BG_Pt3D *Centre,BG_Matrice3_3 *Rot,float Vitesse)
	{
	int			index;
	BG_Aircraft	*Obj;
	
	for (index=0;index<BG_NBAIRCRAFTMAX;index++)
		if (BG_ListAppareil[index].Type < 0)
			{
			Obj = BG_ListAppareil+index;
			Obj->Centre.X = Centre->X;
			Obj->Centre.Y = Centre->Y;
			Obj->Centre.Z = Centre->Z;
			Obj->Vitesse = Vitesse;
			Obj->Type = Type;
			Obj->Color = Color;
			Obj->Rayon = TypeToRayon[Type];
			Obj->Duree = TypeToDuree[Type];
			BG_BlockMove((char*)Rot,(char*)&Obj->Rotation,sizeof(BG_Matrice3_3));
			BG_AddObject(Obj);
			return index;
			}
	return -1;
	}

/***********************************************************
* What :	Move an aircraft	
*			
* State : 	In progress
* Last Modif :	20/10/95 (Pierre)
*					
***********************************************************/
void BG_BoumAircraft(BG_Aircraft *Obj,int choc)
	{
	if (choc & 4)
		{
		if (Obj->Color == 6)
		 	BG_PtCampA++;
		else
			BG_PtCampB++;
		}
	if (Obj->Type != 2)
		{
		if (Obj->Color == 6)
		 	BG_PtCampB+=5;
		else
			BG_PtCampA+=5;
		}
	Obj->Type = -1;
	if ((choc == 1) && (GroundColor == 0))
		BG_NewAircraft(3,0,&Obj->Centre,&Rot0,0.0);
	else
		BG_NewAircraft(3,Obj->Color,&Obj->Centre,&Rot0,0.0);
	}
	
/***********************************************************
* What :	Move an aircraft	
*			
* State : 	In progress
* Last Modif :	20/10/95 (Pierre)
*					
***********************************************************/
void BG_MoveAircraft(BG_Aircraft *Obj,float deltaT)
	{
	int			i,pas,choc;
	float		vitesse;
	
	vitesse = Obj->Vitesse*deltaT;
// remove link between the object and the ground
	BG_RemoveObject(Obj);
// move the aircraft (by small step)
	pas = (int)((vitesse+0.049)*(1/0.05));
	vitesse /= (float)pas;
	curRayon = Obj->Rayon;
	for (;pas>0;pas--)
		{
		Obj->Centre.X += vitesse*Obj->Rotation.X1_Y1;
		Obj->Centre.Y += vitesse*Obj->Rotation.X2_Y1;
		Obj->Centre.Z += vitesse*Obj->Rotation.X3_Y1;
		choc = BG_CheckContact(&Obj->Centre);
		if (choc != 0)
			{
			if ((Obj->Type == 2) || (choc & (6+BG_GroundCrash)))
				{
				BG_BoumAircraft(Obj,choc);
				return;
				}
			for (i=0;i<7;i++)
				{
				choc = BG_CheckContact(&Obj->Centre);
				if (choc == 0) break;
				if (choc & (6+BG_GroundCrash))
					{
					BG_BoumAircraft(Obj,choc);
					return;
					}
				}
			}
		}
	if (Obj->Centre.X < 0.0) Obj->Centre.X += BG_GROUNDDH;
	else if (Obj->Centre.X >= BG_GROUNDDH) Obj->Centre.X -= BG_GROUNDDH;
	if (Obj->Centre.Y < 0.0) Obj->Centre.Y += BG_GROUNDDV;
	else if (Obj->Centre.Y >= BG_GROUNDDV) Obj->Centre.Y -= BG_GROUNDDV;
// build link between ground and object back
	BG_AddObject(Obj);
	}

/***********************************************************
* What :	Move an aircraft	
*			
* State : 	In progress
* Last Modif :	20/10/95 (Pierre)
*					
***********************************************************/
void BG_RemoveObject(BG_Aircraft *Obj)
	{
	int				index;
	BG_Aircraft 	*link;
	
	index = INDEX0((int)Obj->Centre.X,(int)Obj->Centre.Y);
// link in first place
	if (BG_VirtualGround[index].lien == Obj)
		{
		BG_VirtualGround[index].lien = Obj->next;
		return;
		}
// link in second place or more
	link = BG_VirtualGround[index].lien;
	while (link->next != Obj)
		link = link->next;
	link->next = Obj->next;
	}

/***********************************************************
* What :	Move an aircraft	
*			
* State : 	In progress
* Last Modif :	20/10/95 (Pierre)
*					
***********************************************************/
void BG_AddObject(BG_Aircraft *Obj)
	{
	int				index;
	
	index = INDEX0((int)Obj->Centre.X,(int)Obj->Centre.Y);
	Obj->next = BG_VirtualGround[index].lien;
	BG_VirtualGround[index].lien = Obj;
	}

/***********************************************************
* What :	Check ground contact
*			
* State : 	In progress
* Last Modif :	20/10/95 (Pierre)
*					
***********************************************************/
int BG_CheckContact(BG_Pt3D *Centre)
	{
	int				CaseH,CaseV;
	float			FracH,FracV,alt;
	Boolean			Contact;
	
// automatic calcul of the DrawingSquare
	CaseV = ((int)Centre->Y)&BG_GROUNDMASK;
	CaseH = ((int)Centre->X)&BG_GROUNDMASK;
	FracV = Centre->Y-(float)((int)Centre->Y);
	FracH = Centre->X-(float)((int)Centre->X);
	Contact = BG_CheckContactSquare(Centre,CaseH,CaseV);
	if (FracH < BG_RAYONAIRCRAFT)
		{
		Contact |= BG_CheckContactSquare(Centre,CaseH-1,CaseV);
		if (FracV < BG_RAYONAIRCRAFT)
			Contact |= BG_CheckContactSquare(Centre,CaseH-1,CaseV-1);
		else if (FracV > (1.0-BG_RAYONAIRCRAFT))
			Contact |= BG_CheckContactSquare(Centre,CaseH-1,CaseV+1);
		}
	else if (FracH > (1.0-BG_RAYONAIRCRAFT))
		{
		Contact |= BG_CheckContactSquare(Centre,CaseH+1,CaseV);
		if (FracV < BG_RAYONAIRCRAFT)
			Contact |= BG_CheckContactSquare(Centre,CaseH+1,CaseV-1);
		else if (FracV > (1.0-BG_RAYONAIRCRAFT))
			Contact |= BG_CheckContactSquare(Centre,CaseH+1,CaseV+1);
		}
	if (FracV < BG_RAYONAIRCRAFT)
		Contact |= BG_CheckContactSquare(Centre,CaseH,CaseV-1);
	else if (FracV > (1.0-BG_RAYONAIRCRAFT))
		Contact |= BG_CheckContactSquare(Centre,CaseH,CaseV+1);
// check that the object is up the ground
	alt = BG_GetGroundAlt(Centre->X,Centre->Y);
	if (Centre->Z < alt+curRayon)
		{
		Contact |= 1;
		Centre->Z = alt+curRayon;
		}
// result
	return Contact;
	}

/***********************************************************
* What :	Get ground's alt
*			
* State : 	In progress
* Last Modif :	20/10/95 (Pierre)
*					
***********************************************************/
float BG_GetGroundAlt(float X,float Y)
	{
	int				CaseH,CaseV;
	float			FracH,FracV;
	BG_Pt3D			Pt;
	BG_Pt3D			*Vect;
	BG_VGCase		*theSquare;
	
	CaseV = ((int)Y)&BG_GROUNDMASK;
	CaseH = ((int)X)&BG_GROUNDMASK;
	FracV = Y-(float)((int)Y);
	FracH = X-(float)((int)X);
	theSquare = BG_VirtualGround+INDEX0(CaseH,CaseV);
	if (theSquare->Orientation == 0)
		{
		Pt.X = 0.0;
		Pt.Y = 0.0;
		Pt.Z = BG_Altimetric[INDEX0(CaseH,CaseV)];
		if (FracH > FracV)
			Vect = theSquare->FNorm+0;
		else
			Vect = theSquare->FNorm+1;
		}
	else
		{
		Pt.X = 1.0;
		Pt.Y = 0.0;
		Pt.Z = BG_Altimetric[INDEX0(CaseH+1,CaseV)];
		if ((FracH+FracV) < 1.0)
			Vect = theSquare->FNorm+0;
		else
			Vect = theSquare->FNorm+1;
		}
// alt calculus
	return (Pt.Z+((Pt.X-FracH)*Vect->X+(Pt.Y-FracV)*Vect->Y)/Vect->Z);
	}

/***********************************************************
* What :	Check contact on a square
*			
* State : 	In progress
* Last Modif :	20/10/95 (Pierre)
*					
***********************************************************/
int BG_CheckContactSquare(BG_Pt3D *Centre,int i,int j)
	{
	float			dist,ecart;
	BG_Pt3D			P1,P2,P3,P4,Bonus;
	BG_Aircraft		*link;
	
	if (BG_VirtualGround[INDEX0(i,j)].Orientation == 0)
		{
		P1.X = (float)(i+1);
		P1.Y = (float)(j);
		P1.Z = BG_Altimetric[INDEX0(i+1,j)];
		P2.X = (float)(i+1);
		P2.Y = (float)(j+1);
		P2.Z = BG_Altimetric[INDEX0(i+1,j+1)];
		P3.X = (float)(i);
		P3.Y = (float)(j);
		P3.Z = BG_Altimetric[INDEX0(i,j)];
		P4.X = (float)(i);
		P4.Y = (float)(j+1);
		P4.Z = BG_Altimetric[INDEX0(i,j+1)];
		}
	else
		{
		P2.X = (float)(i+1);
		P2.Y = (float)(j);
		P2.Z = BG_Altimetric[INDEX0(i+1,j)];
		P4.X = (float)(i+1);
		P4.Y = (float)(j+1);
		P4.Z = BG_Altimetric[INDEX0(i+1,j+1)];
		P1.X = (float)(i);
		P1.Y = (float)(j);
		P1.Z = BG_Altimetric[INDEX0(i,j)];
		P3.X = (float)(i);
		P3.Y = (float)(j+1);
		P3.Z = BG_Altimetric[INDEX0(i,j+1)];
		}
// check contact against bonus
	if (BG_TableBonus[BG_VirtualGround[INDEX0(i,j)].bonus])
		{
		Bonus.X = 0.5*(P2.X+P3.X);
		Bonus.Y = 0.5*(P2.Y+P3.Y);
		Bonus.Z = 0.5*(P2.Z+P3.Z)+0.09;
		ecart = Centre->X-Bonus.X;
		dist = ecart*ecart;
		ecart = Centre->Y-Bonus.Y;
		dist += ecart*ecart;
		ecart = Centre->Z-Bonus.Z;
		dist += ecart*ecart;
		if (dist < ((BG_RAYONBONUS+curRayon)*(BG_RAYONBONUS+curRayon)))
			{
			BG_TableBonus[BG_VirtualGround[INDEX0(i,j)].bonus] = 0;
			BG_NbBonus--;
			BG_NewAircraft(3,5,&Bonus,&Rot0,0.0);
			return 4;
			}
		}
// check contact against object 
	link = BG_VirtualGround[INDEX0(i,j)].lien;
	while (link != 0L)
		{
		ecart = Centre->X-link->Centre.X;
		dist = ecart*ecart;
		ecart = Centre->Y-link->Centre.Y;
		dist += ecart*ecart;
		ecart = Centre->Z-link->Centre.Z;
		dist += ecart*ecart;
		if (dist < ((link->Rayon+curRayon)*(link->Rayon+curRayon)))
			{
			if (link->Type == 1)
				{
				if (link->Color == 6)
				 	BG_PtCampB+=10;
				else
					BG_PtCampA+=10;
				}
			BG_RemoveObject(link);
			link->Type = -1;
			BG_NewAircraft(3,link->Color,&link->Centre,&Rot0,0.0);
			return 2;
			}
		link = link->next;
		}
// check contact against ground
	return (BG_CheckChocTriangle(Centre,&P1,&P2,&P3,
								 BG_VirtualGround[INDEX0(i,j)].FNorm+0,
								 BG_VirtualGround[INDEX0(i,j)].Color[0]) |
			BG_CheckChocTriangle(Centre,&P4,&P3,&P2,
								 BG_VirtualGround[INDEX0(i,j)].FNorm+1,
								 BG_VirtualGround[INDEX0(i,j)].Color[1]));
	}


/***********************************************************
* What :	Solve BSP aircraft and draw triangle	
*			
* State : 	In progress
* Last Modif :	21/10/95 (Pierre)
*					
***********************************************************/
int BG_CheckChocTriangle(BG_Pt3D	*Centre,
						 BG_Pt3D	*P1,
						 BG_Pt3D	*P2,
						 BG_Pt3D	*P3,
						 BG_Pt3D	*norm,
						 char		Color)
	{
	int				imin,i;
	float			dist,pdist[3],dmin,binorme,scal;
	Boolean			Rebond;
	BG_Pt3D			Vtemp,Vtemp2,normales[3];

// test le contact potentiel sur cette plaque
	dist = (Centre->X-P1->X)*norm->X+(Centre->Y-P1->Y)*norm->Y+(Centre->Z-P1->Z)*norm->Z;
	if ((dist < 0.0) || (dist > curRayon)) return 0;
// prepare l'analyse de la facette principale
	Rebond = TRUE;
// calcule les pseudo-distances aux deux premiers plans delimitant la facette
	Vtemp2.X = P2->X-Centre->X;
	Vtemp2.Y = P2->Y-Centre->Y;
	Vtemp2.Z = P2->Z-Centre->Z;
	Vtemp.X = P2->X-P1->X;
	Vtemp.Y = P2->Y-P1->Y;
	Vtemp.Z = P2->Z-P1->Z;
	BG_ProduitVectoriel(&Vtemp,norm,normales+0);
	pdist[0] = normales[0].X*Vtemp2.X+normales[0].Y*Vtemp2.Y+normales[0].Z*Vtemp2.Z;
	if (pdist[0] < 0) Rebond = FALSE;
	Vtemp.X = P3->X-P2->X;
	Vtemp.Y = P3->Y-P2->Y;
	Vtemp.Z = P3->Z-P2->Z;
	BG_ProduitVectoriel(&Vtemp,norm,normales+1);
	pdist[1] = normales[1].X*Vtemp2.X+normales[1].Y*Vtemp2.Y+normales[1].Z*Vtemp2.Z;
	if (pdist[1] < 0) Rebond = FALSE;
	Vtemp2.X = P3->X-Centre->X;
	Vtemp2.Y = P3->Y-Centre->Y;
	Vtemp2.Z = P3->Z-Centre->Z;
	Vtemp.X = P1->X-P3->X;
	Vtemp.Y = P1->Y-P3->Y;
	Vtemp.Z = P1->Z-P3->Z;
	BG_ProduitVectoriel(&Vtemp,norm,normales+2);
	pdist[2] = normales[2].X*Vtemp2.X+normales[2].Y*Vtemp2.Y+normales[2].Z*Vtemp2.Z;
	if (pdist[2] < 0) Rebond = FALSE;
// traitement du rebond franc sur la facette
	if (Rebond == TRUE)
		{
	// eloigne le point a la distance souhaitee
		dmin = curRayon-dist;
		Centre->X += dmin*norm->X;
		Centre->Y += dmin*norm->Y;
		Centre->Z += dmin*norm->Z;
		GroundColor = Color;
		return 1;
		}
// recherche un rebond sur une arete
	dmin = 99999.0;
	for (i=0;i<3;i++)
		{
	// convertit les pseudo-distances en distances reelles
		scal =	normales[i].X*normales[i].X+
				normales[i].Y*normales[i].Y+
				normales[i].Z*normales[i].Z+0.0000000001;
		pdist[i] *= BG_InvSqrt(scal);
	// elimine les points hors-zone;
		if (pdist[i] >= -curRayon)
			{
			if (pdist[i] < dmin)
				{
				dmin = pdist[i];
				imin = i;
				}
			}
		else return 0;
		}
// effectue le test final de proximite
	if (dist-dmin < curRayon)
// execute le rebond sur l'arete appropriee
// pas de simulation de rebond sur les angles.
		{
		dist -= dmin;
		if (dist < 0.0001) dist = 0.0001;
	// recupere les deux points de l'arete
		switch (imin)
			{
			case 1 :	// 2->1, 3->2
				P1 = P2;
				P2 = P3;
				break;
			case 2 :	// 3->1, 1->2
				P2 = P1;
				P1 = P3;
				break;
			}
		Vtemp.X = Centre->X-P1->X;
		Vtemp.Y = Centre->Y-P1->Y;
		Vtemp.Z = Centre->Z-P1->Z;
		Vtemp2.X = P2->X-P1->X;
		Vtemp2.Y = P2->Y-P1->Y;
		Vtemp2.Z = P2->Z-P1->Z;
		binorme = (Vtemp2.X*Vtemp2.X+Vtemp2.Y*Vtemp2.Y+Vtemp2.Z*Vtemp2.Z);
		if (binorme > 0.0001)
			dmin = (Vtemp.X*Vtemp2.X+Vtemp.Y*Vtemp2.Y+Vtemp.Z*Vtemp2.Z)/binorme;
		else dmin = 0;
	// calcul le vecteur projete orthogonal du point sur l'arete
		Vtemp.X -= dmin*Vtemp2.X;
		Vtemp.Y -= dmin*Vtemp2.Y;
		Vtemp.Z -= dmin*Vtemp2.Z;
	// eloigne le point dans cette direction
		dmin = curRayon/dist-1.0;
		Centre->X += dmin*Vtemp.X;
		Centre->Y += dmin*Vtemp.Y;
		Centre->Z += dmin*Vtemp.Z;
		GroundColor = Color;
		return 1;
		}
	return 0;
	}



