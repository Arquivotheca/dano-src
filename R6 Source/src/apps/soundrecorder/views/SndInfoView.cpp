#include <stdio.h>
#include <string.h>
#include <SoundFile.h>

#include "SndInfoView.h"

static const float kViewW = 170, kViewH = 120, kMarginH = 30, kMarginW = 10;

SndFInfoView::SndFInfoView( BPoint where, const char *name, uint32 resizeFlags, uint32 flags )
	: BBox( BRect( where.x, where.y, where.x+kViewW, where.y+kViewH ), name, resizeFlags, flags, B_FANCY_BORDER )
{
	SetLabel( "File Info" );
	sndFile = new BSoundFile;
}

SndFInfoView::~SndFInfoView( void )
{
	if( sndFile )
		delete sndFile;
}

status_t SndFInfoView::SetTo( entry_ref *ref )
{
	char		s[256];
	int32		format;
	
	sndFile->SetTo( ref, B_READ_ONLY );
	fileNameS.SetTo( ref->name );
	format = sndFile->FileFormat();
	switch( format )
	{
		case B_AIFF_FILE:
			formatS.SetTo( "AIFF" );
			break;
		case B_WAVE_FILE:
			formatS.SetTo( "WAVE" );
			break;
		case B_UNIX_FILE:
			formatS.SetTo( "UNIX" );
			break;
		default:
			formatS.SetTo( "Unknown" );
			break;
	}
	if( sndFile->IsCompressed() )
		compressionS.SetTo( sndFile->CompressionName() );
	else
		compressionS.SetTo( "None" );
	sprintf( s, "%ld", sndFile->CountChannels() );
	channelsS.SetTo( s );
	sprintf( s, "%ld bits", sndFile->SampleSize()*8 );
	sampleSizeS.SetTo( s );
	sprintf( s, "%ld Hz", sndFile->SamplingRate() );
	sampleRateS.SetTo( s );
	
	int32		frames;
	
	frames = sndFile->CountFrames();
	//int32		seconds = int32(float(frames)/float(sndFile->SamplingRate()));
	//int32		minutes = seconds / 60;
	//int32		hours = seconds / 3600;
	//seconds %= 60;
	
	sprintf( s, "%.2f seconds", float(frames)/float(sndFile->SamplingRate()) );
	lengthS.SetTo( s );
	Looper()->Lock();
	FillRect( Bounds(), B_SOLID_LOW );
	Draw( Bounds() );
	Looper()->Unlock();
	
	return B_OK;
}

void SndFInfoView::Unset( void )
{
	this->sndFile = NULL;
}

void SndFInfoView::Draw( BRect bounds )
{
	PushState();
	
	//BFont		font( be_plain_font );
	//font.SetSize( font.Size() +1 );
	
	SetFont( be_plain_font );
	font_height		fheight;
	
	float			x = kMarginW, y = kMarginW+20;
	
	BFont		renderFont;
	renderFont = *be_plain_font;
	renderFont.SetSize( 10 );
	renderFont.GetHeight( &fheight );
	SetFont( &renderFont );
	
	fheight.leading = fheight.ascent + fheight.descent+1;
	
	MovePenTo( x, y );
	DrawString( "File Name: " ); DrawString( fileNameS.String() );
	y += fheight.leading; MovePenTo( x, y );
	DrawString( "Format: " ); DrawString( formatS.String() );
	y += fheight.leading; MovePenTo( x, y );
	DrawString( "Compression: " ); DrawString( compressionS.String() );
	y += fheight.leading; MovePenTo( x, y );
	DrawString( "Channels: " ); DrawString( channelsS.String() );
	y += fheight.leading; MovePenTo( x, y );
	DrawString( "Sample Size: " ); DrawString( sampleSizeS.String() );
	y += fheight.leading; MovePenTo( x, y );
	DrawString( "Rate: " ); DrawString( sampleRateS.String() );
	y += fheight.leading; MovePenTo( x, y );
	DrawString( "Duration: " ); DrawString( lengthS.String() );
	
	PopState();
	BBox::Draw( bounds );
}
