#include "BG_Erreur.h"
#include "BG_Types3D.h"
#include "BG_Memory.h"
#include "BG_Graphic.h"
#include "BG_Matrice.h"
#include "BG_PlayerInterface.h"
#include "BG_Ground.h"
#include "BG_AirCraft.h"
#include "BG_Main.h"
#include "BG_Network.h"
#include "BG_Superviseur.h"
#include <stdio.h>

		char		BG_StatusJoueur[BG_NBPLAYERMAX];
		char		BG_IndexPlayer[BG_NBPLAYERMAX];
		char		BG_YellowPlayer[BG_NBPLAYERMAX];
		long		BG_LastGroundRandomKey,BG_LastBonusRandomKey;
		int			BG_IndexLocalPlayer,BG_IndexLocalPlayer2;
static	char		LastFire[BG_NBPLAYERMAX];
		BG_VPaddle	BG_ListPaddle[BG_NBPLAYERMAX];
static	ulong		Alea;
static	long		LastClock;

/***********************************************************
* Descriptif :	Initialise le gestionnaire graphique
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_InitSuperviseur()
	{
	int			i,j;
	
	Alea = BG_Clock()|1;
	for (i=0;i<BG_NBPLAYERMAX;i++)
		{
		BG_ListPaddle[i].deltaH = 0;
		BG_ListPaddle[i].deltaV = 0;
		for (j=0;j<4;j++) BG_ListPaddle[i].button[j] = 0;
		}
	}

/***********************************************************
* Descriptif :	Initialise le gestionnaire graphique
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_NewSuperviseur()
	{
	BG_IndexLocalPlayer = BG_ReservePlayer(BG_MaWindow[0]->YellowEnable);
	}

/***********************************************************
* Descriptif :	Initialise le gestionnaire graphique
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_NewSuperviseur2()
	{
	BG_IndexLocalPlayer2 = BG_ReservePlayer(BG_MaWindow[1]->YellowEnable);
	}

/***********************************************************
* Descriptif :	Un CRC, ca fait jamais de mal…
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
int BG_Random(int mask)
	{
	Alea <<= 1;
	if (Alea & 0x80000000L) Alea ^= 0x17B5EA5;
	Alea <<= 1;
	if (Alea & 0x80000000L) Alea ^= 0x17B5EA5;
	return (Alea>>8)%mask;
	}

/***********************************************************
* Descriptif :	Initialise le gestionnaire graphique
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_AbortSuperviseur()
	{
	BG_ReleasePlayer(BG_IndexLocalPlayer);
	}

/***********************************************************
* Descriptif :	Initialise le gestionnaire graphique
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_AbortSuperviseur2()
	{
	BG_ReleasePlayer(BG_IndexLocalPlayer2);
	}

/***********************************************************
* Descriptif :	Alloue un joueur dans la table multi-joueur
*				(renvoie BG_NBPLAYERMAX si c'est plein)
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
int BG_ReservePlayer(char YellowMode)
	{
	int			i;
	
	acquire_sem(BG_semJoueur);
	for (i=0;i<BG_NBPLAYERMAX;i++)
		if (BG_StatusJoueur[i] == FALSE)
			{
			BG_StatusJoueur[i] = -1;
			BG_YellowPlayer[i] = YellowMode;
			break;
			}
	release_sem(BG_semJoueur);
	return i;
	}

/***********************************************************
* Descriptif :	Alloue un joueur dans la table multi-joueur
*				(renvoie BG_NBPLAYERMAX si c'est plein)
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_ReleasePlayer(int index)
	{
	acquire_sem(BG_semJoueur);
	if (BG_ListAppareil[BG_IndexPlayer[index]].Type >= 0)
		{
		BG_RemoveObject(BG_ListAppareil+BG_IndexPlayer[index]);
		BG_ListAppareil[BG_IndexPlayer[index]].Type = -1;
		}
	BG_StatusJoueur[index] = FALSE;
	release_sem(BG_semJoueur);
	}

/***********************************************************
* Descriptif :	initialise l'etat 0 de la gestion multi-joueur
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_ResetSuperviseur()
	{
	int			i;
	
	acquire_sem(BG_semJoueur);
	for (i=0;i<BG_NBPLAYERMAX;i++)
		BG_StatusJoueur[i] = FALSE;
	for (i=0;i<BG_NBAIRCRAFTMAX;i++)
		BG_ListAppareil[i].Type = -1;
	release_sem(BG_semJoueur);
	}

/***********************************************************
* Descriptif :	Initialise le gestionnaire graphique
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_JoinSuperviseur()
	{
	BG_NewGround(32,3570564L);
	BG_ResetGround(84651L);
	}

/***********************************************************
* Descriptif :	Initialise le gestionnaire graphique
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_PlaySuperviseur(BG_Aircraft *ListCamera[2])
	{
	int				i,h,v,ecart;
	long			NewClock;
	float			DureePas;
	BG_Pt3D			Pt;
	BG_VPaddle		*Pad;
	BG_Aircraft		*Obj,*Tir;
	BG_Matrice3_3	MRot;
	
// mode maitre
	if (BG_MasterEnable)
		{
		NewClock = BG_Clock();
		DureePas = ((float)(NewClock-LastClock))*0.002;
		if (DureePas < 0.0) DureePas = 0.05;
		if (DureePas > 0.15) DureePas = 0.15;
		LastClock = NewClock;
	// play players	
		acquire_sem(BG_semJoueur);
		for (i=0;i<BG_NBPLAYERMAX;i++)
		// deplace un joueur actif
			if (BG_StatusJoueur[i] == TRUE)
				{
				Pad = BG_ListPaddle+i;
				Obj = BG_ListAppareil+BG_IndexPlayer[i];
				if (Obj->Type != 1)
					BG_StatusJoueur[i] = -1;
				else
					{
					ecart = 75.0*Obj->Rotation.X3_Y2;
					BG_Matrice3_3Rotation3D((Pad->deltaH/5)&8191,(Pad->deltaV/2)&8191,
											(-Pad->deltaH/2)&8191,&MRot);
					BG_Matrice3_3MultMatrice3_3(MRot,Obj->Rotation,&Obj->Rotation);
					BG_Matrice3_3Rotation3D(ecart&8191,0,0,&MRot);
					BG_Matrice3_3MultMatrice3_3(Obj->Rotation,MRot,&Obj->Rotation);
					if (Pad->button[1]) Obj->Vitesse += 0.01;
					if (Pad->button[2]) Obj->Vitesse -= 0.01;
					if (Obj->Vitesse > 1.0) Obj->Vitesse = 1.0;
					if (Obj->Vitesse < 0.2) Obj->Vitesse = 0.2;
					BG_MoveAircraft(Obj,DureePas);
					if (Pad->button[0] && (!LastFire[i]))
						{
						Pt.X = Obj->Centre.X + 0.13*Obj->Rotation.X1_Y1;
						Pt.Y = Obj->Centre.Y + 0.13*Obj->Rotation.X2_Y1;
						Pt.Z = Obj->Centre.Z + 0.13*Obj->Rotation.X3_Y1;
						Tir = BG_ListAppareil+BG_NewAircraft(2,Obj->Color,&Pt,&Obj->Rotation,2.0);
						}
					LastFire[i] = Pad->button[0];
					}
				}
		// active un joueur reserve
		for (i=0;i<BG_NBPLAYERMAX;i++)
			if (BG_StatusJoueur[i] == -1)
				{
				BG_StatusJoueur[i] = TRUE;
				Pt.X = h = BG_Random(BG_GROUNDMASK);
				Pt.Y = v = BG_Random(BG_GROUNDMASK);
				Pt.Z = BG_Altimetric[INDEX0(h,v)]+3.0;
				BG_IndexPlayer[i] = BG_NewAircraft(1,7-BG_YellowPlayer[i],&Pt,&Rot0,0.5);
				}
	// play missiles
		Obj = BG_ListAppareil;
		for (i=0;i<BG_NBAIRCRAFTMAX;i++)
			{
			if (Obj->Type == 2)
				{
				BG_MoveAircraft(Obj,DureePas);
				}
		// gestion mortalite temporelle
			Obj->Duree -= DureePas;
			if (Obj->Duree < 0)
				if (Obj->Type >= 0)
					{
					Obj->Type = -1;
					BG_RemoveObject(Obj);
					}
			Obj++;
			}
		release_sem(BG_semJoueur);
	// renvoie la reference de la camera locale
		ListCamera[0] = BG_ListAppareil+BG_IndexPlayer[BG_IndexLocalPlayer];
		ListCamera[1] = BG_ListAppareil+BG_IndexPlayer[BG_IndexLocalPlayer2];
		}
// mode esclave
	else
		{
	// renvoie la reference de la camera locale
		ListCamera[0] = BG_ListAppareil+BG_IndexLocalPlayer;
		ListCamera[1] = BG_ListAppareil+BG_IndexLocalPlayer2;
		}
	}

/***********************************************************
* Descriptif :	Initialise le gestionnaire graphique
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_ReadSuperviseur()
	{
// read next command
	BG_MaWindow[0]->Lock();
	acquire_sem(BG_semJoueur);
	BG_GetVPaddle(BG_ListPaddle+BG_IndexLocalPlayer,BG_Clock(),0);
	if (BG_SndPlrEnable)
		BG_GetVPaddle(BG_ListPaddle+BG_IndexLocalPlayer2,BG_Clock(),1);
	release_sem(BG_semJoueur);
	BG_MaWindow[0]->Unlock();
	}

/***********************************************************
* Descriptif :	Initialise le gestionnaire graphique
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_DisposeSuperviseur()
	{
	}







