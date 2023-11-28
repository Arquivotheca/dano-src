#include "BG_Erreur.h"
#include "BG_Types3D.h"
#include "BG_Memory.h"
#include "BG_Graphic.h"
#include "BG_PlayerInterface.h"
#include <unistd.h>
#include <Joystick.h>
#include <fcntl.h>
#include <Debug.h>

// macro de decodage des touches
#define	TestCode(x)		(theKey.key_states[x>>3]&(128>>(x&7)))
#define	TestCode2(x)	(RefKeyMap.key_states[x>>3]&(128>>(x&7)))

// configuration du VPaddleClavier
static	BG_CfgVPaddleClavier	BG_ConfigClavier;
static	BJoystick				*BG_Joystick[2];
static	int						VPaddleMode[2] = {BG_VPADDLECLAVIER,BG_VPADDLECLAVIER};
static	int32					joystick_count = 0;
static 	char					joystick_names[4][B_OS_NAME_LENGTH];
static	char					keyboard_name[] = "Keyboard";

// courbe de reponse progressive des Paddles (durant les 3 premieres secondes)
static	short	DureeToVitesse[24] =
	{
	16,	24,	32,	40,	48,	56,	64,	64,
	64,	64,	64,	64,	64,	72,	80,	88,
	96,	104,112,120,128,128,128,128
	};

/***********************************************************
* Descriptif :	Initialise le gestionnaire d'interface utilisateur
*				en mode jeu
* Etat : En cours
* Dernière Modif :	29/08/95 (Pierre)
*					
***********************************************************/
void BG_InitPlayerInterface() {
	int32		i, count;
	BJoystick	a_joy;
	
// configuration du VPaddle clavier par defaut
	BG_ConfigClavier.Haut = 		0x38;
	BG_ConfigClavier.Droit = 		0x4A;		
	BG_ConfigClavier.Bas = 			0x59;
	BG_ConfigClavier.Gauche = 		0x48;
	BG_ConfigClavier.Bouton[0] = 	0x57;
	BG_ConfigClavier.Bouton[1] = 	0x61;
	BG_ConfigClavier.Bouton[2] = 	0x63;
	BG_ConfigClavier.Bouton[3] = 	0x01;
	BG_ConfigClavier.Tempo[0] = 0L;
	BG_ConfigClavier.Tempo[1] = 0L;
	BG_ConfigClavier.Tempo[2] = 0L;
	BG_ConfigClavier.Tempo[3] = 0L;
// get the list of available devices
	count = a_joy.CountDevices();
	if (count > 4)
		count = 4;
	joystick_count = count;
	for (i=0; i<count; i++)
		a_joy.GetDeviceName(i, joystick_names[i]);
// init BJoystick context
	BG_Joystick[0] = NULL;
	BG_Joystick[1] = NULL;
}

/***********************************************************
* Descriptif :	Libere le gestionnaire d'interface utilisateur
*				en mode jeu
* Etat : En cours
* Dernière Modif :	29/08/94 (Pierre)
*					
***********************************************************/
void BG_DisposePlayerInterface() {
	int		i;
	
	for (i=0; i<2; i++)
		if (BG_Joystick[i]) {
			BG_Joystick[i]->Close();
			delete BG_Joystick[i];
		}
}

/***********************************************************
* Descriptif :	Access input device name list.
* Etat : Pending
* Dernière Modif :	07/09/98 (Pierre)
*					
***********************************************************/
int32 BG_GetDeviceCount() {
	return joystick_count+1;
}

char *BG_GetDeviceName(int32 index) {
	if ((index < 0) || (index > joystick_count))
		return NULL;
	if (index == 0)
		return keyboard_name;
	return joystick_names[index-1];
}

/***********************************************************
* Descriptif :	Selectionne le Paddle courant (0 le clavier,
*				1 a 4 les joysticks).
* Etat : En cours
* Dernière Modif :	21/11/95 (Pierre)
*					
***********************************************************/
void BG_SelectVPaddle(int PaddleRef,int index) {
	if (PaddleRef > 0) {
		BG_SelectJoystick(PaddleRef-1,index);
		VPaddleMode[index] = BG_VPADDLEJOYSTICK;
	}
	else
		VPaddleMode[index] = BG_VPADDLECLAVIER;
}

