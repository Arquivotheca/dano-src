#include "BG_Erreur.h"
#include "BG_Types3D.h"
#include "BG_Memory.h"
#include "BG_Graphic.h"
#include "BG_Matrice.h"
#include "BG_Projection.h"
#include "BG_Main.h"
#include "BG_PlayerInterface.h"
#include "BG_Ground.h"
#include "BG_AirCraft.h"
#include "BG_Goureaud.h"
#include "BG_Superviseur.h"
#include "BG_Network.h"
#include "BG_Digital.h"
#include <InterfaceDefs.h>
#include <string.h>
#include <stdio.h>
#include <Debug.h>
#include <Screen.h>
 
// codes d'evenements des menus
#define	MENU_QUIT2		1000
#define	MENU_QUIT		1001

#define MENU_JOIN		2000
#define MENU_PLAYSINGLE	2001
#define MENU_PLAYNETWORK 2002
#define MENU_PAUSE		2003
#define MENU_RESUME		2004
#define MENU_ABORT		2005
#define	MENU_SNDPLR		2006

#define MENU_JAUNE		2100
#define MENU_VIOLET		2101
#define	MENU_SIZE32		2102
#define	MENU_SIZE64		2103
#define	MENU_SIZE128	2104
#define	MENU_GCRASH		2105

#define	MENU_MAP		3000
#define	MENU_FOCALE1	3001
#define	MENU_FOCALE2	3002
#define	MENU_FOCALE3	3003
#define	MENU_FOCALE4	3004
#define	MENU_FOCALE5	3005

#define	MENU_KEYBOARD	4000
#define	MENU_JOYSTICK1	4001
#define	MENU_JOSYTICK1B	4002
#define	MENU_JOSYTICK1C	4003
#define	MENU_JOSYTICK1D	4004
#define	MENU_JOSYTICK2	4010
#define	MENU_JOSYTICK2B	4011
#define	MENU_JOSYTICK2C	4012
#define	MENU_JOSYTICK2D	4013

// palette entry struct
typedef struct
	{
	long	red;
	long	green;
	long	blue;
	long	index;
	} rgb_color3;

// acces to assembly language primitive
extern "C" {
short           tabgauche[600];
short           tabdroite[600];
long            OffScreen_largeur,OffScreen_baseAddr;
short           tabgauche2[600];
short           tabdroite2[600];
long            OffScreen_largeur2,OffScreen_baseAddr2;
}

// graphic globals
unsigned short	*BG_thePalette;		// current palette
BG_Window		*BG_MaWindow[2];
char			*BG_OffscreenMap;

// global status
Boolean		BG_IsRunning = FALSE;
Boolean		BG_QuitEnable = FALSE;
Boolean		BG_MasterEnable = TRUE;
Boolean		BG_GameEnable = FALSE;
Boolean		BG_GroundEnable = FALSE;
Boolean		BG_SndPlrEnable = FALSE;
Boolean		BG_GroundCrash = FALSE;
Boolean     BG_Screen24 = FALSE;

// menu interface
static	float		BG_ListFocales[5] = {0.6,0.73,0.9,1.15,1.5};
static	short		SizeGround[3] = {32,64,128};

/***********************************************************
* Descriptif :	Initialise le gestionnaire graphique
*				
* Etat : En cours
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_InitGraphic()
	{
	int				i;
	BRect			newcadre;

// check screen depth
	if (BScreen(B_MAIN_SCREEN_ID).ColorSpace() != B_CMAP8)
		BG_Screen24 = TRUE;
	else BG_Screen24 = FALSE;		   
// alloue le tampon de stockage local de la palette
	if (!BG_Screen24)
	{
		BScreen screen( B_MAIN_SCREEN_ID );
		const color_map* OldColor = screen.ColorMap();
		
		if( OldColor != NULL ) {
			BG_thePalette = (unsigned short*)BG_GetMemLow(256*3*sizeof(unsigned short));
			for (i=0;i<256;i++)
			{
				BG_thePalette[3*i+0] = 257L*OldColor->color_list[i].red;
				BG_thePalette[3*i+1] = 257L*OldColor->color_list[i].green;
				BG_thePalette[3*i+2] = 257L*OldColor->color_list[i].blue;
			}
		}
	}
// ouverture de la fenetre
	newcadre.Set(70.0,32.0,529.0,327.0);
	BG_MaWindow[0] = new BG_Window(newcadre,"Flight",B_TITLED_WINDOW, B_NOT_CLOSABLE, true);
	BG_MaWindow[0]->SetSizeLimits(80.0,BG_WINDOWMAXH,60.0,BG_WINDOWMAXV);
	BG_MaWindow[0]->SetZoomLimits(BG_WINDOWMAXH,BG_WINDOWMAXV);
	BG_MaWindow[0]->Show();
// ouverture de la fenetre
	newcadre.Set(70.0,335.0,529.0,629.0);
	BG_MaWindow[1] = new BG_Window(newcadre,"Second Player",B_TITLED_WINDOW, B_NOT_CLOSABLE, false);
	BG_MaWindow[1]->SetSizeLimits(80.0,BG_WINDOWMAXH,60.0,BG_WINDOWMAXV);
	BG_MaWindow[1]->SetZoomLimits(BG_WINDOWMAXH,BG_WINDOWMAXV);
	BG_MaWindow[1]->Run();
// ouverture de la map
	if (BG_Screen24)
		BG_OffscreenMap = (char*)BG_GetMemLow(256*256*4);
	else
		BG_OffscreenMap = (char*)BG_GetMemLow(256*256);
	}

/***********************************************************
* Descriptif :	Libere le gestionnaire graphique
*				
* Etat : En cours
* Dernière Modif :	10/07/95 (Pierre)
*					
***********************************************************/
void BG_DisposeGraphic()
	{
	BG_FreeMemLow((char*)BG_OffscreenMap);
	if (!BG_Screen24)
		BG_FreeMemLow((char*)BG_thePalette);
	}

/***********************************************************
* Descriptif :	Time base in 1024us (~1ms).
*				
* Etat : En cours
* Dernière Modif :	02/10/95 (Pierre)
*					
***********************************************************/
long BG_Clock()
	{
	return (long)(system_time()>>10);
	}

	
