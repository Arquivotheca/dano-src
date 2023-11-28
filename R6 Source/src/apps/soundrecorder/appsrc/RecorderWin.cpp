#include <stdio.h>
#include <Application.h>
#include <Box.h>
#include <ListView.h>
#include <List.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Entry.h>
#include <Directory.h>
#include <NodeInfo.h>
#include <MessageQueue.h>
#include <Messenger.h>
#include <MediaRoster.h>
#include <Path.h>
#include <TimeSource.h>
#include <FindDirectory.h>
#include <File.h>
#include <Alert.h>
#include <ByteOrder.h>
#include <math.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <FilePanel.h>
#include <Volume.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ParameterWeb.h>

#include "RecorderWin.h"
#include "NuTransportView.h"
#include "FileListView.h"
#include "SndInfoView.h"
#include "MediaTrackController.h"
#include "SoundConsumer.h"
#include "ScopeView.h"
#include "KnobSwitch.h"
#include "LevelMeter.h"
#include "LoopbackBM.h"
#include "BitmapButton.h"
#include "StatusWindow.h"

static const float kIWinW = 400, kTrasH = 175, kIWinH = 136+kTrasH;
static const float kListW = 200, kMarginW = 10, kMarginHT = 15, kMarginHB = 10,
	kScrollW = 14, kLabelH = 16, kLabelMarginH = 5, kKnobWidth = 16, kKnobHeight = 20, kMeterWidth=40;

static const off_t kMinFreeBytes = 5000000LL;

enum {
	MSG_FILE_SELECT = B_SPECIFIERS_END+1,
	MSG_STOP_RECORDING,
	MSG_LOOP
};


