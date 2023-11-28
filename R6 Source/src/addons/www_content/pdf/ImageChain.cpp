#include <Region.h>
#include <Bitmap.h>
#include <DataIO.h>

#include "Object2.h"
#include "Transform2D.h"
#include "ImageChain.h"
#include "CMYKtoRGB.h"
#include "GreyToRGB.h"
#include "LUTPusher.h"
#include "SampleExpander.h"
#include "SimpleImage.h"
#include "StencilImage.h"

static
void
IndexedCMYKtoRGB(uint8 *lut_dst, const uint8 *lut_src, int32 highval)
{
	for (int i = 0; i <= highval; i++)
	{
		uint8 black = lut_src[3];
		uint16 comp;
		comp = (uint16)*lut_src++ + black;	// C
		if (comp > 255) comp = 255;
		*lut_dst++ = 255 - comp;
		comp = (uint16)*lut_src++ + black;	// M
		if (comp > 255) comp = 255;
		*lut_dst++ = 255 - comp;
		comp = (uint16)*lut_src++ + black;	// Y
		if (comp > 255) comp = 255;
		*lut_dst++ = 255 - comp;
		lut_src++;	// drop the K
	}
}

static
Pusher *
MakeIndexedChain(Pusher *sink, object_array *colorSpace, uint32 samplesPerLine)
{
	// an indexed color space is [ /Indexed base highval lookup ]
	// the lookup table is in the 4th slot
	PDFObject *lut_obj = (*colorSpace)[3];
	const uint8 *lut_data;
	if (lut_obj->IsString())
	{
		lut_data = lut_obj->Contents();
	}
	else if (lut_obj->IsDictionary())
	{
		BMallocIO *mio = dynamic_cast<BMallocIO *>(lut_obj->RealizeStream());
		ASSERT(mio != 0);
		lut_data = (uint8 *)mio->Buffer();
		//printf("lut_data has %ld bytes\n", mio->BufferLength());
	}
	else
	{
		// oops
		return 0;
	}
	// build a lut sized for the # of elements
	uint8 *lut = new uint8[256 * 3];
	// get the high value
	PDFObject *tmp = (*colorSpace)[2];
	int32 highval = tmp->GetInt32();
	tmp = (*colorSpace)[1];
	const char *base_name;
	if (tmp->IsName())
	{
		base_name = tmp->GetCharPtr();
		if (base_name == PDFAtom.DeviceRGB)
		{
			// transfer highval * 3 bytes of data fromthe lut_data to the lut
			memcpy(lut, lut_data, (highval+1) * 3);
		}
		else if (base_name == PDFAtom.DeviceGray)
		{
			const uint8 *lut_src = lut_data;
			uint8 *lut_dst = lut;
			for (int i = 0; i <= highval; i++)
			{
				*lut_dst++ = *lut_src;
				*lut_dst++ = *lut_src;
				*lut_dst++ = *lut_src++;
			}
		}
		else if (base_name == PDFAtom.DeviceCMYK)
		{
			IndexedCMYKtoRGB(lut, lut_data, highval);
		}
		else printf("UNSUPPORTED indexed colorspace name %s\n", base_name);
	}
	else if (tmp->IsArray())
	{
		object_array *base_array = tmp->Array();
		base_name = ((*base_array)[0])->GetCharPtr();
		// create the lut from the "base" colorspace
		if (base_name == PDFAtom.CalRGB)
		{
			PDFObject *dict = (*base_array)[1];
			// get the gamma, if there is one
			PDFObject *ga = dict->Find(PDFAtom.Gamma);
			if (ga->IsArray())
			{
				object_array *garray = ga->Array();
				double rg = ((*garray)[0])->GetFloat() / (2.8);
				double gg = ((*garray)[1])->GetFloat() / (2.8);
				double bg = ((*garray)[2])->GetFloat() / (2.8);
				double val;
				const uint8 *lut_src = lut_data;
				uint8 *lut_dst = lut;
				for (int i = 0; i <= highval; i++)
				{
					val = *lut_src++ / (double)highval;
					val = pow(val, rg) * 255;
					if (val > 255) val = 255;
					*lut_dst++ = (uint8)val;
					//printf("%3d r: %d, ", i, (int)val);
					val = *lut_src++ / (double)highval;
					val = pow(val, gg) * 255;
					if (val > 255) val = 255;
					*lut_dst++ = (uint8)val;
					//printf("g: %d, ", (int)val);
					val = *lut_src++ / (double)highval;
					val = pow(val, bg) * 255;
					if (val > 255) val = 255;
					*lut_dst++ = (uint8)val;
					//printf("b: %d\n", (int)val);
				}
			}
			else
			{
				// transfer highval * 3 bytes of data from the lut_data to the lut
				memcpy(lut, lut_data, (highval+1) * 3);
			}
		}
		else if (base_name == PDFAtom.CalGray)
		{
			PDFObject *dict = (*base_array)[1];
			// get the gamma, if there is one
			PDFObject *ga = dict->Find(PDFAtom.Gamma);
			double gg = 1.0;
			if (ga->IsNumber()) gg = (double)ga->GetFloat() / (2.8);
			double val;
			const uint8 *lut_src = lut_data;
			uint8 *lut_dst = lut;
			for (int i = 0; i <= highval; i++)
			{
				val = *lut_src++ / (double)highval;
				val = pow(val, gg) * 255;
				if (val > 255) val = 255;
				*lut_dst++ = (uint8)val;
				*lut_dst++ = (uint8)val;
				*lut_dst++ = (uint8)val;
			}
		}
		else if (base_name == PDFAtom.CalCMYK)
		{
			// PDFSPEC 1.3 says treat this as DeviceCMYK
			IndexedCMYKtoRGB(lut, lut_data, highval);
		}
		else printf("UNSUPPORTED indexed colorspace in array %s\n", base_name);
#if 0
		else if (base_name == PDFAtom.Lab)
		{
		}
		else if (base_name == PDFAtom.ICCBased)
		{
		}
		else if (base_name == PDFAtom.Separation)
		{
		}
		else if (base_name == PDFAtom.DeviceN)
		{
		}
#endif
	}
	// throw away the lut data in the dictionary
	if (lut_obj->IsDictionary()) lut_obj->Erase(PDFAtom.__stream_data__);
	return new LUTPusher(sink, true, samplesPerLine, 3, lut);
}


