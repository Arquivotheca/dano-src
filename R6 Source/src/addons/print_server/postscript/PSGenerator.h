#ifndef _PSGENERATOR_H_
#define _PSGENERATOR_H_

#include <String.h>
#include <Rect.h>
#include <File.h>
#include <Message.h>
#include <List.h>
#include <Path.h>

#include <image.h>

class TPrintDrv;
class Convert;
class FontHandler;

class PSGenerator
{
 public:
						PSGenerator(TPrintDrv *parent, BNode *printerEntry);
						~PSGenerator();
						
	status_t			HandleJob(BFile *spool);

	enum
	{
		B_USER_CANCELED	= B_ERRORS_END+1
	};

 private:

	status_t			Assemble();

	status_t			GetPrinterInfo();

	status_t			GetPrintHeader();
	status_t			GetSetupMsgInfo();
	status_t			GetPageOffsets();
	
	status_t			LoadAndOpenTransport();
	status_t			CloseAndUnloadTransport();

	void 				ScanForUnhandled();

	status_t			GenerateAllPages();
	status_t			GeneratePage(int32 pagenum);
	
	status_t			ReadPrinterAttr(const char *attr, char *buf,
										int32 len);

	status_t			SendFileTo(const char *filename, BDataIO *output);
	
	void				WriteJclHeader(BMessage*, BDataIO*);
	void				WriteJclTrailer(BDataIO*);

	void				WritePpdFeatures(BMessage*, BDataIO*);

	TPrintDrv			*fPrintDrv;
	Convert				*fConvert;
	FontHandler			*fFontHandler;
	
	BPath				print_addon_dir;
	BPath				print_user_addon_dir;
	
	BNode				*fPrinterEntry;
	BFile				*fSpoolFile;

	BDataIO				*fTransport;
	image_id			fTransportImage;

	BString				fTransportName;
	BString				fPPDFile;

	int32				fNumPages;
	BList				fPageOffsets;

	BMessage			*fSetupMsg;
	
	BMessage			fPPDMsg;
	BRect				fPrintableRect;
	float				fReqResolution;
	bool				fFirstUp;
	bool				fUseBitmapConversion;
	int32				fNumCopies;
};


/* support classes */
class page_position
{
 public:
	off_t	position;
	long	nb_pictures;
	bool	Rasterize;
};

class _page_header_
{
 public:
	int32	pictureCount;
	off_t	nextPage;
	int32	reserved[10];
};


#endif
