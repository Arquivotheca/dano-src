#include "IconEditor.h"
#include "BitmapEditor.h"
#include "BitmapControls.h"
#include "RawDataWindow.h"

#include "utils.h"

#include <ResourceEditor.h>
#include <ResourceParser.h>

#include <experimental/BitmapTools.h>

#include <Clipboard.h>

#include <Bitmap.h>
#include <Menu.h>
#include <MenuItem.h>
#include <TextControl.h>
#include <View.h>

#include <Mime.h>

#include <Autolock.h>
#include <Debug.h>
#include <TypeConstants.h>

#include <BitmapStream.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>

#include <stdio.h>

static void init_translator_info(translator_info* info)
{
	info->type = 0;
	info->translator = 0;
	info->group = B_TRANSLATOR_BITMAP;
	info->quality = info->capability = 0;
	info->name[0] = 0;
	info->MIME[0] = 0;
}

static status_t GetBufferBitmap(BPositionIO& buf,
								const char* mime_type = 0,
								type_code typecode = 0,
								translator_info* out_info = 0,
								TranslatorBitmap* out_header = 0,
								BMessage* out_io_extension = 0,
								BBitmap** out_bitmap = 0)
{
	if( out_info ) init_translator_info(out_info);
	if( out_header ) memset(out_header, sizeof(out_header), 0);
	if( out_io_extension ) *out_io_extension = BMessage();
	if( out_bitmap ) *out_bitmap = 0;
	
	translator_info info;
	ssize_t err;
	err = BTranslatorRoster::Default()->Identify(&buf, NULL, &info, typecode,
												 mime_type, B_TRANSLATOR_BITMAP);
	if( err >= B_OK ) {
		if( out_info ) *out_info = info;
		
		BMallocIO io;
		buf.Seek(0, SEEK_SET);
		BMessage ioExtension;
		if( !out_bitmap ) {
			ioExtension.AddBool(B_TRANSLATOR_EXT_HEADER_ONLY, true);
		}
		err = BTranslatorRoster::Default()->Translate(&buf, &info, &ioExtension,
													  &io, B_TRANSLATOR_BITMAP);
		TranslatorBitmap header;
		if( err >= B_OK ) err = io.ReadAt(0, &header, sizeof(header));
		if( err >= B_OK ) {
			header.magic = B_BENDIAN_TO_HOST_INT32(header.magic);
			header.bounds.left = B_BENDIAN_TO_HOST_FLOAT(header.bounds.left);
			header.bounds.top = B_BENDIAN_TO_HOST_FLOAT(header.bounds.top);
			header.bounds.right = B_BENDIAN_TO_HOST_FLOAT(header.bounds.right);
			header.bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(header.bounds.bottom);
			header.rowBytes = B_BENDIAN_TO_HOST_INT32(header.rowBytes);
			header.colors = (color_space) B_BENDIAN_TO_HOST_INT32(header.colors);
			header.dataSize = B_BENDIAN_TO_HOST_INT32(header.dataSize);
			//const void* data = ((uint8*)io.Buffer()) + sizeof(header);
			
			PRINT(("Bitmap header: colors=%lx, size=%lx, bounds=",
					header.colors, header.dataSize));
			#if DEBUG
			header.bounds.PrintToStream();
			#endif
			
			// Sanity check data in header.
			if( header.magic != B_TRANSLATOR_BITMAP ) {
				err = B_ERROR;
			} else  if( !header.bounds.IsValid() || header.bounds.Width() < 0.0
					|| header.bounds.Height() < 0.0 ) {
				err = B_ERROR;
			#if 0	// this is a little -too- rigid.
			} else if( (header.rowBytes*int32(header.bounds.Width()+1.5))
							!= header.dataSize ) {
				err = B_ERROR;
			#endif
			}
		}
		
		if( err >= B_OK ) {
			if( out_header ) *out_header = header;
			if( out_io_extension ) {
				ioExtension.RemoveName(B_TRANSLATOR_EXT_HEADER_ONLY);
				*out_io_extension = ioExtension;
			}
			if( out_bitmap ) {
				*out_bitmap = new BBitmap(header.bounds, header.colors);
				if( !(*out_bitmap) ) {
					err = B_ERROR;
				} else if( (*out_bitmap)->InitCheck() != B_OK ) {
					err = (*out_bitmap)->InitCheck();
				} else if( !(*out_bitmap)->IsValid() ) {
					err = B_ERROR;
				}
				if( err >= B_OK ) {
					err = io.ReadAt(sizeof(header), (*out_bitmap)->Bits(),
									(*out_bitmap)->BitsLength());
					#if 0	// this is a little -too- rigid.
					if( err != (*out_bitmap)->BitsLength() ) err = B_ERROR;
					#endif
				}
				if( err < B_OK ) {
					delete *out_bitmap;
					*out_bitmap = 0;
				}
			}
		}
	}
	
	return err >= B_OK ? 0 : err;
}

