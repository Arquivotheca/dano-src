#define		BG_MASKHMIN		1
#define		BG_MASKHMAX		2
#define		BG_MASKVMIN		4
#define		BG_MASKVMAX		8
#define		BG_MASKXMIN		16

#define		BG_ANTIZERO		0.00000001

// prototypes
void BG_Objet3DProjection(		BG_Point3D 		*v2,			// stockage des points projetes
								short 			nb_vecteurs,	// nb de points a traiter
								BG_ControlView	*theView);		// definition de la vue
void BG_Point3DMultMatrice3_3(	BG_Matrice3_3 	*m1,			// matrice de rotation
								BG_Pt3D 		*v1,			// table des points a projeter
								BG_Pt3D 		*o1,			// vecteur de translation
								BG_Point3D 		*v2,			// stockage des points projetes
								short 			nb_vecteurs,	// nb de points a traiter
								BG_ControlView	*theView);		// definition de la vue
void BG_Vect3DMultMatrice3_3(	BG_Matrice3_3 	*m1,			// matrice de rotation
								BG_Pt3D 		*v1,			// table des points a projeter
								BG_Pt3D 		*v2,			// stockage des points projetes
								short 			nb_vecteurs);
void BG_Objet3DMultMatrice3_3(	BG_Matrice3_3 	*m1,			// matrice de rotation
								BG_Pt3D 		*v1,			// table des points a projeter
								BG_Pt3D 		*o1,			// vecteur de translation
								BG_Point3D 		*v2,			// stockage des points projetes
								short 			nb_vecteurs,	// nb de points a traiter
								BG_ControlView	*theView);		// definition de la vue
