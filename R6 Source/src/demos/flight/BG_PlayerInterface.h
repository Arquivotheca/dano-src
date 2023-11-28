#define		BG_VPADDLECLAVIER	0
#define		BG_VPADDLEJOYSTICK	1

// structure decrivant l'etat d'un paddle de pilotage
typedef struct BG_VPaddle
	{
	short		deltaH;		// position (H et V)
	short		deltaV;
	char		button[4];	// etat des 4 boutons
	} BG_VPaddle;

// structure decrivant la configuration d'un VPaddle clavier
typedef struct BG_CfgVPaddleClavier
	{
	char		Haut;		// panel de deplacament
	char		Bas;
	char		Gauche;
	char		Droit;		
	char		Bouton[4];	// boutons d'action
	long 		Tempo[4];	// temps de premiere pressions des touches de direction
	} BG_CfgVPaddleClavier;

// prototypes generaux
void	BG_InitPlayerInterface(void);
void	BG_DisposePlayerInterface(void);
int32	BG_GetDeviceCount();
char	*BG_GetDeviceName(int32 index);
void 	BG_SelectVPaddle(int PaddleRef,int index);
void	BG_GetVPaddle(BG_VPaddle *thePad,long Tempo,int index);
void	BG_SelectJoystick(int joyref,int index);