static status_t GetResourceBitmap(const BResourceItem* from,
								translator_info* out_info = 0,
								TranslatorBitmap* out_header = 0,
								BMessage* out_io_extension = 0,
								BBitmap** out_bitmap = 0)
{
	if( out_info ) init_translator_info(out_info);
	if( out_header ) memset(out_header, sizeof(out_header), 0);
	if( out_io_extension ) *out_io_extension = BMessage();
	if( out_bitmap ) *out_bitmap = 0;
	
	#if DEBUG
	BString buf;
	#endif
	PRINT(("Looking for bitmap in resource %s...\n",
			BResourceParser::TypeIDToString(from->Type(), from->ID(), &buf)));
	
	if( from->Type() == 'ICON' &&
			(from->Size() == 1024 || from->Size() == 0) ) {
		PRINT(("Found 32x32 icon.\n"));
		if( out_header ) {
			out_header->magic = B_TRANSLATOR_BITMAP;
			out_header->bounds = BRect(0, 0, 31, 31);
			out_header->rowBytes = 32;
			out_header->colors = B_CMAP8;
			out_header->dataSize = 1024;
		}
		if( out_io_extension ) {
			out_io_extension->RemoveName(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE);
			out_io_extension->AddInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE, B_CMAP8);
		}
		if( out_bitmap ) {
			*out_bitmap = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
			if( from->Size() > 0 ) {
				(*out_bitmap)->SetBits(from->Data(), from->Size(), 0, B_CMAP8);
			}
		}
		return B_OK;
		
	} else if( from->Type() == 'MICN' &&
			(from->Size() == 256 || from->Size() == 0) ) {
		PRINT(("Found 16x16 icon.\n"));
		if( out_header ) {
			out_header->magic = B_TRANSLATOR_BITMAP;
			out_header->bounds = BRect(0, 0, 15, 15);
			out_header->rowBytes = 16;
			out_header->colors = B_CMAP8;
			out_header->dataSize = 256;
		}
		if( out_io_extension ) {
			out_io_extension->RemoveName(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE);
			out_io_extension->AddInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE, B_CMAP8);
		}
		if( out_bitmap ) {
			*out_bitmap = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);
			if( from->Size() ) {
				(*out_bitmap)->SetBits(from->Data(), from->Size(), 0, B_CMAP8);
			}
		}
		return B_OK;
		
	} else if( from->Type() == B_CURSOR_TYPE &&
			(from->Size() == 68 || from->Size() == 0) ) {
		PRINT(("Found 16x16 cursor.\n"));
		if( out_io_extension ) {
			out_io_extension->RemoveName(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE);
		}
		return BitmapFromCursor((const uint8*)from->Data(), from->Size(),
								out_info, out_header,
								out_io_extension, out_bitmap);
		
	} else {
		status_t err;
		
		BMemoryIO io(from->Data(), from->Size());
		err = GetBufferBitmap(io, NULL, from->Type(),
							  out_info, out_header, out_io_extension,
							  out_bitmap);
		PRINT(("Translator %s result = %lx\n",
				BResourceParser::TypeToString(from->Type(), &buf), err));
		if( err != B_OK ) {
			io.Seek(0, SEEK_SET);
			BMessage archive;
			if( (err=archive.Unflatten(&io)) == B_OK ) {
				PRINT(("Found archive in resource.\n"));
				BArchivable *a = instantiate_object(&archive);
				BBitmap* b;
				if((b = dynamic_cast<BBitmap *>(a)) == 0) {
					err = B_ERROR;
					delete a;
				} else {
					if( out_info ) out_info->type = B_BITMAP_TYPE;
					if( out_header ) {
						out_header->magic = B_TRANSLATOR_BITMAP;
						out_header->bounds = b->Bounds();
						out_header->rowBytes = b->BytesPerRow();
						out_header->colors = b->ColorSpace();
						out_header->dataSize = b->BitsLength();
					}
					if( out_io_extension ) {
						out_io_extension->RemoveName(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE);
						out_io_extension->AddInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE,
												   b->ColorSpace());
					}
					if( out_bitmap ) {
						*out_bitmap = b;
					} else {
						delete b;
					}
				}
			}
			PRINT(("Unarchive result = %lx\n", err));
		}
		
		if( err != B_OK && from->Type() ) {
			io.Seek(0, SEEK_SET);
			err = GetBufferBitmap(io, NULL, 0,
								  out_info, out_header, out_io_extension,
								  out_bitmap);
			PRINT(("Any translator result = %lx\n", err));
		}
		
		if( err == B_OK ) return err;
	}
	
	if( from->Size() == 1024 ) {
		PRINT(("Found 32x32 raw.\n"));
		if( out_header ) {
			out_header->magic = B_TRANSLATOR_BITMAP;
			out_header->bounds = BRect(0, 0, 31, 31);
			out_header->rowBytes = 32;
			out_header->colors = B_CMAP8;
			out_header->dataSize = 1024;
		}
		if( out_io_extension ) {
			out_io_extension->RemoveName(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE);
			out_io_extension->AddInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE, B_CMAP8);
		}
		if( out_bitmap ) {
			*out_bitmap = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
			(*out_bitmap)->SetBits(from->Data(), from->Size(), 0, B_CMAP8);
		}
		return B_OK;
	
	} else if( from->Size() == 256 ) {
		PRINT(("Found 16x16 raw.\n"));
		if( out_header ) {
			out_header->magic = B_TRANSLATOR_BITMAP;
			out_header->bounds = BRect(0, 0, 15, 15);
			out_header->rowBytes = 16;
			out_header->colors = B_CMAP8;
			out_header->dataSize = 256;
		}
		if( out_io_extension ) {
			out_io_extension->RemoveName(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE);
			out_io_extension->AddInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE, B_CMAP8);
		}
		if( out_bitmap ) {
			*out_bitmap = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);
			(*out_bitmap)->SetBits(from->Data(), from->Size(), 0, B_CMAP8);
		}
		return B_OK;
		
	} else if( from->Size() == 68 ) {
		PRINT(("Found 16x16 raw cursor.\n"));
		if( out_io_extension ) {
			out_io_extension->RemoveName(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE);
		}
		return BitmapFromCursor((const uint8*)from->Data(), from->Size(),
								out_info, out_header,
								out_io_extension, out_bitmap);
		
	}
	
	return B_ERROR;
}

static BBitmap* GetBitmap(const BResourceItem* from,
						  translator_info* out_info = 0,
						  BMessage* out_io_extension = 0)
{
	BBitmap* b = 0;
	GetResourceBitmap(from, out_info, 0, out_io_extension, &b);
	return b;
}


// -----------------------------------------------------------------------------

class IconMiniEditor : public BMiniItemEditor
{
public:
	IconMiniEditor(const BResourceAddonArgs& args, BResourceHandle primaryItem)
		: BMiniItemEditor(args, primaryItem),
		  fBitmap(0), fColorSpace(B_NO_COLOR_SPACE),
		  fBitmapWidth(-1), fBitmapHeight(-1),
		  fLastWidth(-1), fLastHeight(-1)
	{
	}

	~IconMiniEditor()
	{
		delete fBitmap;
		fBitmap = 0;
	}
	
	void DataChanged(BResourceHandle& item)
	{
		BMiniItemEditor::DataChanged(item);
		delete fBitmap;
		fBitmap = 0;
		fBitmapWidth = fBitmapHeight = -1;
	}
	
	BBitmap* ResourceBitmap() const
	{
		BBitmap* bm = 0;
		const BResourceCollection* c = ReadLock();
		if( c ) {
			const BResourceItem* it = c ? c->ReadItem(PrimaryItem()) : 0;
			if( it ) {
				#if DEBUG
				BString buf;
				PRINT(("Reading bitmap from resource %s...\n",
						BResourceParser::TypeIDToString(it->Type(), it->ID(), &buf)));
				#endif
				bm = GetBitmap(it);
				PRINT(("Returning bitmap %p: w=%.2f, h=%.2f, color=0x%lx\n",
						bm, bm ? bm->Bounds().Width()+1 : 0,
						bm ? bm->Bounds().Height()+1 : 0,
						bm ? bm->ColorSpace() : 0));
			}
			ReadUnlock(c);
		}
		return bm;
	}
	
