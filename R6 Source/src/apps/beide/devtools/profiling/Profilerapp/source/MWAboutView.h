#include <iostream>

const BRect kAboutText(0,0, 50,50);
const BRect AboutFrame(0,120, 361,361);

class MWAboutView: public BView
{
	public:
		MWAboutView(BRect frame, const char *name, uint32 resizingMode, uint32 flags);
		~MWAboutView();
		virtual void Draw (BRect updateRect);
		BBitmap* LoadBitmap(const char 	*inBitMapData,
									float		inWidth,
									float		inHeight);
									
		 BBitmap* LoadBitmap(BResources	&inFile,
									type_code	inResType,
									int32		inResID,
									float		inWidth,
									float		inHeight);
	private:
	int32 err; 
   	app_info info;
   	BResources res; BFile file;
	BBitmap *AboutBitmap;
	BTextView *AboutText;
	
};