/***********************************************************
* Descriptif :	Constructeur par defaut
*				
* Etat : En cours
* Derniere Modif :	07/10/94 (Pierre)
*					
***********************************************************/
BG_Window::BG_Window(BRect frame,const char *title,window_type type,ulong flags,bool master):
				   BWindow(frame,title,type,flags)
	{
	int			i, count;
	BRect		menuRect,newcadre;
	BMenu		*theMenu;
	BPopUpMenu	*mainMenu;
	BMenuBar	*fMenuBar;

// menubar
	menuRect.Set(0.0, 0.0, 2000.0, 14.0);
	fMenuBar = new BMenuBar(menuRect, "BG");
	fMenuBar->SetBorder(B_BORDER_FRAME);

// menu Play
	if (master)
		{
		MenuPlay = new BMenu("File");
		itemSndPlr = new BMenuItem("Second Player",new BMessage(MENU_SNDPLR));
		itemPlay = new BMenuItem("Play Game",new BMessage(MENU_PLAYSINGLE),'N');
		itemPlayNet = new BMenuItem("Play NetGame",new BMessage(MENU_PLAYNETWORK));
		itemJoin = new BMenuItem("Join NetGame",new BMessage(MENU_JOIN));
		itemPause = new BMenuItem("Pause",new BMessage(MENU_PAUSE),'P');
		itemResume = new BMenuItem("Resume",new BMessage(MENU_RESUME),'R');
		itemAbort = new BMenuItem("Abort",new BMessage(MENU_ABORT),'A');
		itemQuit = new BMenuItem("Quit",new BMessage(MENU_QUIT),'Q');
		fMenuBar->AddItem(MenuPlay);
		MenuPlay->AddItem(itemPlay);
		MenuPlay->AddItem(itemPlayNet);
		MenuPlay->AddItem(itemJoin);
		MenuPlay->AddItem(new BSeparatorItem());
		MenuPlay->AddItem(itemSndPlr);
		MenuPlay->AddItem(new BSeparatorItem());
		MenuPlay->AddItem(itemPause);
		MenuPlay->AddItem(itemResume);
		MenuPlay->AddItem(itemAbort);
		MenuPlay->AddItem(new BSeparatorItem());
		MenuPlay->AddItem(itemQuit);
		}
	else
	{
		MenuPlay = new BMenu("File");
		itemQuit = new BMenuItem("Quit Second Player",new BMessage(MENU_QUIT2),'Q');
		fMenuBar->AddItem(MenuPlay);
		MenuPlay->AddItem(itemQuit);
	}

// menu Options
	theMenu = new BMenu("Options");
	fMenuBar->AddItem(theMenu);
	itemYellow = new BMenuItem("Yellow team",new BMessage(MENU_JAUNE),'Y');
	itemRed = new BMenuItem("Purple team",new BMessage(MENU_VIOLET),'Z');
	theMenu->AddItem(itemYellow);
	theMenu->AddItem(itemRed);
	if (master)
		{
		itemSize32 = new BMenuItem("Small 32x32",new BMessage(MENU_SIZE32),'1');
		itemSize64 = new BMenuItem("Medium 64x64",new BMessage(MENU_SIZE64),'2');
		itemSize128 = new BMenuItem("Big 128x128",new BMessage(MENU_SIZE128),'3');
		itemGroundCrash = new BMenuItem("Crash on Ground",new BMessage(MENU_GCRASH));
		theMenu->AddItem(new BSeparatorItem());
		theMenu->AddItem(itemSize32);
		theMenu->AddItem(itemSize64);
		theMenu->AddItem(itemSize128);
		theMenu->AddItem(new BSeparatorItem());
		theMenu->AddItem(itemGroundCrash);
		}

// menu View
	MenuView = new BMenu("View");
	itemMap = new BMenuItem("Map",new BMessage(MENU_MAP),'M');
	itemFocale[0] = new BMenuItem("Widest view",new BMessage(MENU_FOCALE1),'5');
	itemFocale[1] = new BMenuItem("Wide view",new BMessage(MENU_FOCALE2),'6');
	itemFocale[2] = new BMenuItem("Medium view",new BMessage(MENU_FOCALE3),'7');
	itemFocale[3] = new BMenuItem("Narrow view",new BMessage(MENU_FOCALE4),'8');
	itemFocale[4] = new BMenuItem("Narrowest view",new BMessage(MENU_FOCALE5),'9');
	fMenuBar->AddItem(MenuView);
	MenuView->AddItem(itemMap);
	MenuView->AddItem(new BSeparatorItem());
	for (i=0;i<5;i++) MenuView->AddItem(itemFocale[i]);

// menu control
	theMenu = new BMenu("Control");
	count = BG_GetDeviceCount();
	for (i=0; i<count; i++) {
		if (i == 0)
			itemControl[i] = new BMenuItem(BG_GetDeviceName(i), new BMessage(MENU_KEYBOARD),'K');
		else if (i == 1)
			itemControl[i] = new BMenuItem(BG_GetDeviceName(i), new BMessage(MENU_JOYSTICK1+(i-1)),'J');
		else
			itemControl[i] = new BMenuItem(BG_GetDeviceName(i), new BMessage(MENU_JOYSTICK1+(i-1)));
	}
	for (i=count; i<5; i++)
		itemControl[i] = NULL;
	fMenuBar->AddItem(theMenu);
	for (i=0;i<count;i++) theMenu->AddItem(itemControl[i]);

	AddChild(fMenuBar);
    menuHeight = fMenuBar->Bounds().Height();

// create the attached map window
	newcadre.Set(frame.right+10.0,frame.top+30.0,frame.right+265.0,frame.top+285.0);
	if (master)
		MapWindow = new BWindow(newcadre,"Flight Map",B_TITLED_WINDOW,
						B_NOT_ZOOMABLE | 
						B_NOT_CLOSABLE | B_NOT_RESIZABLE);
	else
		MapWindow = new BWindow(newcadre,"Second Map",B_TITLED_WINDOW,
						B_NOT_ZOOMABLE | 
						B_NOT_CLOSABLE | B_NOT_RESIZABLE);
	newcadre.OffsetTo(B_ORIGIN);
	MapView = new BG_ViewMap(newcadre);
	MapWindow->AddChild(MapView);
	
// open the attached view canal3D
	newcadre.Set(0.0, menuHeight+1.0, frame.Width(), frame.Height());
	theCanal = new BG_Canal3D(newcadre);
	theCanal->EraseCanal();
	AddChild(theCanal);

// default options
	MapEnable = FALSE;
	YellowEnable = master;
	MasterEnable = master;
	if (master)
		{
		PaddleRef = BG_PADDLEKEYBOARD;
		BG_SelectVPaddle(BG_PADDLEKEYBOARD,0);
		}
	else
		PaddleRef = BG_PADDLEKEYBOARD;
		
// Show map if necessary
	if (master && MapEnable) MapWindow->Show();
	else MapWindow->Run();

	CheckMenuBar();
}

/***********************************************************
* Descriptif :	Met a jour les Enable et Mark du menubar
*				
* Etat : En cours
* Derniere Modif :	07/10/94 (Pierre)
*					
***********************************************************/
void BG_Window::CheckMenuBar()
	{
	int			i;
	
// menu Be
	itemQuit->SetEnabled(BG_QuitEnable);
	
// menu Play
	if (MasterEnable)
		{
		itemSndPlr->SetEnabled(!BG_SndPlrEnable);
		itemPlay->SetEnabled(!BG_GameEnable);
		itemPlayNet->SetEnabled(!BG_GameEnable);
		itemJoin->SetEnabled(!BG_GameEnable);
		itemPause->SetEnabled(BG_IsRunning);
		itemResume->SetEnabled((!BG_IsRunning) && (BG_GameEnable));
		itemAbort->SetEnabled(BG_GameEnable);
		MenuPlay->SetEnabled(BG_QuitEnable);
		}

// menu options
	itemYellow->SetEnabled(!BG_GameEnable);
	itemYellow->SetMarked(YellowEnable);
	itemRed->SetEnabled(!BG_GameEnable);
	itemRed->SetMarked(!YellowEnable);
	if (MasterEnable)
		{
		itemSize32->SetEnabled(!BG_GameEnable);
		itemSize64->SetEnabled(!BG_GameEnable);
		itemSize128->SetEnabled(!BG_GameEnable);
		itemSize32->SetMarked(BG_SizeGround == 32);
		itemSize64->SetMarked(BG_SizeGround == 64);
		itemSize128->SetMarked(BG_SizeGround == 128);
		itemGroundCrash->SetMarked(BG_GroundCrash);
		}

// menu View
	itemMap->SetMarked(MapEnable);
	for (i=0;i<5;i++)
		itemFocale[i]->SetMarked(theCanal->indexFocale == i);
	MenuView->SetEnabled(BG_QuitEnable);
	
// menu control
	for (i=0; i<5; i++)
		if (itemControl[i])
			itemControl[i]->SetMarked(PaddleRef == i);
	}
	
/***********************************************************
* Descriptif :	Gere le resize de la window
*				
* Etat : En cours
* Derniere Modif :	07/10/94 (Pierre)
*					
***********************************************************/
void BG_Window::FrameResized(float new_width,float new_height)
	{	
	if (BG_IsRunning) acquire_sem(BG_semRender);
	new_width = Frame().Width();
	new_height = Frame().Height()-menuHeight-1.0;
// Enregistre les nouvelles dimensions
	theCanal->Largeur = ((int)new_width+3)&0xFFC;
	theCanal->Hauteur = (int)new_height;
	if (theCanal->Largeur > BG_WINDOWMAXH) theCanal->Largeur = BG_WINDOWMAXH;
	if (theCanal->Hauteur > BG_WINDOWMAXV)
		theCanal->Hauteur = BG_WINDOWMAXV;
// Initialise le BG_ControlView
	theCanal->CanalView.offsetH = theCanal->Largeur/2;
	theCanal->CanalView.offsetV = theCanal->Hauteur/2;
	theCanal->ChangeCanalView(new_width*BG_ListFocales[theCanal->indexFocale]);
	if (BG_IsRunning) release_sem(BG_semRender);
	}