	virtual void DrawData(BView* into, BRect frame, float baseline) const
	{
		IconMiniEditor* This = const_cast<IconMiniEditor*>(this);
		
		if( frame.Width() <= 0 || frame.Height() <= 0 ) return;
		
		BBitmap* orig = 0;
		
		if( fBitmapWidth < 0 || fBitmapHeight < 0 ) {
			
			// Don't know original bitmap dimensions -- contruct bitmap
			// from resources to retrieve them.
			orig = ResourceBitmap();
			if( !orig ) return;
			
			This->fBitmapWidth = orig->Bounds().Width();
			This->fBitmapHeight = orig->Bounds().Height();
			PRINT(("*** Found new bitmap width=%f, height=%f\n",
					This->fBitmapWidth, This->fBitmapHeight));
			if( This->fBitmapWidth < 0 || This->fBitmapHeight < 0
						|| orig->ColorSpace() == B_NO_COLOR_SPACE ) {
				delete orig;
				return;
			}
		}

		// Now figure out the scaled dimensions to display in the
		// mini editor.
		float wd = frame.Width() / fBitmapWidth;
		float hd = frame.Height() / fBitmapHeight;
//		PRINT(("wd = %f, hd=%f\n", wd, hd));
		if( hd < wd ) wd = hd;
		if( wd > 1.0 ) wd = 1.0;
		BRect d(0, 0, floor(fBitmapWidth*wd+.5), floor(fBitmapHeight*wd+.5));
		#if DEBUG
//		PRINT(("Bitmap width=%f, height=%f\n", fBitmapWidth, fBitmapHeight));
//		PRINT(("Orig frame = "));
//		frame.PrintToStream();
//		PRINT(("New frame = "));
//		d.PrintToStream();
		#endif
			
		// If the scaled dimensions have changed or we don't have
		// a cached bitmap, create a new cached bitmap.
		if( !This->fBitmap || fLastWidth != d.Width()
				|| fLastHeight != d.Height() ) {
			
			This->fLastWidth = d.Width();
			This->fLastHeight = d.Height();
//			PRINT(("*** Found new scaled width=%f, height=%f\n",
//					This->fLastWidth, This->fLastHeight));
			
			// If didn't already retrieve the original bitmap from
			// above, retrieve it now.
			if( !orig ) orig = ResourceBitmap();
			if( !orig ) return;
			
			This->fColorSpace = orig->ColorSpace();
			
			// This is pretty ghastly...  hopefully we won't have to do it much.
			BBitmap drawBM(d, B_BITMAP_ACCEPTS_VIEWS, orig->ColorSpace());
			drawBM.Lock();
			BView drawer(d, "drawer", B_FOLLOW_NONE, B_WILL_DRAW);
			drawBM.AddChild(&drawer);
			drawer.SetDrawingMode(B_OP_COPY);
			drawer.SetHighColor(B_TRANSPARENT_COLOR);
			drawer.FillRect(d);
			drawer.DrawBitmap(orig, d);
			drawer.Sync();
			drawBM.RemoveChild(&drawer);
			drawBM.Unlock();
			
			delete This->fBitmap;
			This->fBitmap = 0;
			This->fBitmap = new BBitmap(&drawBM);
		}
		
		delete orig;
		
		if( This->fBitmap ) {
			BRect d(This->fBitmap->Bounds());
			int32 bpp = 0;
			BString dim;
			dim << int32(fBitmapWidth+1) << "x" << int32(fBitmapHeight+1);
			switch( fColorSpace ) {
				case B_GRAY1:
					bpp = 1;
					break;
				case B_CMAP8:
				case B_GRAY8:
					bpp = 8;
					break;
				case B_RGB15:
				case B_RGBA15:
				case B_RGB15_BIG:
				case B_RGBA15_BIG:
					bpp = 15;
				case B_RGB16:
				case B_RGB16_BIG:
					bpp = 16;
					break;
				case B_RGB24:
				case B_RGB24_BIG:
					bpp = 24;
					break;
				case B_RGB32:
				case B_RGBA32:
				case B_RGB32_BIG:
				case B_RGBA32_BIG:
					bpp = 32;
					break;
				default:
					break;
			}
			if( bpp ) dim << "x" << bpp;
			float w = into->StringWidth(dim.String());
			if( w > frame.Width()-d.Width()-4 ) {
				w = frame.Width()-d.Width()-4;
			}
			//d.OffsetTo( frame.left + (frame.Width()-d.Width())/2,
			//			frame.top + (frame.Height()-d.Height())/2 );
			d.OffsetTo( frame.left,
						frame.top + (frame.Height()-d.Height())/2 );
			#if DEBUG
//			PRINT(("Drawing bitmap at frame = "));
//			d.PrintToStream();
			#endif
			into->PushState();
			into->DrawString(dim.String(), BPoint(frame.right-w, baseline));
			into->SetDrawingMode(B_OP_ALPHA);
			into->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
			into->DrawBitmapAsync(This->fBitmap, d);
			into->PopState();
		}
	}
	
private:
	BBitmap* fBitmap;
	color_space fColorSpace;
	float fBitmapWidth, fBitmapHeight;
	float fLastWidth, fLastHeight;
};

// -----------------------------------------------------------------------------

class IconFullEditor;

class TCustomIconEditor : public TIconEditor
{
public:
	TCustomIconEditor(IconFullEditor* editor, BRect r,
					  const BMessage* configuration);
	~TCustomIconEditor();

	virtual void BitmapChanged(TBitmapEditor* editor,
							   const char* what, bool mini);
	virtual void HotSpotChanged(TBitmapEditor* editor,
								int32 x, int32 y, bool mini);
	virtual void MessageReceived(BMessage* msg);
	virtual void AttachedToWindow();
	
	void SetTargetBitmap(const BBitmap* primary,
						 const BBitmap* secondary,
						 type_code primaryFormat = B_RAW_TYPE,
						 type_code secondaryFormat = B_RAW_TYPE,
						 const BMessage* primaryExtension = 0,
						 const BMessage* secondaryExtension = 0);
	
	void SetAttributes(const BBitmap* primary, type_code primaryFormat,
						const BMessage* primaryExtension = 0,
						bool updateEditor=true);
	
private:
	typedef TIconEditor inherited;
	
	IconFullEditor* fEditor;
	bool fWatchHotSpot;
};

class IconFullEditor : public BFullItemEditor
{
public:
	IconFullEditor(const BResourceAddonArgs& args, BResourceHandle primaryItem,
				   const BMessage* configuration)
		: BFullItemEditor(args, primaryItem),
		  fEditor(0), fCopyCursorItem(0), fHotSpotItem(0),
		  fSelectionCopyItem(0), fSelectionAlphaItem(0), fSelectionBackgroundItem(0),
		  fDitherItem(0), fRawData(false)
	{
		fMenus[0] = fMenus[1] = 0;
		if( configuration ) fConfiguration = *configuration;
		init_translator_info(&fInfo);
	}
	
	~IconFullEditor()
	{
		delete fMenus[0];
		delete fMenus[1];
		delete fEditor;
		fEditor = 0;
	}

