/*	STXTHandler.cpp	*/
/*	Datatypes translator for the Targa format	*/
/*	Targa does 16bpp and 32bpp with alpha, too. We don't, at least not yet.	*/

#include <InterfaceDefs.h>
#include <Debug.h>
#include <Rect.h>
#include <View.h>
#include <StringView.h>
#include <string.h>
#include <stdio.h>
#include <Mime.h>
#include <unistd.h>
#include <stdlib.h>
#include <Font.h>
#include <TranslatorAddOn.h>
#include <TranslatorFormats.h>
#include <DataIO.h>
#include <byteorder.h>
#include <alloca.h>
#include <fs_attr.h>

#include <FindDirectory.h>
#include <Path.h>
#include <File.h>

char			translatorName[] = "StyledEdit Files";		//	required, C string, ex "Jon's Sound"
char			translatorInfo[100];
int32			translatorVersion = B_BEOS_VERSION;		//	required, integer, ex 100

namespace BPrivate {
class infoFiller {
public:
	infoFiller()
		{
			sprintf(translatorInfo, "StyledEdit file translator v%d.%d.%d, %s",
				translatorVersion>>8, (translatorVersion>>4)&0xf, translatorVersion&0xf,
				__DATE__);
		}
};
static infoFiller theFiller;
}

static	int	debug = 0;


#define STYLES_CHANGED '%stc'
#define SETTINGS_MAGIC 0x10202100

struct settings {
	uint32	magic;
	bool	text_styles;
	bool	unused[3];
};
static settings s_default_settings = {
	SETTINGS_MAGIC,
	true,
};
settings g_settings = s_default_settings;
static int32 settings_initialized;

static void load_settings()
{
	if (atomic_or(&settings_initialized, 1) != 0) {
		return;	/* already loaded */
	}
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK) return;
	path.Append("StyledTextTranslator");
	
	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() == B_OK) {
		settings new_settings(g_settings);
		file.Read(&new_settings, sizeof(new_settings));
		if (new_settings.magic == SETTINGS_MAGIC) {
			/* only update if valid */
			g_settings = new_settings;
		}
	}
}

static void save_settings()
{
	if (!(atomic_and(&settings_initialized, ~2) & 2)) {
		return;	/* not changed */
	}
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK) return;
	path.Append("StyledTextTranslator");
	
	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	status_t err = file.InitCheck();
	if (err == B_OK) {
		file.Write(&g_settings, sizeof(g_settings));
	}
}


/*
	uint32		type;						//	BeOS data type
	uint32		t_cls;						//	class of this format
	float		quality;					//	format quality 0.0-1.0
	float		capability;					//	handler capability 0.0-1.0
	char		MIME[B_MIME_TYPE_LENGTH+11];	//	MIME string
	char		formatName[B_MIME_TYPE_LENGTH+11];	//	only descriptive
*/
enum {
	kSTXT = B_STYLED_TEXT_FORMAT
};

translation_format inputFormats[] = {
	{ kSTXT, B_TRANSLATOR_TEXT, 0.5, 0.5, "text/x-vnd.Be-stxt", "Be styled text file" },
	{ B_TRANSLATOR_TEXT, B_TRANSLATOR_TEXT, 0.4, 0.6, "text/plain", "Plain text file" },
	{ 0, 0, 0, 0, 0, 0 }
};		//	optional (else Identify is always called)

translation_format outputFormats[] = {
	{ kSTXT, B_TRANSLATOR_TEXT, 0.5, 0.5, "text/x-vnd.Be-stxt", "Be styled text file" },
	{ B_TRANSLATOR_TEXT, B_TRANSLATOR_TEXT, 0.4, 0.6, "text/plain", "Plain text file" },
	{ 0, 0, 0, 0, 0, 0 }
};	//	optional (else Translate is called anyway)


	//	Return B_NO_TRANSLATOR if not handling this data.
	//	Even if inputFormats exists, may be called for data without hints
	//	If outType is not 0, must be able to export in wanted format

