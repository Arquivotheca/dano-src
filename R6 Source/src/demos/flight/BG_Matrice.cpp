#include <math.h>
#include "BG_Types3D.h"
#include "BG_Math.h"
#include "BG_Matrice.h"

//*********************************************************************************
// Calcul d'une matrice de rotation 3D en fonction de ses angles
// Etat : Stable
// Derniére Modif : 22/9/94 (Pierre)
//*********************************************************************************
void BG_Matrice3_3Rotation3D(short direction,short inclinaison,short assiette,BG_Matrice3_3 *m)
	{
	m->X1_Y1=fcos(direction)*fcos(inclinaison);
	m->X2_Y1=-fsin(direction)*fcos(inclinaison);
	m->X3_Y1=-fsin(inclinaison);
	m->X1_Y2=fsin(direction)*fcos(assiette)-fcos(direction)*fsin(inclinaison)*fsin(assiette);
	m->X2_Y2=fcos(direction)*fcos(assiette)+fsin(direction)*fsin(inclinaison)*fsin(assiette);
	m->X3_Y2=-fsin(assiette)*fcos(inclinaison);
	m->X1_Y3=fsin(direction)*fsin(assiette)+fcos(direction)*fcos(assiette)*fsin(inclinaison);
	m->X2_Y3=fcos(direction)*fsin(assiette)-fsin(direction)*fsin(inclinaison)*fcos(assiette);
	m->X3_Y3=fcos(inclinaison)*fcos(assiette);
	}

//*********************************************************************************
// Calcul d'une matrice de rotation 3D en fonction de ses angles, et d'un facteur d'echelle
// Etat : Stable
// Derniére Modif : 25/07/94 (Pierre)
//*********************************************************************************
void BG_Matrice3_3Rotation3DEchelle(short direction,short inclinaison,short assiette,BG_Matrice3_3 *m,float echelle)
	{
	m->X1_Y1=(fcos(direction)*fcos(inclinaison))*echelle;
	m->X2_Y1=(-fsin(direction)*fcos(inclinaison))*echelle;
	m->X3_Y1=(-fsin(inclinaison))*echelle;
	m->X1_Y2=(fsin(direction)*fcos(assiette)-fcos(direction)*fsin(inclinaison)*fsin(assiette))*echelle;
	m->X2_Y2=(fcos(direction)*fcos(assiette)+fsin(direction)*fsin(inclinaison)*fsin(assiette))*echelle;
	m->X3_Y2=(-fsin(assiette)*fcos(inclinaison))*echelle;
	m->X1_Y3=(fsin(direction)*fsin(assiette)+fcos(direction)*fcos(assiette)*fsin(inclinaison))*echelle;
	m->X2_Y3=(fcos(direction)*fsin(assiette)-fsin(direction)*fsin(inclinaison)*fcos(assiette))*echelle;
	m->X3_Y3=(fcos(inclinaison)*fcos(assiette))*echelle;
	}

