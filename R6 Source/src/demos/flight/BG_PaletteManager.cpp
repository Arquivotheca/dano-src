#include "BG_Erreur.h"
#include "BG_Types3D.h"
#include "BG_Memory.h"
#include "BG_Graphic.h"
#include "BG_PaletteManager.h"

//*********************************************************************************
// parametrage du gestionnaire de tramage
// Reglage du tramage
  // seuil de reduction du tramage
#define	SeuilMax	1800
  // amplification du tramage
#define	MultiTrame	2
  // niveau de tramage (1 a 4) (nombre max de couleurs utilisees pour emuler une couleur)
#define	LevelTrame	4
//*********************************************************************************

// variables globales temporaires et internes du palette manager
static	BG_Couleur			*PalRef;

/***********************************************************
* Descriptif :	Alloue les tables de traitement temporaires
*				du palette manager, et les initialise.
* Etat : Stable
* Dernière Modif :	17/06/95 (Pierre)
*					
***********************************************************/
void BG_OpenPaletteManager()
	{
	int				i,j;
	BG_Couleur		temp;
	Boolean			Actif;

// alloue le table d'analyse temporaire
	PalRef = (BG_Couleur*)BG_GetMemHigh(sizeof(BG_Couleur)*256L);
// recopie la palette dans la table d'analyse.
	for (i=0;i<256;i++)
		{
		PalRef[i].Total  = (long)(PalRef[i].Red = BG_thePalette[i*3+0]);
		PalRef[i].Total += (long)(PalRef[i].Green = BG_thePalette[i*3+1]);
		PalRef[i].Total += (long)(PalRef[i].Blue = BG_thePalette[i*3+2]);
		PalRef[i].Index = i;
		}
// reclassement par Total decroissant (tri a bulle)
	Actif = TRUE;
	while (Actif)
		{
		Actif = FALSE;
	// pour chaque element restant a trier
		for (i=0;i<255;i++)
			{
			j = i+1;
		// cherche jusqu'ou on peut l'enfoncer
			if (PalRef[i].Total < PalRef[j].Total)
				{
				temp.Red 		= PalRef[i].Red;
				temp.Green 		= PalRef[i].Green;
				temp.Blue 		= PalRef[i].Blue;
				temp.Total 		= PalRef[i].Total;
				temp.Index 		= PalRef[i].Index;
				PalRef[i].Red 	= PalRef[j].Red;
				PalRef[i].Green = PalRef[j].Green;
				PalRef[i].Blue 	= PalRef[j].Blue;
				PalRef[i].Total = PalRef[j].Total;
				PalRef[i].Index = PalRef[j].Index;
				PalRef[j].Red 	= temp.Red;
				PalRef[j].Green = temp.Green;
				PalRef[j].Blue 	= temp.Blue;
				PalRef[j].Total = temp.Total;
				PalRef[j].Index = temp.Index;
				Actif = TRUE;
				}
			}
		}
	}
	
/***********************************************************
* Descriptif :	Libere les tables de traitement temporaires
*				du palette manager
* Etat : Stable
* Dernière Modif :	07/06/95 (Pierre)
*					
***********************************************************/
void BG_ClosePaletteManager()
	{
	BG_FreeMemHigh((char*)PalRef);
	}
	
/***********************************************************
* Descriptif :	Donne la distance entre deux BG_Couleurs
*
* Etat : Stable
* Dernière Modif :	17/06/95 (Pierre)
*					
***********************************************************/
long BG_GetDistance(long a1,long a2,long a3,long b1,long b2,long b3)
	{
	long		ecart,ecarttot;

	ecart = a1-b1;
	if (ecart<0) ecarttot = -ecart;
	else ecarttot = ecart;
	ecart = a2-b2;
	if (ecart<0) ecarttot -= ecart;
	else ecarttot += ecart;
	ecart = a3-b3;
	if (ecart<0) ecarttot -= ecart;
	else ecarttot += ecart;
	if (ecarttot >= SeuilMax) ecarttot = 2*ecarttot-SeuilMax;
	return ecarttot;
	}

