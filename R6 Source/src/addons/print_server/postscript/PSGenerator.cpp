#include "PSGenerator.h"
#include "Postscript.h"
#include "Convert.h"
#include "FontHandler.h"
#include "ppd_control.h"
#include "Scanner.h"

#include <Alert.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <PrintJob.h>
#include <Picture.h>

#define TRANSPORT_SEARCH_PATHS 3
BPath	gTransportSearchPaths[TRANSPORT_SEARCH_PATHS];

PSGenerator::PSGenerator(TPrintDrv *parent, BNode *printerEntry)
{
	fPrintDrv = parent;
	fPrinterEntry = printerEntry;
	fUseBitmapConversion = false;
	fFontHandler = NULL;
	fSetupMsg = NULL;

	// where to find the transport add-ons...
	find_directory (B_USER_ADDONS_DIRECTORY, &(gTransportSearchPaths[0]));
	gTransportSearchPaths[0].Append ("Print/transport");

	find_directory (B_COMMON_ADDONS_DIRECTORY, &(gTransportSearchPaths[1]));
	gTransportSearchPaths[1].Append ("Print/transport");

	find_directory (B_BEOS_ADDONS_DIRECTORY, &(gTransportSearchPaths[2]));
	gTransportSearchPaths[2].Append ("Print/transport");
}

PSGenerator::~PSGenerator()
{
	delete fConvert;
	delete fFontHandler;
}

status_t
PSGenerator::HandleJob(BFile *spool)
{
	status_t err = B_ERROR;
	fSpoolFile = spool;

	err = GetPrinterInfo();
	if(err < 0) {
		return err;
	}

	err = fPrintDrv->ReadPPD(fPPDFile.String());
	if(err < 0) {
		return err;
	}
		
	err = GetPrintHeader();
	if(err < 0) {
		return err;
	}
	
	err = GetSetupMsgInfo();
	if(err < 0) {
		return err;
	}
	
	GetPageOffsets();

	fFontHandler = new FontHandler();

	fConvert = new Convert(fSpoolFile, fFontHandler);
	fConvert->SetLanguageLevel((PPD::Instance())->LanguageLevel());		
	fConvert->InitConvertion(fSetupMsg);

	ScanForUnhandled();

	err = GenerateAllPages();
	if(err < 0) {
		return err;
	}
	
	err = fFontHandler->PrepareFontFile();
	if(err < 0) {
		return err;
	}	
		
	err = Assemble();
	if(err < 0) {
		return err;
	}

	return err;
}

status_t
PSGenerator::GenerateAllPages()
{
	status_t err = B_OK;
	char status[64];
	
	if(fFirstUp) {
		for(int32 i=0; i < fNumCopies; i++) {
			for(int32 page=0; page < fNumPages; page++) {
				sprintf(status, "Printing page %ld", page+1);
				fSpoolFile->WriteAttr("_spool/Status", B_STRING_TYPE, 0, status,
							strlen(status)+1);
				err = GeneratePage(page);
				if(err < 0) {
					return err;
				}
			}
		}
	} else {
		for(int32 i=0; i < fNumCopies; i++) {
			for(int32 page=fNumPages; page > -1; page--) {
				sprintf(status, "Printing page %ld", page+1);
				fSpoolFile->WriteAttr("_spool/Status", B_STRING_TYPE, 0, status,
							strlen(status)+1);
				err = GeneratePage(page);
				if(err < 0) {
					return err;
				}
			}
		}	
	}
	
	return err;
}

void
PSGenerator::ScanForUnhandled()
{
	if(fUseBitmapConversion) return;
	
	page_position *page_pos;
	Scanner *scanner = new Scanner(fSpoolFile);

	for(int32 page= 0; page < fNumPages; page++) {
		page_pos = static_cast<page_position*>(fPageOffsets.ItemAt(page));
		fSpoolFile->Seek(page_pos->position, SEEK_SET);
		for(int32 pic=0; pic < page_pos->nb_pictures; pic++) {
			page_pos->Rasterize = scanner->CheckPictureForUnhandledOps();
		}	
	}

	delete scanner;
}