//*********************************************************************************
// Produit matriciel 3*3
// Etat : Stable
// Derniére Modif : 11/10/94 (Pierre)
//*********************************************************************************
void BG_Matrice3_3MultMatrice3_3(BG_Matrice3_3 m1,BG_Matrice3_3 m2,BG_Matrice3_3 *m)
	{
	float	r11,r12,r13,r21,r22,r23,r31,r32,r33;
	
	r11 = m1.X1_Y1*m2.X1_Y1;
	r21 = m1.X1_Y1*m2.X2_Y1;
	r31 = m1.X1_Y1*m2.X3_Y1;
	r12 = m1.X1_Y2*m2.X1_Y1;
	r22 = m1.X1_Y2*m2.X2_Y1;
	r32 = m1.X1_Y2*m2.X3_Y1;
	r13 = m1.X1_Y3*m2.X1_Y1;
	r23 = m1.X1_Y3*m2.X2_Y1;
	r33 = m1.X1_Y3*m2.X3_Y1;
	
	r11 += m1.X2_Y1*m2.X1_Y2;
	r21 += m1.X2_Y1*m2.X2_Y2;
	r31 += m1.X2_Y1*m2.X3_Y2;
	r12 += m1.X2_Y2*m2.X1_Y2;
	r22 += m1.X2_Y2*m2.X2_Y2;
	r32 += m1.X2_Y2*m2.X3_Y2;
	r13 += m1.X2_Y3*m2.X1_Y2;
	r23 += m1.X2_Y3*m2.X2_Y2;
	r33 += m1.X2_Y3*m2.X3_Y2;
	
	r11 += m1.X3_Y1*m2.X1_Y3;
	r21 += m1.X3_Y1*m2.X2_Y3;
	r31 += m1.X3_Y1*m2.X3_Y3;
	r12 += m1.X3_Y2*m2.X1_Y3;
	r22 += m1.X3_Y2*m2.X2_Y3;
	r32 += m1.X3_Y2*m2.X3_Y3;
	r13 += m1.X3_Y3*m2.X1_Y3;
	r23 += m1.X3_Y3*m2.X2_Y3;
	r33 += m1.X3_Y3*m2.X3_Y3;
	
	m->X1_Y1 = r11;
	m->X2_Y1 = r21;
	m->X3_Y1 = r31;
	m->X1_Y2 = r12;
	m->X2_Y2 = r22;
	m->X3_Y2 = r32;
	m->X1_Y3 = r13;
	m->X2_Y3 = r23;
	m->X3_Y3 = r33;
	}

//*********************************************************************************
// Calcul du déterminant d'une matrice 3*3
// Etat : Stable
// Derniére Modif : 22/9/94 (Pierre)
//*********************************************************************************
float BG_Matrice3_3Determinant(BG_Matrice3_3 m)
	{
	return	m.X1_Y1*m.X2_Y2*m.X3_Y3+
			m.X1_Y2*m.X2_Y3*m.X3_Y1+
			m.X1_Y3*m.X2_Y1*m.X3_Y2-
			m.X1_Y1*m.X2_Y3*m.X3_Y2-
			m.X1_Y2*m.X2_Y1*m.X3_Y3-
			m.X1_Y3*m.X2_Y2*m.X3_Y1;
	}

//*********************************************************************************
// Calcul de la matrice inverse d'une matrice 3*3
// Etat : Stable
// Derniére Modif : 22/9/94 (Pierre)
//*********************************************************************************
void BG_Matrice3_3Invert(BG_Matrice3_3 m1,BG_Matrice3_3 *m)
	{
	float	f;
	
	f = 1/BG_Matrice3_3Determinant(m1);
	(*m).X1_Y1=(m1.X2_Y2*m1.X3_Y3-m1.X2_Y3*m1.X3_Y2)*f;
	(*m).X1_Y2=(m1.X1_Y3*m1.X3_Y2-m1.X1_Y2*m1.X3_Y3)*f;
	(*m).X1_Y3=(m1.X1_Y2*m1.X2_Y3-m1.X1_Y3*m1.X2_Y2)*f;
	(*m).X2_Y1=(m1.X2_Y3*m1.X3_Y1-m1.X2_Y1*m1.X3_Y3)*f;
	(*m).X2_Y2=(m1.X1_Y1*m1.X3_Y3-m1.X1_Y3*m1.X3_Y1)*f;
	(*m).X2_Y3=(m1.X1_Y3*m1.X2_Y1-m1.X1_Y1*m1.X2_Y3)*f;
	(*m).X3_Y1=(m1.X2_Y1*m1.X3_Y2-m1.X2_Y2*m1.X3_Y1)*f;
	(*m).X3_Y2=(m1.X1_Y2*m1.X3_Y1-m1.X1_Y1*m1.X3_Y2)*f;
	(*m).X3_Y3=(m1.X1_Y1*m1.X2_Y2-m1.X1_Y2*m1.X2_Y1)*f;
	}