status_t
Identify(	//	required
	BPositionIO *		inSource,
	const translation_format *	inFormat,
	BMessage *			ioExtension,
	translator_info *			outInfo,
	uint32				outType)
{
	char data[512];
	if (outType == 0) {
		outType = B_TRANSLATOR_TEXT;
	}
	if (outType != B_TRANSLATOR_TEXT && outType != kSTXT) {
		return B_NO_TRANSLATOR;
	}
	int len = inSource->Read(data, sizeof(data));
	if (len < 0) return len;
	TranslatorStyledTextStreamHeader hdr;
	if (len > sizeof(hdr)) {
		memcpy(&hdr, data, sizeof(hdr));
		if (hdr.header.magic != B_HOST_TO_BENDIAN_INT32(TranslatorStyledTextStreamHeader::STREAM_HEADER_MAGIC)) {
			goto text;
		}
		int s = B_BENDIAN_TO_HOST_INT32(hdr.header.header_size);
		if (s < 0x10 || s > 0x100) {
			goto text;
		}
		/* so I think it's a styled text thingie */
		outInfo->type = inputFormats[0].type;
		outInfo->group = inputFormats[0].group;
		outInfo->quality = inputFormats[0].quality;
		outInfo->capability = inputFormats[0].capability;
		strcpy(outInfo->name, inputFormats[0].name);
		strcpy(outInfo->MIME, inputFormats[0].MIME);
		return B_OK;
	}
text:
	for (int ix=0; ix<len; ix++) {
		int s = (unsigned char)data[ix];
		if ((s < 8) || (s == 11) || ((s > 13) && (s < 32))) {
			return B_NO_TRANSLATOR;
		}
	}
	/* so we believe it's regular text */
	outInfo->type = inputFormats[1].type;
	outInfo->group = inputFormats[1].group;
	outInfo->quality = inputFormats[1].quality;
	outInfo->capability = inputFormats[1].capability;
	strcpy(outInfo->name, inputFormats[1].name);
	strcpy(outInfo->MIME, inputFormats[1].MIME);
	return B_OK;
}


static status_t
CopyStream(
	BPositionIO * input,
	BPositionIO * output,
	ssize_t amount)
{
	size_t len = 1024L*256L;
	bool f = true;
	void * memory = malloc(len);
	if (!memory) {
		len = 1024;
		memory = alloca(len);
		f = false;
	}
	status_t x;
	if ((amount >= 0) && (len > amount)) {
		len = amount;
	}
	while ((x = input->Read(memory, len)) > 0) {
		status_t err = output->Write(memory, x);
		if (err != x) {
			if (err >= 0) {
				return B_DEVICE_FULL;
			}
			return err;
		}
		amount -= len;
		if ((amount >= 0) && (len > amount)) {
			len = amount;
		}
	}
	if (f) {
		free(memory);
	}
	return x;
}


static status_t
ExtractTEXTFromSTXT(
	BPositionIO * input,
	BPositionIO * output)
{
	TranslatorStyledTextRecordHeader rhdr;
	status_t err;
	while ((err = input->Read(&rhdr, sizeof(rhdr))) == sizeof(rhdr)) {
		switch (B_BENDIAN_TO_HOST_INT32(rhdr.magic)) {
		case TranslatorStyledTextTextHeader::TEXT_HEADER_MAGIC:
			err = input->Seek(B_BENDIAN_TO_HOST_INT32(rhdr.header_size) -
				sizeof(rhdr), SEEK_CUR);
			if (err >= B_OK) {
				err = CopyStream(input, output, B_BENDIAN_TO_HOST_INT32(rhdr.data_size));
			}
			break;
		case TranslatorStyledTextStyleHeader::STYLE_HEADER_MAGIC:
			if (dynamic_cast<BFile *>(output) != NULL) {
				BFile * ptr = dynamic_cast<BFile *>(output);
				TranslatorStyledTextStyleHeader shdr;
				shdr.header = rhdr;
				err = input->Read(&(&shdr.header)[1], sizeof(shdr)-sizeof(rhdr));
				if (err != sizeof(shdr)-sizeof(rhdr)) {
					return (err < 0) ? err : B_IO_ERROR;
				}
				void * data = malloc(rhdr.data_size);
				if (data != NULL) {
					err = input->Read(data, rhdr.data_size);
					if (err != rhdr.data_size) {
						free(data);
						return (err < 0) ? err : B_IO_ERROR;
					}
					ptr->RemoveAttr("styles");
					ptr->WriteAttr("styles", B_RAW_TYPE, 0, data, rhdr.data_size);
					free(data);
				}
				else {
					input->Seek(rhdr.data_size, SEEK_SET);
					err = B_NO_MEMORY;
				}
			}
			break;
		default:	/* skip record and data */
			err = input->Seek(B_BENDIAN_TO_HOST_INT32(rhdr.header_size)+
				B_BENDIAN_TO_HOST_INT32(rhdr.data_size) - sizeof(rhdr), 
				SEEK_CUR);
			break;
		}
		if (err < B_OK) {
			break;
		}
	}
	return (err > 0) ? B_IO_ERROR : err;	/* 0 is OK */
}