/***********************************************************
* Descriptif :	Dispatcher de message Menu
*				
* Etat : En cours
* Derniere Modif :	07/10/94 (Pierre)
*					
***********************************************************/
void BG_Window::MessageReceived(BMessage* theMessage)
	{
	switch(theMessage->what)
		{
// settings
		case MENU_JAUNE :
			YellowEnable = TRUE;
			break;
		case MENU_VIOLET :
			YellowEnable = FALSE;
			break;
		case MENU_SIZE32 :
			BG_SizeGround = 32;
			break;
		case MENU_SIZE64 :
			BG_SizeGround = 64;
			break;
		case MENU_SIZE128 :
			BG_SizeGround = 128;
			break;
		case MENU_FOCALE1 :
		case MENU_FOCALE2 :
		case MENU_FOCALE3 :
		case MENU_FOCALE4 :
		case MENU_FOCALE5 :
			theCanal->indexFocale = theMessage->what-MENU_FOCALE1;
			FrameResized(Bounds().Width()+12.0,Frame().Height()+29.0);
			break;
		case MENU_KEYBOARD :
		case MENU_JOYSTICK1 :
		case MENU_JOSYTICK1B :
		case MENU_JOSYTICK1C :
		case MENU_JOSYTICK1D :
			PaddleRef = theMessage->what-MENU_KEYBOARD;
			BG_SelectVPaddle(PaddleRef,1-MasterEnable);
			break;
		case MENU_MAP :
			if (MapEnable)
				{
				MapEnable = FALSE;
				MapWindow->Hide();
				}
			else
				{
				MapEnable = TRUE;
				MapWindow->Show();
				}
			break;
		case MENU_GCRASH :
			BG_GroundCrash = 1-BG_GroundCrash;
			break;
// game control
		case MENU_PLAYSINGLE :
		// create a new ground
			BG_NewGround(BG_SizeGround,BG_Clock());
			BG_ResetGround(BG_Clock());
			BG_GroundEnable = TRUE;
		// allow game control
			BG_MasterEnable = TRUE;
		// init supervisor
			BG_ResetSuperviseur();
			BG_NewSuperviseur();
			if (BG_SndPlrEnable) BG_NewSuperviseur2();
		// launch game
			BG_GameEnable = TRUE;
			BG_IsRunning = TRUE;
			release_sem(BG_semRender);
			break;
		case MENU_PLAYNETWORK :
		// create a new ground
			BG_NewGround(BG_SizeGround,BG_Clock());
			BG_ResetGround(BG_Clock());
			BG_GroundEnable = TRUE;
		// allow game control
			BG_MasterEnable = TRUE;
		// network on
			BG_SelectNetworkMode(BG_NETWORKMASTER);
		// init supervisor
			BG_NewSuperviseur();
			if (BG_SndPlrEnable) BG_NewSuperviseur2();
		// launch game
			BG_GameEnable = TRUE;
			BG_IsRunning = TRUE;
			release_sem(BG_semRender);
			break;
		case MENU_JOIN :
		// inhibit game control
			BG_MasterEnable = FALSE;
		// network on in slave mode
			BG_SelectNetworkMode(BG_NETWORKSLAVE);
		// init supervisor
			BG_JoinSuperviseur();
		// launch game
			BG_GameEnable = TRUE;
			BG_IsRunning = TRUE;
			release_sem(BG_semRender);
			break;
		case MENU_PAUSE :
			if (BG_IsRunning) acquire_sem(BG_semRender);
			BG_IsRunning = FALSE;
			break;
		case MENU_RESUME :
			BG_IsRunning = TRUE;
			release_sem(BG_semRender);
			break;
		case MENU_QUIT :
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		case MENU_ABORT :
		// pause game
			if (BG_IsRunning) acquire_sem(BG_semRender);
			BG_IsRunning = FALSE;
		// close the network
			BG_SelectNetworkMode(BG_NETWORKNULL);
		// destroy players
			BG_AbortSuperviseur();
			if (BG_SndPlrEnable) BG_AbortSuperviseur2();
		// stop game
			BG_GameEnable = FALSE;
			break;
		case MENU_SNDPLR :
			if (BG_GameEnable)
				{
			// activate second player in the flight
				if (BG_IsRunning) acquire_sem(BG_semRender);
				BG_NewSuperviseur2();
				if (BG_IsRunning) release_sem(BG_semRender);
				}
			BG_SndPlrEnable = TRUE;
			BG_MaWindow[1]->Show();
			BG_MaWindow[0]->Activate();
			if (BG_MaWindow[1]->MapEnable) {
				BG_MaWindow[1]->MapWindow->Show();
			}
			break;
		case MENU_QUIT2 :
		// desactivate second player
			if (BG_GameEnable)
				{
			// game on
				if (BG_IsRunning) acquire_sem(BG_semRender);
				BG_SndPlrEnable = FALSE;
				BG_AbortSuperviseur2();
				Hide();
				if (MapEnable) MapWindow->Hide();
				if (BG_IsRunning) release_sem(BG_semRender);
				}
			else
				{
			// game off
				BG_SndPlrEnable = FALSE;
				Hide();
				if (MapEnable) MapWindow->Hide();
				}
			BG_MaWindow[0]->CheckMenuBar();
			break;
		}
	CheckMenuBar();
	}

/***********************************************************
* Descriptif :	Libere le gestionnaire graphique
*				
* Etat : En cours
* Derniere Modif :	07/10/94 (Pierre)
*					
***********************************************************/
bool BG_Window::QuitRequested() {
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

/***********************************************************
* Descriptif :	Libere le gestionnaire graphique
*				
* Etat : En cours
* Derniere Modif :	07/10/94 (Pierre)
*					
***********************************************************/
static bool FirstKill = true;

void BG_Window::Quit() {
	acquire_sem(BG_semKill);
	if (FirstKill) {
		if (BG_GameEnable)
			BG_MaWindow[0]->MessageReceived(new BMessage(MENU_ABORT));
		BG_Kill();
		if (BG_SndPlrEnable)
			BG_MaWindow[1]->MessageReceived(new BMessage(MENU_QUIT2));
		FirstKill = FALSE;
	}
	release_sem(BG_semKill);
	BWindow::Quit();
}

/***********************************************************
* Descriptif :	Refresh the last interface and draw a new frame
*				
* Etat : En cours
* Derniere Modif :	11/01/96 (Pierre)
*					
***********************************************************/
void BG_Window::RefreshFrame(BG_Aircraft *camera)
	{
	BRect			Draw(0.0,0.0,1000.0,1000.0);
	long			color;

	BG_DrawDigital(theCanal,0,5,3,BG_PtCampA,BG_CurseurColor[1]);
	BG_DrawDigital(theCanal,1,5,3,BG_PtCampB,BG_CurseurColor[2]);
	if (YellowEnable) color = BG_CurseurColor[1];
	else color = BG_CurseurColor[2];
	BG_DrawDigital(theCanal,2,5,3,(int)(camera->Vitesse*1500.0),color);
	BG_DrawDigital(theCanal,3,5,3,BG_NbBonus,BG_CurseurColor[0]);
	BG_DrawViseur(theCanal,color);
	acquire_sem(BG_semKill);
	Lock();
	theCanal->Draw(Draw);
	if (MapEnable)
		{
		MapWindow->Lock();
		MapView->Draw2(MasterEnable);
		MapWindow->Unlock();
		}
	Unlock();
	release_sem(BG_semKill);
	}

/***********************************************************
* Descriptif : Initialise un canal de trace 3D
*				
* Etat : En cours
* Derniere Modif :	27/07/95 (Pierre)
*					
***********************************************************/
BG_Canal3D::BG_Canal3D(BRect frame):BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW) {
	long    LargeurZone, HauteurZone;
	float   menuHeight;

	menuHeight = frame.top;
	LargeurZone = (int32)frame.Width()+1;
	HauteurZone = (int32)frame.Height()+1;

	SetDrawingMode(B_OP_COPY);
// calcul le rectangle enveloppant
	screenRect.Set(0.0, 0.0, BG_WINDOWMAXH, BG_WINDOWMAXV-menuHeight);
	if (BG_Screen24)
		theBitmap = new BBitmap(screenRect, B_RGB_32_BIT);
	else
		theBitmap = new BBitmap(screenRect, B_COLOR_8_BIT);
// Calcule la taille de l'offscreen
	if (BG_Screen24)
		rowBytes = theBitmap->BytesPerRow();
	else
		rowBytes = theBitmap->BytesPerRow();
	baseAddr = (char*)(theBitmap->Bits());
// memorise les informations crutiales concernant l'offscreen
	OffScreen_largeur = rowBytes;
	OffScreen_baseAddr = (long)baseAddr;
// Enregistre les dimensions
	Largeur = (LargeurZone+3)&0xFFC;
	Hauteur = HauteurZone;
// Initialise le BG_ControlView
	indexFocale = 0;
	CanalView.offsetH = LargeurZone/2;
	CanalView.offsetV = HauteurZone/2;
	ChangeCanalView(((float)LargeurZone)*BG_ListFocales[indexFocale]);
// local map for render
	InitLocalMap();
}

/***********************************************************
* Descriptif :	Affichage de l'offscreen si besoin est
*				
* Etat : En cours
* Derniere Modif :	07/10/94 (Pierre)
*					
***********************************************************/
BG_Canal3D::~BG_Canal3D() {
    delete theBitmap;
}

/***********************************************************
* Descriptif :	Affichage de l'offscreen si besoin est
*				
* Etat : En cours
* Derniere Modif :	07/10/94 (Pierre)
*					
***********************************************************/
void BG_Canal3D::Draw(BRect where) {
	DrawBitmap(theBitmap,
			   BRect(0.0,0.0,Largeur-1.0,Hauteur-1.0),
			   BRect(0.0,0.0,Largeur-1.0,Hauteur-1.0));
}

