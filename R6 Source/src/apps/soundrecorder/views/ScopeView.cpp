#include <stdio.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <Bitmap.h>
#include <stdlib.h>
#include <MediaDefs.h>
#include <math.h>
#include <ByteOrder.h>
#include <string.h>
#include <Mime.h>
#include <String.h>

#include "ScopeView.h"
#include "ResampleArray.h"
#include "RenderScope.h"
#include "AudioIcon.h"

ScopeView::ScopeView( BRect frame, BList *save_fmts, const char *name,
					  uint32 resizeMask, uint32 flags )
	: RenderView( frame, B_RGB32, name, resizeMask, flags )
{
	saveFormats = save_fmts;
	soundFile = NULL;
	fileRef = NULL;
	soundChannels = 1;
	waitingThread = -1;
	inPoint = 0;
	outPoint = 1.0;
	position = 0;
	lastPos = 0;
	duration = 0;
	drawScope = true;
	newFile = false;
	initOK = false;
	interrupt = false;
}
	
ScopeView::~ScopeView( void )
{
	
}

void ScopeView::SetTo( entry_ref *ref )
{
	drawScope = true;
	fileRef = ref;
	newFile = true;
	
	float x = (bounds.right-bounds.left)/2;
	float y = (bounds.bottom-bounds.top)/2;
	
	targetBounds.Set( x-1, y-1, x+1, y+1 );
	if( ref != NULL )
	{
		closeComplete = false;
		scopeBounds = targetBounds;
	}
	Resume();
}

void ScopeView::RenderScope( BBitmap *bmap, BRect frame )
{
	if( drawScope )
	{
		// Set all pixels to black
		for( int32 *next = (int32 *)bmap->Bits(), *last = (next + bmap->BitsLength()/sizeof(int32)); next < last; )
			*next++ = 0;
		soundChannels = 1;
		float		height = (frame.bottom - frame.top)/soundChannels;
		BRect		targetRect = frame;
		
		float		scopeGain;
		if( outPoint == inPoint ) // Avoid div/0 error
			scopeGain = 100;
		else
			scopeGain = 1/(outPoint - inPoint);
		
		if( scopeGain > (255/60) )
			scopeGain = 255/60;
		
		rgb_color 	color;
		color.red = uint8(15.0*scopeGain); 
		color.green = uint8(60.0*scopeGain); 
		color.blue = uint8(15.0*scopeGain);
		
		targetRect.bottom = height+targetRect.top;
		
		int32 firstElement = int32(float(16384) * inPoint);
		int32 lastElement = int32(float(16384) * outPoint);
		
		// Render Scope
		for( int32 i=0; i<soundChannels; i++ )
		{
			render_scope( bmap, targetRect, color, scopePoints[0]+firstElement, lastElement-firstElement );
			targetRect.OffsetBy( 0, height );
		}
	}
	else // erase last position
		RenderPosition( bmap, lastPos );
	RenderPosition( bmap, position );
	lastPos = position;
	drawScope = false;
}

void ScopeView::RenderPosition( BBitmap *bmap, float where )
{
	// Render Position
	if( (where >= 0)&&(where <= 1)&&(outPoint != inPoint) )
	{
		rgb_color	color;
		int32 height = int32(bounds.bottom - bounds.top)+1;
		int32 width = int32(bounds.right - bounds.left)+1;
		float pos = (where-inPoint)/(outPoint-inPoint);
		int32 column = int32((bounds.right - bounds.left)*pos);
		int32 rowBytes = bmap->BytesPerRow();
		uint8 *next = ((uint8 *)bmap->Bits())+(column*sizeof( rgb_color ));
		rgb_color *pixelPtr;
		
		color.red = 40;
		color.green = 255;
		color.blue = 60;
		
		// Do range check
		if( (column >= 0)&&(column < width) )
		{
			for( int32 i=0; i<height; i++, next += rowBytes )
			{
				pixelPtr = (rgb_color *)next;
				
				pixelPtr->red ^= color.red;
				pixelPtr->green ^= color.green;
				pixelPtr->blue ^= color.blue;
			}
		}
	}
}