Pusher *
MakeImageChain(PDFObject *image, BBitmap *bitmap, const BRegion &clip, const Transform2D &t, uint8 *aColor)
{
	Pusher *aPusher = 0;
	float width, height;
	bool imageMask;
	uint32 bitsPerComponent;
	PDFObject *colorSpace;

	// make sure this components are resolved
	image->ResolveArrayOrDictionary();

	// get image width and height
	PDFObject *oo = image->Find(PDFAtom.Width);
	if (!oo) goto exit1;
	width = oo->GetFloat();

	oo = image->Find(PDFAtom.Height);
	if (!oo) goto exit1;
	height = oo->GetFloat();

	oo = image->Find(PDFAtom.ImageMask);
	imageMask = oo ? oo->GetBool() : false;

	if (imageMask)
	{
		bitsPerComponent = 1;
		Pusher *p = new StencilImage(bitmap, clip, t, (uint32)width, (uint32)height, aColor);
		if (!p) goto exit2;
		aPusher = p;
		p = new SampleExpander(aPusher, (uint32)width, bitsPerComponent, false);
		if (!p) goto exit2;
		aPusher = p;
		goto exit1;
	}
	else
	{
		oo = image->Find(PDFAtom.BitsPerComponent);
		if (!oo) goto exit1;
		bitsPerComponent = (uint32)oo->GetInt32();
		bool scaleData = bitsPerComponent != 8;

		colorSpace = image->Find(PDFAtom.ColorSpace);
		if (colorSpace->IsName())
		{
			PDFObject *tmp = PDFObject::makeArray();
			colorSpace->Acquire();
			tmp->push_back(colorSpace);
			colorSpace = tmp;
			image->Assign(PDFObject::makeName(PDFAtom.ColorSpace), colorSpace);
		}
		if (colorSpace->IsArray())
		{
			// [ /spaceName ... ]
			colorSpace->ResolveArrayOrDictionary();
			object_array *oa = colorSpace->Array();
			PDFObject *base = (*oa)[0];
			const char *base_name = base->GetCharPtr();
			Pusher *p = 0;

			// build the sink of the rendering push chain
			aPusher = new SimpleImage(bitmap, clip, t, (uint32)width, (uint32)height);
			if (!aPusher) goto exit1;

			if ((base_name == PDFAtom.CalRGB) || (base_name == PDFAtom.DeviceRGB))
			{
				// shouldn't have to do anything, except for the CalRGB?
			}
			else if ((base_name == PDFAtom.CalGray) || (base_name == PDFAtom.DeviceGray))
			{
				p = new GreyToRGB(aPusher, (uint32)width);
				if (!p) goto exit2;
				aPusher = p;
				//printf("Made a GreyToRGB() for /DeviceGray\n");
			}
			else if ((base_name == PDFAtom.CalCMYK) || (base_name == PDFAtom.DeviceCMYK))
			{
				p = new CMYKtoRGB(aPusher, (uint32)width);
				if (!p) goto exit2;
				aPusher = p;
			}
			else if (base_name == PDFAtom.Lab)
			{
			}
			else if (base_name == PDFAtom.ICCBased)
			{
				// second element must contain a stream dictionary
				PDFObject *aDict = (*oa)[1];
				aDict->ResolveArrayOrDictionary();
				if (aDict->IsDictionary())
				{
					// search for required /N key and get # of components
					PDFObject *N = aDict->Find(PDFAtom.N);
					PDFObject *alternate = aDict->Find(PDFAtom.Alternate);
					// search for optional /Alternate key and grab color space
					if (alternate)
					{
						const char *alt_name = alternate->GetCharPtr();
						if (alt_name == PDFAtom.DeviceGray)
						{
							// needs testing
							p = new GreyToRGB(aPusher, (uint32)width);
							if (!p) goto exit2;
							aPusher = p;
						}
						else if (alt_name == PDFAtom.DeviceRGB)
						{
							// pass through, in theory (needs testing)
						}
						else if (alt_name == PDFAtom.DeviceCMYK)
						{
							p = new CMYKtoRGB(aPusher, (uint32)width);
							if (!p) goto exit2;
							aPusher = p;
						}
						else goto exit2;
					}
					else
					{
						switch (N->GetInt32())
						{
							case 1:	// grey - needs testing
								p = new GreyToRGB(aPusher, (uint32)width);
								if (!p) goto exit2;
								aPusher = p;
								break;
							case 3:	// RGB - just pass through, in theory (needs testing)
								break;
							case 4:
								p = new CMYKtoRGB(aPusher, (uint32)width);
								if (!p) goto exit2;
								aPusher = p;
								break;
							default:
								goto exit2;
						}
					}
				}
				// for now, if we don't find an alternate we can handle, bail
				// eventually, we should try to decode the ICC info
				else goto exit2;
			}
			else if (base_name == PDFAtom.Separation)
			{
			}
			else if (base_name == PDFAtom.DeviceN)
			{
			}
			else if (base_name == PDFAtom.Indexed)
			{
				scaleData = false;
				p = MakeIndexedChain(aPusher, oa, (uint32)width);
				if (!p) goto exit2;
				aPusher = p;
			}
		}
		// expand non 8-bit per component data to 8 bits
		if (aPusher && (bitsPerComponent != 8))
		{
			Pusher *p = new SampleExpander(aPusher, (uint32)width, bitsPerComponent, scaleData);
			if (!p) goto exit2;
			aPusher = p;
		}
		goto exit1;
	}
exit2:
	delete aPusher;
	aPusher = 0;
exit1:
	//printf("MakeImageChain returns %p\n", aPusher);
	return aPusher;
}

