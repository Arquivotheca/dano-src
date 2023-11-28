/* jpeg_handler.cpp
 *
 * Translator for the JPEG file format
 *
 */

#include <stdio.h>
#include <setjmp.h>
#include <unistd.h>

#include <ByteOrder.h>
#include <TranslatorAddOn.h>
#include <TranslatorFormats.h>
#include <DataIO.h>
#include <Node.h>
#include <Message.h>

#include "libjpeg_glue.h"
#include "settings.h"

#define zprintf printf

#define JPEG_TYPE		'JPEG'


typedef struct {
	struct jpeg_compress_struct pub;
	TranslatorBitmap info;
} bitmap_info_struct;

static inline bool
can_handle(uint32 in_type, uint32 out_type)
{
	switch(in_type) {
	case B_TRANSLATOR_BITMAP:
		return (out_type == 0 || out_type == B_TRANSLATOR_BITMAP || out_type == JPEG_TYPE);
	case JPEG_TYPE:
		return (out_type == 0
				|| out_type == B_TRANSLATOR_BITMAP
				|| out_type == JPEG_TYPE);	
	}
	
	return false;
}

static bool
validate_jpeg(BPositionIO *in_data)
{
	struct jpeg_decompress_struct cinfo;
	be_error_mgr err;

	/* initialize the barbaric error handler */
	cinfo.err = be_std_error(&err.pub);
	if (setjmp(err.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		return false;
	}

	/* set up */
	jpeg_create_decompress(&cinfo);
	be_position_io_src(&cinfo, in_data);

	/* read the header;  no need to check for errors, this will
	 * automagically longjmp back to the error handler if it fails,
	 * at which point we'll return B_NO_TRANSLATOR.  it sucks, but
	 * it works and it's better than having to implement JPEG.
	 */
	jpeg_read_header(&cinfo, TRUE);
	
	/* clean up */
	jpeg_destroy_decompress(&cinfo);
	
	return true;
}

static bool
guess_type(BPositionIO *in_data, uint32 *type_buf)
{
	union {
		unsigned char bytes[16];
	} buffer;
	
	if (in_data->Read(&buffer, sizeof(buffer)) != sizeof(buffer))
		return false;
	
	if ((*(TranslatorBitmap *)&buffer).magic
			== B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP)) {
		*type_buf = B_TRANSLATOR_BITMAP;
		return true;
	}
	
	if (buffer.bytes[0] == 0xff && buffer.bytes[1] == 0xd8) {
		*type_buf = JPEG_TYPE;
		return true;
	}

	return false;
}

status_t
Identify(BPositionIO *in_data, const translation_format *in_fmt,
			BMessage *config_msg, translator_info *out_info, uint32 out_type)
{
	uint32 in_type;
	int guessed = 0;
	
	if (out_type == 0) out_type = B_TRANSLATOR_BITMAP;
	
	if (in_fmt == NULL) {
		if (guess_type(in_data, &in_type))
			guessed = 1;
		else
			return B_NO_TRANSLATOR;
	} else {
		in_type = in_fmt->type;
	}

	/* make sure we can handle the type */
	if (!can_handle(in_type, out_type))
			return B_NO_TRANSLATOR;

	/* make sure the JPEG header is valid */
	if (in_type == JPEG_TYPE && !guessed) {
		if (!validate_jpeg(in_data))
			return B_NO_TRANSLATOR;
	}
	
	/* find the right output format for our specified output type */
	out_info->type = in_type;
	int32 i = 0;
	while (inputFormats[i].type != 0) {
		if (inputFormats[i].type == out_info->type)
			break;
		i++;
	}

	/* sanity check */
	if (inputFormats[i].type == 0)
		return B_NO_TRANSLATOR;
		
	/* fill out translator_info and return EHH_OK */
	out_info->translator	= 0;
	out_info->group			= inputFormats[i].group;
	out_info->quality		= inputFormats[i].quality;
	out_info->capability	= inputFormats[i].capability;
	strcpy(out_info->name, inputFormats[i].name);
	strcpy(out_info->MIME, inputFormats[i].MIME);

	return B_OK;
}


size_t
bytes_per_row(struct jpeg_compress_struct *cinfo)
{
	TranslatorBitmap *bmp = (TranslatorBitmap*)&cinfo[1];
	if(bmp->magic != B_TRANSLATOR_BITMAP) {
		zprintf("JPEG:bytes_per_row: where's the magic, baby?\n");
		return 0;
	}

	return bmp->rowBytes;
}


