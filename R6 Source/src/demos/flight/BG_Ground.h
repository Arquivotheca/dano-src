#define		BG_GROUNDHMAX		128
#define		BG_GROUNDVMAX		128
#define		BG_NBBONUSMAX		108
#define		INDEX0(x,y)		(((x)&BG_GROUNDMASK)+(((y)&BG_GROUNDMASK)<<BG_GROUNDSHIFT))
#define		INDEX1(x,y)		((((x)+1)&BG_GROUNDMASK)+(((y)&BG_GROUNDMASK)<<BG_GROUNDSHIFT))
#define		INDEX2(x,y)		((((x)+1)&BG_GROUNDMASK)+((((y)+1)&BG_GROUNDMASK)<<BG_GROUNDSHIFT))
#define		INDEX3(x,y)		(((x)&BG_GROUNDMASK)+((((y)+1)&BG_GROUNDMASK)<<BG_GROUNDSHIFT))

extern	int		BG_GROUNDDH;
extern	int		BG_GROUNDDV;
extern	int		BG_GROUNDMASK;
extern	int		BG_GROUNDSHIFT;
extern	float	BG_AltFact;

typedef struct BG_VGCase
	{
	float				Altitude;			// Z coordonate of top left point
	short				Normales[4];		// index of the 4 normales
	BG_Pt3D				FNorm[2];			// normales of the 2 triangles
	char				Orientation;		// orientation of triangle cut.
	char				Color[2];			// index of ground color
	uchar				bonus;				// presence d'un bonus a collecter
	struct BG_Aircraft	*lien;				// lien sur des objets volants incrustes
	} BG_VGCase;

typedef struct BG_GroundHeader
	{
	int			BG_NbNorm;
	int			BG_NbFNorm;
	} BG_GroundHeader;
	
extern	float			*BG_Altimetric;
extern	BG_VGCase		*BG_VirtualGround;
extern	char			*BG_TableBonus;		// table d'etat des bonus
extern	unsigned short	*BG_OffsetBonus;	// offset radar bonus
extern	unsigned char	*BG_Lumieres[8];

// prototypes
void	BG_InitGroundModel(void);
void	BG_DisposeGroundModel(void);
void 	BG_AddCaseNormales(	BG_Pt3D		*NCase1,
							BG_Pt3D		*NCase2,
							char		*CaseColor,
							char		orient,
							BG_Pt3D		*norm,
							int			pt,
							char		color);
char	BG_ChooseColor(int i0,int i1,int i2);
char	BG_GetBorderColor(int i,int j,int border);
char	BG_GetSmoothColor(char theColor,char colorH,char color0,char color1);
void	BG_SmoothColor(int i,int j);
void	BG_ProcessVirtualGround(void);
void	BG_AddBonus(long RamdomKey);
Boolean	BG_PutBonus(int h,int v,int piste,int iBonus);
