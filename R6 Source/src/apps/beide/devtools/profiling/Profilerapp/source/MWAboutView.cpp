#include "MWAboutView.h"



MWAboutView::MWAboutView(BRect frame, const char *name, uint32 resizingMode, uint32 flags):
		BView(frame,name,resizingMode,flags)
{
	if ((err=be_app->GetAppInfo(&info))!=B_OK)
		cout <<"Can't Find myself" <<endl;
		
	if ((file.SetTo(&info.ref, O_RDONLY))!=B_OK)
		cout <<"Can't open myself" <<endl;
	   
   	if ((err = res.SetTo(&file))== B_OK )
	{	
		/** Lets get that kewl about box image and draw it **/
		SetDrawingMode(B_OP_OVER);
		AboutBitmap=LoadBitmap(res,'zeid',1,244,241);
	}
}


MWAboutView::~MWAboutView()
{
	delete AboutBitmap;

}


/* Draw the whole bitmap when called, no need to use the BRect since image is
small and anyway, we aren't doing anything spectacular*/
void
MWAboutView::Draw (BRect updateRect)
{
	if (AboutBitmap != NULL)
		DrawBitmap(AboutBitmap);
	else  /** Yes I know, this must be the worst error checking on earth
			but I didn't have anytime for this **/
		cout<< "Bitmap not loaded!" <<endl;
}

// -----------------------------------------------------------------------
//		? LoadBitmap
// -----------------------------------------------------------------------
//	Take an array holding the bitmap data and return a bitmap with that data.

BBitmap*
MWAboutView::LoadBitmap(
	const char * 	inBitMapData,
	float			inWidth,
	float			inHeight)
{
	BRect			bounds(0.0, 0.0, inWidth - 1, inHeight - 1);
	BBitmap*		bitmap = new BBitmap(bounds, B_COLOR_8_BIT);
	
	const char * 		ptr = inBitMapData;
	uint32				wid = (uint32) inWidth;
	int 				i;
	int					height = inHeight;
	int					rowBytes = bitmap->BytesPerRow();
	
	for (i = 0; i < height; i++)
	{
		bitmap->SetBits(ptr, wid, rowBytes * i, B_COLOR_8_BIT);
		ptr += wid;
	}
	
	return bitmap;
}

// -----------------------------------------------------------------------
//		? LoadBitmap
// -----------------------------------------------------------------------
//	Take an array holding the bitmap data and return a bitmap with that data.

BBitmap*
MWAboutView::LoadBitmap(
	BResources&		inFile,
	type_code		inResType,
	int32			inResID,
	float			inWidth,
	float			inHeight)
{
	size_t			length;
	BBitmap*		bitmap = NULL;
	void*			resData = inFile.FindResource(inResType, inResID, &length);
		
	if (resData != NULL)
	{
		BRect			bounds(0.0, 0.0, inWidth - 1, inHeight - 1);
		bitmap = new BBitmap(bounds, B_COLOR_8_BIT);
		
		const char * 		ptr = static_cast<const char *>(resData);
		const uint32		wid = inWidth;
		int 				i;
		int					height = inHeight;
		int					rowBytes = bitmap->BytesPerRow();
		
		for (i = 0; i < height; i++)
		{
			bitmap->SetBits(ptr, wid, rowBytes * i, B_COLOR_8_BIT);
			ptr += wid;
		}

		free(resData);
	}
	return bitmap;
}