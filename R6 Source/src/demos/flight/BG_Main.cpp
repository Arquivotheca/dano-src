#include "BG_Erreur.h"
#include "BG_Types3D.h"
#include "BG_Memory.h"
#include "BG_Graphic.h"
#include "BG_Matrice.h"
#include "BG_PaletteManager.h"
#include "BG_PlayerInterface.h"
#include "BG_Math.h"
#include "BG_Fractal.h"
#include "BG_Ground.h"
#include "BG_AirCraft.h"
#include "BG_Goureaud.h"
#include "BG_Main.h"
#include "BG_Digital.h"
#include "BG_Superviseur.h"
#include "BG_Network.h"

static	long	RGB[24] =
	{
	27250L,27250L,65500L,
	60000L,45000L,15000L,
	35000L,65000L,35000L,
	25000L,50000L,25000L,
	65000L,65000L,65000L,
	65000L,35000L,30000L,
	65000L,65000L,5000L,
	65000L,5000L,65000L
	};
	
sem_id				BG_semRender,BG_semRenderIn,BG_semRenderOut,BG_semJoueur,BG_semKill;
int					BG_SizeGround = 64;
int					BG_PaddleRef;
int					BG_PtCampA,BG_PtCampB,BG_NbBonus;
unsigned long		BG_CurseurColor[3];

static	thread_id 	Renderer,Renderer2;
static	BG_Aircraft	*ListCamera[2];

//********************************************************
// main test actuel
void BG_Rendering()
	{
	while (TRUE)
		{
		if (!BG_MasterEnable) acquire_sem(BG_semClient1);
		acquire_sem(BG_semRender);
		BG_PlaySuperviseur(ListCamera);
		if (BG_SndPlrEnable) release_sem(BG_semRenderIn);
		BG_MaWindow[0]->theCanal->DrawLocalGround(0,ListCamera[0]->Centre,&ListCamera[0]->Rotation);
		if (BG_SndPlrEnable) acquire_sem(BG_semRenderOut);
		release_sem(BG_semRender);
		if (!BG_MasterEnable) release_sem(BG_semClient0);
		BG_MaWindow[0]->RefreshFrame(ListCamera[0]);
		if (BG_SndPlrEnable)
			BG_MaWindow[1]->RefreshFrame(ListCamera[1]);
		BG_ReadSuperviseur();
		}
	}

//********************************************************
// main test actuel
void BG_Rendering2()
	{
	while (TRUE)
		{
		acquire_sem(BG_semRenderIn);
		BG_MaWindow[1]->theCanal->DrawLocalGround(1,ListCamera[1]->Centre,&ListCamera[1]->Rotation);
		release_sem(BG_semRenderOut);
		}
	}

//********************************************************
// initialise le moteur de rendu
void BG_Init()
	{
	int			i;	

// lance le moteur de rendu
	BG_semRender = create_sem(0,"SRender");
	BG_semRenderIn = create_sem(0,"SRenderIn");
	BG_semRenderOut = create_sem(0,"SRenderOut");
	BG_semJoueur = create_sem(1,"SJoueur");
	BG_semKill = create_sem(1,"KillWindow");
	
	BG_InitMemory();
	BG_InitSuperviseur();
	BG_InitPlayerInterface();
	BG_InitGraphic();
	if (!BG_Screen24)
		BG_OpenPaletteManager();
	for (i=0;i<8;i++)
		{
		BG_Lumieres[i] = (unsigned char*)BG_GetMemLow(sizeof(unsigned char)*4*256);
		BG_CreatePalette(BG_Lumieres[i],RGB[3*i+0],RGB[3*i+1],RGB[3*i+2]);
		}
	if (!BG_Screen24)
		BG_ClosePaletteManager();
	BG_InitCosTable();
	BG_InitGroundModel();

// choisit les couleurs d'incrustation
	if (BG_Screen24)
	{
		BG_CurseurColor[0] = ((long*)(BG_Lumieres[5]))[255];
		BG_CurseurColor[1] = ((long*)(BG_Lumieres[6]))[255];
		BG_CurseurColor[2] = ((long*)(BG_Lumieres[7]))[255];
	}
	else
	{
		BG_CurseurColor[0] = (BG_Lumieres[5][511])*0x01010101L;
		BG_CurseurColor[1] = (BG_Lumieres[6][511])*0x01010101L;
		BG_CurseurColor[2] = (BG_Lumieres[7][511])*0x01010101L;
	}
// lance le thread de rendering
	Renderer = spawn_thread((thread_entry)BG_Rendering,"Render",B_NORMAL_PRIORITY,0L);
	resume_thread(Renderer);
	Renderer2 = spawn_thread((thread_entry)BG_Rendering2,"Render2",B_NORMAL_PRIORITY,0L);
	resume_thread(Renderer2);

// initialisation terminee
	BG_QuitEnable = TRUE;
	BG_MaWindow[0]->CheckMenuBar();
	BG_MaWindow[1]->CheckMenuBar();
	BG_MaWindow[0]->SetFlags(0);
	BG_MaWindow[1]->SetFlags(0);
	}
	
//********************************************************
// libere le moteur de rendu
void BG_Kill()
{
	BG_MaWindow[0]->Lock();
	BG_MaWindow[1]->Lock();
	kill_thread(Renderer);
	kill_thread(Renderer2);
	BG_MaWindow[1]->Unlock();
	BG_MaWindow[0]->Unlock();
}