/***********************************************************
* Descriptif :	Change l'ouverture et la focale d'un canal
*				(ouverture en 1024iem)
* Etat : Stable
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_Canal3D::ChangeCanalView(float zoom)
	{
	long		DemiLargeur,DemiHauteur;
	
	DemiLargeur = Largeur>>1;
	DemiHauteur = Hauteur>>1;
	CanalView.zoom = zoom;
	CanalView.YXmin = -((float)DemiLargeur+0.5)/zoom;
	CanalView.YXmax = ((float)DemiLargeur+0.5)/zoom;
	CanalView.ZXmin = -((float)DemiHauteur+0.5)/zoom;
	CanalView.ZXmax = ((float)DemiHauteur+0.5)/zoom;
	CanalView.Hmin = CanalView.offsetH-DemiLargeur;
	CanalView.Hmax = CanalView.offsetH+DemiLargeur;
	CanalView.Vmin = CanalView.offsetV-DemiHauteur;
	CanalView.Vmax = CanalView.offsetV+DemiHauteur;
	}

/***********************************************************
* Descriptif :	 Efface l'offscreen du canal
*				
* Etat : Stable
* Dernière Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_Canal3D::EraseCanal()
{
	if (BG_Screen24)
		BG_EraseTampon((uint32*)baseAddr,rowBytes,Largeur*4,Hauteur,0L);
	else
		BG_EraseTampon((uint32*)baseAddr,rowBytes,Largeur,Hauteur,0L);
	}

/***********************************************************
* What :	Extract the model of the local part of the ground	
*			corresponding to the position of the camera.
* State : 	In progress
* Last Modif :	16/10/95 (Pierre)
*					
***********************************************************/
void BG_Canal3D::InitLocalMap()
	{
	int			i,j;
	BG_Pt3D		*thePt;
	
// init the map for point
	MapPoint = (BG_Pt3D*)BG_GetMemLow(sizeof(BG_Pt3D)*BG_MAPSIZE*BG_MAPSIZE);
	thePt = MapPoint;
	for (i=0;i<BG_MAPSIZE;i++)
		for (j=0;j<BG_MAPSIZE;j++)
			{
			thePt->X = (float)j; 
			thePt->Y = (float)i; 
			thePt++;
			}
// allocate buffer for square
	MapSquare = (BG_Square*)BG_GetMemLow(sizeof(BG_Square)*BG_MAPSIZE*BG_MAPFULL);
// allocate buffer for triangle
	MapTriangle = (BG_Triangle*)BG_GetMemLow(sizeof(BG_Triangle)*(BG_NBCLIPPTMAX/2));
// allocate square sort buffer
	SquareOrder = (short*)BG_GetMemLow(sizeof(short)*(BG_MAPFULL*BG_MAPFULL+(BG_NBCLIPPTMAX/2)));
// allocate buffer to project point and cliping point
	MapPtProj = (BG_Point3D*)BG_GetMemLow(sizeof(BG_Point3D)*(BG_MAPSIZE*BG_MAPSIZE+BG_NBCLIPPTMAX));
// allocate square sort buffer
	BorderLight = (short*)BG_GetMemLow(sizeof(short)*BG_MAPFULL*BG_MAPFULL);
	for (i=0;i<BG_MAPFULL*BG_MAPFULL;i++) BorderLight[i] = 256;
	BorderConvert = (short*)BG_GetMemLow(sizeof(short)*512);
	for (i=BG_LOWBORDER;i<256;i++) BorderConvert[i+256] = i;
	for (i=0;i<256+BG_LOWBORDER;i++) BorderConvert[i] = BG_LOWBORDER;
	}

