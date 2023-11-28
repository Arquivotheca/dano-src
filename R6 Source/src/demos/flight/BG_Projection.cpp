#include <math.h>
#include "BG_Erreur.h"
#include "BG_Types3D.h"
#include "BG_Graphic.h"
#include "BG_Projection.h"

/***********************************************************
* Descriptif :	Effectue une transformation (rotation/
*				translation) sur une liste de point puis
*				projete ces points sur l'ecran de vision
*				(selon theView) et met a jour les indicateurs
*				de clipping pour chaque point.
* Etat : Stable
* Dernire Modif :	19/10/95	(Pierre)
*					
***********************************************************/
void BG_Point3DMultMatrice3_3(	BG_Matrice3_3 	*m1,			// matrice de rotation
								BG_Pt3D 		*v1,			// table des points a projeter
								BG_Pt3D 		*o1,			// vecteur de translation
								BG_Point3D 		*v2,			// stockage des points projetes
								short 			nb_vecteurs,	// nb de points a traiter
								BG_ControlView	*theView)		// definition de la vue
	{
	char		masque;
	short		offH,offV;
	float		divi,Hmin,Hmax,Vmin,Vmax,zoom,memoH,memoV;
	BG_Pt3D		vr;

// copie en local les parametres caracteristiques du cone de vision et de la projection
	zoom = theView->zoom;
	Hmin = -zoom*theView->YXmin;
	Hmax = -zoom*theView->YXmax;
	Vmin = -zoom*theView->ZXmin;
	Vmax = -zoom*theView->ZXmax;
	offH = theView->offsetH;
	offV = theView->offsetV;
// pour chaque points
	for (;nb_vecteurs>0;nb_vecteurs--)
		{
		vr.X = v1->X-o1->X;
		vr.Y = v1->Y-o1->Y;
		vr.Z = v1->Z-o1->Z;
	// effectue la rotation-translation
		v2->X = m1->X1_Y1*vr.X;
		v2->Y = m1->X1_Y2*vr.X;
		v2->Z = m1->X1_Y3*vr.X;
		v2->X += m1->X2_Y1*vr.Y;
		v2->Y += m1->X2_Y2*vr.Y;
		v2->Z += m1->X2_Y3*vr.Y;
		v2->X += m1->X3_Y1*vr.Z;
		v2->Y += m1->X3_Y2*vr.Z;
		v2->Z += m1->X3_Y3*vr.Z;
	// si le point est projetable...
		if (v2->X > 0)
			{
		// effectue la projection
			divi = -zoom/(v2->X+BG_ANTIZERO);
			v2->H = (short)(memoH = v2->Y*divi)+offH;
			v2->V = (short)(memoV = v2->Z*divi)+offV;
		// teste le clipping
			masque = 0;
		// test debordement horizontal
			if (memoH > Hmin)
				masque |= BG_MASKHMIN;
			else if (memoH < Hmax)
				masque |= BG_MASKHMAX;
		// test debordement vertical
			if (memoV > Vmin)
				masque |= BG_MASKVMIN;
			else if (memoV < Vmax)
				masque |= BG_MASKVMAX;
			v2->Masque = masque;
			}
		else
			v2->Masque = BG_MASKXMIN;
		v2++;
		v1++;
		}
	}

/***********************************************************
* Descriptif :	Projete une liste de points sur l'ecran de vision
*				(selon theView) et met a jour les indicateurs
*				de clipping pour chaque point.
* Etat : Stable
* Dernire Modif :	19/10/95	(Pierre)
*					
***********************************************************/
void BG_Objet3DProjection(	BG_Point3D 		*v2,			// stockage des points projetes
							short 			nb_vecteurs,	// nb de points a traiter
							BG_ControlView	*theView)		// definition de la vue
	{
	short		offH,offV;
	float		divi,zoom;

// copie en local les parametres caracteristiques du cone de vision et de la projection
	zoom = theView->zoom;
	offH = theView->offsetH;
	offV = theView->offsetV;
// pour chaque points
	for (;nb_vecteurs>0;nb_vecteurs--)
		{
		divi = -zoom/(v2->X+BG_ANTIZERO);
		v2->H = (short)(v2->Y*divi)+offH;
		v2->V = (short)(v2->Z*divi)+offV;
		v2++;
		}
	}

