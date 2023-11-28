#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _ALERT_H
#include <Alert.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _MENU_BAR_H
#include <MenuBar.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _MENU_H
#include <Menu.h>
#endif

#define	BG_WINDOWMAXH	800
#define	BG_WINDOWMAXV	600

#define	BG_VIEWMAPH		256
#define	BG_VIEWMAPV		256

#define		BG_MAPSIZE		26
#define		BG_MAPHALF		12
#define		BG_MAPFULL		25
#define		BG_NBCLIPPTMAX	1024

// general square structure
typedef struct BG_Square
	{
	short				Index[4];
	short				Norm[4];
	short				FNorm[2];
	BG_Pt3D				*TNorm;
	char				Color[2];
	char				bonus;
	char				pipo;
	struct BG_Aircraft	*lien;
	} BG_Square;

// specific triangle structure
typedef struct BG_Triangle
	{
	short		Index[3];
	short		Norm[3];
	char		Color;
	char		pipo;
	short		FNorm;
	} BG_Triangle;

// structure de controle de clipping visibilite
typedef struct BG_ControlView
	{
	float		zoom;
	float 		YXmin;
	float 		YXmax;
	float 		ZXmin;
	float 		ZXmax;
	short 		Hmin;
	short 		Hmax;
	short 		Vmin;
	short 		Vmax;
	short 		offsetH;
	short 		offsetV;
	} BG_ControlView;

// la classe View de l'appli
class BG_Canal3D : public BView
	{	
	public:
	int					indexFocale,NbSquareOrder,NbTriangle,NbClipPoint,CaseH,CaseV,curIndexPt;
	char				curColor;
	char				*baseAddr,*Offscreen;
	long				rowBytes;
	short				Largeur,Hauteur;
	short				*SquareOrder,*BorderLight,*BorderConvert;
	BRect				screenRect;
	BBitmap				*theBitmap;
	BG_Pt3D				*MapPoint,*BG_Camera;
	BG_Pt3D				ProjNorm[5];
	BG_Square			*MapSquare;
	BG_Point3D			*MapPtProj,*ProjPt;
	BG_Triangle			*MapTriangle;
	struct BG_BSP		*ListBSP;
	unsigned char 		*ListFace;
	BG_Matrice3_3		*BG_CameraRot;
	BG_ControlView		*LocalView,*BG_theView;
	BG_ControlView		CanalView;

	BG_Canal3D(BRect frame);
    ~BG_Canal3D(void);
				
	virtual	void	Draw(BRect updateRect);
			void	ChangeCanalView(float zoom);
			void	EraseCanal(void);
			void	InitLocalMap(void);
			void	DisposeLocalMap(void);
			void	DrawLocalGround(int				indexWindow,
									BG_Pt3D			Camera,
									BG_Matrice3_3	*CameraRot);
			void	ProcessSquare(int i,int j);
			void	ProcessClipping(int index);
			short	CalculateIntersection(	BG_Point3D	*pt1,
											BG_Point3D	*pt2,
											float		rel1,
											float		rel2,
											short		Norm1,
											short		Norm2,
											short		*NNorm);
			void	DrawAirCraft(	char			type,
									char			color,
									BG_Pt3D			*Camera,
									BG_Matrice3_3	*CameraRot,
									BG_Pt3D			*Centre,
									BG_Matrice3_3	*Rotation,
									float			Duree);
			void	SolveBSP(struct BG_BSP	*theBSP);
			void	DrawTriangle(unsigned char	*theFace);
	};

// View class of the map
class BG_ViewMap : public BView
	{
	BRect			screenRect;
	BBitmap			*theBitmap;
	
	public:
	short			Largeur;		// largeur de la zone utile en pixel
	short			Hauteur;		// hauteur de la zone utile en ligne
	long			rowBytes;		// ecart ligne a ligne de la zone de trace offscreen
	char			*baseAddr;		// adresse de base de l'offscreen
	char			*Offscreen;

	BG_ViewMap(	BRect frame);
    ~BG_ViewMap(void);
				
	virtual	void	Draw(BRect updateRect);
			void	Draw2(Boolean master);
	};

// la classe Window de l'appli
class BG_Window : public BWindow
	{
	public:
	int				PaddleRef;
	bool			YellowEnable,MapEnable,MasterEnable;
    float           menuHeight;
	BMenu			*MenuPlay,*MenuView;
	BWindow			*MapWindow;
	BMenuItem		*itemQuit,*itemSndPlr,*itemPause,*itemResume,*itemYellow,*itemRed;
	BMenuItem		*itemJoin,*itemAbort,*itemSize32,*itemSize64,*itemSize128;
	BMenuItem		*itemMap,*itemControlK,*itemPlay,*itemPlayNet;
	BMenuItem		*itemGroundCrash;
	BMenuItem		*itemFocale[5],*itemControl[5];
	BG_ViewMap		*MapView;
	BG_Canal3D		*theCanal;

	BG_Window(BRect frame,const char *title,window_type type,ulong flags,bool master);

	virtual void	FrameResized(float new_width,float new_height);	
	virtual	void	MessageReceived(BMessage* theMessage);
	virtual	void	Quit(void);
	virtual	bool	QuitRequested(void);
			void	CheckMenuBar(void);
			void	RefreshFrame(struct BG_Aircraft *camera);
	};

// globales diverses et variees
extern short	tabgauche[600];
extern short	tabdroite[600];
extern long		OffScreen_largeur, OffScreen_baseAddr;
extern short	tabgauche2[600];
extern short	tabdroite2[600];
extern long		OffScreen_largeur2, OffScreen_baseAddr2;

// interface
extern	unsigned short	*BG_thePalette;		// palette courante
extern	BG_Window		*BG_MaWindow[2];
extern	char			*BG_OffscreenMap;

// global status
extern	Boolean			BG_IsRunning,BG_QuitEnable,BG_MasterEnable;
extern	Boolean			BG_GameEnable,BG_GroundEnable;
extern	Boolean			BG_SndPlrEnable,BG_GroundCrash;
extern  Boolean         BG_Screen24;

// prototypes
void	BG_InitGraphic(void);
void	BG_DisposeGraphic(void);
long	BG_Clock(void);
