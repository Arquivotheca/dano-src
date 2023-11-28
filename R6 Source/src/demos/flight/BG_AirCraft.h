// le double du rayon maximum d'un objet volant
#define	BG_RAYONAIRCRAFT	0.2
#define	BG_RAYONBONUS		0.1
#define	BG_NBAIRCRAFTMAX	20

typedef struct BG_BSP
	{
	int16		Pt;
	int16		Norm;
	int16		Sup;
	int16		Inf;
	} BG_BSP;
	
typedef struct BG_Aircraft
	{
	BG_Pt3D				Centre;
	BG_Matrice3_3		Rotation;
	float				Vitesse;
	float				Duree;
	float				Rayon;
	int8				Type;		// 0 helicoptere, 1 avion, 2 missile, 3 explo, -1 detruit
	int8				Color;		// 5 a 7 : 5 cibles, 6 et 7 les 2 camps
	int8				pipo[2];
	struct BG_Aircraft	*next;		// index de l'objet suivant se trouvant dans la meme case
	} BG_Aircraft;

#define		BG_NbPt0	16
#define		BG_NbVect0	2
extern	BG_Pt3D			BG_ListPt0[];
extern	BG_Pt3D			BG_ListVect0[];
extern	unsigned char	BG_ListFace0[];
extern	BG_BSP			BG_ListBSP0[];

#define		BG_NbPt1	18
#define		BG_NbVect1	5
extern	BG_Pt3D			BG_ListPt1[];
extern	BG_Pt3D			BG_ListVect1[];
extern	unsigned char	BG_ListFace1[];
extern	BG_BSP			BG_ListBSP1[];

#define		BG_NbPt2	4
#define		BG_NbVect2	1
extern	BG_Pt3D			BG_ListPt2[];
extern	BG_Pt3D			BG_ListVect2[];
extern	unsigned char	BG_ListFace2[];
extern	BG_BSP			BG_ListBSP2[];

#define		BG_NbPt3	6
#define		BG_NbVect3	1
extern	BG_Pt3D			BG_ListPt3[];
extern	BG_Pt3D			BG_ListVect3[];
extern	unsigned char	BG_ListFace3[];
extern	BG_BSP			BG_ListBSP3[];

extern	BG_Aircraft		BG_ListAppareil[BG_NBAIRCRAFTMAX];
extern	BG_Matrice3_3	Rot0;

// prototype
int		BG_NewAircraft(int Type,int Color,BG_Pt3D *Centre,BG_Matrice3_3 *Rot,float Vitesse);
void	BG_BoumAircraft(BG_Aircraft *Obj,int choc);
void	BG_MoveAircraft(BG_Aircraft *Obj,float deltaT);
void	BG_RemoveObject(BG_Aircraft *Obj);
void	BG_AddObject(BG_Aircraft *Obj);
float	BG_GetGroundAlt(float X,float Y);
int		BG_CheckContact(BG_Pt3D *Centre);
int 	BG_CheckContactSquare(BG_Pt3D *Centre,int i,int j);
int 	BG_CheckChocTriangle(BG_Pt3D	*Centre,
						 	BG_Pt3D	*P1,
						 	BG_Pt3D	*P2,
						 	BG_Pt3D	*P3,
						 	BG_Pt3D	*norm,
						 	char	Color);