	status_t Retarget(BResourceHandle new_item)
	{
		SetPrimaryItem(new_item);
		AttachData();
		return B_OK;
	}
	
	void AttachData()
	{
		if( !fEditor ) return;
		
		type_code primaryType = B_RAW_TYPE;
		
		// We need to get write access so that we can subscribe to
		// additional items.
		
		BResourceCollection* c = WriteLock();
		if( c ) {
			init_translator_info(&fInfo);
			fMiniItem = BResourceHandle();
			fLargeItem = BResourceHandle();
			
			const BResourceItem* pri = c->ReadItem(PrimaryItem());
			
			BBitmap* primaryBitmap = 0;
			BBitmap* secondaryBitmap = 0;
			
			if( pri->Type() == 'ICON' ) {
				fLargeItem = PrimaryItem();
				primaryBitmap = GetBitmap(pri);
				c->FindAndSubscribe(&fMiniItem, this, 'MICN', pri->ID());
				const BResourceItem* sec = c->ReadItem(fMiniItem);
				if( sec ) secondaryBitmap = GetBitmap(sec);
				fRawData = true;
				
			} else if( pri->Type() == 'MICN' ) {
				fMiniItem = PrimaryItem();
				secondaryBitmap = GetBitmap(pri);
				c->FindAndSubscribe(&fLargeItem, this, 'ICON', pri->ID());
				const BResourceItem* sec = c->ReadItem(fLargeItem);
				if( sec ) primaryBitmap = GetBitmap(sec);
				fRawData = true;
			
			} else {
				fLargeItem = PrimaryItem();
				primaryBitmap = GetBitmap(pri, &fInfo, &fIOExtension);
				fRawData = false;
				primaryType = fInfo.type;
			}
			
			fEditor->SetTargetBitmap(primaryBitmap, secondaryBitmap,
									 primaryType, 0,
									 &fIOExtension, 0);
			delete primaryBitmap;
			delete secondaryBitmap;
			
			WriteUnlock(c);
		}
	}

	void BitmapChanged(TBitmapEditor* editor, const char* name, bool mini,
					   bool report=false)
	{
		PRINT(("Updating bitmap resource.\n"));
		
		BResourceCollection* c = WriteLock(name);
		if( !c ) return;
		
		BResourceItem* it = c->WriteItem(mini ? fMiniItem : fLargeItem,
										 report ? 0 : this);
		const BBitmap* bits = editor ? editor->RealBitmap() : 0;
		PRINT(("Item is %p, bitmap object is %p\n", it, bits));
		if( it && bits ) {
			if( fRawData ) {
				PRINT(("Writing raw data length 0x%lx\n", bits->BitsLength()));
				it->SetData(bits->Bits(), bits->BitsLength());
				
			} else if( fInfo.type == B_MESSAGE_TYPE ) {
				BMessage arch;
				PRINT(("Writing BMessage archive...\n"));
				if( bits->Archive(&arch, false) == B_OK ) {
					it->Seek(0, SEEK_SET);
					it->SetSize(0);
					arch.Flatten(it);
					PRINT(("Wrote 0x%lx bytes.\n", (size_t)it->Position()));
				}
				
			} else if( fInfo.type == B_BITMAP_TYPE ) {
				BMessage arch;
				PRINT(("Writing BMessage archive...\n"));
				if( bits->Archive(&arch, false) == B_OK ) {
					it->Seek(0, SEEK_SET);
					it->SetSize(0);
					arch.Flatten(it);
					PRINT(("Wrote 0x%lx bytes.\n", (size_t)it->Position()));
				}
				
			} else if( fInfo.type == B_CURSOR_TYPE ) {
				PRINT(("Writing cursor data...\n"));
				fIOExtension.RemoveName("be:x_hotspot");
				fIOExtension.RemoveName("be:y_hotspot");
				fIOExtension.AddInt32("be:x_hotspot", editor->HotSpotX());
				fIOExtension.AddInt32("be:y_hotspot", editor->HotSpotY());
				
				size_t size=0;
				uint8* data = CursorFromBitmap(bits, &fIOExtension, &size);
				if( data ) {
					it->Seek(0, SEEK_SET);
					it->SetData(data, size);
					free(data);
					PRINT(("Wrote 0x%lx bytes.\n", size));
				}
			
			} else {
				PRINT(("Writing translated bitmap...\n"));
				it->Seek(0, SEEK_SET);
				it->SetSize(0);
				BBitmap bm(bits->Bounds(), B_RGBA32);
				#if DEBUG
				uint8* data = (uint8*)bm.Bits();
				for( int32 i=0; i<bm.BitsLength(); i++ ) {
					data[i] = (uint8)i;
				}
				#endif
				status_t err =
				set_bitmap(&bm, bits, false);
				(void)err;
				PRINT(("Set bits orig %.2fx%.2fx$%lx to %.2fx%.2fx$%lx: err=0x%lx\n",
						bits->Bounds().Width()+1,
						bits->Bounds().Height()+1,
						bits->ColorSpace(),
						bm.Bounds().Width()+1,
						bm.Bounds().Height()+1,
						bm.ColorSpace(),
						err));
				BBitmapStream bs(&bm);
				err =
				BTranslatorRoster::Default()->Translate(fInfo.translator,
														&bs, &fIOExtension, it,
														fInfo.type);
				(void)err;
				PRINT(("Wrote translator %4.4s bitmap %4.4s: err=0x%lx\n",
						(const char*)&fInfo.translator,
						(const char*)&fInfo.type, strerror(err)));
				BBitmap* tmp=0;
				bs.DetachBitmap(&tmp);
			}
		}
		
		WriteUnlock(c);
	}
	
	void AttributesChanged(TBitmapEditor* editor, float width=-1, float height=-1,
						   color_space cspace = B_NO_COLOR_SPACE)
	{
		const char* name = "Change Attributes";
		if( height < 0 && cspace == B_NO_COLOR_SPACE ) {
			name = "Change Width";
		} else if( width < 0 && cspace == B_NO_COLOR_SPACE ) {
			name = "Change Height";
		} else if( cspace == B_NO_COLOR_SPACE ) {
			name = "Change Size";
		} else if( width < 0 && height < 0 ) {
			name = "Change Color Space";
		}
		
		if( cspace != B_NO_COLOR_SPACE ) {
			fIOExtension.RemoveName(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE);
			fIOExtension.AddInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE,
								  (int32)cspace);
		}
		
		BResourceCollection* c = WriteLock(name);
		if( !c ) return;
		
		// Change the bitmap and write the new data into the resource.
		// Have this change reported back to us so that any changes a
		// translator makes will be shown to the user.
		fEditor->SetPrimaryAttributes(width, height, cspace);
		BitmapChanged(editor, "", false, true);
		