status_t
PSGenerator::Assemble()
{
	status_t err;
	err = LoadAndOpenTransport();
	if(err != B_OK) {
		return err;
	}

	// jcl header
	WriteJclHeader(&fPPDMsg, fTransport);
	
	// fonts
	const char *fontFile = fFontHandler->FontFilePath();
	SendFileTo(fontFile, fTransport);

	// ps header
	fConvert->WriteHeader(fTransport);

	// ppd features
	WritePpdFeatures(&fPPDMsg, fTransport);
	
	// font aliases
	fFontHandler->WriteFontAliases(fTransport);
	
	// ps body
	const char *docBodyFile = fConvert->DocumentBodyFile();
	SendFileTo(docBodyFile, fTransport);
		
	// jcl trailer	
	WriteJclTrailer(fTransport);
	
	err = CloseAndUnloadTransport(); 

	return err;
}

status_t
PSGenerator::GeneratePage(int32 pagenum)
{
	status_t err = B_OK;
	page_position *page_pos;
	page_pos = static_cast<page_position*>(fPageOffsets.ItemAt(pagenum));
	fSpoolFile->Seek(page_pos->position, SEEK_SET);

	fConvert->TopOfPage();

	if(fUseBitmapConversion || page_pos->Rasterize) {

		BPicture	**picture;
		BPoint		*where;
		BRect		*clips;

		picture = new BPicture *[page_pos->nb_pictures];
		where = new BPoint[page_pos->nb_pictures];
		clips = new BRect[page_pos->nb_pictures];

		for (int32 p = 0; p < page_pos->nb_pictures; p++) {
			fSpoolFile->Read(&where[p], sizeof(BPoint));
			fSpoolFile->Read(&clips[p], sizeof(BRect));
			picture[p] = new BPicture();
			picture[p]->Unflatten(fSpoolFile);
		}

		err = fConvert->RasterizePictures(page_pos->nb_pictures, picture, where, clips,
											fPrintableRect, fReqResolution);

		for (int32 p = 0; p < page_pos->nb_pictures; p++) {
			delete picture[p];
		}
		
		delete [] picture;
		delete [] where;
		delete [] clips;

	} else {
		fSpoolFile->Seek(page_pos->position, SEEK_SET);
		
		for(int32 pic=0; pic < page_pos->nb_pictures; pic++) {
			err = fConvert->DoConvertionForPicture();
			if(err < 0) {
				return err;
			}
		}
	}
	
	fConvert->NewPage();
		
	return err;
}

status_t
PSGenerator::GetPrinterInfo()
{
	ssize_t rv;
	char buf[B_FILE_NAME_LENGTH];

	rv = ReadPrinterAttr("ppd_name", buf, sizeof(buf));
	if(rv < 0) {
		return rv;
	}
	fPPDFile = buf;
	
	rv = ReadPrinterAttr("transport", buf, sizeof(buf));
	if(rv < 0) {
		return rv;
	}
	fTransportName = buf;			

	return rv;
}

status_t
PSGenerator::GetSetupMsgInfo()
{
  	/* Needs to be called immediately after GetPrintHeader() */
	fSetupMsg = new BMessage;
	fSetupMsg->Unflatten(fSpoolFile);
	/* check return value! */
	
	if(fSetupMsg->HasBool("save") && fSetupMsg->FindBool("save")){
		fTransportName = "File";
	}
	
	if(fSetupMsg->HasMessage("ppd_item")){
		fSetupMsg->FindMessage("ppd_item", &fPPDMsg);
	}	

	if(fSetupMsg->HasRect("printable_rect")){
		fSetupMsg->FindRect("printable_rect", &fPrintableRect);
	}
	
	if(fSetupMsg->HasFloat("resolution")){
		fSetupMsg->FindFloat("resolution", &fReqResolution);
	} else {
		fReqResolution = (PPD::Instance(fSetupMsg))->Resolution();
	}
	
	if(fSetupMsg->HasBool("first up")){
		fSetupMsg->FindBool("first up", &fFirstUp);
	} else {
		fFirstUp = true;
	}
	
	if(fSetupMsg->HasInt32("copies")){
		fSetupMsg->FindInt32("copies", &fNumCopies);
	} else {
		fNumCopies = 1;
	}
	
	if(fSetupMsg->HasBool("bitmap")) {
		fUseBitmapConversion = fSetupMsg->FindBool("bitmap");
	} else {
		fUseBitmapConversion = false;
	}
	

	return B_OK;	
}