/***********************************************************
* Descriptif :	Lit l'etat courant du VPaddle du joueur
*				concerne
* Etat : En cours
* Dernière Modif :	21/11/95 (Pierre)
*					
***********************************************************/
void BG_GetVPaddle(BG_VPaddle *thePad,long Tempo,int index) {
	int						i;
	long					duree;
	key_info				theKey;
	BG_CfgVPaddleClavier	*Conf;
	
// Gestion VPaddle Clavier
	if (VPaddleMode[index] == BG_VPADDLECLAVIER) {
		Conf = &BG_ConfigClavier;
		get_key_info(&theKey);
		thePad->deltaV = 0;
		thePad->deltaH = 0;
	// mouvements verticaux
		if (TestCode(Conf->Bas)!=0) {
			if (Conf->Tempo[0] == 0L)
				Conf->Tempo[0] = Tempo;
			if ((duree = (Tempo-Conf->Tempo[0])>>7) > 23) duree = 23;
			thePad->deltaV -= DureeToVitesse[duree];
		}
		else Conf->Tempo[0] = 0L;
		if (TestCode(Conf->Haut)!=0) {
			if (Conf->Tempo[1] == 0L)
				Conf->Tempo[1] = Tempo;
			if ((duree = (Tempo-Conf->Tempo[1])>>7) > 23) duree = 23;
			thePad->deltaV += DureeToVitesse[duree];
		}
		else Conf->Tempo[1] = 0L;
	// mouvements horizontaux
		if (TestCode(Conf->Gauche)!=0) {
			if (Conf->Tempo[2] == 0L)
				Conf->Tempo[2] = Tempo;
			if ((duree = (Tempo-Conf->Tempo[2])>>7) > 23) duree = 23;
			thePad->deltaH -= DureeToVitesse[duree];
		}
		else Conf->Tempo[2] = 0L;
		if (TestCode(Conf->Droit)!=0) {
			if (Conf->Tempo[3] == 0L)
				Conf->Tempo[3] = Tempo;
			if ((duree = (Tempo-Conf->Tempo[3])>>7) > 23) duree = 23;
			thePad->deltaH += DureeToVitesse[duree];
		}
		else Conf->Tempo[3] = 0L;
	// boutons
		for (i=0;i<4;i++) {
			if (TestCode(Conf->Bouton[i])!=0)
				thePad->button[i] = 1;
			else
				thePad->button[i] = 0;
		}
	}
// Gestion VPaddle Joystick
	else if (BG_Joystick[index]) {
		BJoystick *stick;

		stick = BG_Joystick[index];
		stick->Update();
		/* support multi-button joysticks */
		/* maybe we should support throttle controls, too? */
		if (stick->CountButtons() > 3) {
			uint32 buttons = stick->ButtonValues();
			thePad->button[0] = (buttons & 1) != 0;
			/* button 2 does nothing */
			thePad->button[1] = (buttons & 4) != 0;
			thePad->button[2] = (buttons & 8) != 0;
			thePad->button[3] = 0;
		}
		else {
			thePad->button[0] = (stick->button1 == 0) && (stick->button2 != 0);
			thePad->button[1] = (stick->button1 != 0) && (stick->button2 == 0);
			thePad->button[2] = (stick->button1 == 0) && (stick->button2 == 0);
			thePad->button[3] = 0;
		}
		thePad->deltaV = (((int)stick->vertical)-2048)/24;
		if (thePad->deltaV > 16) thePad->deltaV = (thePad->deltaV*thePad->deltaV)/64-4;
		else if (thePad->deltaV < -16) thePad->deltaV = (-thePad->deltaV*thePad->deltaV)/64+4;
		else thePad->deltaV = 0;
		thePad->deltaH = (2048-((int)stick->horizontal))/24;
		if (thePad->deltaH > 16) thePad->deltaH = (thePad->deltaH*thePad->deltaH)/64-4;
		else if (thePad->deltaH < -16) thePad->deltaH = (-thePad->deltaH*thePad->deltaH)/64+4;
		else thePad->deltaH = 0;
	}
}
	
/***********************************************************
* Descriptif :	Initialise la gestion du joystick d'index
*				concerne (1 a 4)
* Etat : En cours
* Dernière Modif :	21/11/95 (Pierre)
*					
***********************************************************/
void BG_SelectJoystick(int joyref,int index) {
	if (BG_Joystick[index]) {
		BG_Joystick[index]->Close();
		delete BG_Joystick[index];
	}
	BG_Joystick[index] = new BJoystick();
	if (BG_Joystick[index]->Open(joystick_names[joyref]) < 0)
		BG_Joystick[index] = NULL;
}
