#define	BG_NBPLAYERMAX		8

extern	char		BG_StatusJoueur[BG_NBPLAYERMAX];
extern	char		BG_IndexPlayer[BG_NBPLAYERMAX];
extern	char		BG_YellowPlayer[BG_NBPLAYERMAX];
extern	long		BG_LastGroundRandomKey,BG_LastBonusRandomKey;
extern	int			BG_IndexLocalPlayer,BG_IndexLocalPlayer2;
extern	BG_VPaddle	BG_ListPaddle[BG_NBPLAYERMAX];

// prototypes
void				BG_InitSuperviseur(void);
void				BG_NewSuperviseur(void);
void				BG_NewSuperviseur2(void);
int					BG_Random(int mask);
void				BG_AbortSuperviseur(void);
void				BG_AbortSuperviseur2(void);
int					BG_ReservePlayer(char YellowMode);
void				BG_ReleasePlayer(int index);
void				BG_ResetSuperviseur(void);
void				BG_JoinSuperviseur(void);
void				BG_PlaySuperviseur(struct BG_Aircraft *ListCamera[2]);
void				BG_ReadSuperviseur(void);
void				BG_DisposeSuperviseur(void);
