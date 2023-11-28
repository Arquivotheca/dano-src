/* 
 * If running on a LAN, this should be defined to be 0
 * If testing on a single machine, it should be 1
 */ 
 
#define LOCAL_TESTING		0

#if LOCAL_TESTING
#	define BROADCAST_ADDR htonl(0x7f000001)
#	define BOUND_ADDR	  htonl(0x7f000001)
#else
#	define BROADCAST_ADDR htonl(0xffffffff)
#	define BOUND_ADDR	  htonl(0x00000000)
#endif

#define BG_WELL_KNOWN_PORT	9173	

#define	BG_NETWORKNULL		0
#define	BG_NETWORKMASTER	1
#define	BG_NETWORKSLAVE		2

// structure d'emission du maitre
typedef struct BG_MasterPack
	{
	int32		GroundKey;
	int32		BonusKey;
	int16		SizeGround;
	int16		BG_NbBonus;
	int16		LocalIndex[2];
	int16		PtCampA;
	int16		PtCampB;
	int8		TableBonus[BG_NBBONUSMAX];
	BG_Aircraft	List[BG_NBAIRCRAFTMAX];
	} BG_MasterPack;

// structure d'emission de l'esclave
typedef struct BG_SlavePack
	{
	char		YellowMode[2];
	char		KillFlag;
	char		SndPlrEnable;
	BG_VPaddle	Pad[2];
	} BG_SlavePack;

#define BUFSIZE				sizeof(BG_MasterPack)

extern	sem_id		BG_semClient0,BG_semClient1;

// prototypes
void	BG_FlattenMasterPack(BG_MasterPack *m, int32 *indexJoueur);
void	BG_UnflattenMasterPack(BG_MasterPack *m);
void	BG_SwapVPaddle(BG_VPaddle *from, BG_VPaddle *to);
long	BG_TcpHandler(void *v);
long	BG_ServerThread(void *v);
int		BG_StartTcpListener(void);
long	BG_Server(void);
int		BG_FindServer(struct sockaddr_in *sinp);
long	BG_Client(void);
void	BG_SelectNetworkMode(int mode);

