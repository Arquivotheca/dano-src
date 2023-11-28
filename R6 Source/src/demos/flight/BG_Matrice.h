// prototypes a usage externe
void		BG_Matrice3_3Rotation3D(short direction,short inclinaison,short assiette,BG_Matrice3_3 *m);
void		BG_Matrice3_3Rotation3DEchelle(short direction,short inclinaison,short assiette,BG_Matrice3_3 *m,float echelle);
void		BG_Matrice3_3MultMatrice3_3(BG_Matrice3_3 m1,BG_Matrice3_3 m2,BG_Matrice3_3 *m);
float		BG_Matrice3_3Determinant(BG_Matrice3_3 m);
void		BG_Matrice3_3Invert(BG_Matrice3_3 m1,BG_Matrice3_3 *m);
float		BG_ProduitScalaire(BG_Pt3D *v1,BG_Pt3D *v2);
void		BG_ExprimeVect(BG_Pt3D *Pt,int Pt1,int Pt2,BG_Pt3D *v1);
void 		BG_ProduitVectoriel(BG_Pt3D *v1,BG_Pt3D *v2,BG_Pt3D *v3);
float 		BG_NormaliserVect(BG_Pt3D *v3);
void		BG_Matrice3_3InvertEchelle(BG_Matrice3_3 m1,BG_Matrice3_3 *m,float coeff);