class ConfigWindow: public BWindow
{
	public:
		ConfigWindow(BView *view, BWindow *parent)
			: BWindow(view->Bounds(),"Encoder settings",
				B_FLOATING_WINDOW_LOOK,B_MODAL_SUBSET_WINDOW_FEEL,
				B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
		{
			AddChild(view);
			AddToSubset(parent);
			MoveTo(parent->Frame().LeftTop());
			Show();
		}
};


RecorderWin::RecorderWin( BPoint where )
	: BWindow( BRect(where.x, where.y, where.x+kIWinW, where.y+kTrasH), "Sound Recorder", 
	B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_NOT_V_RESIZABLE )
{
	mediaController = NULL;
	recordNode = NULL;
	recordFile = NULL;
	saveFile = NULL;
	saveTrack = NULL;
	recordRef = NULL;
	currentRef = NULL;
	knob = NULL;
	recording = false;
	hasInput = false;
	hasOutput = false;
	tempFileList = new BList;
	isZoomed = false;
	SetSizeLimits( kIWinW, 5000, kTrasH, kIWinH );
	
	InitSaveFormats();

	filePanel = new BFilePanel( B_SAVE_PANEL, new BMessenger( this, this ) );

	// Add a menu to the file panel to allow the user to select
	// the file format to save in.
	formatMenu = new BMenu("File Format");
	formatMenu->SetRadioMode(true);
	for (int32 i = 0; i < saveFormats->CountItems(); i++) {
		save_format *sf = (save_format *)saveFormats->ItemAt(i);
		BString label(sf->name);

		BMediaFile file(&sf->file_format);
		media_format format;
		BMediaTrack *track=file.CreateTrack(&format, &sf->codec_info);
		if(track)
		{
			BView *webview = track->GetParameterView();
			if(webview)
			{
				label << B_UTF8_ELLIPSIS;
				delete webview;
			}
			file.ReleaseTrack(track);   // This prevents MediaTrack::Flush from being called when the mediafile
										// is deleted, which saves us from getting blamed for a bug in the Vorbis
										// encoder.
		}
		BMenuItem *menuitem;
		formatMenu->AddItem(menuitem=new BMenuItem(label.String(), new BMessage('conf')));
		if	(defaultSaveFormat->file_format.id==sf->file_format.id &&
			 defaultSaveFormat->codec_info.id==sf->codec_info.id)
			menuitem->SetMarked(true);
	}
	formatMenu->SetTargetForItems(this);
	
	BMenuBar *mb = dynamic_cast<BMenuBar *>(filePanel->Window()->FindView("MenuBar"));
	if (mb)
		mb->AddItem(formatMenu);
	
	BWindow::Zoom();
	InitChildren();
	BWindow::Zoom();
	mInitStatus = InitMedia();
	if( !hasInput ) // If no input, disable record button
		transport->EnableRecord( false );
	Show();
	SetupSoundInput();
}

RecorderWin::~RecorderWin( void )
{
	if( mediaController )
		mediaController->Delete(this);
	if( recordRef )
		delete recordRef;
	if( recordFile )
		delete recordFile;
	if( currentRef )
		delete currentRef;
	if( filePanel )
		delete filePanel;
	if( saveFile )
		delete saveFile;
		
	// Remove Temp Files
	entry_ref		*ref;
	BEntry			entry;
	for( int32 i=0; (ref=(entry_ref *)tempFileList->ItemAt(i)); i++ )
	{
		if( entry.SetTo( ref ) == B_OK )
			entry.Remove();
		delete ref;
	}
	delete tempFileList;

	// Delete cached save formats
	for (int32 i = 0; i < saveFormats->CountItems(); i++) {
		delete (save_format *)saveFormats->ItemAt(i);
	}
	delete saveFormats;
}

bool RecorderWin::QuitRequested( void )
{
	scopeView->SetTo( NULL );
	Unlock();
	scopeView->WaitForClose();
	Lock();
	TeardownSoundInput();
	be_app->PostMessage( B_QUIT_REQUESTED );
	return true;
}

status_t 
RecorderWin::InitSaveFormats(void)
{
	saveFormats = new BList();
	
	media_file_format mff;
	media_codec_info mci;
	media_format in_f, out_f;
	int32 cookie = 0, cookie2;

	in_f.type = B_MEDIA_RAW_AUDIO;
	in_f.u.raw_audio = media_raw_audio_format::wildcard;
	while (get_next_file_format(&cookie, &mff) == B_OK) {
		// Skip video formats like AVI & QuickTime
		if (mff.capabilities & (media_file_format::B_KNOWS_RAW_VIDEO |
								media_file_format::B_KNOWS_ENCODED_VIDEO)) {
			continue;
		}
		
		cookie2 = 0;
		while (get_next_encoder(&cookie2, &mff, &in_f, &out_f, &mci) == B_OK) {
			save_format *sf = new save_format;
			sf->name << mff.short_name << "/" << mci.pretty_name;
			sf->file_format = mff;
			sf->codec_info = mci;
			saveFormats->AddItem(sf);
			if (mff.family == B_WAV_FORMAT_FAMILY && out_f.type == B_MEDIA_RAW_AUDIO)
				defaultSaveFormat = sf;
		}
	}
}

status_t RecorderWin::InitChildren( void )
{
	BRect		frame;
	
	// Backgroud
	BView *background = new BView( Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS );
	background->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	AddChild( background );
	
	// Transport Group
	frame = Bounds();
	frame.bottom = kTrasH;
	BBox *transportBox = new BBox( frame, "", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER );
	background->AddChild( transportBox );
	
	// Transport Controls
	transport = new TransportView( BRect( 2, transportBox->Bounds().bottom-70, transportBox->Bounds().right-10-kKnobWidth, transportBox->Bounds().bottom-2 ) );
  	transportBox->AddChild( transport );
  	transport->EnablePlay( false );
  	
  	// Scope View
  	scopeView = new ScopeView( BRect( kMeterWidth+2, 2, transportBox->Bounds().right-2, transportBox->Bounds().bottom-71  ), saveFormats );
  	transportBox->AddChild( scopeView );
  	
  	// Level Meter
  	meter = new LevelMeter( BRect( 2, 2, kMeterWidth+1, transportBox->Bounds().bottom-71) );
  	transportBox->AddChild( meter );
  	
  	// Sound File Info View
	infoView = new SndFInfoView( BPoint( kMarginW, kTrasH+kMarginHT-7 ), "" );
	background->AddChild( infoView );
	
	// File List
	frame.Set( kIWinW-kListW-kMarginW, kMarginHT+kTrasH, kIWinW-kMarginW-kScrollW, kIWinH-kMarginHB );
	BScrollView	*scrollView;
	fileListView = new FListView( frame, "", B_FOLLOW_ALL_SIDES );
	scrollView = new BScrollView( "", fileListView, B_FOLLOW_TOP_BOTTOM | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, false, true, B_FANCY_BORDER );
	background->AddChild( scrollView );
	fileListView->SetSelectionMessage( new BMessage( MSG_FILE_SELECT ) );
	
	// Knob Switch
	frame.Set( transportBox->Bounds().right-5-kKnobWidth, transportBox->Bounds().bottom-kKnobHeight-5, 0, 0 );
	knob = new KnobSwitch( frame, "", B_FOLLOW_TOP | B_FOLLOW_RIGHT );
	knob->ResizeToPreferred();
	knob->SetMessage( new BMessage( B_ZOOM ) );
	transportBox->AddChild( knob );
	
	// Loopback Control
	BPoint where( transportBox->Bounds().right-3-kLoopBMWidth, transportBox->Bounds().bottom-kKnobHeight-14-(kLoopBMHeight/6) );
	loopButton = new BitmapCheckBox( where, "", kLoopBMBits, kLoopBMWidth, kLoopBMHeight/6, new BMessage( MSG_LOOP ), 
		B_FOLLOW_TOP | B_FOLLOW_RIGHT );
	loopButton->SetValue( true );
	transportBox->AddChild( loopButton );
	
	return B_OK;
}

void RecorderWin::Zoom(BPoint, float , float  )
{
	if( isZoomed )
		ResizeTo( Bounds().right, kTrasH );
	else
		ResizeTo( Bounds().right, kIWinH );
	isZoomed = !isZoomed;
	
	if( knob && (isZoomed != knob->Value()) )
		knob->SetValue( isZoomed );
}

status_t RecorderWin::InitMedia( void )
{
	status_t		status;
	
	// Get Media Roster
	m_roster = BMediaRoster::Roster( &status );
	if( !m_roster )
		return status;
	
	// Get Input Node
	if( (status = m_roster->GetAudioInput( &audioInput )) != B_OK )
		hasInput = false;
	else
		hasInput = true;
	
	media_node outNode; // MediaNode.h
	
	// Check for audio output
	if( (status = m_roster->GetAudioOutput( &outNode )) != B_OK )
		hasOutput = false;
	else
	{
		m_roster->ReleaseNode( outNode );
		hasOutput = true;
	}
	
	recordRef = new entry_ref;
	
	return B_OK;
}


void RecorderWin::ConfigureCodec()
{
	BMenuItem *mi = formatMenu->FindMarked();
	static int32 lastindex=-1;
	int32 index=formatMenu->IndexOf(mi);

	if(index!=lastindex || !saveFile)
	{
		lastindex=index;
		if(saveFile)
			delete saveFile;
		save_format *sf = (save_format *)saveFormats->ItemAt(index);
		saveFile=new BMediaFile(&sf->file_format);
		media_format format;
		saveTrack=saveFile->CreateTrack(&format, &sf->codec_info);
	}

	if(saveTrack)
	{
		BView *webview = saveTrack->GetParameterView();
		if(webview)
		{
			// show view in window, for user-configuration
			new ConfigWindow(webview,filePanel->Window());
			return; // don't close file!!
		}
	}
	delete saveFile;
	saveFile=NULL;
	saveTrack=NULL;
}

void RecorderWin::MessageReceived( BMessage *msg )
{
	switch( msg->what )
	{
		case 'conf':
			ConfigureCodec();
			break;

		case MSG_TRANSPORT_UPDATE:
			UpdateTransport();
			break;
		case B_COPY_TARGET:
			ActionCopy( msg );
			break;
		case 1145132097: // Tracker Refs...
		case B_REFS_RECEIVED:
			RefsReceived( msg );
			break;
		case MSG_FILE_SELECT:
			FileSelected( msg );
			break;
		case kPlayPause:
			Play( msg );
			break;
		case M_DONE_PLAYING:
			transport->SetButtonState( kStopped );
			break;
		case kStop:
			Stop( msg );
			break;
		case kFastForward:
			ScanForward();
			break;
		case kRewind:
			ScanBackward();
			break;
		case kNudgeForward:
			NudgeForward();
			break;
		case kNudgeBackward:
			NudgeBackward();
			break;
		case kStartScrub:
			StartScrubbing();
			break;
		case kStopScrub:
			StopScrubbing();
			break;
		case kScrub:
		{
			scopeView->SetPosition( transport->GetPosition( kNowThumb ) );
			if( !mediaController )
			{
				bigtime_t newTime = bigtime_t(transport->GetPosition( kNowThumb ) * float(scopeView->Duration()));
				transport->UpdateTime( newTime, kNowThumb );
			}
			break;
		}
		case kInPointChanged:
			InPointChanged();
			break;
		case kOutPointChanged:
			OutPointChanged();
			break;
		case kVolumeChanged:
			VolumeChanged();
			break;
		case kRecord:
			Record( msg );
			break;
		case MSG_STOP_RECORDING:
			RecordComplete();
			break;
		case kSave:
			filePanel->Show();
			break;
		case B_SAVE_REQUESTED:
			Save( msg );
			break;
		case MSG_LOOP:
			if( mediaController )
				mediaController->SetAutoLoop( !loopButton->Value() );
			break;	
		default:
			BWindow::MessageReceived( msg );
			break;
	}
}

void RecorderWin::ActionCopy( BMessage *request )
{
	const char *fmt = NULL;
	const char * name;
	entry_ref dir, ref;
	
	if (!request->FindString("name", &name) &&
		!request->FindRef("directory", &dir))
	{
		BDirectory d( &dir );
		BEntry entry( &d, name );
		entry.GetRef( &ref );
		
		request->FindString("be:type_descriptions", &fmt);
		SaveSound( &ref, currentRef, fmt );
	}
}

void RecorderWin::RefsReceived( BMessage *msg )
{
	msg->what = B_REFS_RECEIVED;
	fileListView->MessageReceived( msg );
}

void RecorderWin::FileSelected( BMessage *msg )
{	
	int32			index = msg->FindInt32( "index" );
	FileListItem	*item;
	
	if( (item = (FileListItem *)fileListView->ItemAt( index )) )
	{
		status_t result;
		
		if( hasOutput )
		{
			// Try to open new media file
			MediaController	*newController = MediaTrackController::Open( &item->ref, this, &result );
			
			// Was the init good?
			if( !newController || ((result = newController->InitCheck())) != B_NO_ERROR )
			{
				ErrorAlert( "Could not open media file for playback!\n", B_OK );
				if( newController )
					newController->Delete(this);
	
				// Remove item from file list
				fileListView->RemoveItem( index );
				delete item;
				return;
			}
			
			// Stop recording if recording
			if( recording )
				RecordComplete();
			else
			{
				if( mediaController && (mediaController->IsPlaying()||mediaController->IsPaused()) )
					mediaController->Stop();
				transport->SetButtonState( kStopped );
			}
			
			// Delete previous controller
			if( mediaController )
				mediaController->Delete(this);
			
			mediaController = newController;
		}
		
		
		scopeView->SetTo( &item->ref );
		
		scopeView->SetPosition( 0 );
		scopeView->SetInPoint( 0 );
		scopeView->SetOutPoint( 1.0 );
		
		transport->UpdatePosition( 0, kStartThumb );
		transport->UpdatePosition( 0, kNowThumb );
		transport->UpdatePosition( 1.0, kStopThumb );
		
		infoView->SetTo( &item->ref );
		fileListView->MakeFocus( true );
		if( !currentRef )
			currentRef = new entry_ref;
		*currentRef = item->ref;
		
		if( hasOutput )
		{
			// Wait until ready to play
			bigtime_t		startTime = system_time();
			while( !mediaController->ReadyToPlay() && (system_time() < startTime + 1000000) ) { snooze( 50000 ); }
			mediaController->SetAutoLoop( !loopButton->Value() );
			mediaController->SetTarget( this, this );
			transport->EnablePlay( true );
			transport->SetVolume( mediaController->Volume() );
		}
	}
}

void RecorderWin::Play( BMessage * )
{
	if( mediaController->IsPlaying() )
	{
		transport->SetButtonState( kPaused );
		mediaController->Pause();
	}
	else if( mediaController->ReadyToPlay() )
	{
		transport->SetButtonState( kPlaying );
		mediaController->SetVolume( transport->GetVolume() );
		mediaController->Play();
	}
}

void RecorderWin::Stop( BMessage * )
{
	if( recording )
		RecordComplete();
	else
	{
		if( mediaController->IsPlaying()||mediaController->IsPaused() )
			mediaController->Stop();
		transport->SetButtonState( kStopped );
	}
}

void RecorderWin::UpdateTransport( void )
{
	if( mediaController && (mediaController->InitCheck() == B_OK) )
	{
		bigtime_t position = mediaController->Position();
		
		if( position != lastPos )
		{
			float pos = float(position)/float(mediaController->Length());
			transport->UpdatePosition( pos, kNowThumb );
			transport->UpdateTime( position, kNowThumb );
			scopeView->SetPosition( pos );
			lastPos = position;
		}
		
		position = mediaController->InPoint();
		if( position != lastIn )
		{
			float pos = float(position)/float(mediaController->Length());
			scopeView->SetInPoint( pos );
			lastIn = position;
		}
		
		position = mediaController->OutPoint();
		if( position != lastOut )
		{
			float pos = float(position)/float(mediaController->Length());
			scopeView->SetOutPoint( pos );
			lastOut = position;
		}
	}
}

void RecorderWin::ScanForward( void )
{
	if( mediaController && (mediaController->InitCheck() == B_OK) )
		mediaController->ScanForward();
}

void RecorderWin::ScanBackward( void )
{
	if( mediaController && (mediaController->InitCheck() == B_OK) )
		mediaController->ScanBackward();
}

void RecorderWin::NudgeForward( void )
{
	if( mediaController && (mediaController->InitCheck() == B_OK) )
		mediaController->NudgeForward();
}

void RecorderWin::NudgeBackward( void )
{
	if( mediaController && (mediaController->InitCheck() == B_OK) )
		mediaController->NudgeBackward();
}

void RecorderWin::StartScrubbing( void )
{
	if( mediaController && (mediaController->InitCheck() == B_OK) )
	{
		BMessenger		msngr( transport->GetMediaSlider(), this );
		mediaController->StartScrubbing( msngr );
	}
}

void RecorderWin::StopScrubbing( void )
{
	if( mediaController && (mediaController->InitCheck() == B_OK) )
		mediaController->StopScrubbing();
}

void RecorderWin::InPointChanged( void )
{
	scopeView->SetInPoint( transport->GetPosition( kStartThumb ) );
	bigtime_t newTime = bigtime_t(transport->GetPosition( kStartThumb ) * float(scopeView->Duration()));
	transport->UpdateTime( newTime, kStartThumb );
	
	if( mediaController && (mediaController->InitCheck() == B_OK) )
	{
		BMessage *others;
		while( true )
		{
			others = MessageQueue()->FindMessage( kInPointChanged, 0 );
			if( !others )
				break;
			MessageQueue()->RemoveMessage(others);
			delete others;
		}
		if( (mediaController->IsPlaying()||mediaController->IsPaused()) &&
		(transport->GetPosition( kStartThumb )+0.005 >= transport->GetPosition( kStopThumb )) )
		{
			mediaController->Stop();
			transport->SetButtonState( kStopped );
		}
		mediaController->SetInPoint( newTime );
	}
}

void RecorderWin::OutPointChanged( void )
{
	scopeView->SetOutPoint( transport->GetPosition( kStopThumb ) );
	bigtime_t newTime = bigtime_t(transport->GetPosition( kStopThumb ) * float(scopeView->Duration()));
	transport->UpdateTime( newTime, kStopThumb );
	
	if( mediaController && (mediaController->InitCheck() == B_OK) )
	{
		BMessage *others;
		while( true )
		{
			others = MessageQueue()->FindMessage( kOutPointChanged, 0 );
			if( !others )
				break;
			MessageQueue()->RemoveMessage(others);
			delete others;
		}
		if( (mediaController->IsPlaying()||mediaController->IsPaused()) &&
		(transport->GetPosition( kStartThumb )+0.005 >= transport->GetPosition( kStopThumb )) )
		{
			mediaController->Stop();
			transport->SetButtonState( kStopped );
		}
		mediaController->SetOutPoint( newTime );
	}
}

void RecorderWin::VolumeChanged( void )
{
	BMessage *others;
	while( true )
	{
		others = MessageQueue()->FindMessage( kVolumeChanged, 0 );
		if( !others )
			break;
		MessageQueue()->RemoveMessage(others);
		delete others;
	}
	if( mediaController && (mediaController->InitCheck() == B_OK) )
		mediaController->SetVolume( transport->GetVolume() );
}

void RecorderWin::RecordFile( void * cookie, bigtime_t, void *data, size_t size, const media_raw_audio_format & )
{
	//	Callback called from the SoundConsumer when receiving buffers.
	RecorderWin *window = (RecorderWin *)cookie;
	
	int32 sampleSize = int32(window->m_fmt.u.raw_audio.format & 0xf);
	int32 frameSize = sampleSize * window->m_fmt.u.raw_audio.channel_count;
	int32 totalFrames = int32(size/frameSize);
	
	//	Write the data to file (we don't buffer or guard file access or anything)
	if( window->recording && window->recordTrack )
	{
		if( window->recordVol->FreeBytes() < kMinFreeBytes )
		{
			window->PostMessage( MSG_STOP_RECORDING );
			window->recording = false;
		}
		window->recordTrack->WriteFrames( data, totalFrames );
	}
	
	// Find peak level
	// Not safe if the input is not 16bit stereo... This needs to be fixed... but not now
	
	int16	high[2] = { 0, 0 };
	int16	*framePtr, *lastFrame;
	for( framePtr = (int16 *)data, lastFrame = framePtr+(totalFrames*2); framePtr<lastFrame; framePtr += 2 )
	{
		if( framePtr[0] > high[0] )
			high[0] = framePtr[0];
		if( framePtr[1] > high[1] )
			high[1] = framePtr[1];
	}
	
	window->meter->SetLevel( 0, float(abs(high[0]))/32767.0 );
	window->meter->SetLevel( 1, float(abs(high[1]))/32767.0 );
	
	window->lastBuffer = false;
}

void RecorderWin::NotifyRecordFile( void * cookie, int32 code, ... )
{
	if ((code == B_WILL_STOP) || (code == B_NODE_DIES))
	{
		RecorderWin *window = (RecorderWin *)cookie;
		// Tell the window we've stopped, if it doesn't already know.
		if( window->recording )
			window->PostMessage( MSG_STOP_RECORDING );
	}
}

void RecorderWin::Record( BMessage * )
{
	if( recording )
	{
		RecordComplete();
		return;
	}
	status_t		status = B_OK;
	
	try
	{
		media_codec_info	codecInfo = defaultSaveFormat->codec_info;
		media_file_format	mediaFileFormat = defaultSaveFormat->file_format;
		
		if( recordFile )
			delete recordFile;
		
		// Create new temp file
		if( (status = NewTempFile( recordRef, "Audio Clip" )) != B_OK )
			throw "Could not create temporary file for new recording.";
		
		// Check disk space
		recordVol = new BVolume( recordRef->device );
		if( recordVol->FreeBytes() < kMinFreeBytes )
			throw "There is not enough disk space to record!";
		
		// Open media file
		if( (recordFile = new BMediaFile( recordRef, &mediaFileFormat ))->InitCheck() != B_OK )
			throw "Could not open media file!";
		
		// Create media track
		if( !(recordTrack = recordFile->CreateTrack( &m_fmt, &codecInfo )) )
			throw "Could not create audio track.";
		
		// Burn-in new header
			recordFile->CommitHeader();
	}
	catch( const char *errorStr )
	{
		if( recordFile )
		{
			if( recordTrack )
			{
				recordFile->ReleaseTrack( recordTrack );
				recordTrack = NULL;
			}
			delete recordFile;
			recordFile = NULL;
		}
		return;
	}
	catch(...)
	{
		ErrorAlert( "Unknow Exception", status );
		return;
	}
	recording = true;
	
	// Set button state to reflect recording status
	transport->EnablePlay( false );
	transport->SetButtonState( kRecording );
}

status_t RecorderWin::NewTempFile( entry_ref *ref, const char *nameHint )
{
	BPath		tempPath;
	BPath		filePath;
	BEntry		entry;
	BFile		file;
	status_t	status;
	char		candidateName[256];
	
	if( (status = find_directory( B_COMMON_TEMP_DIRECTORY, &tempPath )) != B_OK )
		return status;
	
	filePath.SetTo( tempPath.Path(), nameHint );
	// Look for a unique name
	for( int32 i=1; true; i++ )
	{
		if( (status = file.SetTo( filePath.Path(), B_READ_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS )) == B_OK )
			break;
		else if( status != B_FILE_EXISTS )
			return status;
		sprintf( candidateName, "%s %ld", nameHint, i );
		filePath.SetTo( tempPath.Path(), candidateName );
	}
	if( (status = entry.SetTo( filePath.Path() )) != B_OK )
		return status;
	entry.GetRef( ref );
	return B_OK;
}

void RecorderWin::ErrorAlert( const char *action, status_t err )
{
	char msg[300];
	if( err == B_OK )
		sprintf( msg, "%s", action );
	else
		sprintf( msg, "%s.\n%s[%lx]", action, strerror(err), (int32) err);
	(new BAlert( "", msg, "Stop" ))->Go();
}

void RecorderWin::RecordComplete( void )
{
	// Break Record Connections
	lastBuffer = true;
	recording = false;
	
	while( lastBuffer ) // Wait for the last buffer
		snooze( 20000 );
	
	// Close the file
	recordFile->CloseFile();
	delete recordFile;
	recordFile = NULL;
	
	// Delete the record volume
	delete recordVol;
	
	// Set MIME type
	BNode		node( recordRef );
	BNodeInfo 	info( &node );
	info.SetType( defaultSaveFormat->file_format.mime_type );
	
	// Update Buttons
	transport->SetButtonState( kStopped );
	
	// Add new sound to list
	BMessage		msg( B_REFS_RECEIVED );
	msg.AddRef( "refs", recordRef );
	fileListView->MessageReceived( &msg );
	
	// Add file to temp file list
	tempFileList->AddItem( new entry_ref( *recordRef ) );
}

status_t RecorderWin::SetupSoundInput( void )
{
	if( !hasInput )
		return B_ERROR;
	
	status_t		status = B_OK;
	tsobj = 0;
	
	try
	{
		// Create Record Node and register it
		recordNode = new SoundConsumer( "Sound Recorder" );
		if( (status = m_roster->RegisterNode( recordNode )) != B_OK )
			throw "Could not create record node!";
		
		//	Find an available output for the given input node.
		int32 count = 0;
		if( (status = m_roster->GetFreeOutputsFor( audioInput, &m_audioOutput, 1, &count, B_MEDIA_RAW_AUDIO )) != B_OK )
			throw "get free outputs from audio input node";
		if( count < 1 )
			throw "no free outputs from audio input node";
		
		//	Find an available input for our own Node.
		if( (status = m_roster->GetFreeInputsFor( recordNode->Node(), &m_recInput, 1, &count, B_MEDIA_RAW_AUDIO )) != B_OK )
			throw "get free inputs for sound recorder";
		if( count < 1 )
			throw "no free inputs for sound recorder";
		
		//	Find out what the time source of the input is.
		media_node use_time_source;
		tsobj = m_roster->MakeTimeSourceFor( audioInput );
		if( !tsobj )
			throw "clone time source from audio input node";
		
		// Apply the time source in effect to our own Node.
		if( (status = m_roster->SetTimeSourceFor( recordNode->Node().node, tsobj->Node().node)) != B_OK )
			throw "set the sound recorder's time source";
		
		//	Get a format, any format.
		m_fmt.u.raw_audio = m_audioOutput.format.u.raw_audio;
		m_fmt.type = B_MEDIA_RAW_AUDIO;
		
		//	Tell the consumer where we want data to go.
		if( (status = recordNode->SetHooks( RecordFile, NotifyRecordFile, this )) != B_OK )
			throw "set the sound recorder's hook functions";
		
		// Connect Audio Source to our recorder node
		if( (status = m_roster->Connect( m_audioOutput.source, m_recInput.destination, &m_fmt, &m_audioOutput, &m_recInput)) != B_OK )
			throw "connect sound recorder to audio input node.";
		
		//	Start the time source if it's not running.
		if ((tsobj->Node() != audioInput) && !tsobj->IsRunning())
			m_roster->StartNode( tsobj->Node(), BTimeSource::RealTime() );
		
		tsobj->Release();	//	we're done with this time source instance!
		
		//	Start Recording
		bigtime_t then = recordNode->TimeSource()->Now()+50000LL;
		m_roster->StartNode( recordNode->Node(), then );
		if( audioInput.kind & B_TIME_SOURCE )
			m_roster->StartTimeSource( audioInput, recordNode->TimeSource()->RealTimeFor(then, 0) );
		else
			m_roster->StartNode( audioInput, then );
	}
	catch( const char *errorStr )
	{
		if( tsobj )
			tsobj->Release();
		ErrorAlert( errorStr, status );
		return status;;
	}
	catch(...)
	{
		if( tsobj )
			tsobj->Release();
		ErrorAlert( "Unknow Exception", status );
		return B_ERROR;
	}
	return B_OK;
}

void RecorderWin::TeardownSoundInput( void )
{
	if( hasInput )
	{
		status_t		status;
		
		status = m_roster->StopNode( m_recInput.node, 0 );
		status = m_roster->Disconnect( m_audioOutput.node.node, m_audioOutput.source, m_recInput.node.node, m_recInput.destination );
		m_audioOutput.source = media_source::null;
		m_recInput.destination = media_destination::null;
		
		// recordNode->Release();
		m_roster->ReleaseNode( recordNode->Node() );
		m_roster->ReleaseNode( audioInput );
	}
}

void RecorderWin::Save( BMessage *msg )
{
	const char *name;
	const char *fmt;
	entry_ref dir;

	BMenuItem *mi = formatMenu->FindMarked();
	if (!mi)
		fmt = defaultSaveFormat->name.String();
	else
		fmt = mi->Label();
		
	if (!msg->FindString("name", &name) &&
		!msg->FindRef("directory", &dir))
	{
		BDirectory d( &dir );
		
		// Check if it's a new file
		entry_ref	*item;
		for( int32 i=0; (item=(entry_ref *)tempFileList->ItemAt(i)); i++ )
		{
			if (*item == *currentRef && defaultSaveFormat->name.Compare(fmt) == 0)
			{
				status_t status;
				BEntry tempFile( currentRef );
				// Remove item from temp list
				tempFileList->RemoveItem( i );
				
				
				// Move it to the new location
				status = tempFile.MoveTo( &d, name, true );
				
				// Update entry in fileListView
				FileListItem *fItem;
				for( int32 j=0; (fItem=(FileListItem *)fileListView->ItemAt(j)); j++ )
				{
					if( fItem->ref == *currentRef )
					{
						tempFile.GetRef( &fItem->ref );
						fItem->SetText( fItem->ref.name );
						fileListView->InvalidateItem(j);
						fileListView->SortItems();
						break;
					}
				}
				
				// Update the current ref
				tempFile.GetRef( currentRef );
				return;
			}
		}
		
		// If not, copy the file
		BEntry entry( &d, name );
		
		entry_ref ref;
		entry.GetRef( &ref );

		SaveSound( &ref, currentRef, fmt );
	}
}

struct save_cookie {
public:
	save_cookie(RecorderWin *win, entry_ref d, entry_ref s, const char *fmt) {
		window = win;
		dst = d;
		src = s;
		if (fmt) {
			file_fmt = new char[strlen(fmt)+1];
			strcpy(file_fmt, fmt);
		}
		else
			file_fmt = NULL;
	}
	