/***********************************************************
* What :	Extract the model of the local part of the ground	
*			corresponding to the position of the camera.
* State : 	In progress
* Last Modif :	17/10/95 (Pierre)
*					
***********************************************************/
void BG_Canal3D::DrawLocalGround(	int				indexWindow,
									BG_Pt3D			Camera,
									BG_Matrice3_3	*CameraRot)
	{
	int				i,j,index,imin,imax;
	short			value,value0,value1,value2,value3;
	BG_Pt3D			*thePt;
	BG_VGCase		*theCase;
	BG_Square		*theSquare;
	BG_Point3D		*theProj;
	BG_Triangle		*theTriangle;
	BG_GoureaudPt	tab[4];
	BG_ControlView	*theView;

// erase canal
	theView = &CanalView;
	EraseCanal();
// copy view parameters
	BG_Camera = &Camera;
	BG_CameraRot = CameraRot;
	BG_theView = theView;
// automatic calcul of the DrawingSquare
	CaseV = (((int)Camera.Y)-BG_MAPHALF)&BG_GROUNDMASK;
	CaseH = (((int)Camera.X)-BG_MAPHALF)&BG_GROUNDMASK;
// move the camera in the local repere
	Camera.Y += (float)(BG_MAPHALF-(int)Camera.Y);
	Camera.X += (float)(BG_MAPHALF-(int)Camera.X);
// make a copy of the ControlView pointer for clipping use.
	LocalView = theView;
// postfirst, preprocess the lighting with distance
	value0 = 64-(int)((Camera.Y-BG_MAPHALF)*64.0);
	value1 = 64-(int)((BG_MAPHALF+1.0-Camera.Y)*64.0);
	value2 = 64-(int)((Camera.X-BG_MAPHALF)*64.0);
	value3 = 64-(int)((BG_MAPHALF+1.0-Camera.X)*64.0);
	value = value0;
	imin = 1;
	imax = BG_MAPFULL;
	for (j=0;j<4;j++)
		{
		for (i=imin;i<imax;i++) BorderLight[i] = value;
		value += 64;
		imin += BG_MAPSIZE+1;
		imax += BG_MAPSIZE-1;
		}
	value = value1;
	imin = 1+BG_MAPSIZE*BG_MAPFULL;
	imax = BG_MAPFULL+BG_MAPSIZE*BG_MAPFULL;
	for (j=0;j<4;j++)
		{
		for (i=imin;i<imax;i++) BorderLight[i] = value;
		value += 64;
		imin += -BG_MAPSIZE+1;
		imax += -BG_MAPSIZE-1;
		}
	value = value2;
	imin = BG_MAPSIZE;
	imax = BG_MAPSIZE*BG_MAPFULL;
	for (j=0;j<4;j++)
		{
		for (i=imin;i<imax;i+=BG_MAPSIZE) BorderLight[i] = value;
		value += 64;
		imin += BG_MAPSIZE+1;
		imax += -BG_MAPSIZE+1;
		}
	value = value3;
	imin = BG_MAPSIZE+BG_MAPFULL;
	imax = BG_MAPSIZE*BG_MAPFULL+BG_MAPFULL;
	for (j=0;j<4;j++)
		{
		for (i=imin;i<imax;i+=BG_MAPSIZE) BorderLight[i] = value;
		value += 64;
		imin += BG_MAPSIZE-1;
		imax += -BG_MAPSIZE-1;
		}
	if (value0 < value2) value = value0;
	else				 value = value2;
	for (i=0;i<BG_MAPSIZE*4;i+=BG_MAPSIZE+1)
		{
		BorderLight[i] = value;
		value += 64;
		}
	if (value0 < value3) value = value0;
	else				 value = value3;
	for (i=BG_MAPFULL;i<4*BG_MAPSIZE;i+=BG_MAPSIZE-1)
		{
		BorderLight[i] = value;
		value += 64;
		}
	if (value1 < value2) value = value1;
	else				 value = value2;
	for (i=BG_MAPSIZE*BG_MAPFULL;i>BG_MAPSIZE*(BG_MAPFULL-3);i-=BG_MAPSIZE-1)
		{
		BorderLight[i] = value;
		value += 64;
		}
	if (value1 < value3) value = value1;
	else				 value = value3;
	for (i=BG_MAPSIZE*BG_MAPFULL+BG_MAPFULL;i>BG_MAPSIZE*(BG_MAPFULL-3);i-=BG_MAPSIZE+1)
		{
		BorderLight[i] = value;
		value += 64;
		}
// first, extract the points that can be seen.	
	thePt = MapPoint;
	theSquare = MapSquare;
	index = 0;
	for (j=CaseV;j<CaseV+BG_MAPSIZE;j++)
		for (i=CaseH;i<CaseH+BG_MAPSIZE;i++)
			{
			theCase = BG_VirtualGround+INDEX0(i,j);
		// complete point definition
			thePt->Z = theCase->Altitude;
		// create triangle definitions
			if (theCase->Orientation == 0)
				{
				theSquare->Index[0] = index+1;
				theSquare->Index[2] = index+1+BG_MAPSIZE;
				theSquare->Index[1] = index;
				theSquare->Index[3] = index+BG_MAPSIZE;
				theSquare->Norm[0] = BorderConvert[theCase->Normales[1]+BorderLight[theSquare->Index[0]]];
				theSquare->Norm[1] = BorderConvert[theCase->Normales[0]+BorderLight[theSquare->Index[1]]];
				theSquare->Norm[2] = BorderConvert[theCase->Normales[2]+BorderLight[theSquare->Index[2]]];
				theSquare->Norm[3] = BorderConvert[theCase->Normales[3]+BorderLight[theSquare->Index[3]]];
				}
			else
				{
				theSquare->Index[0] = index;
				theSquare->Index[2] = index+1;
				theSquare->Index[1] = index+BG_MAPSIZE;
				theSquare->Index[3] = index+1+BG_MAPSIZE;
				theSquare->Norm[0] = BorderConvert[theCase->Normales[0]+BorderLight[theSquare->Index[0]]];
				theSquare->Norm[1] = BorderConvert[theCase->Normales[3]+BorderLight[theSquare->Index[1]]];
				theSquare->Norm[2] = BorderConvert[theCase->Normales[1]+BorderLight[theSquare->Index[2]]];
				theSquare->Norm[3] = BorderConvert[theCase->Normales[2]+BorderLight[theSquare->Index[3]]];
				}
			theSquare->Color[0] = theCase->Color[0];
			theSquare->Color[1] = theCase->Color[1];
			theSquare->TNorm = theCase->FNorm;
			theSquare->bonus = theCase->bonus;
			theSquare->lien = theCase->lien;
		// next square
			theSquare++;
			index++;
			thePt++;
			}
// second : do the projection and check the visibility
	BG_Point3DMultMatrice3_3(CameraRot,MapPoint,&Camera,MapPtProj,BG_MAPSIZE*BG_MAPSIZE,theView);
// third : painter algorithm.
	NbSquareOrder = 0;
	NbTriangle = 0;
	NbClipPoint = BG_MAPSIZE*BG_MAPSIZE;
	for (j=0;j<BG_MAPHALF;j++)
		for (i=0;i<BG_MAPHALF;i++)
			ProcessSquare(i,j);
	for (j=0;j<BG_MAPHALF;j++)
		for (i=BG_MAPFULL-1;i>=BG_MAPHALF;i--)
			ProcessSquare(i,j);
	for (j=BG_MAPFULL-1;j>=BG_MAPHALF;j--)
		for (i=0;i<BG_MAPHALF;i++)
			ProcessSquare(i,j);
	for (j=BG_MAPFULL-1;j>=BG_MAPHALF;j--)
		for (i=BG_MAPFULL-1;i>=BG_MAPHALF;i--)
			ProcessSquare(i,j);
// annulation de l'image en cas de debordement
	if ((NbSquareOrder > (BG_MAPFULL*BG_MAPFULL+(BG_NBCLIPPTMAX/2))) ||
		(NbTriangle > (BG_NBCLIPPTMAX/2))) return;
// fourth : project extra-point
	BG_Objet3DProjection(MapPtProj+BG_MAPSIZE*BG_MAPSIZE,NbClipPoint-BG_MAPSIZE*BG_MAPSIZE,theView);
// fifth : draw the result of the process
	if (BG_Screen24)
	{
		if (indexWindow == 0)
		{
			OffScreen_largeur = rowBytes;
			OffScreen_baseAddr = (long)baseAddr;
			for (i=0;i<NbSquareOrder;i++)
			{
				// draw full square
				if (SquareOrder[i] < 10000)
				{
					// the square
					theSquare = MapSquare+SquareOrder[i];
					// 1 commun point
					theProj = MapPtProj+theSquare->Index[1];
					tab[1].Level = theSquare->Norm[1];
					tab[1].H = theProj->H;
					tab[1].V = theProj->V;
					// first triangle
					theProj = MapPtProj+theSquare->Index[0];
					tab[0].Level = theSquare->Norm[0];
					tab[0].H = theProj->H;
					tab[0].V = theProj->V;
					theProj = MapPtProj+theSquare->Index[2];
					tab[2].Level = theSquare->Norm[2];
					tab[2].H = theProj->H;
					tab[2].V = theProj->V;
					BG_Goureaud24(tab,BG_Lumieres[theSquare->Color[0]]);
					// second triangle
					theProj = MapPtProj+theSquare->Index[2];
					tab[0].Level = theSquare->Norm[2];
					tab[0].H = theProj->H;
					tab[0].V = theProj->V;
					theProj = MapPtProj+theSquare->Index[3];
					tab[2].Level = theSquare->Norm[3];
					tab[2].H = theProj->H;
					tab[2].V = theProj->V;
					BG_Goureaud24(tab,BG_Lumieres[theSquare->Color[1]]);
				}
				// draw clipped triangle
				else
				{
					theTriangle = MapTriangle+SquareOrder[i]-10000;
					theProj = MapPtProj+theTriangle->Index[0];
					tab[0].Level = theTriangle->Norm[0];
					tab[0].H = theProj->H;
					tab[0].V = theProj->V;
					theProj = MapPtProj+theTriangle->Index[1];
					tab[1].Level = theTriangle->Norm[1];
					tab[1].H = theProj->H;
					tab[1].V = theProj->V;
					theProj = MapPtProj+theTriangle->Index[2];
					tab[2].Level = theTriangle->Norm[2];
					tab[2].H = theProj->H;
					tab[2].V = theProj->V;
					BG_Goureaud24(tab,BG_Lumieres[theTriangle->Color]);
				}
			}
		}
		else
		{
			OffScreen_largeur2 = rowBytes;
			OffScreen_baseAddr2 = (long)baseAddr;
			for (i=0;i<NbSquareOrder;i++)
			{
				// draw full square
				if (SquareOrder[i] < 10000)
				{
					// the square
					theSquare = MapSquare+SquareOrder[i];
					// 1 commun point
					theProj = MapPtProj+theSquare->Index[1];
					tab[1].Level = theSquare->Norm[1];
					tab[1].H = theProj->H;
					tab[1].V = theProj->V;
					// first triangle
					theProj = MapPtProj+theSquare->Index[0];
					tab[0].Level = theSquare->Norm[0];
					tab[0].H = theProj->H;
					tab[0].V = theProj->V;
					theProj = MapPtProj+theSquare->Index[2];
					tab[2].Level = theSquare->Norm[2];
					tab[2].H = theProj->H;
					tab[2].V = theProj->V;
					BG_Goureaud242(tab,BG_Lumieres[theSquare->Color[0]]);
					// second triangle
					theProj = MapPtProj+theSquare->Index[2];
					tab[0].Level = theSquare->Norm[2];
					tab[0].H = theProj->H;
					tab[0].V = theProj->V;
					theProj = MapPtProj+theSquare->Index[3];
					tab[2].Level = theSquare->Norm[3];
					tab[2].H = theProj->H;
					tab[2].V = theProj->V;
					BG_Goureaud242(tab,BG_Lumieres[theSquare->Color[1]]);
				}
				// draw clipped triangle
				else
				{
					theTriangle = MapTriangle+SquareOrder[i]-10000;
					theProj = MapPtProj+theTriangle->Index[0];
					tab[0].Level = theTriangle->Norm[0];
					tab[0].H = theProj->H;
					tab[0].V = theProj->V;
					theProj = MapPtProj+theTriangle->Index[1];
					tab[1].Level = theTriangle->Norm[1];
					tab[1].H = theProj->H;
					tab[1].V = theProj->V;
					theProj = MapPtProj+theTriangle->Index[2];
					tab[2].Level = theTriangle->Norm[2];
					tab[2].H = theProj->H;
					tab[2].V = theProj->V;
					BG_Goureaud242(tab,BG_Lumieres[theTriangle->Color]);
				}
			}
		}
	}
	else
	{
		if (indexWindow == 0)
		{
			OffScreen_largeur = rowBytes;
			OffScreen_baseAddr = (long)baseAddr;
			for (i=0;i<NbSquareOrder;i++)
			{
				// draw full square
				if (SquareOrder[i] < 10000)
				{
					// the square
					theSquare = MapSquare+SquareOrder[i];
					// 1 commun point
					theProj = MapPtProj+theSquare->Index[1];
					tab[1].Level = theSquare->Norm[1];
					tab[1].H = theProj->H;
					tab[1].V = theProj->V;
					// first triangle
					theProj = MapPtProj+theSquare->Index[0];
					tab[0].Level = theSquare->Norm[0];
					tab[0].H = theProj->H;
					tab[0].V = theProj->V;
					theProj = MapPtProj+theSquare->Index[2];
					tab[2].Level = theSquare->Norm[2];
					tab[2].H = theProj->H;
					tab[2].V = theProj->V;
					BG_Goureaud(tab,BG_Lumieres[theSquare->Color[0]]);
					// second triangle
					theProj = MapPtProj+theSquare->Index[2];
					tab[0].Level = theSquare->Norm[2];
					tab[0].H = theProj->H;
					tab[0].V = theProj->V;
					theProj = MapPtProj+theSquare->Index[3];
					tab[2].Level = theSquare->Norm[3];
					tab[2].H = theProj->H;
					tab[2].V = theProj->V;
					BG_Goureaud(tab,BG_Lumieres[theSquare->Color[1]]);
				}
				// draw clipped triangle
				else
				{
					theTriangle = MapTriangle+SquareOrder[i]-10000;
					theProj = MapPtProj+theTriangle->Index[0];
					tab[0].Level = theTriangle->Norm[0];
					tab[0].H = theProj->H;
					tab[0].V = theProj->V;
					theProj = MapPtProj+theTriangle->Index[1];
					tab[1].Level = theTriangle->Norm[1];
					tab[1].H = theProj->H;
					tab[1].V = theProj->V;
					theProj = MapPtProj+theTriangle->Index[2];
					tab[2].Level = theTriangle->Norm[2];
					tab[2].H = theProj->H;
					tab[2].V = theProj->V;
					BG_Goureaud(tab,BG_Lumieres[theTriangle->Color]);
				}
			}
		}
		else
		{
			OffScreen_largeur2 = rowBytes;
			OffScreen_baseAddr2 = (long)baseAddr;
			for (i=0;i<NbSquareOrder;i++)
			{
				// draw full square
				if (SquareOrder[i] < 10000)
				{
					// the square
					theSquare = MapSquare+SquareOrder[i];
					// 1 commun point
					theProj = MapPtProj+theSquare->Index[1];
					tab[1].Level = theSquare->Norm[1];
					tab[1].H = theProj->H;
					tab[1].V = theProj->V;
					// first triangle
					theProj = MapPtProj+theSquare->Index[0];
					tab[0].Level = theSquare->Norm[0];
					tab[0].H = theProj->H;
					tab[0].V = theProj->V;
					theProj = MapPtProj+theSquare->Index[2];
					tab[2].Level = theSquare->Norm[2];
					tab[2].H = theProj->H;
					tab[2].V = theProj->V;
					BG_Goureaud2(tab,BG_Lumieres[theSquare->Color[0]]);
					// second triangle
					theProj = MapPtProj+theSquare->Index[2];
					tab[0].Level = theSquare->Norm[2];
					tab[0].H = theProj->H;
					tab[0].V = theProj->V;
					theProj = MapPtProj+theSquare->Index[3];
					tab[2].Level = theSquare->Norm[3];
					tab[2].H = theProj->H;
					tab[2].V = theProj->V;
					BG_Goureaud2(tab,BG_Lumieres[theSquare->Color[1]]);
				}
				// draw clipped triangle
				else
				{
					theTriangle = MapTriangle+SquareOrder[i]-10000;
					theProj = MapPtProj+theTriangle->Index[0];
					tab[0].Level = theTriangle->Norm[0];
					tab[0].H = theProj->H;
					tab[0].V = theProj->V;
					theProj = MapPtProj+theTriangle->Index[1];
					tab[1].Level = theTriangle->Norm[1];
					tab[1].H = theProj->H;
					tab[1].V = theProj->V;
					theProj = MapPtProj+theTriangle->Index[2];
					tab[2].Level = theTriangle->Norm[2];
					tab[2].H = theProj->H;
					tab[2].V = theProj->V;
					BG_Goureaud2(tab,BG_Lumieres[theTriangle->Color]);
				}
			}
		}
	}
}