void ScopeView::ThreadInit( void )
{
	targetBounds = bounds;
	float x = (bounds.right-bounds.left)/2;
	float y = (bounds.bottom-bounds.top)/2;
	
	scopeBounds.Set( x-20, y-1, x+20, y+1 );
	
	scopePointsPtr = (float *)malloc( 32768*sizeof(float) );
	scopePoints[0] = scopePointsPtr;
	scopePoints[1] = scopePointsPtr + 16384;
	
	closeComplete = false;
	
	CalculateScopePoints();
}

status_t ScopeView::ThreadExit( void )
{
	free( scopePointsPtr );
	if( soundFile )
		delete soundFile;
	return B_OK;
}

void ScopeView::Resize( void )
{
	RenderView::Resize();
	
	scopeBounds = targetBounds = bounds;
	drawScope = true;
}

void ScopeView::CalculateScopePoints( void )
{
	char 		*frameBuffer = NULL;
	float		*floatSampleBuf = NULL;
	float		*floatSamples[2];
	BMediaTrack	*track = NULL;
	
	try
	{
		if( soundFile && (soundFile->InitCheck() == B_NO_ERROR) )
		{
			media_format	nativeFmt, wantFmt;
			int64 			frames;
			int64			readSize;
			int32 			channels;
			int32			sampleSize;
			int32 			frameSize;
			int32			byteOrder;
			int32			sampleFormat;
			int32			framesPerBuffer = 4096;
			float 			pointsPerFrame;
			char			*framePtr;
			char			*samplePtr;
			char			*samplePtruchar;
			short			*samplePtrshort;
			float			*samplePtrfloat;
			int32			frameIndex;
			int32			channelIndex;
			int32			destElements = 0;
			int64			processedFrames = 0;
			int32			nextOffset;
			
			// Find an audio track
			for( int32 i=0; ; i++ )
			{
				if( track )
					soundFile->ReleaseTrack( track );
				if( !(track = soundFile->TrackAt( i )) )
					throw "No audio track found in file";
				track->EncodedFormat( &nativeFmt );
				// Is it audio?
				if( (nativeFmt.type == B_MEDIA_RAW_AUDIO)||(nativeFmt.type == B_MEDIA_ENCODED_AUDIO) )
					break;
			}
			// We want raw audio
			wantFmt.type = B_MEDIA_RAW_AUDIO;
			wantFmt.u.raw_audio = media_raw_audio_format::wildcard;
			// ...in float format
			wantFmt.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
			
			if( track->DecodedFormat( &wantFmt ) != B_OK )
				throw "MediaTrack track did not accept format format\n";
			
			// Get info about sound
			frames = track->CountFrames();
			duration = track->Duration();
			
			if( (channels = wantFmt.u.raw_audio.channel_count) > 2 )
				throw "More than two sound channels";
			sampleSize = int32(wantFmt.u.raw_audio.format & 0xf);
			frameSize = sampleSize * channels;
			byteOrder = wantFmt.u.raw_audio.byte_order;
			sampleFormat = wantFmt.u.raw_audio.format;
			pointsPerFrame = 16384.0/float(frames);
			
			// Setup frame buffer
			frameBuffer = (char *)malloc( framesPerBuffer*frameSize );
			floatSampleBuf = (float *)malloc( framesPerBuffer*sizeof(float)*2 );
			floatSamples[0] = floatSampleBuf;
			floatSamples[1] = floatSampleBuf + framesPerBuffer;
			
			readSize = 0;
			track->SeekToFrame( &readSize );
			
			float *spoints[2] = { scopePoints[0], scopePoints[1] };
			
			// Read frame buffers
			readSize = framesPerBuffer;
			while( track->ReadFrames( frameBuffer, &readSize ) == B_OK )
			{
				if( readSize <= 0 )
					break;
				
				if( !Running() || interrupt || newFile )
					throw "scope calculation interupted";
				// Swap byte order to host order
				if( (byteOrder == 1) && (sampleSize > 1) ) // B_BIG_ENDIAN
				{
					if( sampleFormat == media_raw_audio_format::B_AUDIO_FLOAT )
						swap_data( B_FLOAT_TYPE, frameBuffer, readSize*frameSize, B_SWAP_BENDIAN_TO_HOST );
					else if( sampleSize == 2 )
						swap_data( B_INT16_TYPE, frameBuffer, readSize*frameSize, B_SWAP_BENDIAN_TO_HOST );
				}
				else if( (byteOrder == 2) && (sampleSize > 1) ) // B_LITTLE_ENDIAN
				{
					if( sampleFormat == B_FLOAT_SAMPLES )
						swap_data( B_FLOAT_TYPE, frameBuffer, readSize*frameSize, B_SWAP_LENDIAN_TO_HOST );
					else if( sampleSize == 2 )
						swap_data( B_INT16_TYPE, frameBuffer, readSize*frameSize, B_SWAP_LENDIAN_TO_HOST );
				}
				
				// Process frames
				for( frameIndex = 0, framePtr = (char *)frameBuffer; frameIndex < int32(readSize); frameIndex++, framePtr += frameSize )
				{
					// process channels
					for( channelIndex = 0, samplePtr = framePtr; channelIndex < channels; channelIndex++, samplePtr += sampleSize )
					{
						// Convert sample value from native encoding to float encoding
						switch( sampleFormat )
						{
							case media_raw_audio_format::B_AUDIO_UCHAR:
							{
								samplePtruchar = (char *)samplePtr;
								//if( format == B_WAVE_FILE )
								*(floatSamples[channelIndex]+frameIndex) = float(*((uint8 *)samplePtruchar)-128)/127.0;
								//else
									//*(floatSamples[channelIndex]+frameIndex) = float(*samplePtruchar)/127.0;
							}
								break;
							case media_raw_audio_format::B_AUDIO_FLOAT:
							{
								samplePtrfloat = (float *)samplePtr;
								*(floatSamples[channelIndex]+frameIndex) = *samplePtrfloat;
							}
								break;
							case media_raw_audio_format::B_AUDIO_SHORT:
							{
								samplePtrshort = (short *)samplePtr;
								*(floatSamples[channelIndex]+frameIndex) = float(*samplePtrshort)/32768;
							}
								break;
						} // End switch
					}
				}
				
				// Resample To scope points
				nextOffset = int32(processedFrames*pointsPerFrame);
				
				// Fill in resample roundoff gaps
				if( (spoints[0]+destElements)<(scopePoints[0]+nextOffset) )
				{
					// Copy value from previous element
					*(spoints[0]+destElements) = *(spoints[0]+destElements-1);
					*(spoints[1]+destElements) = *(spoints[1]+destElements-1);
				}
				
				spoints[0] = scopePoints[0] + nextOffset;
				spoints[1] = scopePoints[1] + nextOffset;
				
				destElements = int32(readSize*pointsPerFrame);
				
				for( int32 i=0; i<channels; i++ )
				{
					if( (spoints[i] + destElements) >= (scopePoints[i]+16384)  )
						destElements = scopePoints[i]+16384-spoints[i];
					resample_farray( spoints[i], floatSamples[i], destElements, readSize );
				}
				processedFrames += readSize;
				readSize = framesPerBuffer;
			}
		}
		else
			throw "No Sound file";
	}
	catch( const char *err ) // create flat line
	{
		printf( "CalculateScopePoints Error: %s\n", err );
		for( int32 i=0; i<32768; i++ )
			scopePointsPtr[i] = 0;
	}
	if( frameBuffer )
		free( frameBuffer );
	if( floatSampleBuf )
		free( floatSampleBuf );
	if( track )
		soundFile->ReleaseTrack( track );
	if( soundFile )
	{
		delete soundFile;
		soundFile = NULL;
	}
}