static int
expand_scanline(struct jpeg_decompress_struct *cinfo, JSAMPROW in, JSAMPROW out)
{
	int ctr, ctr3, ctr4;
	
	switch (cinfo->output_components) {
	case 3:
		for (ctr = 0; ctr < cinfo->output_width; ctr++) {
			ctr3 = ctr*3;
			ctr4 = ctr*4;

			out[ctr4] = in[ctr3+2];
			out[ctr4+1] = in[ctr3+1];
			out[ctr4+2] = in[ctr3];
			out[ctr4+3] = 255;
		}
		break;
	
	case 1:
		for (ctr = 0; ctr < cinfo->output_width; ctr++) {
			ctr4 = ctr*4;
			
			out[ctr4] = in[ctr];
			out[ctr4+1] = in[ctr];
			out[ctr4+2] = in[ctr];
			out[ctr4+3] = 255;
		}
		break;

	default:
		zprintf("!!! expand_scanline called with output_components == %d\n",
				cinfo->output_components);
		return 0;
	}
		
	return cinfo->output_width * 4;
}

static void
collapse_scanline(struct jpeg_compress_struct *cinfo, JSAMPROW in, JSAMPROW out)
{
	int ctr, ctr2, ctr3, ctr4;
	uint16 val;

	TranslatorBitmap *bmp = (TranslatorBitmap*)&cinfo[1];
	if(bmp->magic != B_TRANSLATOR_BITMAP) {
		zprintf("JPEG:collapse_scanline: where's the magic, baby?\n");
		return;
	}

	switch(bmp->colors) {
	case B_CMAP8: {
		// in: [Y]
		// out: [R][G][B]
		const color_map *map = system_colors();
		rgb_color c;
		for (ctr = 0; ctr < cinfo->image_width; ctr++) {
			ctr3 = ctr*3;

			c = map->color_list[in[ctr]];
			out[ctr3] = c.red;
			out[ctr3+1] = c.green;
			out[ctr3+2] = c.blue;
		}
	}	break;
	case B_RGBA15:
		// in: [GB][ARG]	(arrrgh.)
	case B_RGB15:
		// in: [GB][-RG]
		// out: [R][G][B]
		for (ctr = 0; ctr < cinfo->image_width; ctr++) {
			ctr2 = ctr*2;
			ctr3 = ctr*3;

			val = in[ctr2]+(in[ctr2+1]<<8);
			out[ctr3] = ((val&0x7c00)>>7)|((val&0x7c00)>>12);
			out[ctr3+1] = ((val&0x3e0)>>2)|((val&0x3e0)>>7);
			out[ctr3+2] = ((val&0x1f)<<3)|((val&0x1f)>>2);
		}
		break;
	case B_RGB16:
		// in: [GB][RG]
		// out: [R][G][B]
		for (ctr = 0; ctr < cinfo->image_width; ctr++) {
			ctr2 = ctr*2;
			ctr3 = ctr*3;

			val = in[ctr2]+(in[ctr2+1]<<8);
			out[ctr3] = ((val&0xf800)>>8)|((val&0xf800)>>13);
			out[ctr3+1] = ((val&0x7e0)>>3)|((val&0x7e0)>>9);
			out[ctr3+2] = ((val&0x1f)<<3)|((val&0x1f)>>2);
		}
		break;
	case B_RGB32:
	case B_RGBA32:
		// in: [B][G][R][A]
		// out: [R][G][B]
		for (ctr = 0; ctr < cinfo->image_width; ctr++) {
			ctr3 = ctr*3;
			ctr4 = ctr*4;

			out[ctr3] = in[ctr4+2];
			out[ctr3+1] = in[ctr4+1];
			out[ctr3+2] = in[ctr4];
		}
		break;
	default:
		zprintf("JPEG:collapse_scanline: can't translate from this "
			"colorspace (0x%x)\n", bmp->colors);
	}
}


static status_t
write_bitmap_header(struct jpeg_decompress_struct *cinfo, BPositionIO *out_data)
{
	TranslatorBitmap bm_header;

	bm_header.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	bm_header.bounds.left = B_HOST_TO_BENDIAN_FLOAT(0.0);
	bm_header.bounds.right = B_HOST_TO_BENDIAN_FLOAT((float)cinfo->output_width-1);
	bm_header.bounds.top = B_HOST_TO_BENDIAN_FLOAT(0.0);
	bm_header.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT((float)cinfo->output_height-1);
	bm_header.rowBytes = B_HOST_TO_BENDIAN_INT32(cinfo->output_width*4);
	bm_header.colors = (color_space)B_BENDIAN_TO_HOST_INT32(B_RGB32);
	bm_header.dataSize = B_BENDIAN_TO_HOST_INT32(cinfo->output_width*4*cinfo->output_height);

	if (out_data->Write(&bm_header, sizeof(bm_header)) != sizeof(bm_header))
		return B_ERROR;
	
	return B_OK;
}