/***********************************************************
* What :	Process one square.
*			
* State : 	In progress
* Last Modif :	17/10/95 (Pierre)
*					
***********************************************************/
void BG_Canal3D::ProcessSquare(int i,int j)
	{
	int			index;
	char			visible1,invisible1,masque,visible2,invisible2;
	BG_Pt3D			Centre;
	BG_Square		*theSquare;
	BG_Triangle		*theTriangle;
	BG_Aircraft		*link;
	BG_Matrice3_3	Rotation;
		
// index of the square
	index = BG_MAPSIZE*j+i;
	theSquare = MapSquare+index;
// check visibility on the whole square
	masque = MapPtProj[theSquare->Index[1]].Masque;
	visible1 = masque;
	invisible1 = masque;
	masque = MapPtProj[theSquare->Index[2]].Masque;
	visible1 |= masque;
	invisible1 &= masque;
	masque = MapPtProj[theSquare->Index[3]].Masque;
	visible2 = masque|visible1;
	invisible2 = masque&invisible1;
	masque = MapPtProj[theSquare->Index[0]].Masque;
	visible1 |= masque;
	invisible1 &= masque;
// seperate the 2 simple cases
	if ((invisible1&invisible2) == 0)
		{
		if ((visible1|visible2) == 0)
			{
			SquareOrder[NbSquareOrder] = index;
			NbSquareOrder++;
			}
		else
			{
		// first triangle
			if (invisible1 == 0)
				{
				theTriangle = MapTriangle+NbTriangle;
				theTriangle->Index[0] = theSquare->Index[0];
				theTriangle->Index[1] = theSquare->Index[1];
				theTriangle->Index[2] = theSquare->Index[2];
				theTriangle->Norm[0] = theSquare->Norm[0];
				theTriangle->Norm[1] = theSquare->Norm[1];
				theTriangle->Norm[2] = theSquare->Norm[2];
				theTriangle->Color = theSquare->Color[0];
				theTriangle->FNorm = theSquare->FNorm[0];
				SquareOrder[NbSquareOrder] = 10000+NbTriangle;
				NbSquareOrder++;
				NbTriangle++;
				if (visible1 != 0)
					ProcessClipping(0);
				}
		// second triangle
			if (invisible2 == 0)
				{
				theTriangle = MapTriangle+NbTriangle;
				theTriangle->Index[0] = theSquare->Index[2];
				theTriangle->Index[1] = theSquare->Index[1];
				theTriangle->Index[2] = theSquare->Index[3];
				theTriangle->Norm[0] = theSquare->Norm[2];
				theTriangle->Norm[1] = theSquare->Norm[1];
				theTriangle->Norm[2] = theSquare->Norm[3];
				theTriangle->Color = theSquare->Color[1];
				theTriangle->FNorm = theSquare->FNorm[1];
				SquareOrder[NbSquareOrder] = 10000+NbTriangle;
				NbSquareOrder++;
				NbTriangle++;
				if (visible2 != 0)
					ProcessClipping(0);
				}
			}
		}
// incrustation of bonus
	if (BG_TableBonus[theSquare->bonus])
		{
		Centre.X = 0.5*(MapPoint[theSquare->Index[1]].X+MapPoint[theSquare->Index[2]].X);
		Centre.Y = 0.5*(MapPoint[theSquare->Index[1]].Y+MapPoint[theSquare->Index[2]].Y);
		Centre.Z = 0.5*(MapPoint[theSquare->Index[1]].Z+MapPoint[theSquare->Index[2]].Z)+0.09;
		BG_Matrice3_3Rotation3D(BG_Clock()&8191,0,0,&Rotation);
		DrawAirCraft(0,5,BG_Camera,BG_CameraRot,&Centre,&Rotation,0.0);
		}
// incrustation des objets
	link = (BG_Aircraft *)theSquare->lien;
	while (link != 0L)
		{
		if (&link->Rotation != BG_CameraRot)
			{
			Centre.X = link->Centre.X-(float)CaseH;
			if (Centre.X < 0.0) Centre.X += BG_GROUNDDH;
			else if (Centre.X > (BG_MAPSIZE+1.0)) Centre.X -= BG_GROUNDDH;
			Centre.Y = link->Centre.Y-(float)CaseV;
			if (Centre.Y < 0.0) Centre.Y += BG_GROUNDDH;
			else if (Centre.Y > (BG_MAPSIZE+1.0)) Centre.Y -= BG_GROUNDDH;
			Centre.Z = link->Centre.Z;
			DrawAirCraft(link->Type,link->Color,BG_Camera,BG_CameraRot,
						 &Centre,&link->Rotation,link->Duree);
			}
		link = (BG_Aircraft *)link->next;
		}
	}

