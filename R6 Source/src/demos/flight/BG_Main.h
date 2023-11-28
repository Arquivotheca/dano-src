#ifndef _OS_H
#include <OS.h>
#endif

#define	BG_PADDLEKEYBOARD	0
#define	BG_PADDLEJOYSTICK1A	1
#define	BG_PADDLEJOYSTICK1B	2
#define	BG_PADDLEJOYSTICK2A 3
#define	BG_PADDLEJOYSTICK2B 4

#define		BG_LOWBORDER	32
#define		BG_HIGHBORDER	32

//#define	fprintf(a,b)	acquire_sem(zsem);fprintf(a,b);release_sem(zsem)

extern	int				BG_SizeGround,BG_PaddleRef;
extern	int				BG_PtCampA,BG_PtCampB,BG_NbBonus;
extern	sem_id			BG_semRender,BG_semRenderIn,BG_semRenderOut,BG_semJoueur,BG_semKill;
extern	unsigned long	BG_CurseurColor[3];
extern  int             BG_DeadFlag;

// prototypes
void BG_Rendering(void);
void BG_Rendering2(void);
void BG_Init(void);
void BG_Kill(void);
void BG_Dispose(void);
void BG_InitJoueurs(void);
void BG_NewGround(int taille,long random);
void BG_ResetGround(long random);
void BG_CreatePalette(unsigned char *eclairage,long Red,long Green,long Blue);