		WriteUnlock(c);
	}
	
	void FormatChanged(TBitmapEditor* editor, uint32 translator, type_code type)
	{
		printf("Changing format %4.4s to %4.4s\n",
				(char*)&fInfo.type, (char*)&type);
		if( translator == fInfo.translator && type == fInfo.type ) return;
		if( !fLargeItem.IsValid() ) return;
		
		PRINT(("Changing bitmap format.\n"));
		
		BResourceCollection* c = WriteLock("Change Format");
		if( !c ) return;
		
		// Change the format and write the new data into the resource.
		// Have this change reported back to us so that any changes a
		// translator makes will be shown to the user.
		BResourceItem* it = c->WriteItem(fLargeItem);
		if( it->Type() == fInfo.type ) {
			it->SetType(type);
		}
		fInfo.translator = translator;
		fInfo.type = type;
		
		BitmapChanged(editor, "", false, true);
		
		WriteUnlock(c);
	}
	
	void ExecDataChanged(const BResourceCollection* c,
						 BResourceHandle& item, uint32 changes)
	{
		if( !fEditor ) return;
		
		if( fEditor->Window() ) {
			if( !fEditor->Window()->Lock() ) return;
		}
		
		if( item == fLargeItem &&
				(changes&(B_RES_DATA_CHANGED|B_RES_SIZE_CHANGED)) != 0 &&
				fEditor ) {
			const BResourceItem* it = c->ReadItem(fLargeItem);
			if( it ) {
				init_translator_info(&fInfo);
				fIOExtension = BMessage();
				BBitmap* bm = GetBitmap(it, &fInfo, &fIOExtension);
				if( bm ) {
					fEditor->SetAttributes(bm, fInfo.type, &fIOExtension);
					fEditor->NewPrimaryImage(bm, bm->Bounds(), false);
				}
				delete bm;
				SetCursorState();
			}
		}
		
		if( item == fMiniItem &&
				(changes&(B_RES_DATA_CHANGED|B_RES_SIZE_CHANGED)) != 0 &&
				fEditor ) {
			const BResourceItem* it = c->ReadItem(fMiniItem);
			if( it ) {
				translator_info info;
				fIOExtension = BMessage();
				init_translator_info(&info);
				BBitmap* bm = GetBitmap(it, &info);
				if( bm ) fEditor->NewSecondaryImage(bm, bm->Bounds(), false);
				delete bm;
			}
		}
		
		if( fEditor->Window() ) fEditor->Window()->Unlock();
	}
		
	virtual status_t GetConfiguration(BMessage* into) const
	{
		status_t err = B_OK;
		if( fEditor ) err = fEditor->GetConfiguration(into);
		else *into = fConfiguration;
		return err;
	}
	
	virtual status_t SetConfiguration(const BMessage* from)
	{
		if( fEditor ) fEditor->SetConfiguration(from);
		else fConfiguration = *from;
		return B_OK;
	}
	
	virtual BMenuItem* EditMenuItem(int32 /*which*/)
	{
		return 0;
	}
	
	virtual BMenu* CustomMenu(int32 which)
	{
		if( !fMenus[0] ) {
			fMenus[0] = new BMenu("Image");
			
			BMenuItem* item;
			BMessage* msg;
			
			item = new BMenuItem("Copy as Text", new BMessage(kDumpIcons), 'C',
				B_COMMAND_KEY | B_OPTION_KEY);
			item->SetTarget(View());
			fMenus[0]->AddItem(item);
		
			fCopyCursorItem = new BMenuItem("Copy as Cursor Text",
				new BMessage(kDumpCursor), 'C', B_COMMAND_KEY | B_SHIFT_KEY);
			fCopyCursorItem->SetTarget(View());
			fMenus[0]->AddItem(fCopyCursorItem);
		
			fMenus[0]->AddSeparatorItem();
			
			msg = new BMessage(kSetSelectionMode);
			msg->AddInt32("mode", kPasteCopy);
			fSelectionCopyItem = new BMenuItem("Paste Includes Everything", msg);
			fSelectionCopyItem->SetTarget(View());
			fMenus[0]->AddItem(fSelectionCopyItem);
			msg = new BMessage(kSetSelectionMode);
			msg->AddInt32("mode", kPasteAlpha);
			fSelectionAlphaItem = new BMenuItem("Paste Uses Alpha", msg);
			fSelectionAlphaItem->SetTarget(View());
			fMenus[0]->AddItem(fSelectionAlphaItem);
			msg = new BMessage(kSetSelectionMode);
			msg->AddInt32("mode", kPasteBackground);
			fSelectionBackgroundItem = new BMenuItem("Paste Excludes Background", msg);
			fSelectionBackgroundItem->SetTarget(View());
			fMenus[0]->AddItem(fSelectionBackgroundItem);
			
			fMenus[0]->AddSeparatorItem();
			
			msg = new BMessage(kToggleDithering);
			fDitherItem = new BMenuItem("Dither Color Conversions", msg);
			fDitherItem->SetTarget(View());
			fMenus[0]->AddItem(fDitherItem);
			
			fMenus[0]->AddSeparatorItem();
			
			item = new BMenuItem("Set Background Color...", new BMessage(msg_set_bg_color), 'B');
			item->SetTarget(View());
			fMenus[0]->AddItem(item);
		}
		
		if( !fMenus[1] ) {
			fMenus[1] = new BMenu("Tools");
		
			BMenuItem* item;
			
			item = new BMenuItem("Previous Tool", new BMessage(msg_prev_tool), '1');
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			item = new BMenuItem("Next Tool", new BMessage(msg_next_tool), '2');
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			fMenus[1]->AddSeparatorItem();
			
			item = new BMenuItem("Selection", new BMessage(msg_selection_tool), '3');
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			item = new BMenuItem("Eraser", new BMessage(msg_eraser_tool), '4');
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			item = new BMenuItem("Pencil", new BMessage(msg_pencil_tool), '5');
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			item = new BMenuItem("Eye Dropper", new BMessage(msg_eye_tool), '6');
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			item = new BMenuItem("Fill", new BMessage(msg_fill_tool), '7');
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			item = new BMenuItem("Line", new BMessage(msg_line_tool),'8');
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			item = new BMenuItem("Rectangle", new BMessage(msg_rect_tool),'9');
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			item = new BMenuItem("Filled Rectangle", new BMessage(msg_frect_tool));
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			item = new BMenuItem("Round Rectangle", new BMessage(msg_rrect_tool));
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			item = new BMenuItem("Filled Round Rectangle", new BMessage(msg_frrect_tool));
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			item = new BMenuItem("Oval", new BMessage(msg_oval_tool));
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			item = new BMenuItem("Filled Oval", new BMessage(msg_foval_tool));
			item->SetTarget(View());
			fMenus[1]->AddItem(item);
			fMenus[1]->AddSeparatorItem();
			
			fHotSpotItem = new BMenuItem("Set Hot Spot", new BMessage(msg_hotspot_tool));
			fHotSpotItem->SetTarget(View());
			fMenus[1]->AddItem(fHotSpotItem);
		}
		
		SetCursorState();
		SetSelectionState();
			
		if( which >= 0 && which < 2 ) return fMenus[which];
		return 0;
	}
	
	void SetCursorState()
	{
		bool enabled = fInfo.type == B_CURSOR_TYPE ? true : false;
		if( fCopyCursorItem )
			fCopyCursorItem->SetEnabled(enabled);
		if( fHotSpotItem )
			fHotSpotItem->SetEnabled(enabled);
	}
	
	void SetSelectionState()
	{
		if (!fEditor) return;
		
		const paste_selection_mode mode = fEditor->SelectionMode();
		
		if (fSelectionCopyItem) fSelectionCopyItem->SetMarked(mode == kPasteCopy);
		if (fSelectionAlphaItem) fSelectionAlphaItem->SetMarked(mode == kPasteAlpha);
		if (fSelectionBackgroundItem) fSelectionBackgroundItem->SetMarked(mode == kPasteBackground);
		if (fDitherItem) fDitherItem->SetMarked(fEditor->DitherColorConversions());
	}
	
	virtual BView* View()
	{
		if( !fEditor ) {
			fEditor = new TCustomIconEditor(this, BRect(0, 0, 800, 800),
											&fConfiguration);
			SetChangeTarget(fEditor);
			SetSelectionState();
			AttachData();
		}
		
		return fEditor;
	}

