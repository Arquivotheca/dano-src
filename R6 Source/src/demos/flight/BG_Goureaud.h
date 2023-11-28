#include <SupportDefs.h>

typedef struct BG_GoureaudPt
	{
	short		H;
	short		V;
	short		Level;
	} BG_GoureaudPt;
	
// prototypes

#ifdef __cplusplus
extern "C" {
#endif

void BG_EraseTampon(uint32 *dest, int rowBytes, int rowSize, int NbLine, uint32 valeur);
void BG_Goureaud(BG_GoureaudPt *Triangle,unsigned char *Color);
void BG_Goureaud2(BG_GoureaudPt *Triangle,unsigned char *Color);
void BG_Goureaud24(BG_GoureaudPt *Triangle,unsigned char *Color);
void BG_Goureaud242(BG_GoureaudPt *Triangle,unsigned char *Color);

#ifdef __cplusplus
}
#endif
