
void 	PASCAL	  GetImage525_RGBQ( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL    GetImage625_RGBQ( PBYTE pImage, int Stride, PDCT_DATA pY ); 
void 	PASCAL mmxGetImage525_RGBQ( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL mmxGetImage625_RGBQ( PBYTE pImage, int Stride, PDCT_DATA pY ); 

void 	PASCAL	  GetImage525_RGB16( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL    GetImage625_RGB16( PBYTE pImage, int Stride, PDCT_DATA pY ); 
void 	PASCAL mmxGetImage525_RGB16( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL mmxGetImage625_RGB16( PBYTE pImage, int Stride, PDCT_DATA pY ); 

void 	PASCAL	  GetImage525_RGB15( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL    GetImage625_RGB15( PBYTE pImage, int Stride, PDCT_DATA pY ); 
void 	PASCAL mmxGetImage525_RGB15( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL mmxGetImage625_RGB15( PBYTE pImage, int Stride, PDCT_DATA pY ); 

void 	PASCAL    GetImage525_YUY2( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL    GetImage625_YUY2( PBYTE pImage, int Stride, PDCT_DATA pY ); 
void 	PASCAL mmxGetImage525_YUY2( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
void    PASCAL mmxGetImage625_YUY2( PBYTE pImage, int Stride, PDCT_DATA pY ); 
