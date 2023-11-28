#define fcos(n) CosTable[n]
#define fsin(n) CosTable[(n-(BG_NB_ENTREES_COSTABLE/4)) & (BG_NB_ENTREES_COSTABLE-1)]

extern	float	*CosTable;

// protos
void	BG_InitCosTable(void);
void	BG_DisposeCosTable(void);
float	BG_InvSqrt(float x);
float	BG_Sqrt(float x);

