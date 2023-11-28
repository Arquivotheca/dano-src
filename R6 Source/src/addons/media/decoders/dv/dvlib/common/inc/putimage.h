void 	PASCAL	  PutImage525_RGBQ( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL    PutImage625_RGBQ( PBYTE pImage, int Stride, PDCT_DATA pY ); 
void 	PASCAL mmxPutImage525_RGBQ( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL mmxPutImage625_RGBQ( PBYTE pImage, int Stride, PDCT_DATA pY ); 

void 	PASCAL	  PutImage525_RGB16( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL    PutImage625_RGB16( PBYTE pImage, int Stride, PDCT_DATA pY ); 
void 	PASCAL mmxPutImage525_RGB16( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL mmxPutImage625_RGB16( PBYTE pImage, int Stride, PDCT_DATA pY ); 

void 	PASCAL	  PutImage525_RGB15( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL    PutImage625_RGB15( PBYTE pImage, int Stride, PDCT_DATA pY ); 
void 	PASCAL mmxPutImage525_RGB15( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL mmxPutImage625_RGB15( PBYTE pImage, int Stride, PDCT_DATA pY ); 

void 	PASCAL    PutImage525_YUY2( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL    PutImage625_YUY2( PBYTE pImage, int Stride, PDCT_DATA pY ); 
void 	PASCAL mmxPutImage525_YUY2( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL mmxPutImage625_YUY2( PBYTE pImage, int Stride, PDCT_DATA pY ); 