status_t
PSGenerator::GetPageOffsets()
{
	/* find the offsets into the spool file for each page description */
	_page_header_ pageHeader;
	page_position *page_pos;
	for (int32 page = 0; page < fNumPages; page++) {
		page_pos = new page_position();
		fSpoolFile->Read(&pageHeader,sizeof(pageHeader));
		page_pos->nb_pictures = pageHeader.pictureCount;
		page_pos->position = fSpoolFile->Position();
		page_pos->Rasterize = false; // don't rasterize
		fPageOffsets.AddItem(page_pos);
		fSpoolFile->Seek(pageHeader.nextPage,SEEK_SET);
	}
	
	return B_OK;	
}

status_t
PSGenerator::GetPrintHeader()
{
	off_t offset = fSpoolFile->Seek(0, SEEK_SET);
	if(offset < 0) {
		return offset;
	}
	
	ssize_t rv;
	BPrintJob::print_file_header header;
	rv = fSpoolFile->Read(&header, sizeof(header));
	if(rv < 0) {
		return rv;
	}
	
	fNumPages = header.page_count;

	return rv;
}

status_t
PSGenerator::LoadAndOpenTransport()
{
	status_t rv;
	
	/* First, find and load the transport add-on */
	BPath path;
	fTransportImage = -1;

	for(int i=0; ((i < TRANSPORT_SEARCH_PATHS) && (fTransportImage < 0)); i++) {
		path = gTransportSearchPaths[i];
		path.Append(fTransportName.String());
		fTransportImage = load_add_on(path.Path());	
	}

	if(fTransportImage < 0)	{
		(new BAlert("", "Couldn't find the transport add-on.", "Cancel"))->Go();
		return fTransportImage;
	}		

	/* Next, find the transport entry function */	
	BDataIO* (*entry_func)(BMessage*, BMessage*);
	rv = get_image_symbol(fTransportImage, "init_transport",
							B_SYMBOL_TYPE_TEXT, (void**)&entry_func);
	if(rv < 0) {
		unload_add_on(fTransportImage);
		(new BAlert("", "Couldn't find init_transport function.", "Cancel"))->Go();
		return rv;
	}

	/* create the message to pass to init_transport */
	node_ref tmpRef;
	fPrinterEntry->GetNodeRef(&tmpRef);

	BDirectory tmpDir;
	tmpDir.SetTo(&tmpRef);
	BPath tmpPath(&tmpDir, NULL);

	BMessage initMsg('TRIN');	
	initMsg.AddString("printer_file", tmpPath.Path());
	

	/* open the transport */
	BMessage reply;	
	fTransport = (*entry_func)(&initMsg, &reply);
	if(fTransport == NULL){
		unload_add_on(fTransportImage);
		(new BAlert("", "Error loading transport add-on! (null transport)", "Cancel"))->Go();
		return B_ERROR;		
	}		

	if(reply.what == 'canc') {
		CloseAndUnloadTransport();
		return B_USER_CANCELED;
	}

	return B_OK;
}

status_t
PSGenerator::CloseAndUnloadTransport()
{
	status_t rv;

	bool (*exit_func)();
	rv = get_image_symbol(fTransportImage, "exit_transport",
							B_SYMBOL_TYPE_TEXT, (void**)&exit_func);
	if(rv < 0) {
		unload_add_on(fTransportImage);
		(new BAlert("", "Couldn't find exit_transport function.", "Cancel"))->Go();
		return rv;
	}

	(*exit_func)();
	unload_add_on(fTransportImage);

	return B_OK;
}