/***********************************************************
* What :	Process the clipping of one of the four face of
*			the vision pyramid, on the last recorded triangle.
* State : 	In progress
* Last Modif :	17/10/95 (Pierre)
*					
***********************************************************/
void BG_Canal3D::ProcessClipping(int index)
	{
	short		select,NPt1,NPt2,NNorm1,NNorm2,Pt1,Color,Norm1,FNorm;
	float		rel0,rel1,rel2;
	BG_Pt3D		Norm;
	BG_Point3D	*pt0,*pt1,*pt2;
	BG_Triangle	*theT;
	
	theT = MapTriangle+NbTriangle-1;
// select the clipping face.
	switch (index)
		{
	// face associated to YXmin
		case 0 :
			Norm.X = -LocalView->YXmin;
			Norm.Y = 1.0;
			Norm.Z = 0.0;
			break;
	// face associated to YXmax
		case 1 :
			Norm.X = LocalView->YXmax;
			Norm.Y = -1.0;
			Norm.Z = 0.0;
			break;
	// face associated to ZXmin
		case 2 :
			Norm.X = -LocalView->ZXmin;
			Norm.Y = 0.0;
			Norm.Z = 1.0;
			break;
	// face associated to ZXmax
		case 3 :
			Norm.X = LocalView->ZXmax;
			Norm.Y = 0.0;
			Norm.Z = -1.0;
			break;
		}
// calculate relative position of the three points of the triangle
	pt0 = MapPtProj+theT->Index[0];
	pt1 = MapPtProj+theT->Index[1];
	pt2 = MapPtProj+theT->Index[2];
	rel0 = Norm.X*pt0->X+Norm.Y*pt0->Y+Norm.Z*pt0->Z;
	rel1 = Norm.X*pt1->X+Norm.Y*pt1->Y+Norm.Z*pt1->Z;
	rel2 = Norm.X*pt2->X+Norm.Y*pt2->Y+Norm.Z*pt2->Z;
// dispatcher 
	select = 0;
	if (rel0 < 0.0) select|=1;
	if (rel1 < 0.0) select|=2;
	if (rel2 < 0.0) select|=4;
	switch (select)
		{
	// all the points out : remove the triangle
		case 7 :
			NbSquareOrder--;
			NbTriangle--;
			break;
	// point 0 visible : modify the triangle and pursued clipping
		case 6 :
		// create new points
			NPt1 = CalculateIntersection(pt0,pt1,rel0,rel1,
											theT->Norm[0],theT->Norm[1],&NNorm1);
			NPt2 = CalculateIntersection(pt0,pt2,rel0,rel2,
											theT->Norm[0],theT->Norm[2],&NNorm2);
		// modify the triangle
			theT->Index[1] = NPt1;
			theT->Index[2] = NPt2;
			theT->Norm[1] = NNorm1;
			theT->Norm[2] = NNorm2;
		// pursued
			if (index < 3) ProcessClipping(index+1);
			break;
	// point 1 visible : modify the triangle and pursued clipping
		case 5 :
		// create new points
			NPt1 = CalculateIntersection(pt1,pt0,rel1,rel0,
											theT->Norm[1],theT->Norm[0],&NNorm1);
			NPt2 = CalculateIntersection(pt1,pt2,rel1,rel2,
											theT->Norm[1],theT->Norm[2],&NNorm2);
		// modify the triangle
			theT->Index[0] = NPt1;
			theT->Index[2] = NPt2;
			theT->Norm[0] = NNorm1;
			theT->Norm[2] = NNorm2;
		// pursued
			if (index < 3) ProcessClipping(index+1);
			break;
	// point 2 invisible : recursive clipping
		case 4 :
		// create new points
			NPt1 = CalculateIntersection(pt2,pt0,rel2,rel0,
											theT->Norm[2],theT->Norm[0],&NNorm1);
			NPt2 = CalculateIntersection(pt2,pt1,rel2,rel1,
											theT->Norm[2],theT->Norm[1],&NNorm2);
		// memorize old points
			Pt1 = theT->Index[1];
			Norm1 = theT->Norm[1];
			Color = theT->Color;
			FNorm = theT->FNorm;
		// modify the first triangle
			theT->Index[2] = NPt1;
			theT->Norm[2] = NNorm1;
			if (index < 3) ProcessClipping(index+1);
		// create new triangle
			theT = MapTriangle+NbTriangle;
			theT->Index[0] = Pt1;
			theT->Index[1] = NPt2;
			theT->Index[2] = NPt1;
			theT->Norm[0] = Norm1;
			theT->Norm[1] = NNorm2;
			theT->Norm[2] = NNorm1;
			theT->Color = Color;
			theT->FNorm = FNorm;
			SquareOrder[NbSquareOrder] = 10000+NbTriangle;
			NbSquareOrder++;
			NbTriangle++;
			if (index < 3) ProcessClipping(index+1);
			break;
	// point 2 visible : modify the triangle and pursued clipping
		case 3 :
		// create new points
			NPt1 = CalculateIntersection(pt2,pt0,rel2,rel0,
											theT->Norm[2],theT->Norm[0],&NNorm1);
			NPt2 = CalculateIntersection(pt2,pt1,rel2,rel1,
											theT->Norm[2],theT->Norm[1],&NNorm2);
		// modify the triangle
			theT->Index[0] = NPt1;
			theT->Index[1] = NPt2;
			theT->Norm[0] = NNorm1;
			theT->Norm[1] = NNorm2;
		// pursued
			if (index < 3) ProcessClipping(index+1);
			break;
	// point 1 invisible : recursive clipping
		case 2 :
		// create new points
			NPt1 = CalculateIntersection(pt1,pt2,rel1,rel2,
											theT->Norm[1],theT->Norm[2],&NNorm1);
			NPt2 = CalculateIntersection(pt1,pt0,rel1,rel0,
											theT->Norm[1],theT->Norm[0],&NNorm2);
		// memorize old points
			Pt1 = theT->Index[0];
			Norm1 = theT->Norm[0];
			Color = theT->Color;
			FNorm = theT->FNorm;
		// modify the first triangle
			theT->Index[1] = NPt1;
			theT->Norm[1] = NNorm1;
			if (index < 3) ProcessClipping(index+1);
		// create new triangle
			theT = MapTriangle+NbTriangle;
			theT->Index[0] = Pt1;
			theT->Index[1] = NPt2;
			theT->Index[2] = NPt1;
			theT->Norm[0] = Norm1;
			theT->Norm[1] = NNorm2;
			theT->Norm[2] = NNorm1;
			theT->Color = Color;
			theT->FNorm = FNorm;
			SquareOrder[NbSquareOrder] = 10000+NbTriangle;
			NbSquareOrder++;
			NbTriangle++;
			if (index < 3) ProcessClipping(index+1);
			break;
	// point 0 invisible : recursive clipping
		case 1 :
		// create new points
			NPt1 = CalculateIntersection(pt0,pt1,rel0,rel1,
											theT->Norm[0],theT->Norm[1],&NNorm1);
			NPt2 = CalculateIntersection(pt0,pt2,rel0,rel2,
											theT->Norm[0],theT->Norm[2],&NNorm2);
		// memorize old points
			Pt1 = theT->Index[2];
			Norm1 = theT->Norm[2];
			Color = theT->Color;
			FNorm = theT->FNorm;
		// modify the first triangle
			theT->Index[0] = NPt1;
			theT->Norm[0] = NNorm1;
			if (index < 3) ProcessClipping(index+1);
		// create new triangle
			theT = MapTriangle+NbTriangle;
			theT->Index[0] = Pt1;
			theT->Index[1] = NPt2;
			theT->Index[2] = NPt1;
			theT->Norm[0] = Norm1;
			theT->Norm[1] = NNorm2;
			theT->Norm[2] = NNorm1;
			theT->Color = Color;
			theT->FNorm = FNorm;
			SquareOrder[NbSquareOrder] = 10000+NbTriangle;
			NbSquareOrder++;
			NbTriangle++;
			if (index < 3) ProcessClipping(index+1);
			break;
	// all points are visible : do the next cliping
		case 0 :
			if (index < 3) ProcessClipping(index+1);
			break;
		}
	}

/***********************************************************
* What :	Process the clipping of one of the four face of
*			the vision pyramid, on the last recorded triangle.
* State : 	In progress
* Last Modif :	17/10/95 (Pierre)
*					
***********************************************************/
short BG_Canal3D::CalculateIntersection(BG_Point3D	*pt1,
										BG_Point3D	*pt2,
										float		rel1,
										float		rel2,
										short		Norm1,
										short		Norm2,
										short		*NNorm)
	{
	short			index;
	float			base;
	BG_Point3D		*newPt;
	
// gestion des cas de saturation
	if (NbClipPoint == (BG_MAPSIZE*BG_MAPSIZE+BG_NBCLIPPTMAX))
		return (short)(((long)pt1-(long)MapPtProj)/sizeof(BG_Point3D));
// get a new point
	index = NbClipPoint;
	NbClipPoint++;
// calculate the point
	newPt = MapPtProj+index;
	base = 1.0/(rel1-rel2);
	newPt->X = base*(pt2->X*rel1-pt1->X*rel2);
	newPt->Y = base*(pt2->Y*rel1-pt1->Y*rel2);
	newPt->Z = base*(pt2->Z*rel1-pt1->Z*rel2);
	*NNorm = (short)((((float)Norm2+0.5)*rel1-((float)Norm1+0.5)*rel2)*base+0.5);
// return index point
	return index;
	}

/***********************************************************
* What :	Draw an aircraft	
*			
* State : 	In progress
* Last Modif :	20/10/95 (Pierre)
*					
***********************************************************/
void BG_Canal3D::DrawAirCraft(	char			type,
								char			color,
								BG_Pt3D			*Camera,
								BG_Matrice3_3	*CameraRot,
								BG_Pt3D			*Centre,
								BG_Matrice3_3	*Rotation,
								float			Duree)
	{
	int				NbPt,NbVect;
	float			Scale;
	BG_Pt3D			*ListPt,*ListVect;
	BG_Pt3D			NewCenter,pt;
	BG_Matrice3_3	MRot,MProd;
	
	curColor = color;
// calcule la rotation propre de l'objet cible
	pt.X = Centre->X-Camera->X;
	pt.Y = Centre->Y-Camera->Y;
	pt.Z = Centre->Z-Camera->Z;
	NewCenter.X = CameraRot->X1_Y1*pt.X;
	NewCenter.Y = CameraRot->X1_Y2*pt.X;
	NewCenter.Z = CameraRot->X1_Y3*pt.X;
	NewCenter.X += CameraRot->X2_Y1*pt.Y;
	NewCenter.Y += CameraRot->X2_Y2*pt.Y;
	NewCenter.Z += CameraRot->X2_Y3*pt.Y;
	NewCenter.X += CameraRot->X3_Y1*pt.Z;
	NewCenter.Y += CameraRot->X3_Y2*pt.Z;
	NewCenter.Z += CameraRot->X3_Y3*pt.Z;
// gestion facteur d'echelle
	if (type == 3) Scale = (1.2-Duree)*Duree*(1.0/36.0);
	else Scale = 0.01;
// effectue le produit des matrices de rotation
	BG_Matrice3_3InvertEchelle(*Rotation,&MRot,Scale);
	BG_Matrice3_3MultMatrice3_3(*CameraRot,MRot,&MProd);
// choisit le type d'objet
	switch (type)
		{
		case 0 :
			ListPt = BG_ListPt0;
			ListVect = BG_ListVect0;
			NbPt = BG_NbPt0;
			NbVect = BG_NbVect0;
			ListFace = BG_ListFace0;
			ListBSP = BG_ListBSP0;
			break;
		case 1 :
			ListPt = BG_ListPt1;
			ListVect = BG_ListVect1;
			NbPt = BG_NbPt1;
			NbVect = BG_NbVect1;
			ListFace = BG_ListFace1;
			ListBSP = BG_ListBSP1;
			break;
		case 2 :
			ListPt = BG_ListPt2;
			ListVect = BG_ListVect2;
			NbPt = BG_NbPt2;
			NbVect = BG_NbVect2;
			ListFace = BG_ListFace2;
			ListBSP = BG_ListBSP2;
			break;
		case 3 :
			ListPt = BG_ListPt3;
			ListVect = BG_ListVect3;
			NbPt = BG_NbPt3;
			NbVect = BG_NbVect3;
			ListFace = BG_ListFace3;
			ListBSP = BG_ListBSP3;
			break;
		}
// projete les vecteurs et les points
	ProjPt = MapPtProj+NbClipPoint;
	curIndexPt = NbClipPoint;
	NbClipPoint += NbPt;
	BG_Vect3DMultMatrice3_3(&MProd,ListVect,ProjNorm,NbVect);
	BG_Objet3DMultMatrice3_3(&MProd,ListPt,&NewCenter,ProjPt,NbPt,&CanalView);
// resoud le BSP et traite les triangles...
	SolveBSP(ListBSP);
	}

