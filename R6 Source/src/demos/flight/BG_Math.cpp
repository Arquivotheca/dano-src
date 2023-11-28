#include <math.h>
#include "BG_Erreur.h"
#include "BG_Types3D.h"
#include "BG_Memory.h"
#include "BG_Math.h"

static	float	flottant;	// tampon de stockage pour la relecture en notation scientifique
static	float	tampon	;	// tampon de stockage pour le code de l'approximation
		float	*CosTable;

//*********************************************************************************
// Initialise la table de cosinus
// Etat : Stable (ressource courtcircuitees pour reglage de la precision)
// Derniére Modif : 22/9/94 (Pierre)
//*********************************************************************************
void BG_InitCosTable(void)
	{
	long	i;

	i = BG_NB_ENTREES_COSTABLE*sizeof(float);
	CosTable = (float*)BG_GetMemLow(i);
	for (i=0;i<BG_NB_ENTREES_COSTABLE;i++)
		CosTable[i] = (float)cos(((float)i)*((2*3.14159265358979)/BG_NB_ENTREES_COSTABLE));
	}

//*********************************************************************************
// Libere le tampon de la table de cosinus
// Etat : Stable
// Derniére Modif : 22/9/94 (Pierre)
//*********************************************************************************
void BG_DisposeCosTable()
	{
	BG_FreeMemLow((char*)CosTable);
	}

/***********************************************************
* Descriptif :	Calcule 1/sqrt(X) de maniere rapide, avec
*				une precision legerement reduite...
* Etat : Stable
* Dernière Modif :	25/07/95 (Pierre)
*					
***********************************************************/
float BG_InvSqrt(float x)
	{
	unsigned long	val;
	float			y,z,t;
	
// calcule l'approximation initiale a 25% pres...
	flottant = x;
	val = *((unsigned long*)&flottant);
	val -= 0x3F800000L;
	val >>= 1;
	val += 0x3F800000L;
	val &= 0x7FFFFFFFL;
	*((unsigned long*)&tampon) = val;
// effectue deux occurences optimisees de la suite convergente
	y = tampon;
	z = y*y+x;
	t = y*y-x;
	y *= 4;
	x = z*z;
	t = t*t;
	y = z*y;
	t = 2*x-t;
// renvoie le resultat (ou l'inverse tres aisement)
	return y/t;
	}

/***********************************************************
* Descriptif :	Calcule sqrt(X) de maniere rapide, avec
*				une precision legerement reduite... Attention
*				ne marche pas avec 0 ou un negatif.
* Etat : Stable
* Dernière Modif :	03/08/95 (Pierre)
*					
***********************************************************/
float BG_Sqrt(float x)
	{
	unsigned long	val;
	float			y,z,t;
	
// calcule l'approximation initiale a 25% pres...
	flottant = x;
	val = *((unsigned long*)&flottant);
	val -= 0x3F800000L;
	val >>= 1;
	val += 0x3F800000L;
	val &= 0x7FFFFFFFL;
	*((unsigned long*)&tampon) = val;
// effectue deux occurences optimisees de la suite convergente
	y = tampon;
	z = y*y+x;
	t = y*y-x;
	y *= 4;
	x = z*z;
	t = t*t;
	y = z*y;
	t = 2*x-t;
// renvoie le resultat (ou l'inverse tres aisement)
	return t/y;
	}





