#include <iostream>
// -----------------------------------------------------------------------
//		? LoadBitmap
// -----------------------------------------------------------------------
//	Take an array holding the bitmap data and return a bitmap with that data.

BBitmap*
LoadBitmap(
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
LoadBitmap(
	BResources&		inFile,
	type_code		inResType,
	int32			inResID,
	float			inWidth,
	float			inHeight)
{
	size_t			length;
	BBitmap*		bitmap = NULL;
	void*			resData = inFile.FindResource(inResType, inResID, &
length);
		cout <<"Bingo\n";
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