static status_t
WriteSTXTFromTEXT(
	BPositionIO * input,
	BPositionIO * output)
{
	status_t err = input->Seek(0, SEEK_END);
	if (err < B_OK) return err;
	off_t size = input->Position();
	if (size > 1024L*1024L*16L) {	/* can't do more than 16 MB text files */
		return B_BAD_VALUE;
	}
	err = input->Seek(0, SEEK_SET);
	if (err < B_OK) return err;

	TranslatorStyledTextStreamHeader shdr;
	shdr.header.magic = B_HOST_TO_BENDIAN_INT32(TranslatorStyledTextStreamHeader::STREAM_HEADER_MAGIC);
	shdr.header.header_size = B_HOST_TO_BENDIAN_INT32(sizeof(shdr));
	shdr.header.data_size = B_HOST_TO_BENDIAN_INT32(0);
	shdr.version = B_HOST_TO_BENDIAN_INT32(100);
	err = output->Write(&shdr, sizeof(shdr));
	if (err != sizeof(shdr)) {
		return (err >= B_OK) ? B_DEVICE_FULL : err;
	}
	TranslatorStyledTextTextHeader thdr;
	thdr.header.magic = B_HOST_TO_BENDIAN_INT32(TranslatorStyledTextTextHeader::TEXT_HEADER_MAGIC);
	thdr.header.header_size = B_HOST_TO_BENDIAN_INT32(sizeof(thdr));
	thdr.header.data_size = B_HOST_TO_BENDIAN_INT32(size);
	thdr.charset = B_HOST_TO_BENDIAN_INT32(B_UNICODE_UTF8);
	err = output->Write(&thdr, sizeof(thdr));
	if (err != sizeof(thdr)) {
		return (err >= B_OK) ? B_DEVICE_FULL : err;
	}
	err = CopyStream(input, output, size);
	if (err < B_OK) {
		return err;
	}
	/* Now, maybe do styles from the file? */
	if (dynamic_cast<BFile *>(input) != NULL) {
		BFile * ptr = dynamic_cast<BFile *>(input);
		attr_info info;
		ASSERT(ptr != NULL);
		if (B_OK <= ptr->GetAttrInfo("styles", &info)) {
			void * data = malloc(info.size);
			if (data != NULL) {
				if (ptr->ReadAttr("styles", info.type, 0, data, info.size) >= B_OK) {
					TranslatorStyledTextStyleHeader shdr;
					shdr.header.magic = B_HOST_TO_BENDIAN_INT32(TranslatorStyledTextStyleHeader::STYLE_HEADER_MAGIC);
					shdr.header.header_size = B_HOST_TO_BENDIAN_INT32(sizeof(shdr));
					shdr.header.data_size = B_HOST_TO_BENDIAN_INT32(info.size);
					shdr.apply_offset = B_HOST_TO_BENDIAN_INT32(0);
					shdr.apply_length = B_HOST_TO_BENDIAN_INT32(size);
					err = output->Write(&shdr, sizeof(shdr));
					if (err != sizeof(shdr)) {
						free(data);
						return (err < 0) ? err : B_DEVICE_FULL;
					}
					err = output->Write(data, info.size);
					if (err != info.size) {
						free(data);
						return (err < 0) ? err : B_DEVICE_FULL;
					}
					if (debug) fprintf(stderr, "STXT styles size: %Ld\n", info.size);
				}
				free(data);
			}
		}
	}
	return B_OK;
}