/***********************************************************
* Descriptif :	Effectue une rotation sur une liste de vecteurs.
*
* Etat : Stable
* Dernire Modif :	22/09/94	(Pierre)
*					
***********************************************************/
void BG_Vect3DMultMatrice3_3(	BG_Matrice3_3 	*m1,			// matrice de rotation
								BG_Pt3D 		*v1,			// table des points a projeter
								BG_Pt3D 		*v2,			// stockage des points projetes
								short 			nb_vecteurs)	
	{
	float		m11,m12,m13,m21,m22,m23,m31,m32,m33;
	float		t1,t2,t3,w1,w2,w3;
	
	m11 = m1->X1_Y1;
	m12 = m1->X1_Y2;
	m13 = m1->X1_Y3;
	m21 = m1->X2_Y1;
	m22 = m1->X2_Y2;
	m23 = m1->X2_Y3;
	m31 = m1->X3_Y1;
	m32 = m1->X3_Y2;
	m33 = m1->X3_Y3;
	for (;nb_vecteurs>0;nb_vecteurs--)
		{
		t1 = v1->X;
		t2 = v1->Y;
		t3 = v1->Z;
		w1 = m11*t1;
		w2 = m12*t1;
		w3 = m13*t1;
		w1 += m21*t2;
		w2 += m22*t2;
		w3 += m23*t2;
		w1 += m31*t3;
		w2 += m32*t3;
		w3 += m33*t3;
		v2->X = w1;
		v2->Y = w2;
		v2->Z = w3;
		v1++;
		v2++;
		}
	}

/***********************************************************
* Descriptif :	Effectue une transformation (translation/
*				rotation) sur une liste de points puis
*				projete ces points sur l'ecran de vision
*				(selon theView) et met a jour les indicateurs
*				de clipping pour chaque point.
* Etat : Stable
* Dernire Modif :	18/10/94	(Pierre)
*					
***********************************************************/
void BG_Objet3DMultMatrice3_3(	BG_Matrice3_3 	*m1,			// matrice de rotation
								BG_Pt3D 		*v1,			// table des points a projeter
								BG_Pt3D 		*o1,			// vecteur de translation
								BG_Point3D 		*v2,			// stockage des points projetes
								short 			nb_vecteurs,	// nb de points a traiter
								BG_ControlView	*theView)		// definition de la vue
	{
	char		masque;
	short		offH,offV;
	float		divi,Hmin,Hmax,Vmin,Vmax,zoom,memoH,memoV;

// copie en local les parametres caracteristiques du cone de vision et de la projection
	zoom = theView->zoom;
	Hmin = -zoom*theView->YXmin;
	Hmax = -zoom*theView->YXmax;
	Vmin = -zoom*theView->ZXmin;
	Vmax = -zoom*theView->ZXmax;
	offH = theView->offsetH;
	offV = theView->offsetV;
// pour chaque points
	for (;nb_vecteurs>0;nb_vecteurs--)
		{
	// effectue la rotation-translation
		v2->X = o1->X+m1->X1_Y1*v1->X;
		v2->Y = o1->Y+m1->X1_Y2*v1->X;
		v2->Z = o1->Z+m1->X1_Y3*v1->X;
		v2->X += m1->X2_Y1*v1->Y;
		v2->Y += m1->X2_Y2*v1->Y;
		v2->Z += m1->X2_Y3*v1->Y;
		v2->X += m1->X3_Y1*v1->Z;
		v2->Y += m1->X3_Y2*v1->Z;
		v2->Z += m1->X3_Y3*v1->Z;
	// si le point est projetable...
		if (v2->X > 0)
			{
		// effectue la projection
			divi = -zoom/(v2->X+BG_ANTIZERO);
			v2->H = (short)(memoH = v2->Y*divi)+offH;
			v2->V = (short)(memoV = v2->Z*divi)+offV;
		// teste le clipping
			masque = 0;
		// test debordement horizontal
			if (memoH > Hmin)
				masque |= BG_MASKHMIN;
			else if (memoH < Hmax)
				masque |= BG_MASKHMAX;
		// test debordement vertical
			if (memoV > Vmin)
				masque |= BG_MASKVMIN;
			else if (memoV < Vmax)
				masque |= BG_MASKVMAX;
			v2->Masque = masque;
			}
		else
			v2->Masque = BG_MASKXMIN;
		v2++;
		v1++;
		}
	}