private:
	BResourceHandle		fMiniItem;
	BResourceHandle		fLargeItem;
	BMessage			fConfiguration;
	BMessage			fIOExtension;
	translator_info		fInfo;
	
	TCustomIconEditor*	fEditor;
	BMenu*				fMenus[2];
	BMenuItem*			fCopyCursorItem;
	BMenuItem*			fHotSpotItem;
	BMenuItem*			fSelectionCopyItem;
	BMenuItem*			fSelectionAlphaItem;
	BMenuItem*			fSelectionBackgroundItem;
	BMenuItem*			fDitherItem;
	
	bool fRawData;
};

TCustomIconEditor::TCustomIconEditor(IconFullEditor* editor, BRect r,
									 const BMessage* configuration)
	: TIconEditor(r, configuration),
	  fEditor(editor), fWatchHotSpot(false)
{
}

TCustomIconEditor::~TCustomIconEditor()
{
}

void TCustomIconEditor::BitmapChanged(TBitmapEditor* editor,
									  const char* name, bool mini)
{
	PRINT(("Reporting bitmap change: mini=%d, name=%s\n",
			mini, name));
	fEditor->BitmapChanged(editor, name, mini);
}

void TCustomIconEditor::HotSpotChanged(TBitmapEditor* editor,
									   int32, int32, bool mini)
{
	if( !mini && fWatchHotSpot ) {
		PRINT(("Reporting hot spot change: mini=%d\n", mini));
		fEditor->BitmapChanged(editor, "Set Hot Spot", mini);
	}
}

void TCustomIconEditor::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped()) {
		BPoint point;
		uint32 buttons;
		GetMouse(&point, &buttons, false);
		ConvertToScreen(&point);
		status_t err = parse_message_data(BResourceAddonArgs(*fEditor),
										  point, msg);
		if( err == B_OK ) return;
	}
	
	switch(msg->what)
	{
		case B_PASTE: {
			status_t err = B_ERROR;
			
			if( be_clipboard->Lock() ) {
				BMessage *message = be_clipboard->Data();
				BPoint point;
				uint32 buttons;
				GetMouse(&point, &buttons, false);
				ConvertToScreen(&point);
				err = parse_message_data(BResourceAddonArgs(*fEditor),
										 point, message);
				be_clipboard->Unlock();
			}
			
			if( err != B_OK ) {
				inherited::MessageReceived(msg);
			}
		} break;
		
		case B_RESOURCE_DATA_CHANGED: {
			const BResourceCollection* c = fEditor->ReadLock();
			if( c ) {
				BResourceHandle h;
				uint32 changes;
				while( (h=c->GetNextChange(fEditor, &changes)).IsValid() ) {
					PRINT(("Executing object %p change %lx\n",
							c->ReadItem(h), changes));
					fEditor->ExecDataChanged(c, h, changes);
				}
				fEditor->ReadUnlock(c);
			}
		} break;
		
		case kFormatChangedMsg: {
			int32 format;
			int32 translator;
			if( PrimaryBitmapEditor() &&
					msg->FindInt32("format", &format) == B_OK &&
					msg->FindInt32("translator", &translator) == B_OK ) {
				fEditor->FormatChanged(PrimaryBitmapEditor(), translator, format);
			}
		} break;
		
		case kDimensChangedMsg: {
			float width, height;
			if( PrimaryBitmapEditor() ) {
				if( msg->FindFloat("width", &width) != B_OK ) width = -1;
				if( msg->FindFloat("height", &height) != B_OK ) height = -1;
				fEditor->AttributesChanged(PrimaryBitmapEditor(), width, height);
			}
		} break;
		
		case kColorSpaceChangedMsg: {
			color_space cspace;
			if( PrimaryBitmapEditor() &&
					msg->FindInt32("color_space", (int32*)&cspace) == B_OK ) {
				fEditor->AttributesChanged(PrimaryBitmapEditor(), -1, -1, cspace);
			}
		} break;
		
		default :
			inherited::MessageReceived(msg);
			if (msg->what == kSetSelectionMode || msg->what == kToggleDithering) {
				fEditor->SetSelectionState();
			}
			break;
	}
}

void TCustomIconEditor::AttachedToWindow()
{
	inherited::AttachedToWindow();
	TBitmapControls* controls = dynamic_cast<TBitmapControls*>(Controls());
	if( controls ) controls->SetAllTargets(BMessenger(this));
}

void TCustomIconEditor::SetTargetBitmap(const BBitmap* primary,
										const BBitmap* secondary,
										type_code primaryFormat,
										type_code secondaryFormat,
										const BMessage* primaryExtension,
										const BMessage* secondaryExtension)
{
	(void)secondaryFormat;
	(void)secondaryExtension;
	
	if( !Controls() && !secondary &&
			primaryFormat != B_RAW_TYPE && primaryFormat != B_CURSOR_TYPE ) {
		SetControls(new TBitmapControls(BRect(0, 0, 15, 15), "controls",
										B_FOLLOW_NONE));
	} else if( secondary || primaryFormat == B_RAW_TYPE ||
			primaryFormat == B_CURSOR_TYPE ) {
		SetControls(0);
	}
	SetAttributes(primary, primaryFormat, primaryExtension, false);
	SetBitmap(primary, secondary);
	// Do again to make sure the hot spot is installed.
	SetAttributes(primary, primaryFormat, primaryExtension, false);
}

