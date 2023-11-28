#include <stdlib.h>
#include "BG_Memory.h"
#include "BG_Erreur.h"

#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _DEBUGGER_H
#include <debugger.h>
#endif

static	long		MemSize;
static	BG_MemHeader	*MemHigh;
static	char		*MemLow;
static	char		*curMemHigh;
static	char		*curMemLow;

/***********************************************************
* Descriptif :	Initialise le gestionnaire de memoire interne
*				
* Etat : Stable
* Dernière Modif :	06/09/94 (Pierre)
*					
***********************************************************/
void BG_InitMemory()
	{
// alloue un espace memoire suffisant
	MemSize = 4096*1024;
	MemLow = (char*)malloc(MemSize);
	if (MemLow == 0L) Erreur();
	curMemLow = MemLow;
	MemHigh = (BG_MemHeader*)(MemLow+MemSize-MEM_HEADER_SIZE);
	MemHigh->BlockSize = 0L;
	MemHigh->next = 0L;
	MemHigh->prev = 0L;
	curMemHigh = (char*)MemHigh;
	}
	
/***********************************************************
* Descriptif :	Libere le gestionnaire de memoire interne
*				
* Etat : Stable
* Dernière Modif :	06/09/94
*					
***********************************************************/
void BG_DisposeMemory()
	{
//	delete_area(ReadOnlyArea);
	free(MemLow);
	}
	
/***********************************************************
* Descriptif :	Alloue un segment sur la pile basse
*				(segments durables)
* Etat : Stable
* Dernière Modif :	02/08/95
*					
***********************************************************/
char *BG_GetMemLow(long size)
	{
	char		*ptr;
	
// alignement force modulo 4
	size = (size+3)&0x7FFFFFFCL;

	ptr = curMemLow;
	curMemLow += size;
	if (curMemLow <= curMemHigh) return ptr;
	else Erreur();
	return 0L;
	}

/***********************************************************
* Descriptif :	Recoupe le tas du bas
*				(segments durables)
* Etat : Stable
* Dernière Modif :	07/09/94
*					
***********************************************************/
void BG_FreeMemLow(char *ptr)
	{
	if (ptr >= curMemHigh) Erreur(); 
	if (ptr < MemLow) Erreur(); 
	if (ptr < curMemLow) curMemLow = ptr;
	}

/***********************************************************
* Descriptif :	Alloue un segment de memoire en pile haute
*				(segments temporaires)
* Etat : Stable
* Dernière Modif :	02/08/95
*					
***********************************************************/
char *BG_GetMemHigh(long size)
	{
	BG_MemHeader	*theMem,*Best;
	long			Ratio,BestRatio;
	
// surveillance permanente de l'etat de la pile haute (pour debug simplifie)
	BG_CheckMemHigh();
	if ((size > BLOCK_MAX) || (size <= 0)) Erreur();
// smart modif
	size = (size+3)&0x7FFFFFFCL;
// parcourt la liste des segments, en recherche
// un ideal ou a defaut celui qui segmente le moins.
	theMem = MemHigh;
	BestRatio = 1;
	while (theMem != 0L)
		{
		if ((Ratio = theMem->BlockSize+size) == 0)
			{
			BestRatio = 0;
			Best = theMem;
			break;
			}
		if (Ratio < BestRatio)
			{
			BestRatio = Ratio;
			Best = theMem;
			}
		theMem = theMem->next;
		}
// envisage une allocation en fin de pile
	if (((BestRatio == 1) || (BestRatio > RATIO_SEUIL)) && (BestRatio != 0))
		{
		if ((curMemHigh-curMemLow) > (size+MEM_HEADER_SIZE))
			Best = 0L;
		else if (BestRatio == 1) Erreur();
		}
// effectue une réallocation interne a la pile
	if (Best != 0L)
		{
		BG_MemHeader	*prev,/**next,*/*sold;
		long			residu;
		
		residu = Best->BlockSize+size+MEM_HEADER_SIZE;
		if (residu < 0)
			{
			sold = (BG_MemHeader*)((long)Best+MEM_HEADER_SIZE+size);
			prev = Best->prev;
			sold->BlockSize = residu;
			sold->next = Best;
			sold->prev = prev;
			prev->next = sold;
			Best->BlockSize = size;
			Best->prev = sold;
			}
		else
			Best->BlockSize = -Best->BlockSize;
		}
// effectue une nouvelle allocation en bout de pile
	else
		{
		Best = (BG_MemHeader*)((long)curMemHigh-size-MEM_HEADER_SIZE);
		Best->next = 0L;
		Best->prev = (BG_MemHeader*)curMemHigh;
		((BG_MemHeader*)curMemHigh)->next = Best;
		Best->BlockSize = size;
		curMemHigh -= size+MEM_HEADER_SIZE;
		}
	if (((long)Best+MEM_HEADER_SIZE) > (long)MemHigh) Erreur();
// surveillance permanente de l'etat de la pile haute (pour debug simplifie)
	BG_CheckMemHigh();
	return (char*)((long)Best+MEM_HEADER_SIZE);
	}

