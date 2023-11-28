#include "BG_Erreur.h"
#include "BG_Types3D.h"
#include "BG_Memory.h"
#include "BG_Graphic.h"
#include "BG_Digital.h"

static	char	DigitToBarre[10] = {0x77,0x24,0x5D,0x6D,0x2E,0x6B,0x7B,0x25,0x7F,0x6F};

/***********************************************************
* Descriptif :
*
* Etat : Stable
* Dernire Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_FillRect(char *data,int lgn,int col,int rowbyte,long color)
	{
	int		col2;
	
	while (lgn > 0)
		{
		col2 = col;
		while (col2>=4)
			{
			col2 -= 4;
			*((long*)(data+col2)) = color;
			}
		if (col2 >= 2)
			{
			col2 -= 2;
			*((short*)(data+col2)) = color;
			}
		if (col2 != 0) *data = color;
		lgn--;
		data += rowbyte;
		}
	}

/***********************************************************
* Descriptif :
*
* Etat : Stable
* Dernire Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_DrawDigit(BG_Canal3D *theCanal,int h,int v,int val,int zoom,long color)
	{
	int			row,trizoom;
	char		curs;
	char		*base;
	
	row = theCanal->rowBytes*zoom;
	trizoom = 3*zoom;
	curs = DigitToBarre[val];
	base = theCanal->baseAddr+h+v*theCanal->rowBytes;
	if (curs & 1)
		BG_FillRect(base+zoom,zoom,trizoom,theCanal->rowBytes,color);
	if (curs & 2)
		BG_FillRect(base+row,trizoom,zoom,theCanal->rowBytes,color);
	if (curs & 4)
		BG_FillRect(base+row+4*zoom,trizoom,zoom,theCanal->rowBytes,color);
	if (curs & 8)
		BG_FillRect(base+4*row+zoom,zoom,trizoom,theCanal->rowBytes,color);
	if (curs & 16)
		BG_FillRect(base+5*row,trizoom,zoom,theCanal->rowBytes,color);
	if (curs & 32)
		BG_FillRect(base+5*row+4*zoom,trizoom,zoom,theCanal->rowBytes,color);
	if (curs & 64)
		BG_FillRect(base+8*row+zoom,zoom,trizoom,theCanal->rowBytes,color);
	}

/***********************************************************
* Descriptif :
*
* Etat : Stable
* Dernire Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_FillRect24(long *data,int lgn,int col,int rowbyte,long color)
{
	int		col2;
	long    *data2;
	
	while (lgn > 0)
	{
		data2 = data;
		lgn--;
		data = (long*)((long)data+rowbyte);
		for (col2=0;col2<col;col2++)
		{
			*data2 = color;
			data2++;
		}
	}
}

/***********************************************************
* Descriptif :
*
* Etat : Stable
* Dernire Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_DrawDigit24(BG_Canal3D *theCanal,int h,int v,int val,int zoom,long color)
	{
	int			row,trizoom;
	char		curs;
	char		*base;
	
	row = theCanal->rowBytes*zoom;
	trizoom = 3*zoom;
	curs = DigitToBarre[val];
	base = (char*)(theCanal->baseAddr+h*4+v*theCanal->rowBytes);
	if (curs & 1)
		BG_FillRect24((long*)(base+4*zoom),zoom,trizoom,theCanal->rowBytes,color);
	if (curs & 2)
		BG_FillRect24((long*)(base+row),trizoom,zoom,theCanal->rowBytes,color);
	if (curs & 4)
		BG_FillRect24((long*)(base+row+16*zoom),trizoom,zoom,theCanal->rowBytes,color);
	if (curs & 8)
		BG_FillRect24((long*)(base+4*row+4*zoom),zoom,trizoom,theCanal->rowBytes,color);
	if (curs & 16)
		BG_FillRect24((long*)(base+5*row),trizoom,zoom,theCanal->rowBytes,color);
	if (curs & 32)
		BG_FillRect24((long*)(base+5*row+16*zoom),trizoom,zoom,theCanal->rowBytes,color);
	if (curs & 64)
		BG_FillRect24((long*)(base+8*row+4*zoom),zoom,trizoom,theCanal->rowBytes,color);
	}

/***********************************************************
* Descriptif :
*
* Etat : Stable
* Dernire Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_DrawDigital(BG_Canal3D *theCanal,int mode,int dh,int dv,int val,long color)
	{
	int			nb,zoom,h,v;	
	char		chiffre[5];
	
// decompose le nombre et mesure sa longueur
	nb = 0;
	do
		{
		chiffre[nb++] = val%10;
		val /= 10;
		}
	while (val != 0);
// determine l'echelle de zoom
	zoom = (theCanal->Largeur/120)+1;
	h = (theCanal->Hauteur/80)+1;
	if (h<zoom) zoom = h;
// l'offset de placement local
	if (mode & BG_JUSTIFIEDROITE)
		h = theCanal->Largeur-(dh+7*nb-2)*zoom;
	else
		h = dh*zoom;
	if (mode & BG_JUSTIFIEBAS)
		v = theCanal->Hauteur-(dv+9)*zoom;
	else
		v = dv*zoom;
// affiche les digits
	if (BG_Screen24)
		for (nb--;nb>=0;nb--)
		{
			BG_DrawDigit24(theCanal,h,v,chiffre[nb],zoom,color);
			h += 7*zoom;
		}
	else
		for (nb--;nb>=0;nb--)
		{
			BG_DrawDigit(theCanal,h,v,chiffre[nb],zoom,color);
			h += 7*zoom;
		}
	}

/***********************************************************
* Descriptif :
*
* Etat : Stable
* Dernire Modif :	07/09/94 (Pierre)
*					
***********************************************************/
void BG_DrawViseur(BG_Canal3D *theCanal,long color)
	{
	int			h,v,dh,dv,dh2,dv2;	
	
	h = theCanal->Largeur>>1;
	v = theCanal->Hauteur>>1;
	dh = h>>1;
	dv = v>>1;
	if (dh > 50) dh = 50;
	if (dv > 50) dv = 50;
	dh2 = dh>>1;
	dv2 = dv>>1;
	if (dh2 > 15) dh2 = 15;
	if (dv2 > 15) dv2 = 15;
// decompose le nombre et mesure sa longueur
	if (BG_Screen24)
	{
		BG_FillRect24((long*)(theCanal->baseAddr+h*4+(v-dv-dv2)*theCanal->rowBytes),dv,1,theCanal->rowBytes,color);
		BG_FillRect24((long*)(theCanal->baseAddr+h*4+(v+dv2)*theCanal->rowBytes),dv,1,theCanal->rowBytes,color);
		BG_FillRect24((long*)(theCanal->baseAddr+(h-dh-dh2)*4+v*theCanal->rowBytes),1,dh,theCanal->rowBytes,color);
		BG_FillRect24((long*)(theCanal->baseAddr+(h+dh2)*4+v*theCanal->rowBytes),1,dh,theCanal->rowBytes,color);
	}
	else
	{
		BG_FillRect(theCanal->baseAddr+h+(v-dv-dv2)*theCanal->rowBytes,dv,1,theCanal->rowBytes,color);
		BG_FillRect(theCanal->baseAddr+h+(v+dv2)*theCanal->rowBytes,dv,1,theCanal->rowBytes,color);
		BG_FillRect(theCanal->baseAddr+h-dh-dh2+v*theCanal->rowBytes,1,dh,theCanal->rowBytes,color);
		BG_FillRect(theCanal->baseAddr+h+dh2+v*theCanal->rowBytes,1,dh,theCanal->rowBytes,color);
	}
}
