void ScopeView::SetInPoint( float inPoint )
{
	if( inPoint > 1 )
		inPoint = 1;
	else if( inPoint < 0 )
		inPoint = 0;
	if( inPoint > outPoint )
		inPoint = outPoint;
	this->inPoint = inPoint;
	drawScope = true;
	Resume();
}

void ScopeView::SetOutPoint( float outPoint )
{
	if( outPoint > 1 )
		outPoint = 1;
	else if( outPoint < 0 )
		outPoint = 0;
	if( outPoint < inPoint )
		outPoint = inPoint;
	this->outPoint = outPoint;
	drawScope = true;
	Resume();
}

void ScopeView::SetPosition( float position )
{
	if( position > 1 )
		position = 1;
	else if( position < 0 )
		position = 0;
	else
		this->position = position;
	Resume();
}

void ScopeView::MouseDown( BPoint where )
{
	if( !initOK || !saveFormats )
		return;
		
	media_file_format		mediaFormat;
	int32					cookie = 0;
	
	BMessage		msg( B_SIMPLE_DATA );
	msg.AddString("be:types", B_FILE_MIME_TYPE );
	
	for (int32 i = 0; i < saveFormats->CountItems(); i++) {
		save_format *sf = (save_format *)saveFormats->ItemAt(i);

		msg.AddString("be:filetypes", sf->file_format.mime_type);
		msg.AddString("be:type_descriptions", sf->name.String());
	}
	
	msg.AddInt32("be:actions", B_COPY_TARGET);
	msg.AddString("be:clip_name", "Audio Clip");
	
	// Create Dragger Icon Bitmap
	BBitmap		*dragMap;
	dragMap = new BBitmap( BRect( 0, 0, 31, 31 ), B_COLOR_8_BIT );
	dragMap->SetBits( kLargeIconBits, sizeof(kLargeIconBits), 0, B_COLOR_8_BIT );
	DragMessage( &msg, dragMap, B_OP_ALPHA, BPoint( 15, 15 ), Looper() );
}

