#ifndef RECORDER_WIN_H
#define RECORDER_WIN_H

#include <Locker.h>
#include <MediaNode.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <Window.h>
#include <String.h>

class TransportView;
class FListView;
class BMediaFile;
class BMediaTrack;
class SndFInfoView;
class MediaController;
class SoundConsumer;
class BMediaRoster;
class BFile;
class BList;
class ScopeView;
class KnobSwitch;
class LevelMeter;
class BTimeSource;
class BFilePanel;
class BitmapCheckBox;
class BVolume;
class BMenu;

struct save_format {
	BString					name;
	media_file_format		file_format;
	media_codec_info		codec_info;
};
		
class RecorderWin : public BWindow
{
	public:
		RecorderWin( BPoint where );
		virtual ~RecorderWin( void );
		
		virtual bool QuitRequested( void );
		virtual void MessageReceived( BMessage *msg );
		virtual void Zoom( BPoint rec_position, float rec_width, float rec_height );
	
	protected:
		// Message Dispatch
		void ConfigureCodec();
		void RefsReceived( BMessage *msg );
		void FileSelected( BMessage *msg );
		void ActionCopy( BMessage *msg );
		void Play( BMessage *msg );
		void Record( BMessage *msg );
		void Stop( BMessage *msg );
		void ScanForward( void );
		void ScanBackward( void );
		void NudgeForward( void );
		void NudgeBackward( void );
		void UpdateTransport( void );
		void StartScrubbing( void );
		void StopScrubbing( void );
		void InPointChanged( void );
		void OutPointChanged( void );
		void VolumeChanged( void );
		void RecordComplete( void );
		void Save( BMessage *msg );
		
		void SaveSound( entry_ref *dst, entry_ref *src, const char *fileFormat = NULL );
		static int32 _save_thread_(void *data);
		int32 SaveThread(void *data);
				
		status_t SetupSoundInput( void );
		void TeardownSoundInput( void );
		
		// Inits
		status_t InitChildren( void );
		status_t InitMedia( void );
		status_t InitSaveFormats( void );
		
		// Record Callback Functions
		static	void RecordFile(
				void * cookie, 
				bigtime_t timestamp, 
				void * data,
				size_t size, 
				const media_raw_audio_format & format);
				
		static	void NotifyRecordFile(
				void * cookie,
				int32 code,
				...);
		
		status_t NewTempFile( entry_ref *ref, const char *nameHint );
		void ErrorAlert( const char *action, status_t err );
		
	protected:
		TransportView		*transport; // Media Trasport controls
		FListView			*fileListView; // The list of sounds
		SndFInfoView		*infoView; // Displays sound info
		MediaController 	*mediaController; // Sound playback interface
		ScopeView			*scopeView;
		KnobSwitch			*knob;
		LevelMeter			*meter;
		BitmapCheckBox		*loopButton;
		bool				isZoomed;
		
		SoundConsumer		*recordNode; // Our record node
		BMediaRoster		*m_roster; // cached media roster pointer
		media_node			audioInput; // audio input node
		BTimeSource			*tsobj;
		
		BLocker				fileLock; // Locked when sound file is in use
		bigtime_t			lastPos; // used to avoid unnecessary updates of media slider
		bigtime_t			lastOut;
		bigtime_t			lastIn;
		status_t			mInitStatus; // media init status
		
		entry_ref			*recordRef; // entry_ref for recordFile
		BVolume				*recordVol; // the volume on which the sound is being recorded
		entry_ref			*currentRef; // entry_ref for selected file
		BMediaFile			*recordFile; // current record file
		BMediaTrack			*recordTrack; // current record track
		BMediaFile			*saveFile;
		BMediaTrack			*saveTrack;
		bool				recording;
		int32				frameSize;
		
		media_output 		m_audioOutput;
		media_input 		m_recInput;
		media_format		m_fmt;
		bool				lastBuffer;
		bool				hasInput;
		bool				hasOutput;
		
		BList				*tempFileList;
		BFilePanel			*filePanel;
		BMenu				*formatMenu;
		
		BList				*saveFormats;
		save_format			*defaultSaveFormat;
};

#endif