	~save_cookie() {
		delete[] file_fmt;
	}
	
	RecorderWin	*window;
	entry_ref	dst;
	entry_ref	src;
	char		*file_fmt;
};


int32
RecorderWin::_save_thread_(void *data)
{
	save_cookie *s = (save_cookie *)data;
	return s->window->SaveThread(data);
}

int32
RecorderWin::SaveThread(void *data)
{
	save_cookie *cookie = (save_cookie *)data;
	BMediaFile *srcFile = NULL, *dstFile = NULL;;
	BMediaTrack	*srcTrack = NULL, *dstTrack = NULL;
	media_format srcMFmt, dstMFmt;
	entry_ref *src = &(cookie->src);
	entry_ref *dst = &(cookie->dst);
	const char *file_fmt = cookie->file_fmt;
	
	BRect r(0, 0, 250, 25);
	r.OffsetTo(Frame().LeftTop() + BPoint(30, 30));
	StatusWindow *statusWindow = new StatusWindow(r, "Saving...");
	statusWindow->SetTitle("Saving...");
	
	if (file_fmt) {
		char title[256];
		sprintf(title, "Saving as %s...", file_fmt);
		statusWindow->Reset(title);
	} else {
		statusWindow->Reset("Saving file...");
	}
	
	statusWindow->Show();

	try
	{
		if( currentRef )
		{
			// Check disk space
			BVolume	destVol( dst->device );
			BNode srcNode( currentRef );
			off_t srcSize;
			srcNode.GetSize( &srcSize );
			
			if( destVol.FreeBytes() < srcSize + kMinFreeBytes )
				throw "There is not enough disk space to save!";
			
			// ***
			// Setup source file
			// ***
			if( (srcFile = new BMediaFile( src ))->InitCheck() != B_OK )
				throw "Source file failed InitCheck.";
			
			// Find src audio track
			for( int32 i=0; ; i++ )
			{
				if( srcTrack )
					srcFile->ReleaseTrack( srcTrack );
				if( !(srcTrack = srcFile->TrackAt( i )) )
					throw "No audio track found in file";
				srcTrack->EncodedFormat( &srcMFmt );
				// Is it audio?
				if( (srcMFmt.type == B_MEDIA_RAW_AUDIO)||(srcMFmt.type == B_MEDIA_ENCODED_AUDIO) )
					break;
			}
			
			// Setup source audio format
			srcMFmt.type = B_MEDIA_RAW_AUDIO;
			srcMFmt.u.raw_audio = media_raw_audio_format::wildcard;
			
			if( srcTrack->DecodedFormat( &srcMFmt ) != B_OK )
				throw "Source track format error";
				
			// ***
			// Setup dest file
			// ***
			media_file_format		fileFormat;
			media_codec_info		codecInfo;
				
			dstMFmt.type = B_MEDIA_RAW_AUDIO;
			dstMFmt.u.raw_audio = media_raw_audio_format::wildcard;
			dstMFmt.u.raw_audio = srcMFmt.u.raw_audio; // Same as source for now...

			if (saveFile) {
				// copy format of preconfigured file
				if(B_OK!=saveFile->GetFileFormatInfo(&fileFormat))
					printf("GetFileFormatInfo failed\n");
				if(B_OK!=saveTrack->GetCodecInfo(&codecInfo))
					printf("GetCodecInfo failed\n");
				printf("got file/codec\n");
			} else if (file_fmt) {
				for (int32 i = 0; i < saveFormats->CountItems(); i++) {
					save_format *sf = (save_format *)saveFormats->ItemAt(i);
					if (sf->name.Compare(file_fmt) == 0) {
						fileFormat = sf->file_format;
						codecInfo = sf->codec_info;
						break;
					}
				}
			} else {
				fileFormat = defaultSaveFormat->file_format;
				codecInfo = defaultSaveFormat->codec_info;
			}
						
			// Set the MIME type of the target file
			BNode		node( dst );
			BNodeInfo 	info( &node );
			info.SetType( fileFormat.mime_type );
			node.Unset();
			
			if( (dstFile = new BMediaFile( dst, &fileFormat ))->InitCheck() != B_OK )
			{
				printf("error %08x (%s)\n",dstFile->InitCheck(),strerror(dstFile->InitCheck()));
				throw "Dest file failed InitCheck";
			}

			if( !(dstTrack = dstFile->CreateTrack( &srcMFmt, &codecInfo )) )
				throw "Could not create dest track";
			
			if (saveFile)
			{
				// copy parameters from template BMediaFile
				BParameterWeb *srcweb;
				if(	B_OK==saveTrack->GetParameterWeb(&srcweb))
				{
					BParameterWeb *dstweb=NULL;
					if(B_OK==dstTrack->GetParameterWeb(&dstweb))
					{
						char buffer[10000];
						size_t howmuch=sizeof(buffer);
						if(B_OK==srcweb->MakeParameterData(buffer,&howmuch))
							dstweb->ApplyParameterData(buffer,howmuch);
						else
							printf("error making paramaterdata\n");
						delete dstweb;
					}
					else
						printf("no destination web\n");
					delete srcweb;
				}
				else
					printf("no source web\n");
			}

			// ***
			// Copy frames from source to dest
			// ***
			
			char buffer[16384];
			int32 sampleSize = int32(srcMFmt.u.raw_audio.format & 0xf);
			int32 frameSize = sampleSize * srcMFmt.u.raw_audio.channel_count;
			int64 totalFrames = srcTrack->CountFrames();
			int64 firstFrame = int32(float(totalFrames) * transport->GetPosition( kStartThumb ));
			int64 lastFrame = int32(float(totalFrames) * transport->GetPosition( kStopThumb ));
			int64 frameCount = lastFrame-firstFrame;
			int64 inFrames, readSize = 16384/frameSize, framesCopied = 0, framesRemaining;
			
			// Set up status window
			statusWindow->SetMaxValue((float)totalFrames);
			
			// Burn-in new header
			dstFile->CommitHeader();
			
			srcTrack->SeekToFrame( &firstFrame );
			if( readSize == 0 )
				return 0;	// Nothing to copy
			
			inFrames = readSize;
			while( (srcTrack->ReadFrames( buffer, &inFrames )) == B_OK )
			{
				if( inFrames <= 0 )
					break;
				
				if( (framesRemaining = frameCount-framesCopied) < inFrames )
					inFrames = framesRemaining;
				dstTrack->WriteFrames( buffer, inFrames );

				if (statusWindow->Lock()) {
					statusWindow->Update((float)inFrames);				
					statusWindow->Unlock();
				}
				
				framesCopied += inFrames;
				if( framesCopied >= frameCount )
					break;
				inFrames = readSize;
			}
		}
	}
	catch( const char *err )
	{
		ErrorAlert( err, B_OK );
	}
	
	if( dstFile && dstTrack )
	{
		// Finalize changes to new file
		dstFile->CloseFile();
	}
	if( srcTrack )
		srcFile->ReleaseTrack( srcTrack );
	if( srcFile )
		delete srcFile;
	if( dstFile )
		delete dstFile;

	delete cookie;

	if (statusWindow->Lock())
		statusWindow->Quit();
	
	return B_OK;
}

void RecorderWin::SaveSound( entry_ref *dst, entry_ref *src, const char *file_fmt )
{
	save_cookie *s = new save_cookie(this, *dst, *src, file_fmt);
	thread_id t = spawn_thread(_save_thread_, "save thread", B_NORMAL_PRIORITY, s);
	resume_thread(t);
}