static const float kApproachRateX = 0.35, kApproachRateY = 0.1, kSlopDelta = 2;

bool ScopeView::TransformScopeBounds( void )
{
	// Are we there yet?
	if( targetBounds == scopeBounds )
		return true;
	BRect		deltaRect;
	
	drawScope = true;
	// Calculate Delta
	deltaRect.top = targetBounds.top - scopeBounds.top;
	deltaRect.bottom = targetBounds.bottom - scopeBounds.bottom;
	deltaRect.right = targetBounds.right - scopeBounds.right;
	deltaRect.left = targetBounds.left - scopeBounds.left;
	
	// Are we almost there yet?
	if( (fabsf(deltaRect.left ) <= kSlopDelta)&&(fabsf(deltaRect.top ) <= kSlopDelta)
		&&(fabsf(deltaRect.right ) <= kSlopDelta)&&(fabsf(deltaRect.bottom ) <= kSlopDelta) )
	{
		scopeBounds = targetBounds;
		return false;
	}
	
	// Approach Target Bounds
	scopeBounds.left += deltaRect.left * kApproachRateX;
	scopeBounds.top += deltaRect.top * kApproachRateY;
	scopeBounds.right += deltaRect.right * kApproachRateX;
	scopeBounds.bottom += deltaRect.bottom * kApproachRateY;
	return false;
}

void ScopeView::WaitForClose( void )
{
	interrupt = true;
	waitingThread = find_thread( NULL );
	Resume();
	suspend_thread( waitingThread );
}

void ScopeView::Render( BBitmap *bmap )
{
	if( newFile && closeComplete )
	{
		closeComplete = false;
		while( newFile && fileRef )
		{
			if( soundFile )
				delete soundFile;
			soundFile = new BMediaFile( fileRef );
			initOK = true;
			newFile = false;
			CalculateScopePoints();
			targetBounds = bounds;
			closeComplete = true;
		}
	}
	
	RenderScope( bmap, scopeBounds );
}

bool ScopeView::ShouldSuspend( void )
{
	if( TransformScopeBounds() )
	{
		if( newFile && !closeComplete )
		{
			closeComplete = true;
			targetBounds = bounds;
			if( waitingThread >= 0 )
			{
				resume_thread( waitingThread );
				waitingThread = -1;
			}
			
			if( (fileRef == NULL)&& !interrupt )
			{
				newFile = false;
				return true;
			}
			else
				return false;
		}
		else if( !newFile && !interrupt )
			return true;
	}
	return false;
}