static status_t
read_bitmap_header(struct jpeg_compress_struct *cinfo, BPositionIO *in_data)
{
	TranslatorBitmap *bmp = (TranslatorBitmap*)&cinfo[1];

	if (in_data->Read(bmp, sizeof(*bmp)) != sizeof(*bmp))
		return B_ERROR;

	bmp->magic = B_BENDIAN_TO_HOST_INT32(bmp->magic);

	if(bmp->magic != B_TRANSLATOR_BITMAP) {
		zprintf("JPEG:read_bitmap_header: mad bitmap magic\n");
		return B_ERROR;
	}

	bmp->bounds.left = B_BENDIAN_TO_HOST_FLOAT(bmp->bounds.left);
	bmp->bounds.right = B_BENDIAN_TO_HOST_FLOAT(bmp->bounds.right);
	bmp->bounds.top = B_BENDIAN_TO_HOST_FLOAT(bmp->bounds.top);
	bmp->bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(bmp->bounds.bottom);
	bmp->rowBytes = B_BENDIAN_TO_HOST_INT32(bmp->rowBytes);
	bmp->colors = (color_space)B_BENDIAN_TO_HOST_INT32(bmp->colors);
	bmp->dataSize = B_BENDIAN_TO_HOST_INT32(bmp->dataSize);

	cinfo->image_width = (JDIMENSION)
		(bmp->bounds.right - bmp->bounds.left + 1);
	cinfo->image_height = (JDIMENSION)
		(bmp->bounds.bottom - bmp->bounds.top + 1);
	cinfo->input_components = 3;
	cinfo->in_color_space = JCS_RGB;

	return B_OK;
}

