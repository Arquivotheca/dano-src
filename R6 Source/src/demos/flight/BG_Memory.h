#define	MEM_HEADER_SIZE	sizeof(BG_MemHeader)
#define	RATIO_SEUIL		-40			// negation de la taille minimale de residu toleree
#define	BLOCK_MAX		1048576L	// taille maximale autorisé pour un block de pile supérieure
#define	RESERVED_MEMORY	(256*1024L)	// memoire reservé en dehors du MM interne, pour l'OS Mac

// Header de description de bloc pour le MM
typedef struct BG_MemHeader
	{
	long				BlockSize;	// taille du bloc
	struct BG_MemHeader	*next;		// pointeur sur le bloc suivant
	struct BG_MemHeader	*prev;		// pointeur sur le bloc precedent
	} BG_MemHeader;

// prototypes
void	BG_InitMemory(void);
void	BG_DisposeMemory(void);
char	*BG_GetMemLow(long size);
void	BG_FreeMemLow(char *ptr);
char	*BG_GetMemHigh(long size);
void	BG_CheckMemHigh(void);
void	BG_FreeMemHigh(char *ptr);
void	BG_BlockMove(char *src,char *dest,long size);