//*********************************************************************************
// Calcul du produit scalaire de deux vecteurs
// Etat : Stable
// Derniére Modif : 22/9/94 (Pierre)
//*********************************************************************************
float BG_ProduitScalaire(BG_Pt3D *v1,BG_Pt3D *v2)
	{
	return	v2->X*v1->X+v2->Y*v1->Y+v2->Z*v1->Z;
	}

//*********************************************************************************
// Expression d'un vecteur à partir d'une table de coordonnees de points et de deux
// index.
// Etat : Stable
// Derniére Modif : 20/5/94 (Pierre)
//*********************************************************************************
void BG_ExprimeVect(BG_Pt3D *Pt,int Pt1,int Pt2,BG_Pt3D *v1)
	{
	v1->X = Pt[Pt1].X-Pt[Pt2].X;
	v1->Y = Pt[Pt1].Y-Pt[Pt2].Y;
	v1->Z = Pt[Pt1].Z-Pt[Pt2].Z;
	}

//*********************************************************************************
// Produit vectoriel de deux vecteurs
// Etat : Stable
// Derniére Modif : 22/9/94 (Pierre)
//*********************************************************************************
void BG_ProduitVectoriel(BG_Pt3D *v1,BG_Pt3D *v2,BG_Pt3D *v3)
	{
	v3->X = v1->Y*v2->Z-v1->Z*v2->Y;
	v3->Y = v1->Z*v2->X-v1->X*v2->Z;
	v3->Z = v1->X*v2->Y-v1->Y*v2->X;
	}
	
//*********************************************************************************
// Normalise un vecteur et renvoie sa norme initiale
// Etat : Stable
// Derniére Modif : 13/06/95 (Pierre)
//*********************************************************************************
float BG_NormaliserVect(BG_Pt3D *v3)
	{
	float		f,f2;
	
// calcule le facteur de normalisation, c'est a dire l'inverse de la norme
	f2 = v3->X*v3->X+v3->Y*v3->Y+v3->Z*v3->Z;
	if (f2 != 0.0)
		{
		f = BG_InvSqrt(f2);
		v3->X *= f;		// multiplie chaque coordonnee par le facteur de normalisation
		v3->Y *= f;
		v3->Z *= f;
		}
	return f2;
	}

/***********************************************************
* Descriptif :	Calcule de la matrice inverse d'une matrice
*				et multiplication par un facteur d'echelle.
* Etat : Stable
* Dernière Modif :	09/08/95	(Pierre)
*					
***********************************************************/
void BG_Matrice3_3InvertEchelle(BG_Matrice3_3 m1,BG_Matrice3_3 *m,float coeff)
	{
	float	f;
	
	f = coeff/BG_Matrice3_3Determinant(m1);
	(*m).X1_Y1=(m1.X2_Y2*m1.X3_Y3-m1.X2_Y3*m1.X3_Y2)*f;
	(*m).X1_Y2=(m1.X1_Y3*m1.X3_Y2-m1.X1_Y2*m1.X3_Y3)*f;
	(*m).X1_Y3=(m1.X1_Y2*m1.X2_Y3-m1.X1_Y3*m1.X2_Y2)*f;
	(*m).X2_Y1=(m1.X2_Y3*m1.X3_Y1-m1.X2_Y1*m1.X3_Y3)*f;
	(*m).X2_Y2=(m1.X1_Y1*m1.X3_Y3-m1.X1_Y3*m1.X3_Y1)*f;
	(*m).X2_Y3=(m1.X1_Y3*m1.X2_Y1-m1.X1_Y1*m1.X2_Y3)*f;
	(*m).X3_Y1=(m1.X2_Y1*m1.X3_Y2-m1.X2_Y2*m1.X3_Y1)*f;
	(*m).X3_Y2=(m1.X1_Y2*m1.X3_Y1-m1.X1_Y1*m1.X3_Y2)*f;
	(*m).X3_Y3=(m1.X1_Y1*m1.X2_Y2-m1.X1_Y2*m1.X2_Y1)*f;
	}


	
	


