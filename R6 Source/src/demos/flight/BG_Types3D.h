/********************************************************/
/*			DEFINITION DES CONSTANTES				    */
/********************************************************/

#define Boolean					int
#define BG_NB_ENTREES_COSTABLE 	8192

/********************************************************/
/*			DEFINITION DES TYPES UTILISES			    */
/********************************************************/

typedef struct BG_Matrice3_3
	{
	float 	X1_Y1;
	float 	X2_Y1;
	float 	X3_Y1;
	float 	X1_Y2;
	float 	X2_Y2;
	float 	X3_Y2;
	float 	X1_Y3;
	float 	X2_Y3;
	float 	X3_Y3;
	} BG_Matrice3_3;

typedef struct BG_Pt3D
	{
	float 	X;
	float 	Y;
	float 	Z;
	} BG_Pt3D;

typedef struct BG_Point3D
	{
	float	X;
	float 	Y;
	float 	Z;
	short	H;
	short	V;
	char	Projete;
	char	Masque;
	char	b3;
	char	b4;
	} BG_Point3D;
