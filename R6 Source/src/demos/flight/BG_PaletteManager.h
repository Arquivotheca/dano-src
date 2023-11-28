// structure locale de traitement pour le calcul des palettes avec eclairage
typedef struct BG_Couleur
	{
	long				Red;
	long				Green;
	long				Blue;
	long				Total;
	long				Ecart;
	short				next;
	short				prev;
	short				Index;
	} BG_Couleur;
	
// prototypes a usage interne
void		BG_GetBestColor(BG_Couleur *coul,unsigned char *P1col,unsigned char *I1col,unsigned char *P2col,unsigned char *I2col);
long		BG_GetDistance(long a1,long a2,long a3,long b1,long b2,long b3);

// prototypes a usage externe
void		BG_OpenPaletteManager(void);
void		BG_ClosePaletteManager(void);