/***********************************************************
* Descriptif :	Verifie l'integrite de la pile memoire haute
*				Peut etre appele n'importe quand pour simplifie
*				le debug. Appele le debugger en cas de probleme
* Etat : En cours
* Dernière Modif :	31/03/95
*					
***********************************************************/
void BG_CheckMemHigh()
	{
	BG_MemHeader	*theMem,*Prev;
	long			size,cpt;
	
// parcourt la liste des segments, et verifie tous les liens et toutes les tailles
	theMem = MemHigh;
	cpt = 0;
	if (theMem != 0L)
		{
		if (theMem->BlockSize != 0L) Erreur();
		if (theMem->prev != 0L) Erreur();
		Prev = theMem;
		theMem = theMem->next;
		cpt++;
		}
	while (theMem != 0L)
		{
		if ((theMem >= MemHigh) ||
			(theMem < (BG_MemHeader*)curMemHigh) ||
			(theMem < (BG_MemHeader*)MemLow)) Erreur();
		if (theMem->prev != Prev) Erreur();
		size = (long)Prev-(long)theMem-MEM_HEADER_SIZE;
		if ((theMem->BlockSize != -size) && (theMem->BlockSize != size)) Erreur();
		Prev = theMem;
		theMem = theMem->next;
		cpt++;
		}
	}
	
/***********************************************************
* Descriptif :	libere un segment de la pile supérieure
*
* Etat : sécurité limitée (debug)
* Dernière Modif :	07/09/94
*					
***********************************************************/
void BG_FreeMemHigh(char *ptr)
	{
	BG_MemHeader		*theMem;
	
// surveillance permanente de l'etat de la pile haute (pour debug simplifie)
	BG_CheckMemHigh();
// calcule le pointeur de header memoire
	theMem = (BG_MemHeader*)((long)ptr-MEM_HEADER_SIZE);
	if ((long)theMem >= (long)MemHigh) Erreur();
// test de sécurité (limité)
	if ((theMem->BlockSize <= 0) || (theMem->BlockSize > BLOCK_MAX)) Erreur();
// libération du sommet de la pile
	if ((char*)theMem == curMemHigh)
		{
		theMem->prev->next = 0L;
		curMemHigh = (char*)theMem->prev;
		}
// libération interne (avec récupération des trous et assemblage des voisins immédiats)
	else
		{
		BG_MemHeader		*prev,*next,*sold;
		long			residu;
		
		prev = theMem->prev;
		if (prev->BlockSize < 0) prev = prev->prev;
		next = theMem->next;
		if (next->BlockSize < 0)
			{
			residu = (long)next-(long)prev+MEM_HEADER_SIZE;
			next->BlockSize = residu;
			next->prev = prev;
			prev->next = next;
			}
		else
			{
			residu = (long)next-(long)prev+MEM_HEADER_SIZE*2+next->BlockSize;
			sold = (BG_MemHeader*)((long)next+next->BlockSize+MEM_HEADER_SIZE);
			next->prev = sold;
			sold->BlockSize = residu;
			sold->next = next;
			sold->prev = prev;
			prev->next = sold;
			}
		}
// purge le bas de la pile superieur
	theMem = (BG_MemHeader*)curMemHigh;
	if (theMem->BlockSize < 0)
		{
		do theMem = theMem->prev; while (theMem->BlockSize < 0);
		curMemHigh = (char*)theMem;
		theMem->next = 0L;
		}
// surveillance permanente de l'etat de la pile haute (pour debug simplifie)
	BG_CheckMemHigh();
	}

/***********************************************************
* Descriptif :	Recopie un segment de memoire, tolere les 
*				recouvrements
* Etat : Stable
* Dernière Modif :	07/09/94
*					
***********************************************************/
void BG_BlockMove(char *src,char *dest,long size)
	{
	int			i;
	
	if (dest < src)
		{
		for (i=0;i<size;i++)
			dest[i] = src[i];
		}
	else
		{
		for (i=size-1;i>=0;i--)
			dest[i] = src[i];
		}
	}