/***********************************************************
* Descriptif :	Cherche la meilleure BG_Couleur disponible
*				pour emuler une BG_Couleur donnée
* Etat : Stable
* Dernière Modif :	17/06/95 (Pierre)
*					
***********************************************************/
void BG_GetBestColor(BG_Couleur *coul,unsigned char *P1col,unsigned char *I1col,unsigned char *P2col,unsigned char *I2col)
	{
	int			i,j,k,m,n,cur,nbcol;
	long		ecartmin,ecartmax,ecart,poid;
	long		ecart1,ecart2,ecart3,mini1,mini2,mini3;
	long		tot1,tot2,tot3,tot4;
	int			bestP1,bestI1,bestP2,bestI2,best;
	short		First,Last;
	short		Ordre[256];
	BG_Couleur	*Pali,*Palj,*Palm,*Paln;
	
// reglage
	poid = MultiTrame;
// Methode moins bete : on cherche le meilleur point de depart. (par dichotomie)
	i = 128;
	j = 64;
	while (j > 0)
		{
		if (coul->Total > PalRef[i].Total) i -= j;
		else i += j;
		j >>= 1;
		}
// Recherche la meilleure solution en partant de cette BG_Couleur, en sachant que l'ecart ne
// peut pas etre inferieur a la difference des totaux.
	ecartmin = 100000000L;
	ecartmax = 5*100000000L;
// initialise la chaine
	First = Last = -1;
// Partie de code a dupliquer betement.
// precalcul les ecarts partie superieure
	for (k=i;k<256;k++)
		{
	// calcul l'ecart reel
		ecart = PalRef[k].Total-coul->Total;
		if ((ecart <= ecartmax) && (ecart >= -ecartmax))
			{
			ecart = PalRef[k].Ecart =
				BG_GetDistance(PalRef[k].Red,
							PalRef[k].Green,
							PalRef[k].Blue,
							coul->Red,
							coul->Green,
							coul->Blue);
		// valeur a classer ?
			if (ecart < ecartmax)
				{
			// insertion simple
				if (ecart >= ecartmin)
					{
				// recherche la bonne position
					cur = Last;
					while (ecart < PalRef[cur].Ecart) cur = PalRef[cur].prev;
				// enregistre le nouveau venu
					PalRef[k].next = PalRef[cur].next;
					PalRef[k].prev = cur;
					PalRef[cur].next = k;
					if (PalRef[k].next == -1) Last = k;
					else PalRef[PalRef[k].next].prev = k;
					}
			// redecoupe
				else
					{
				// enregistre le nouveau premier
					if (First != -1) PalRef[First].prev = k;
					else Last = k;
					PalRef[k].next = First;
					PalRef[k].prev = -1;
					First = k;
					ecartmin = ecart;
					ecartmax = 5*ecartmin;
				// decoupe tout ce qui depasse
					cur = Last;
					while (PalRef[cur].Ecart > ecartmax) cur = PalRef[cur].prev;
					Last = cur;
					PalRef[cur].next = -1;
					}
				}
			}
		}
// Partie de code a dupliquer betement.
// precalcul les ecarts partie superieure
	for (k=i-1;k>=0;k--)
		{
	// calcul l'ecart reel
		ecart = PalRef[k].Total-coul->Total;
		if ((ecart <= ecartmax) && (ecart >= -ecartmax))
			{
			ecart = PalRef[k].Ecart =
				BG_GetDistance(PalRef[k].Red,
							PalRef[k].Green,
							PalRef[k].Blue,
							coul->Red,
							coul->Green,
							coul->Blue);
		// valeur a classer ?
			if (ecart < ecartmax)
				{
			// insertion simple
				if (ecart >= ecartmin)
					{
				// recherche la bonne position
					cur = Last;
					while (ecart < PalRef[cur].Ecart) cur = PalRef[cur].prev;
				// enregistre le nouveau venu
					PalRef[k].next = PalRef[cur].next;
					PalRef[k].prev = cur;
					PalRef[cur].next = k;
					if (PalRef[k].next == -1) Last = k;
					else PalRef[PalRef[k].next].prev = k;
					}
			// redecoupe
				else
					{
				// enregistre le nouveau premier
					if (First != -1) PalRef[First].prev = k;
					else Last = k;
					PalRef[k].next = First;
					PalRef[k].prev = -1;
					First = k;
					ecartmin = ecart;
					ecartmax = 5*ecartmin;
				// decoupe tout ce qui depasse
					cur = Last;
					while (PalRef[cur].Ecart > ecartmax) cur = PalRef[cur].prev;
					Last = cur;
					PalRef[cur].next = -1;
					}
				}
			}
		}
// calcul la longueur de la chaine a analyser
	cur = First;
	nbcol = 0;
	while (cur != -1)
		{
		Ordre[nbcol] = cur;
		nbcol++;
		cur = PalRef[cur].next;
		}
// prepare l'analyse trame
	mini1 = PalRef[First].Ecart;
	mini2 = 2*mini1;
	mini3 = 3*mini1;
	ecartmin = 100000000L;
	ecart1 = ecartmin-mini1;
	ecart2 = ecartmin-mini2;
	ecart3 = ecartmin-mini3;

	bestP1 = First;
	bestP2 = First;
	bestI1 = First;
	bestI2 = First;
#if (LevelTrame > 1)
// trame de deux BG_Couleurs : 2 2
	for (i=0;i<nbcol;i++)
		{
		Pali = PalRef+Ordre[i];
		tot1 = 2*Pali->Ecart;
		if (tot1 < ecart2) for (j=0;j<=i;j++)
			{
			Palj = PalRef+Ordre[j];
			tot2 = tot1 + 2*Palj->Ecart;
			if (tot2 < ecartmin)
				{
				tot2 += poid*BG_GetDistance(2L*(Pali->Red+Palj->Red),
									2L*(Pali->Green+Palj->Green),
									2L*(Pali->Blue+Palj->Blue),
									4L*coul->Red,
									4L*coul->Green,
									4L*coul->Blue);
				if (tot2<ecartmin)
					{
					ecartmin = tot2;
					ecart1 = ecartmin-mini1;
					ecart2 = ecartmin-mini2;
					ecart3 = ecartmin-mini3;
					bestP2 = bestP1 = Ordre[i];
					bestI2 = bestI1 = Ordre[j];
					k = nbcol-1;
					while ((k >= 0)?(PalRef[Ordre[k]].Ecart >= ecart3):0) k--;
					nbcol = k+1;
					if (j >= nbcol-1) j = i;
					}
				}
			}
		}
// trame de deux BG_Couleurs : 1 3
	for (i=0;i<nbcol;i++)
		{
		Pali = PalRef+Ordre[i];
		tot1 = Pali->Ecart;
		if (tot1 < ecart1) for (j=0;j<=i;j++)
			{
			Palj = PalRef+Ordre[j];
			tot2 = tot1 + 3*Palj->Ecart;
			if (tot2 < ecartmin)
				{
				tot2 += poid*BG_GetDistance((Pali->Red+3*Palj->Red),
									(Pali->Green+3*Palj->Green),
									(Pali->Blue+3*Palj->Blue),
									4L*coul->Red,
									4L*coul->Green,
									4L*coul->Blue);
				if (tot2<ecartmin)
					{
					ecartmin = tot2;
					ecart1 = ecartmin-mini1;
					ecart2 = ecartmin-mini2;
					ecart3 = ecartmin-mini3;
					bestP1 = Ordre[i];
					bestP2 = bestI2 = bestI1 = Ordre[j];
					k = nbcol-1;
					while ((k >= 0)?(PalRef[Ordre[k]].Ecart >= ecart3):0) k--;
					nbcol = k+1;
					if (j > nbcol-1) j = i;
					}
				}
			}
		}
// trame de deux BG_Couleurs : 3 1
	for (i=0;i<nbcol;i++)
		{
		Pali = PalRef+Ordre[i];
		tot1 = 3*Pali->Ecart;
		if (tot1 < ecart3) for (j=0;j<=i;j++)
			{
			Palj = PalRef+Ordre[j];
			tot2 = tot1 + Palj->Ecart;
			if (tot2 < ecartmin)
				{
				tot2 += poid*BG_GetDistance((3*Pali->Red+Palj->Red),
									(3*Pali->Green+Palj->Green),
									(3*Pali->Blue+Palj->Blue),
									4L*coul->Red,
									4L*coul->Green,
									4L*coul->Blue);
				if (tot2 < ecartmin)
					{
					ecartmin = tot2;
					ecart1 = ecartmin-mini1;
					ecart2 = ecartmin-mini2;
					ecart3 = ecartmin-mini3;
					bestI2 = bestP2 = bestP1 = Ordre[i];
					bestI1 = Ordre[j];
					k = nbcol-1;
					while ((k >= 0)?(PalRef[Ordre[k]].Ecart >= ecart3):0) k--;
					nbcol = k+1;
					if (j > nbcol-1) j = i;
					}
				}
			}
		}
#endif
#if (LevelTrame > 2)
// trame de trois BG_Couleurs 1 1 2
	for (i=2;i<nbcol;i++)
		{
		Pali = PalRef+Ordre[i];
		tot1 = Pali->Ecart;
		if (tot1 < ecart1) for (j=1;j<i;j++)
			{
			Palj = PalRef+Ordre[j];
			tot2 = tot1 + Palj->Ecart;
			if (tot2 < ecart2) for (m=0;m<j;m++)
				{
				Palm = PalRef+Ordre[m];
				tot3 = tot2 + 2*Palm->Ecart;
				if (tot3 < ecartmin)
					{
					tot3 += poid*BG_GetDistance((Pali->Red+Palj->Red+2*Palm->Red),
										(Pali->Green+Palj->Green+2*Palm->Green),
										(Pali->Blue+Palj->Blue+2*Palm->Blue),
										4L*coul->Red,
										4L*coul->Green,
										4L*coul->Blue);
					if (tot3 < ecartmin)
						{
						ecartmin = tot3;
						ecart1 = ecartmin-mini1;
						ecart2 = ecartmin-mini2;
						ecart3 = ecartmin-mini3;
						bestP1 = bestP2 = Ordre[m];
						bestI1 = Ordre[i];
						bestI2 = Ordre[j];
						k = nbcol-1;
						while ((k >= 0)?(PalRef[Ordre[k]].Ecart >= ecart3):0) k--;
						nbcol = k+1;
						if (j > nbcol-1) j = i;
						if (m > nbcol-2) m = j;
						}
					}
				}
			}
		}
// trame de trois BG_Couleurs 1 2 1
	for (i=2;i<nbcol;i++)
		{
		Pali = PalRef+Ordre[i];
		tot1 = Pali->Ecart;
		if (tot1 < ecart1) for (j=1;j<i;j++)
			{
			Palj = PalRef+Ordre[j];
			tot2 = tot1 + 2*Palj->Ecart;
			if (tot2 < ecart3) for (m=0;m<j;m++)
				{
				Palm = PalRef+Ordre[m];
				tot3 = tot2 + Palm->Ecart;
				if (tot3 < ecartmin)
					{
					tot3 += poid*BG_GetDistance((Pali->Red+2*Palj->Red+Palm->Red),
										(Pali->Green+2*Palj->Green+Palm->Green),
										(Pali->Blue+2*Palj->Blue+Palm->Blue),
										4L*coul->Red,
										4L*coul->Green,
										4L*coul->Blue);
					if (tot3 < ecartmin)
						{
						ecartmin = tot3;
						ecart1 = ecartmin-mini1;
						ecart2 = ecartmin-mini2;
						ecart3 = ecartmin-mini3;
						bestP1 = bestP2 = Ordre[j];
						bestI1 = Ordre[i];
						bestI2 = Ordre[m];
						k = nbcol-1;
						while ((k >= 0)?(PalRef[Ordre[k]].Ecart >= ecart3):0) k--;
						nbcol = k+1;
						if (j > nbcol-1) j = i;
						if (m > nbcol-2) m = j;
						}
					}
				}
			}
		}
// trame de trois BG_Couleurs 2 1 1
	for (i=2;i<nbcol;i++)
		{
		Pali = PalRef+Ordre[i];
		tot1 = 2*Pali->Ecart;
		if (tot1 < ecart2) for (j=1;j<i;j++)
			{
			Palj = PalRef+Ordre[j];
			tot2 = tot1 + Palj->Ecart;
			if (tot2 < ecart3) for (m=0;m<j;m++)
				{
				Palm = PalRef+Ordre[m];
				tot3 = tot2 + Palm->Ecart;
				if (tot3 < ecartmin)
					{
					tot3 += poid*BG_GetDistance((2*Pali->Red+Palj->Red+Palm->Red),
										(2*Pali->Green+Palj->Green+Palm->Green),
										(2*Pali->Blue+Palj->Blue+Palm->Blue),
										4L*coul->Red,
										4L*coul->Green,
										4L*coul->Blue);
					if (tot3 < ecartmin)
						{
						ecartmin = tot3;
						ecart1 = ecartmin-mini1;
						ecart2 = ecartmin-mini2;
						ecart3 = ecartmin-mini3;
						bestP1 = bestP2 = Ordre[i];
						bestI1 = Ordre[m];
						bestI2 = Ordre[j];
						k = nbcol-1;
						while ((k >= 0)?(PalRef[Ordre[k]].Ecart >= ecart3):0) k--;
						nbcol = k+1;
						if (j > nbcol-1) j = i;
						if (m > nbcol-2) m = j;
						}
					}
				}
			}
		}
#endif
#if (LevelTrame > 3)
// trame de quatre BG_Couleurs 1 1 1 1
	for (i=3;i<nbcol;i++)
		{
		Pali = PalRef+Ordre[i];
		tot1 = Pali->Ecart;
		if (tot1 < ecart1) for (j=2;j<i;j++)
			{
			Palj = PalRef+Ordre[j];
			tot2 = tot1 + Palj->Ecart;
			if (tot2 < ecart2) for (m=1;m<j;m++)
				{
				Palm = PalRef+Ordre[m];
				tot3 = tot2 + Palm->Ecart;
				if (tot3 < ecart3) for (n=0;n<m;n++)
					{
					Paln = PalRef+Ordre[n];
					tot4 = tot3 + Paln->Ecart;
					if (tot4 < ecartmin)
						{
						tot4 += poid*BG_GetDistance((Pali->Red+Palj->Red+Palm->Red+Paln->Red),
											(Pali->Green+Palj->Green+Palm->Green+Paln->Green),
											(Pali->Blue+Palj->Blue+Palm->Blue+Paln->Blue),
											4L*coul->Red,
											4L*coul->Green,
											4L*coul->Blue);
						if (tot4 < ecartmin)
							{
							ecartmin = tot4;
							ecart1 = ecartmin-mini1;
							ecart2 = ecartmin-mini2;
							ecart3 = ecartmin-mini3;
							bestP1 = Ordre[n];
							bestP2 = Ordre[m];
							bestI1 = Ordre[i];
							bestI2 = Ordre[j];
							k = nbcol-1;
							while ((k >= 0)?(PalRef[Ordre[k]].Ecart >= ecart3):0) k--;
							nbcol = k+1;
							if (j > nbcol-1) j = i;
							if (m > nbcol-2) m = j;
							if (n > nbcol-3) n = m;
							}
						}
					}
				}
			}
		}
#endif
// Calcule le meilleur ordonnancement de paires entre les 4 BG_Couleurs
	Pali = PalRef+bestP1;
	Palj = PalRef+bestP2;
	Palm = PalRef+bestI1;
	Paln = PalRef+bestI2;
	ecart = BG_GetDistance(Pali->Red,Pali->Green,Pali->Blue,Palj->Red,Palj->Green,Palj->Blue)+
			BG_GetDistance(Palm->Red,Palm->Green,Palm->Blue,Paln->Red,Paln->Green,Paln->Blue);
	ecartmin = BG_GetDistance(Pali->Red,Pali->Green,Pali->Blue,Palm->Red,Palm->Green,Palm->Blue)+
			BG_GetDistance(Palj->Red,Palj->Green,Palj->Blue,Paln->Red,Paln->Green,Paln->Blue);
	ecartmax = BG_GetDistance(Pali->Red,Pali->Green,Pali->Blue,Paln->Red,Paln->Green,Paln->Blue)+
			BG_GetDistance(Palj->Red,Palj->Green,Palj->Blue,Palm->Red,Palm->Green,Palm->Blue);
	if ((ecart <= ecartmin) && (ecart <= ecartmax))
		{
		}
	else if (ecartmin < ecartmax)
		{
		best = bestI1;
		bestI1 = bestP2;
		bestP2 = best;
		}
	else
		{
		best = bestI2;
		bestI2 = bestP2;
		bestP2 = best;
		}
// Calcule le meilleur ordonnancement entre les deux paires
	ecartmin = PalRef[bestP1].Total+PalRef[bestP2].Total;
	ecartmax = PalRef[bestI1].Total+PalRef[bestI2].Total;
	if (ecartmin < ecartmax)
		{
		best = bestI1;
		bestI1 = bestP1;
		bestP1 = best;
		best = bestI2;
		bestI2 = bestP2;
		bestP2 = best;
		}
// renvoie la meilleure solution
	*P1col = PalRef[bestP1].Index;
	*I1col = PalRef[bestI1].Index;
	*P2col = PalRef[bestP2].Index;
	*I2col = PalRef[bestI2].Index;
	}