//********************************************************
// libere le moteur de rendu
void BG_Dispose()
	{
// stop le renderer
	delete_sem(BG_semKill);
	delete_sem(BG_semRender);
	delete_sem(BG_semRenderIn);
	delete_sem(BG_semRenderOut);
	delete_sem(BG_semJoueur);

	BG_SelectNetworkMode(BG_NETWORKNULL);

	BG_DisposeGroundModel();
	BG_DisposeCosTable();
	BG_DisposeGraphic();
	BG_DisposePlayerInterface();
	BG_DisposeSuperviseur();
	BG_DisposeMemory();
	}

//********************************************************
// calcule un nouveau terrain et change ses dimensions
void BG_NewGround(int taille,long random)
	{
	BG_GROUNDDH = BG_GROUNDDV = taille;
	BG_GROUNDMASK = taille-1;
	if (taille == 128)
		BG_GROUNDSHIFT = 7;
	else if (taille == 64)
		BG_GROUNDSHIFT = 6;
	else if (taille == 32)
		BG_GROUNDSHIFT = 5;
	else Erreur();
	BG_LastGroundRandomKey = random;
	BG_CalculMountains(BG_Altimetric,BG_GROUNDDH,BG_LastGroundRandomKey);
	BG_ProcessVirtualGround();
	}

//********************************************************
// Remet une partie sur un terrain intialise
void BG_ResetGround(long random)
	{
	BG_PtCampA = 0;
	BG_PtCampB = 0;
	BG_NbBonus = 0;
	BG_LastBonusRandomKey = random;
	BG_AddBonus(BG_LastBonusRandomKey);
	}

/***********************************************************
* What :	Create a scaled tramed palette for a RGB color
*			
* State : 	In progress
* Last Modif :	13/10/95 (Pierre)
*					
***********************************************************/
enum {
  iA = 3,
  iR = 2,
  iG = 1,
  iB = 0
};

void BG_CreatePalette(unsigned char *eclairage,long Red,long Green,long Blue)
{
	long			i;
	BG_Couleur 		coul;
	unsigned char	P1,I1,P2,I2;

	if (BG_Screen24)
	{
		for (i=0;i<BG_LOWBORDER;i++)
		{
			eclairage[i*4+0] = 0;
			eclairage[i*4+1] = 0;
			eclairage[i*4+2] = 0;
			eclairage[i*4+3] = 0;
		}
		Red /= (256-BG_LOWBORDER-BG_HIGHBORDER);
		Green /= (256-BG_LOWBORDER-BG_HIGHBORDER);
		Blue /= (256-BG_LOWBORDER-BG_HIGHBORDER);
		for (i=BG_LOWBORDER;i<256-BG_HIGHBORDER;i++)
		{
			coul.Red = (i-BG_LOWBORDER)*Red;
			coul.Green = (i-BG_LOWBORDER)*Green;
			coul.Blue = (i-BG_LOWBORDER)*Blue;
			eclairage[i*4+iB] = coul.Blue>>8;
			eclairage[i*4+iG] = coul.Green>>8;
			eclairage[i*4+iR] = coul.Red>>8;
			eclairage[i*4+iA] = 0;
		}
		coul.Red = (256-BG_LOWBORDER-BG_HIGHBORDER)*Red;
		coul.Green = (256-BG_LOWBORDER-BG_HIGHBORDER)*Green;
		coul.Blue = (256-BG_LOWBORDER-BG_HIGHBORDER)*Blue;
		for (i=256-BG_HIGHBORDER;i<256;i++)
		{
			eclairage[i*4+iB] = coul.Blue>>8;
			eclairage[i*4+iG] = coul.Green>>8;
			eclairage[i*4+iR] = coul.Red>>8;
			eclairage[i*4+iA] = 0;
		}
	}
	else
	{
		coul.Red = 0;
		coul.Green = 0;
		coul.Blue = 0;
		coul.Total = coul.Red+coul.Green+coul.Blue;
		BG_GetBestColor(&coul,&P1,&I1,&P2,&I2);
		for (i=0;i<BG_LOWBORDER;i++)
		{
			eclairage[i*2+0] = P1;
			eclairage[i*2+1] = I1;
			eclairage[i*2+512] = I2;
			eclairage[i*2+513] = P2;
		}
		Red /= (256-BG_LOWBORDER-BG_HIGHBORDER);
		Green /= (256-BG_LOWBORDER-BG_HIGHBORDER);
		Blue /= (256-BG_LOWBORDER-BG_HIGHBORDER);
		for (i=BG_LOWBORDER;i<256-BG_HIGHBORDER;i++)
		{
			coul.Red = (i-BG_LOWBORDER)*Red;
			coul.Green = (i-BG_LOWBORDER)*Green;
			coul.Blue = (i-BG_LOWBORDER)*Blue;
			coul.Total = coul.Red+coul.Green+coul.Blue;
			BG_GetBestColor(&coul,&P1,&I1,&P2,&I2);
			eclairage[i*2+0] = P1;
			eclairage[i*2+1] = I1;
			eclairage[i*2+512] = I2;
			eclairage[i*2+513] = P2;
		}
		coul.Red = (256-BG_LOWBORDER-BG_HIGHBORDER)*Red;
		coul.Green = (256-BG_LOWBORDER-BG_HIGHBORDER)*Green;
		coul.Blue = (256-BG_LOWBORDER-BG_HIGHBORDER)*Blue;
		coul.Total = coul.Red+coul.Green+coul.Blue;
		BG_GetBestColor(&coul,&P1,&I1,&P2,&I2);
		for (i=256-BG_HIGHBORDER;i<256;i++)
		{
			eclairage[i*2+0] = P1;
			eclairage[i*2+1] = I1;
			eclairage[i*2+512] = I2;
			eclairage[i*2+513] = P2;
		}
	}
}