void TCustomIconEditor::SetAttributes(const BBitmap* primary,
										type_code primaryFormat,
										const BMessage* primaryExtension,
										bool updateEditor)
{
	if( !primary ) return;
	
	color_space cspace;
	if( !primaryExtension ||
			primaryExtension->FindInt32(B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE,
										(int32*)&cspace) != B_OK ) {
		cspace = primary->ColorSpace();
	}
	
	int32 x_spot=-1, y_spot=-1;
	if( primaryExtension && primaryFormat == B_CURSOR_TYPE ) {
		int32 val;
		if( primaryExtension->FindInt32("be:x_hotspot", &val) == B_OK ) {
			x_spot = val;
		}
		if( primaryExtension->FindInt32("be:y_hotspot", &val) == B_OK ) {
			y_spot = val;
		}
	}
	SetPrimaryHotSpot(x_spot, y_spot);
	fWatchHotSpot = (primaryFormat==B_CURSOR_TYPE) ? true : false;
	
	if( updateEditor ) {
		SetPrimaryAttributes(primary->Bounds().Width()+1,
							 primary->Bounds().Height()+1,
							 cspace);
	}
	
	TBitmapControls* controls = dynamic_cast<TBitmapControls*>(Controls());
	if( controls ) {
		controls->SetAllTargets(BMessenger());
		controls->SetAttributes(primary->Bounds().Width()+1,
								primary->Bounds().Height()+1,
								cspace,
								primaryFormat);
		controls->SetAllTargets(BMessenger(this));
	}
}

// -----------------------------------------------------------------------------

static BBitmap* GetMsgBitmap(const BMessage* msg, const char* name)
{
	BMessage currMsg;
	status_t err;
	
	err = msg->FindMessage(name, &currMsg);
		
	if (!err) {
		BBitmap* icon = (BBitmap*) BBitmap::Instantiate(&currMsg);
		return icon;
	}
	
	return 0;
}

struct generate_info {
	const char* name;
	type_code type;
};

static generate_info generators[] = {
	{ "Bitmap", B_BITMAP_TYPE },
	{ "Icon", 'ICON' },
	{ "Cursor", B_CURSOR_TYPE },
};

#define NUM_GENERATORS (sizeof(generators) / sizeof(generate_info))

static int32 find_unique_id(const BResourceCollection* c,
							bool large=true, bool small=true, int32 iconid=1)
{
	bool conflict = true;
	while( conflict ) {
		conflict = false;
		int32 lid = large ? c->UniqueIDForType('ICON', iconid) : iconid;
		int32 sid = small ? c->UniqueIDForType('MICN', iconid) : iconid;
		if( lid != sid ) {
			conflict = true;
			iconid = lid > sid ? lid : sid;
		} else {
			iconid = lid;
		}
	}
	return iconid;
}

class IconAddon : public BResourceAddon
{
public:
	IconAddon(const BResourceAddonArgs& args)
		: BResourceAddon(args)
	{
	}
	
	~IconAddon()
	{
	}
	
	virtual status_t GetNthGenerateInfo(int32 n,
										BMessage* out_info) const
	{
		if( n < 0 || (size_t)n >= NUM_GENERATORS ) return B_ERROR;
		out_info->AddString(B_GENERATE_NAME, generators[n].name);
		out_info->AddInt32(B_GENERATE_TYPE, generators[n].type);
		return B_OK;
	}
	
	virtual status_t GenerateResource(BResourceHandle* out_item,
									  const BMessage* info,
									  int32 id, const char* name,
									  bool make_selected = true,
									  BResourceCollection::conflict_resolution
									  resol = BResourceCollection::B_ASK_USER)
	{
		type_code type;
		status_t err = info->FindInt32(B_GENERATE_TYPE, (int32*)&type);
		if( err != B_OK ) return err;
		
		BResourceCollection* c = WriteLock();
		if( !c ) return B_ERROR;
		
		switch(type) {
			case B_BITMAP_TYPE: {
				BBitmap newBM(BRect(0, 0, 31, 31), B_BITMAP_CLEAR_TO_WHITE, B_RGBA32);
				BMessage arch;
				BMallocIO io;
				err = newBM.Archive(&arch, false);
				if( err == B_OK ) err = arch.Flatten(&io);
				if( err == B_OK ) {
					err = c->AddItem(out_item, B_BITMAP_TYPE, id, name,
									 io.Buffer(), io.BufferLength(),
									 make_selected, resol);
				}
			} break;
			
			case 'ICON': {
				id = find_unique_id(c, true, true, id);
				uint8 buffer[1024];
				memset(buffer, B_TRANSPARENT_MAGIC_CMAP8, sizeof(buffer));
				err = c->AddItem(out_item, 'ICON', id, name,
								 buffer, 1024, make_selected, resol);
				if( err == B_OK ) {
					err = c->AddItem(out_item, 'MICN', id, name,
									 buffer, 256, make_selected, resol);
				}
			} break;
			
			case B_CURSOR_TYPE: {
				uint8 buffer[68];
				buffer[0] = 16;
				buffer[1] = 1;
				buffer[2] = buffer[3] = 0;
				for( int32 i=0; i<32; i++ ) {
					buffer[4+i] = 0;
					buffer[4+32+i] = 0;
				}
				err = c->AddItem(out_item, B_CURSOR_TYPE, id, name,
								 buffer, sizeof(buffer), make_selected, resol);
			};
			
			default:
				err = B_BAD_TYPE;
				break;
		}
						 
		WriteUnlock(c);
		return err;
	}
	
	virtual float QuickQuality(const BResourceItem* item) const
	{
		if(item->Type() == B_BITMAP_TYPE)
		{
			return 0.6;
		}
		if(item->Type() == 'MICN' && (item->Size() == 0 || item->Size() == 256))
		{
			return 0.5;
		}
		if(item->Type() == 'ICON' && (item->Size() == 0 || item->Size() == 1024))
		{
			return 0.6;
		}
		if(item->Type() == B_CURSOR_TYPE && (item->Size() == 0 || item->Size() == 68))
		{
			return 0.6;
		}
		return -1;
	}
	
	virtual float PreciseQuality(const BResourceItem* item) const
	{
		float q = QuickQuality(item);
		if( q >= 0 ) return q;
		
		TranslatorBitmap header;
		status_t err = GetResourceBitmap(item, 0, &header);
		if( err == B_OK ) {
			BRect bounds(header.bounds);
			if( ((bounds.Width() == 15 && bounds.Height() == 15) ||
				(bounds.Width() == 31 && bounds.Height() == 31)) ) {
				return 0.6; // we're pretty good at icon sized bitmaps
			}
			
			return 0.5; // we're okay at other bitmaps.
		}
		
		return -1;
	}
	