status_t
PSGenerator::ReadPrinterAttr(const char *attr, char *buf, int32 len)
{
	ssize_t rv;
	rv = fPrinterEntry->ReadAttr(attr, B_STRING_TYPE, 0,
									buf, len);
	if(rv >= 0) {
		buf[rv] = '\0';
	}
	
	return rv;
}

status_t
PSGenerator::SendFileTo(const char *filename, BDataIO *output)
{
	BFile inFile(filename, B_READ_ONLY);
	if(inFile.InitCheck() != B_OK) {
		return B_ERROR;
	}

	int32 len;
	char buf[1024];
	while((len = inFile.Read(buf, sizeof(buf))) > 0) {
		output->Write(buf, len);
	}	
	
	return B_OK;	
}

void
PSGenerator::WriteJclHeader(BMessage *msg, BDataIO *transport)
{
	if(msg == NULL) { return; }

	PPD *ppd = PPD::Instance();
	char *orig_name;
	const char *str;	

	// Write out the JCL header...
	Invocation *inv = ppd->FindJCLHeaderInvocation();
	if(inv) {
		for(str = inv->GetFirstString(); str; str = inv->GetNextString()){
			transport->Write(str, strlen(str));
			delete str;
		}
	}

	// Now write out the JCL options...
	const char *name;
	char *dup_name;
	uint32 type;
	int32 count;
	char *keyword;
	char *option;
	char *save_ptr;
	for(int32 i=0;
		msg->GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK;
		i++){

		orig_name = strdup(name);
		dup_name = strdup(name);
					
		keyword = strtok_r(dup_name, "|", &save_ptr);
		option = strtok_r(NULL, "|", &save_ptr);
		
		if(strncmp(name, "JCL", strlen("JCL"))) {
			continue;
		}
	
		inv = ppd->FindInvocation(keyword, option);
		if(inv == NULL){
			free(dup_name);
			continue;
		}

		// write the invocation line out;
		for(str = inv->GetFirstString(); str; str = inv->GetNextString()){
			transport->Write(str, strlen(str));
			delete str;
		}
		
		msg->RemoveData(orig_name);

		free(orig_name);
		free(dup_name);
	}

	// now write out the JCL to PS command...
	inv = ppd->FindJCLPSInvocation();
	if(!inv) { return; }
	for(str = inv->GetFirstString(); str; str = inv->GetNextString()){
		transport->Write(str, strlen(str));
		delete str;
	}
	
}
//------------------------------------------------------------------
void
PSGenerator::WriteJclTrailer(BDataIO *transport)
{
	PPD *ppd = PPD::Instance();

	// Write out the JCL header...
	Invocation *inv = ppd->FindJCLFooterInvocation();
	if(!inv) { return; }
	const char *str;	
	for(str = inv->GetFirstString(); str; str = inv->GetNextString()){
		transport->Write(str, strlen(str));
		delete str;
	}
}

//------------------------------------------------------------------

void
PSGenerator::WritePpdFeatures(BMessage *msg, BDataIO *transport)
{
	if(msg == NULL) return; 

	PPD *ppd = PPD::Instance();
	const char *name;
	char *dup_name;
	uint32 type;
	int32 count;
	char *keyword;
	char *option;
	char *save_ptr;

	for(int32 i=0;
		msg->GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK;
		i++){
	
		if(!strncmp(name, "JCL", strlen("JCL"))) continue;

		dup_name = strdup(name);
		keyword = strtok_r(dup_name, "|", &save_ptr);
		option = strtok_r(NULL, "|", &save_ptr);

		Invocation *inv = ppd->FindInvocation(keyword, option);
		if(inv == NULL) continue;

		BString bstr;
		bstr = "\n%%Begin Feature: ";
		bstr << inv->Name() << " " << inv->Option() << "\n";
		transport->Write(bstr.String(), bstr.Length());

		const char *str;	
		for(str = inv->GetFirstString(); str; str = inv->GetNextString()) {
			transport->Write(str, strlen(str));
			delete str;
		}

		bstr = "\n%%End Feature: ";
		bstr << inv->Name() << " " << inv->Option() << "\n";
		transport->Write(bstr.String(), bstr.Length());			

		free (dup_name);
	}
}

//------------------------------------------------------------------

