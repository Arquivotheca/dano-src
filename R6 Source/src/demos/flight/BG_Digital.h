#define	BG_JUSTIFIEGAUCHE	0
#define	BG_JUSTIFIEDROITE	1
#define	BG_JUSTIFIEHAUT		0
#define	BG_JUSTIFIEBAS		2

// protos
void BG_FillRect(char *data,int lgn,int col,int rowbyte,long color);
void BG_DrawDigit(BG_Canal3D *theCanal,int h,int v,int val,int zoom,long color);
void BG_FillRect24(long *data,int lgn,int col,int rowbyte,long color);
void BG_DrawDigit24(BG_Canal3D *theCanal,int h,int v,int val,int zoom,long color);
void BG_DrawDigital(BG_Canal3D *theCanal,int mode,int dh,int dv,int val,long color);
void BG_DrawViseur(BG_Canal3D *theCanal,long color);