	virtual BMiniItemEditor* MakeMiniEditor(const BResourceAddonArgs& args,
											BResourceHandle primaryItem,
											const BMessage* /*configuration*/)
	{
		return new IconMiniEditor(args, primaryItem);
	}
	
	virtual BFullItemEditor* MakeFullEditor(const BResourceAddonArgs& args,
											BResourceHandle primaryItem,
											const BMessage* configuration)
	{
		return new IconFullEditor(args, primaryItem, configuration);
	}
	
	virtual float PasteQuality(const BMessage* data) const
	{
		return DropQuality(data);
	}
	
	virtual status_t HandlePaste(const BMessage* data)
	{
		return HandleDrop(data);
	}
	
	virtual float DropQuality(const BMessage* drop) const
	{
		BBitmap* large = GetMsgBitmap(drop, kLargeIconMimeType);
		BBitmap* small = GetMsgBitmap(drop, kMiniIconMimeType);
		if( large || small ) {
			delete large;
			delete small;
			return 0.6;
		}
		return -1;
	}
	
	virtual status_t HandleDrop(const BMessage* drop)
	{
		status_t err = B_BAD_VALUE;
		
		BBitmap* large = GetMsgBitmap(drop, kLargeIconMimeType);
		BBitmap* small = GetMsgBitmap(drop, kMiniIconMimeType);
		
		BResourceCollection* c;
		if( (large || small) && (c=WriteLock()) != 0 ) {
			int32 iconid = find_unique_id(c,
										  large ? true : false,
										  small ? true : false);
			BResourceHandle h;
			if( large ) {
				c->AddItem(&h, 'ICON', iconid, "New Icon",
							large->Bits(), (size_t)large->BitsLength(),
							true, c->B_RENAME_NEW_ITEM);
			}
			if( small ) {
				c->AddItem(&h, 'MICN', iconid, "New Icon",
							small->Bits(), (size_t)small->BitsLength(),
							true, c->B_RENAME_NEW_ITEM);
			}
			WriteUnlock(c);
			err = B_OK;
		}
		
		delete large;
		delete small;
		
		return err;
	}
	
	virtual float BufferQuality(BPositionIO& buf, const char* mime_type = 0,
								entry_ref* /*ref*/ = 0) const
	{
		float quality = -1;
		BBitmap* bm = BTranslationUtils::GetBitmap(&buf);
		if( bm ) {
			if( bm->ColorSpace() == B_CMAP8 ) {
				int32 w = int32(bm->Bounds().Width()+1);
				int32 h = int32(bm->Bounds().Height()+1);
				if( (w==16 && h==16) || (w==32 || h==32) ) quality = 0.6;
			}
			quality = 0.1;
		}
		if( mime_type && strncmp(mime_type, "image/", 6) == 0 ) quality = 0.1;
		delete bm;
		return quality;
	}
	
	virtual status_t HandleBuffer(BPositionIO& buf, const char* mime_type = 0,
								  entry_ref* ref = 0)
	{
		BResourceCollection* c = WriteLock();
		if( !c ) return B_BAD_VALUE;
		
		BResourceHandle hand;
		translator_info info;
		ssize_t err;
		err = BTranslatorRoster::Default()->Identify(&buf, NULL, &info, 0,
													 mime_type, B_TRANSLATOR_BITMAP);
		if( err >= B_OK ) {
			BMallocIO io;
			buf.Seek(0, SEEK_SET);
			err = BTranslatorRoster::Default()->Translate(&buf, &info, 0,
														  &io, B_TRANSLATOR_BITMAP);
			TranslatorBitmap header;
			if( err >= B_OK ) err = io.ReadAt(0, &header, sizeof(header));
			if( err >= B_OK ) {
				header.magic = B_BENDIAN_TO_HOST_INT32(header.magic);
				header.bounds.left = B_BENDIAN_TO_HOST_FLOAT(header.bounds.left);
				header.bounds.top = B_BENDIAN_TO_HOST_FLOAT(header.bounds.top);
				header.bounds.right = B_BENDIAN_TO_HOST_FLOAT(header.bounds.right);
				header.bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(header.bounds.bottom);
				header.rowBytes = B_BENDIAN_TO_HOST_INT32(header.rowBytes);
				header.colors = (color_space) B_BENDIAN_TO_HOST_INT32(header.colors);
				header.dataSize = B_BENDIAN_TO_HOST_INT32(header.dataSize);
				const void* data = ((uint8*)io.Buffer()) + sizeof(header);
				
				#if DEBUG
				PRINT(("Bitmap header: colors=%lx, size=%lx, bounds=",
						header.colors, header.dataSize));
				header.bounds.PrintToStream();
				#endif
				err = B_BAD_VALUE;
				if( header.colors == B_CMAP8 ) {
					int32 w = int32(header.bounds.Width()+1);
					int32 h = int32(header.bounds.Height()+1);
					PRINT(("Width=%ld, Height=%ld\n", w, h));
					if( w==16 && h==16 ) {
						err = c->AddItem(&hand, 'MICN', 1, ref ? ref->name : "New Icon",
										 data, header.dataSize,
										 true, c->B_RENAME_NEW_ITEM);
					} else if( w==32 || h==32 ) {
						err = c->AddItem(&hand, 'ICON', 1, ref ? ref->name : "New Icon",
										 data, header.dataSize,
										 true, c->B_RENAME_NEW_ITEM);
					}
				}
			}
			
			if( err < B_OK ) {
				off_t size = buf.Seek(0, SEEK_END);
				void* data = malloc(size);
				if( data ) {
					buf.ReadAt(0, data, size);
					err = c->AddItem(&hand, info.type, 1, ref ? ref->name : "New Bitmap",
									 data, size, true, c->B_RENAME_NEW_ITEM);
					free(data);
				}
			}
		}
		if( err < B_OK && mime_type && strncmp(mime_type, "image/", 6) == 0 ) {
			off_t size = buf.Seek(0, SEEK_END);
			void* data = malloc(size);
			if( data ) {
				buf.ReadAt(0, data, size);
				err = c->AddItem(&hand, B_TRANSLATOR_BITMAP, 1, ref ? ref->name : "New Bitmap",
								 data, size, true, c->B_RENAME_NEW_ITEM);
				free(data);
			}
		}
		
		WriteUnlock(c);
		return err > 0 ? B_OK : err;
	}
};

extern "C" BResourceAddon* make_nth_resourcer(int32 n, image_id /*you*/, const BResourceAddonArgs& args, uint32 /*flags*/, ...)
{
	if( n == 0 ) return new IconAddon(args);
	return 0;
}