status_t
Translate(	//	required
	BPositionIO *		inSource,
	const translator_info *	inInfo,
	BMessage *			ioExtension,
	uint32				outType,
	BPositionIO *		outDestination)
{
	load_settings();
	if (getenv("STXT_HANDLER_DEBUG") != NULL) debug = 1;
	if (outType == 0) {
		outType = B_TRANSLATOR_TEXT;
	}
	status_t err = B_OK;
	static translator_info info;
	if (inInfo == NULL) {
		err = Identify(inSource, NULL, NULL, &info, outType);
		if (err < B_OK) {
			if (debug) fprintf(stderr, "STXT: Identify() error %x\n", err);
			return err;
		}
		inSource->Seek(0, SEEK_SET);
	}
	if (outType == B_TRANSLATOR_TEXT) {
		if (inInfo->type == B_TRANSLATOR_TEXT) {
			return CopyStream(inSource, outDestination, -1);
		}
		return ExtractTEXTFromSTXT(inSource, outDestination);
	}
	else if (outType == kSTXT) {
		if (inInfo->type == kSTXT) {
			return CopyStream(inSource, outDestination, -1);
		}
		return WriteSTXTFromTEXT(inSource, outDestination);
	}
	else {
		if (debug) fprintf(stderr, "STXT: Unknown output format requested\n");
		return B_NO_TRANSLATOR;
	}
	return B_OK;
}


	//	right now, there is nothing to configure for this handler

	//	The view will get resized to what the parent thinks is 
	//	reasonable. However, it will still receive MouseDowns etc.
	//	Your view should change settings in the translator immediately, 
	//	taking care not to change parameters for a translation that is 
	//	currently running. Typically, you'll have a global struct for 
	//	settings that is atomically copied into the translator function 
	//	as a local when translation starts.
	//	Store your settings wherever you feel like it.

#include <CheckBox.h>

class STXTView : public BView
{
public:
		STXTView(const BRect & frame, const char * name, uint32 resize, uint32 flags) :
			BView(frame, name, resize, flags)
		{
			fTextStyles = NULL;
		}
		void AllAttached()
		{
			BView::AllAttached();
			load_settings();
			fTextStyles->SetTarget(this);
			fTextStyles->SetValue(g_settings.text_styles);
		}
		void DetachedFromWindow()
		{
			save_settings();
			BView::DetachedFromWindow();
		}
		void MessageReceived(BMessage * message)
		{
			if (message->what == STYLES_CHANGED) {
				g_settings.text_styles = fTextStyles->Value();
				atomic_or(&settings_initialized, 2);	/* mark change */
			}
			else {
				BView::MessageReceived(message);
			}
		}
		BCheckBox * fTextStyles;
};


status_t
MakeConfig(	//	optional
	BMessage *			ioExtension,
	BView * *			outView,
	BRect *				outExtent)
{
	//	ignore config

	(*outExtent).Set(0,0,239,239);
	(*outView) = new STXTView(*outExtent, "STXTTranslator Settings", B_FOLLOW_NONE, B_WILL_DRAW);
	(*outView)->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	font_height fh;
	be_bold_font->GetHeight(&fh);
	BRect r(10, 15, 10 + be_bold_font->StringWidth(translatorName), 15 + fh.ascent + fh.descent);

	BStringView *str = new BStringView(r, "title", translatorName);
	str->SetFont(be_bold_font);
	(*outView)->AddChild(str);

	char versStr[100];
	sprintf(versStr, "v%d.%d.%d %s", translatorVersion>>8, (translatorVersion>>4)&0xf,
		translatorVersion&0xf, __DATE__);
	r.top = r.bottom + 20;
	be_plain_font->GetHeight(&fh);
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(versStr);
	
	str = new BStringView(r, "info", versStr);
	str->SetFont(be_plain_font);
	(*outView)->AddChild(str);

	const char *copyright_string = "© 1998-1999 Be Incorporated";
	r.top = r.bottom + 10;
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(copyright_string);

	str = new BStringView(r, "author", copyright_string);
	str->SetFont(be_plain_font);
	(*outView)->AddChild(str);

	r.top = r.bottom + 10;
	r.bottom = r.top + 10;
	r.right = r.left + be_plain_font->StringWidth("Write TEXT Styles") + 20;

	(*(STXTView **)outView)->fTextStyles = new BCheckBox(r, "text_styles", "Write TEXT Styles", 
		new BMessage(STYLES_CHANGED));
	(*outView)->AddChild((*(STXTView **)outView)->fTextStyles);

	return B_OK;
}


	//	Copy your current settings to a BMessage that may be passed 
	//	to DATATranslate at some later time when the user wants to 
	//	use whatever settings you're using right now.

status_t
GetConfigMessage(	//	optional
	BMessage *			ioExtension)
{
	//	ignore config
	return B_OK;
}