/***********************************************************
* What :	Solve BSP aircraft and draw triangle	
*			
* State : 	In progress
* Last Modif :	21/10/95 (Pierre)
*					
***********************************************************/
void BG_Canal3D::SolveBSP(BG_BSP *theBSP)
	{
	BG_Pt3D		*theVect;
	BG_Point3D	*thePt;
	
	thePt = ProjPt+theBSP->Pt;
	theVect = ProjNorm+theBSP->Norm;
	if ((thePt->X*theVect->X+thePt->Y*theVect->Y+thePt->Z*theVect->Z) > 0.0)
		{
		if (theBSP->Sup >= 0)
			DrawTriangle(ListFace+theBSP->Sup);
		else
			SolveBSP(ListBSP-theBSP->Sup);
		if (theBSP->Inf >= 0)
			DrawTriangle(ListFace+theBSP->Inf);
		else
			SolveBSP(ListBSP-theBSP->Inf);
		}
	else
		{
		if (theBSP->Inf >= 0)
			DrawTriangle(ListFace+theBSP->Inf);
		else
			SolveBSP(ListBSP-theBSP->Inf);
		if (theBSP->Sup >= 0)
			DrawTriangle(ListFace+theBSP->Sup);
		else
			SolveBSP(ListBSP-theBSP->Sup);
		}
	}

/***********************************************************
* What :	Solve BSP aircraft and draw triangle	
*			
* State : 	In progress
* Last Modif :	21/10/95 (Pierre)
*					
***********************************************************/
void BG_Canal3D::DrawTriangle(unsigned char *theFace)
	{
	char			visible1,invisible1,masque;
	BG_Triangle		*theTriangle;
	
	while (*theFace != 255)
		{
	// check visibility on the whole square
		masque = ProjPt[theFace[0]].Masque;
		visible1 = masque;
		invisible1 = masque;
		masque = ProjPt[theFace[1]].Masque;
		visible1 |= masque;
		invisible1 &= masque;
		masque = ProjPt[theFace[2]].Masque;
		visible1 |= masque;
		invisible1 &= masque;
	// first triangle
		if (invisible1 == 0)
			{
			theTriangle = MapTriangle+NbTriangle;
			theTriangle->Index[0] = curIndexPt+theFace[0];
			theTriangle->Index[1] = curIndexPt+theFace[1];
			theTriangle->Index[2] = curIndexPt+theFace[2];
			theTriangle->Norm[0] = theFace[3];
			theTriangle->Norm[1] = theFace[4];
			theTriangle->Norm[2] = theFace[5];
			theTriangle->Color = curColor;
			SquareOrder[NbSquareOrder++] = 10000+NbTriangle;
			NbTriangle++;
			if (visible1 != 0)
				ProcessClipping(0);
			}
	// triangle suivant
		theFace += 6;
		}
	}

/***********************************************************
* Descriptif : Initialise un canal de trace 3D
*				
* Etat : En cours
* Derniere Modif :	27/07/95 (Pierre)
*					
***********************************************************/
BG_ViewMap::BG_ViewMap(	BRect frame):BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW)
	{
	SetDrawingMode(B_OP_COPY);
// calcul le rectangle enveloppant
	screenRect.Set(0.0, 0.0, BG_VIEWMAPH-1.0, BG_VIEWMAPV-1.0);
	if (BG_Screen24)
		theBitmap = new BBitmap(screenRect, B_RGB_32_BIT);
	else
		theBitmap = new BBitmap(screenRect, B_COLOR_8_BIT);
// Calcule la taille de l'offscreen
	rowBytes = theBitmap->BytesPerRow();
	baseAddr = (char*)(theBitmap->Bits());
// Enregistre les dimensions
	Largeur = BG_VIEWMAPH;
	Hauteur = BG_VIEWMAPV;
	}

/***********************************************************
* Descriptif :	Affichage de l'offscreen si besoin est
*				
* Etat : En cours
* Derniere Modif :	07/10/94 (Pierre)
*					
***********************************************************/
BG_ViewMap::~BG_ViewMap()
	{
    delete theBitmap;
	}

/***********************************************************
* Descriptif :	Affichage de l'offscreen si besoin est
*				
* Etat : En cours
* Derniere Modif :	07/10/94 (Pierre)
*					
***********************************************************/
void BG_ViewMap::Draw(BRect where)
	{
	DrawBitmap(theBitmap,
			   BRect(0.0,0.0,Largeur-1.0,Hauteur-1.0),
			   BRect(0.0,0.0,Largeur-1.0,Hauteur-1.0));
	}

/***********************************************************
* Descriptif :	Affichage du radar
*				
* Etat : En cours
* Derniere Modif :	07/10/94 (Pierre)
*					
***********************************************************/
void BG_ViewMap::Draw2(Boolean master)
	{
	int				i,j,h,v,offset0,h0,v0,i0;
	float			pas;
	unsigned long	colormark;
	unsigned char	*base;
	unsigned long	*base2;

	if (BG_GROUNDDH == 32) pas = 8.0;
	else if (BG_GROUNDDH == 64) pas = 4.0;
	else pas = 2.0;
// radar center
	if (master)
		i0 = BG_IndexPlayer[BG_IndexLocalPlayer];	
	else
		i0 = BG_IndexPlayer[BG_IndexLocalPlayer2];	
	h0 = (128-(int)(BG_ListAppareil[i0].Centre.X*pas))&255;
	v0 = (128+(int)(BG_ListAppareil[i0].Centre.Y*pas))&255;
	offset0 = h0+v0*256;

	if (BG_Screen24)
	{
/*		if (h0 & 32)
		{
			short    ecart[512];
			
			j = ((h0<<3)&255);
			h = j>>1;
			for (i=0;i<256;i++)
			{
				ecart[i+256] = h>>8;
				ecart[256-i] = -(h>>8);
				h += j;
			}
			for (i=0;i<256;i++)
				for (j=256-ecart[i+256];j<512-ecart[i+256];j++)
					((long*)baseAddr)[((j+ecart[i+256])&0xff)+((i&0xff)<<8)] =
						((long*)BG_OffscreenMap)[(((i+ecart[j])&0xff)<<8)+(j&0xff)];
		}
		else
		{*/
		memcpy(baseAddr+offset0*4,BG_OffscreenMap,256*256L*4-offset0*4);
		memcpy(baseAddr,BG_OffscreenMap+256*256*4-offset0*4,offset0*4);
//		}

		base2 = (unsigned long*)baseAddr;
    // draw ennemi aircraft
		for (i=0;i<BG_NBAIRCRAFTMAX;i++)
			if ((BG_ListAppareil[i].Type == 1) && (i != i0))
			{
				h = (int)(BG_ListAppareil[i].Centre.X*pas)+h0;
				v = (256-(int)(BG_ListAppareil[i].Centre.Y*pas))+v0;
				colormark = BG_CurseurColor[BG_ListAppareil[i].Color-5];
				for (j=-5;j<=5;j++)
				{
					base2[((v&255)<<8)+((h+j)&255)] = colormark;
					base2[(((v+j)&255)<<8)+(h&255)] = colormark;
				}
			}
    // draw helicopter points
		for (i=0;i<BG_NBBONUSMAX;i++)
			if (BG_TableBonus[i] == 1)
			{
				j = BG_OffsetBonus[i]+offset0;
				base2[(j)&65535] = 0;
				base2[(j+1)&65535] = 0;
				base2[(j+256)&65535] = 0;
				base2[(j+257)&65535] = 0;
			}
    // draw central mark
		colormark = BG_CurseurColor[BG_ListAppareil[i0].Color-5];
		for (i=118;i<=138;i++)
		{
			base2[256*128+i] = colormark;
			base2[128+(i<<8)] = colormark;
		}
	}
	else
	{
		memcpy(baseAddr+offset0,BG_OffscreenMap,256*256L-offset0);
		memcpy(baseAddr,BG_OffscreenMap+256*256-offset0,offset0);
		base = (unsigned char*)baseAddr;
    // draw ennemi aircraft
		for (i=0;i<BG_NBAIRCRAFTMAX;i++)
			if ((BG_ListAppareil[i].Type == 1) && (i != i0))
			{
				h = (int)(BG_ListAppareil[i].Centre.X*pas)+h0;
				v = (256-(int)(BG_ListAppareil[i].Centre.Y*pas))+v0;
				colormark = BG_CurseurColor[BG_ListAppareil[i].Color-5];
				for (j=-5;j<=5;j++)
				{
					base[((v&255)<<8)+((h+j)&255)] = colormark;
					base[(((v+j)&255)<<8)+(h&255)] = colormark;
				}
			}
	// draw helicopter points
		for (i=0;i<BG_NBBONUSMAX;i++)
			if (BG_TableBonus[i] == 1)
			{
				j = BG_OffsetBonus[i]+offset0;
				base[(j)&65535] = 0;
				base[(j+1)&65535] = 0;
				base[(j+256)&65535] = 0;
				base[(j+257)&65535] = 0;
			}
	// draw central mark
		colormark = BG_CurseurColor[BG_ListAppareil[i0].Color-5];
		for (i=118;i<=138;i++)
		{
			base[256*128+i] = colormark;
			base[128+(i<<8)] = colormark;
		}
	}
// redraw radar
	DrawBitmap(theBitmap,
			   BRect(0.0,0.0,Largeur-1.0,Hauteur-1.0),
			   BRect(0.0,0.0,Largeur-1.0,Hauteur-1.0));
	}