static status_t
jpeg_to_bitmap(BPositionIO *in_data, BPositionIO *out_data,
				const translator_info *in_info)
{
	struct jpeg_decompress_struct cinfo;
	be_error_mgr err;
	JSAMPARRAY in_buffer, out_buffer;
	int row_stride;
	int n;
	status_t status = B_OK;
	
	/* initialize barbaric error handler */
	cinfo.err = be_std_error(&err.pub);
	if (setjmp(err.setjmp_buffer))
		goto bail;
	
	/* set up */
	jpeg_create_decompress(&cinfo);
	be_position_io_src(&cinfo, in_data);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	/* let the reader know the image details */
	if (write_bitmap_header(&cinfo, out_data) != B_OK)
		goto bail;
	
	/* decompress the image */
	row_stride = cinfo.output_width * cinfo.output_components;
	in_buffer = (*cinfo.mem->alloc_sarray)
				((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
	out_buffer = (*cinfo.mem->alloc_sarray)
				((j_common_ptr) &cinfo, JPOOL_IMAGE, cinfo.output_width * 4, 1);
	while (cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, in_buffer, 1);
		n = expand_scanline(&cinfo, in_buffer[0], out_buffer[0]);
//		fprintf(stderr, "n = %d: %x %x %x %x %x\n", n, out_buffer[0][0], 
//			out_buffer[0][1], out_buffer[0][2], out_buffer[0][3], out_buffer[0][4]);
//		fprintf(stderr, "At pos %Ld\n", out_data->Position());
		status = out_data->Write(out_buffer[0], n);
		if (status < B_OK) {
			fprintf(stderr, "JPEG: error writing: %x (%s)\n", status, strerror(status));
			break;
		}
	}
	if (status > B_OK) status = B_OK;

	/* clean up */
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return status;	

bail:
	jpeg_destroy_decompress(&cinfo);
	return B_NO_TRANSLATOR;
}	


static status_t
bitmap_to_jpeg(BPositionIO *in_data, BPositionIO *out_data,
				const translator_info *in_info)
{
	bitmap_info_struct binfo;
	struct jpeg_compress_struct *cinfo = &binfo.pub;
	be_error_mgr err;
	JSAMPARRAY in_buffer, out_buffer;
	size_t row_stride;
	int n;
	const char *sq;
	int q = 0;
	
	/* set_ashamed(TRUE) */
	cinfo->err = be_std_error(&err.pub);
	if (setjmp(err.setjmp_buffer))
		goto bail;
		
	/* set up */
	jpeg_create_compress(cinfo);
	be_position_io_dst(cinfo, out_data);

	/* get the image width/height/etc */
	if (read_bitmap_header(cinfo, in_data) != B_OK)
		goto bail;

	jpeg_set_defaults(cinfo);

	sq = find_setting(OUTPUT_QUALITY_SETTING);
	if (sq != NULL)
		q = atoi(sq);

	if (q < 1 || q > 100)
		q = OUTPUT_QUALITY_DEFAULT;
	
	jpeg_set_quality(cinfo, q, TRUE);
	jpeg_start_compress(cinfo, TRUE);

	row_stride = bytes_per_row(cinfo);
	in_buffer = (*cinfo->mem->alloc_sarray)
				((j_common_ptr) cinfo, JPOOL_IMAGE, row_stride, 1);
	out_buffer = (*cinfo->mem->alloc_sarray)
				((j_common_ptr) cinfo, JPOOL_IMAGE,
					cinfo->image_width * cinfo->input_components, 1);
	while (cinfo->next_scanline < cinfo->image_height) {
		if (in_data->Read(in_buffer[0], row_stride) <= 0)
			goto bail;		/* is this the right thing to do? */
		collapse_scanline(cinfo, in_buffer[0], out_buffer[0]);
		jpeg_write_scanlines(cinfo, out_buffer, 1);
	}	

	jpeg_finish_compress(cinfo);
	jpeg_destroy_compress(cinfo);
	
	return B_OK;
	
bail:
	jpeg_destroy_compress(cinfo);
	return B_NO_TRANSLATOR;
}

static status_t
jpeg_to_jpeg(BPositionIO *in_data, BPositionIO *out_data,
				const translator_info *in_info)
{
	size_t n;
	char buffer[4096];
	
	/* just a copy loop */
	do {
		n = in_data->Read(buffer, sizeof(buffer));
		if (n < 0)
			return B_NO_TRANSLATOR;
		if (out_data->Write(buffer, n) != n)
			return B_NO_TRANSLATOR;
	} while (n > 0);

	return B_OK;
}

// The user has requested the same format for input and output, so just copy
status_t CopyInToOut(BPositionIO *in, BPositionIO *out) {
	int block_size = 65536;
	void *buffer = malloc(block_size);
	char temp[1024];
	if (buffer == NULL) {
		buffer = temp;
		block_size = 1024;
	}
	status_t err = B_OK;
	
	// Read until end of file or error
	while (1) {
		ssize_t to_read = block_size;
		err = in->Read(buffer, to_read);
		// Explicit check for EOF
		if (err == -1) {
			if (buffer != temp) free(buffer);
			return B_OK;
		}
		if (err <= B_OK) break;
		to_read = err;
		err = out->Write(buffer, to_read);
		if (err != to_read) if (err >= 0) err = B_DEVICE_FULL;
		if (err < B_OK) break;
	}
	
	if (buffer != temp) free(buffer);
	return (err >= 0) ? B_OK : err;
}

status_t
Translate(BPositionIO *in_data, const translator_info *in_info,
			BMessage *config_msg, uint32 out_type, BPositionIO *out_data)
{
	if (in_info == NULL)
		return B_NO_TRANSLATOR;
	
	// The spec requires this
	if (out_type == 0) out_type = B_TRANSLATOR_BITMAP;
	
	if (!can_handle(in_info->type, out_type))
		return B_NO_TRANSLATOR;
	
	if (in_info->type == JPEG_TYPE && out_type == B_TRANSLATOR_BITMAP)
		return jpeg_to_bitmap(in_data, out_data, in_info);
	if (in_info->type == B_TRANSLATOR_BITMAP && out_type == JPEG_TYPE)
		return bitmap_to_jpeg(in_data, out_data, in_info);
	if (in_info->type == JPEG_TYPE && out_type == JPEG_TYPE)
		return jpeg_to_jpeg(in_data, out_data, in_info);
	if (in_info->type == B_TRANSLATOR_BITMAP && out_type == B_TRANSLATOR_BITMAP)
		return CopyInToOut(in_data, out_data);

	return B_NO_TRANSLATOR;
}

char			translatorName[]	= "JPEG Images";
char			translatorInfo[100];
int32			translatorVersion = B_BEOS_VERSION;		//	required, integer, ex 100

namespace BPrivate {
class infoFiller {
public:
	infoFiller()
		{
			sprintf(translatorInfo, "JPEG image translator v%d.%d.%d, %s",
				translatorVersion>>8, (translatorVersion>>4)&0xf, translatorVersion&0xf,
				__DATE__);
		}
};
static infoFiller theFiller;
}

translation_format inputFormats[] = {
	{ JPEG_TYPE, B_TRANSLATOR_BITMAP, 1.0, 1.0, "image/jpeg", "JPEG image" },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 1.0, 1.0, "image/x-be-bitmap", "Be Bitmap image" },
	{ 0, 0, 0, 0, 0, 0 }
};
	
translation_format outputFormats[] = {
	{ JPEG_TYPE, B_TRANSLATOR_BITMAP, 1.0, 1.0, "image/jpeg", "JPEG image" },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 1.0, 1.0, "image/x-be-bitmap", "Be Bitmap image" },
	{ 0, 0, 0, 0, 0, 0 }
};

