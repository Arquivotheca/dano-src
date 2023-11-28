//*****************************************************************************
//
//	File:		 3dmovApp.cpp
//
//	Description: Demo application using the 3d Kit.
//
//	Copyright 1996, Be Incorporated
//
//*****************************************************************************

#include <Debug.h>

#ifndef _3D_MOV_APP_H
#include "3dmovApp.h"
#endif
#ifndef _ALERT_H
#include "Alert.h"
#endif

#include <StorageKit.h>
#include <Path.h>
#include <NodeInfo.h>
#include <TranslationUtils.h>
//#include <vidsource.h>

#include <Debug.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <Debug.h>

extern "C" {
	int _kopen_(char*, int);
	int _kclose_(int);
	int _kioctl_(int, int, void*);
#if __POWERPC__
	void *Xmemcpy(void * dst, const void * src, long len);
	void *Xmemset(void * dst, int val, ulong len);
#endif
};

long refresh_thread(void *data);
long player_thread(void *data);
long bitmap_thread(void *data);

int
main() {	
	Z3dApplication  *app;
	
	app = new Z3dApplication();
// Open the first demo as default one.
//	app->ActiveWindow(0, 1);	// cube
//	app->ActiveWindow(1, 1);	// sphere
//	app->ActiveWindow(2, 1);	// pulse
	app->ActiveWindow(3, 1);	// book
	app->Run();
	delete app;
	return(0);
}

Z3dApplication::Z3dApplication() : BApplication("application/x-vnd.Be-3DMO") {
	long        i;

// Default state for animation and all rotation
	Pause = FALSE;

// create a semaphore to allow modification of active window
	windowSem = create_sem(1,"3dmov window sem");
	for (i=0;i<4;i++)
		myWindow[i] = 0L;
	book_stable = false;
	mapping_stable = true;
	exit_status = false;

// init main synchronisation management
	window_count = 0;
	video_sem = create_sem(0, "video sem");
	video_lock = 0;
	sync_sem = create_sem(0, "sync sem");
	sync_count = 0;
	entry_sem = create_sem(0, "entry sem");
	entry_count = 0L;
	exit_sem = create_sem(0, "exit sem");
	exit_count = 0L;

// benaphore for dynamic allocation of channels
	alloc_lock = 0L;
	alloc_sem = create_sem(0, "alloc sem");
	
// init Video Channel dynamic management.	
	OpenVideoChannels();

// launch the synchronisation thread
	poller = spawn_thread((thread_entry)refresh_thread,
						  "master sync",
						  B_DISPLAY_PRIORITY+5,
						  (void*)this);
	resume_thread(poller);
}

Z3dApplication::~Z3dApplication() {
	long     i;

// close all demo windows
	for (i=0;i<4;i++)
		ActiveWindow(i, -1);
	exit_status = true;
// close all channels
	CloseVideoChannels();
// delete semaphores
	delete_sem(sync_sem);
	delete_sem(windowSem);
	delete_sem(video_sem);	
	delete_sem(entry_sem);	
	delete_sem(exit_sem);
	delete_sem(alloc_sem);
}

void Z3dApplication::AboutRequested() {
	BAlert				*my_alert;

// poor about alert
	my_alert = new BAlert("",
						  "3D Kit & Video preview,\n\n"
						  "by Pierre Raynaud-Richard,\n"
						  "(C) 1997 - Be, Incorporated.\n"
						  "\n"
						  "Drag & Drop pictures (TIFF format)\n"
						  "or movies on objects for more fun.\n",
						  "Big deal");
	my_alert->Go();
}

void Z3dApplication::MessageReceived(BMessage* msg)
{
	long       i;

	switch (msg->what) {
	case FIRST_WINDOW :
	case SECOND_WINDOW :
	case THIRD_WINDOW :
	case FOURTH_WINDOW :
// active demo window if not activated 
		if (myWindow[msg->what-FIRST_WINDOW] == 0L)
			ActiveWindow(msg->what-FIRST_WINDOW, 1);
		else
			myWindow[msg->what-FIRST_WINDOW]->Activate();
		break;
	case PAUSE_WINDOWS :
// Run or stop animation and rotation of all active demos
		if (Pause) {
			for (i=0; i<4; i++)
				if (myWindow[i] != 0L)
				{
					GetWorld(i)->Universe()->SetTimeMode(B_REAL_TIME, 0.0);
					myWindow[i]->mItems[4]->SetMarked(FALSE); // BACO - moved
				}
			Pause = FALSE;
		}
		else {
			for (i=0; i<4; i++)
				if (myWindow[i] != 0L)
				{
					GetWorld(i)->Universe()->SetTimeMode(B_FROZEN_TIME, 0.0);
					myWindow[i]->mItems[4]->SetMarked(TRUE); // BACO - moved
				}
			Pause = TRUE;
		}
		break;
	}
}

void Z3dApplication::ActiveWindow(long index, long state) {
	short ii;
	
	if ((index<0) || (index>3)) return;
	
//	if (index == 2) return;
	
	acquire_sem(windowSem);
// create a new demo window
	if (state == 1) {
		if (myWindow[index] == 0L) {
    // changed marked state of the main menu item
//	for(ii=0;ii<4;ii++)
//		if( myWindow[ii] != 0L )
//			myWindow[ii]->mItems[index]->SetMarked(TRUE);
	// create the new window
			myWindow[index] = new Z3dWindow(index);
	// show the window
			myWindow[index]->Show();
	// froze that demo if pause is enable
			if (Pause)
				GetWorld(index)->Universe()->SetTimeMode(B_FROZEN_TIME, 0.0);
    // set the maximal (and nominal) update rate to 20 frames/s
			GetWorld(index)->SetUpdateRate(20.0);
		}
	}
// free the window from the window active list (the window quit itself).
	else if (state == 0) {
//		for(ii=0;ii<4;ii++)
//			if( myWindow[ii] != 0L )
//				myWindow[ii]->mItems[index]->SetMarked(FALSE);
		myWindow[index] = 0L;
		ClearChannel(index);
	}	
// free the window from the window active list and quit the window.
	else if (state == -1) {
		if (myWindow[index] != 0L) {
			for(ii=0;ii<4;ii++)
				if( myWindow[ii] != 0L )
					myWindow[ii]->mItems[index]->SetMarked(FALSE);
			myWindow[index]->Quit();
			myWindow[index] = 0L;
			ClearChannel(index);
		}
	}
// Close the application when the last window is closed
	if ((myWindow[0] == 0L) &&
		(myWindow[1] == 0L) &&
		(myWindow[2] == 0L) &&
		(myWindow[3] == 0L)) {
		PostMessage(B_QUIT_REQUESTED);
		exit_status	= true;
	}
	release_sem(windowSem);
}

B3dWorld *Z3dApplication::GetWorld(int index) {
	if (index < 3)
		return ((Z3dView*)(myWindow[index]->myView))->World();
	else
		return ((B3dBookView*)(myWindow[index]->myView))->World();
}

void Z3dApplication::EnableSync() {
	atomic_add(&entry_count, 1);
	acquire_sem(entry_sem);
}

void Z3dApplication::DisableSync() {
	if (video_lock) {
		release_sem(sync_sem);
		atomic_add(&exit_count, 1);
		acquire_sem(video_sem);
	}
	else
		atomic_add(&exit_count, 1);
	acquire_sem(exit_sem);
}

void Z3dApplication::Sync() {
	release_sem(sync_sem);
	acquire_sem(video_sem);
}

long refresh_thread(void *data) {
	int             i, count;
	char            usage[13];
	bool			source_modified;
	double          last, next, delay;
	Z3dApplication  *app;
	VideoSource     *v;
	
	for (i=0; i<13; i++)
		usage[i] = 15;
	app = (Z3dApplication*)data;
	last = system_time();

	while (TRUE) {
	// manage entries
		count = app->entry_count;
		if (count > 0) {
			atomic_add(&app->entry_count, -count);
			app->window_count += count;
			release_sem_etc(app->entry_sem, count, 0);
			app->mapping_stable = false;
		}
	// manage exits.
		count = app->exit_count;
		if (count > 0) {
			atomic_add(&app->exit_count, -count);
			app->window_count -= count;
			if (app->window_count < 0) {
				release_sem_etc(app->exit_sem, count, 0);
				break;
			}
			app->mapping_stable = false;
			release_sem_etc(app->exit_sem, count, 0);
		}
	// do the sync on all windows
		app->video_lock = 1;
		source_modified = false;
		for (i=0; i<14; i++)
			if (app->video_list[i] != 0L)
				if (app->video_list[i]->source_modified) {
					app->video_list[i]->source_modified = false;
					source_modified = true;
				}
		if ((app->window_count > 0) &&
			((source_modified ||
			  app->exit_status ||
			  (app->myWindow[0] != NULL) ||
			  (app->myWindow[1] != NULL) ||
			  (app->myWindow[2] != NULL) ||
			  (!app->mapping_stable) ||
			  ((app->myWindow[3] != NULL) && (!app->book_stable))))) {
			acquire_sem_etc(app->sync_sem, app->window_count, 0, 1e12);
			next = system_time();
			app->video_lock = 0;
		// buffer refresh
			for (i=0; i<14; i++)
				if (app->video_list[i] != 0L)
					app->video_list[i]->GetNextFrame();
		// update map_ref_list
			app->LockAlloc();
			for (i=0; i<13; i++)
				app->map_ref_list[i].buf =
					app->video_list[app->map_ref_index[i]]->cur_buffer;
			app->UnlockAlloc();
			release_sem_etc(app->video_sem, app->window_count, B_DO_NOT_RESCHEDULE);
			app->mapping_stable = true;
		}
		else {
			next = system_time();
			app->video_lock = 0;
		}
	// do dynamic source management clean_up.
		app->LockAlloc();
		for (i=0; i<14; i++)
			usage[i] >>= 1;
		for (i=0; i<13; i++)
			usage[app->map_ref_index[i]] |= 8;
		for (i=0; i<14; i++)
			if ((app->video_list[i] != 0L) && (usage[i] == 0)) {
				delete app->video_list[i];
				app->video_list[i] = 0L;
			}
		app->UnlockAlloc();
	// timing
		delay = last+5e4-next;
		if (delay > 3e3) {
			snooze(delay);
			last = system_time();
		}
		else
			last = next;
	}
	return 0L;
}
	
void Z3dApplication::OpenVideoChannels() {
	int            i;
	
	for (i=0; i<14; i++)
		video_list[i] = 0L;
// do video default allocation.
	video_list[14] = new DefaultSource();
	for (i=0; i<13; i++)
		map_ref_index[i] = 14;	
// init map_ref_list
	for (i=0; i<13; i++) {
		map_ref_list[i].buf = video_list[map_ref_index[i]]->cur_buffer;
//		_sPrintf("buffer : %x [%x]\n", map_ref_list[i].buf, map_ref_list+i);
		map_ref_list[i].size_h = 8;
		map_ref_list[i].size_v = 8;
	}
// init video locks.
	for (i=0; i<13; i++)
		map_ref_lock[i] = FALSE;
}

void Z3dApplication::CloseVideoChannels() {
	int            i, cpt;

	while (TRUE) {
		cpt = 0;
		for (i=0; i<14; i++)
			if (video_list[i] != 0L)
				cpt++;
		if (cpt == 0) break;
		snooze(1e5);
	}
	atomic_add(&exit_count, 1);
	acquire_sem(exit_sem);
	delete video_list[14];
}

static long FirstIndex[4] = { 0, 6, 7, 9 };
static long LastIndex[4] = { 6, 7, 9, 13 };

void Z3dApplication::ClearChannel(int index) {
	int       i;
	
	for (i=FirstIndex[index]; i<LastIndex[index]; i++)
		map_ref_index[i] = 14;
}

void Z3dApplication::SelectVideoChannel(int index, entry_ref *file) {
	int		i;
	long		size;
	char		mtype[256];		// mime type of the file dropped on us
	ulong		creator;
	ulong		type;
	VideoSource *source;
	BNodeInfo	*ninfo;
	BNode	*tnode;
	
	if (map_ref_lock[index])
		return;
	LockAlloc();
	mapping_stable = false;
	for (i=0; i<14; i++)
		if (video_list[i] != 0)
			if (video_list[i]->CheckFile(file)) {
				map_ref_index[index] = i;
				delete file;
				goto end;
			}
// uses mime types now:
	tnode = new BNode( file);
	ninfo = new BNodeInfo( tnode );
	ninfo->GetType(mtype);
	delete ninfo;
	
	printf("\nFile of type \"%s\" dropped.\n", mtype);
	
	if(strcmp(mtype, "video/quicktime")==0)
	{
		for (i=0; i<14; i++)
			if (video_list[i] == 0) {
				source = new QuickTimeSource(file);
				source->GetFirstFrame();
				video_list[i] = source;
				map_ref_index[index] = i;
				goto end;
			}
	}
/*	else if (strcmp(mtype, "application/x-vnd.Be-VID!")==0)
	{
		for (i=0; i<14; i++)
			if (video_list[i] == 0) {
				source = new RealTimeSource(file);
				source->GetFirstFrame();
				LockAlloc();
				video_list[i] = source;
				map_ref_index[index] = i;
				goto end;
			}		
	}*/
	else if (strncmp(mtype, "image/", 6) == 0) {
		for (i=0; i<14; i++)
			if (video_list[i] == 0) {
				map_ref_lock[index] = TRUE;
				source = new BitmapSource(file, index);
				source->GetFirstFrame();
				LockAlloc();
				video_list[i] = source;
				map_ref_index[index] = i;
				goto end;
			}		
	}
/*	else if (strcmp(mtype, "application/x-be-executable")==0)
	{
		if(strcmp(file->name, "Bt848")==0)
		{
			for (i=0; i<14; i++)
				if (video_list[i] == 0) {
					source = new TunerSource();
					source->GetFirstFrame();
					LockAlloc();
					video_list[i] = source;
					map_ref_index[index] = i;
					goto end;
				}
		}		
	}*/
	delete file;
 end:
	UnlockAlloc();
}

void Z3dApplication::LockAlloc() {
	int       old;
	
	old = atomic_add(&alloc_lock, 1);
	if (old > 0)
		acquire_sem(alloc_sem);
}

void Z3dApplication::UnlockAlloc() {
	int       old;
	
	old = atomic_add(&alloc_lock, -1);
	if (old > 0)
		release_sem(alloc_sem);
}

/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/


/************************************************** Generic class */
VideoSource::VideoSource(int count, entry_ref *file) {
	BRect     frame;
	
	cur_buffer = 0L;
	myFile = file;
	if (count) {
		frame.Set(0.0, 0.0, 255.0, (256.0*(float)count) - 1.0);
		buffer = new BBitmap(frame, B_RGB_32_BIT);
	}
	else
		buffer = NULL;
	source_modified = false;
}

VideoSource::~VideoSource() {
	if (buffer)
		delete buffer;
}

bool VideoSource::CheckFile(entry_ref *file) {
	return (*file == *myFile);
}

void VideoSource::GetFirstFrame() {
}

void VideoSource::GetNextFrame() {
}


/************************************************ fake source (boring gray) */
DefaultSource::DefaultSource() : VideoSource(1, 0L) {
	cur_buffer = buffer->Bits();
#if __POWERPC__
	Xmemset(cur_buffer, 0xa0, 4*256*256);
#else
	memset(cur_buffer, 0xa0, 4*256*256);
#endif
}

/************************************************ cool source (848 interface)! */
#if 0
TunerSource::TunerSource() : VideoSource(0, NULL) // no files used here...
{
	// instantiate the BTSVideoSource and initialize it:
	// we won't allocate a bitmap, instead we'll just access the one returned by GetNextFrame()
	cur_buffer = NULL;
	vidsrc = new BTSVideoSource("3dmovtuner");
	vidsrc->Init();
	vidsrc->Start();
}

TunerSource::~TunerSource()
{
	if(vidsrc)
	{
		vidsrc->Stop();
		delete vidsrc;
	}
}

void TunerSource::GetFirstFrame()
{
	buffer = vidsrc->GetNextFrame();
	cur_buffer = buffer->Bits();
}

void TunerSource::GetNextFrame()
{
	buffer = vidsrc->GetNextFrame();
	cur_buffer = buffer->Bits();
}
#endif

/*********************************************************** bitmap picture */
BitmapSource::BitmapSource(entry_ref *file, int index) : VideoSource(1, file) {
	ulong           *src, *dst;
	void            *buf;
	char            path[256];
	thread_id       poller;
	BEntry	*eInfo = NULL;	// set to NULL to shut up compiler warnings...
	BPath		pInfo;
	
	channel_index = index;
	cur_buffer = buffer->Bits();
#if __POWERPC__
	Xmemset(cur_buffer, 0x00, 4*256*256);
#else
	memset(cur_buffer, 0x00, 4*256*256);
#endif
	
	eInfo = new BEntry(file);	// talk about a kludge!
	eInfo->GetPath(&pInfo);
	strcpy(path, pInfo.Path());
	my_bitmap = BTranslationUtils::GetBitmap(path);
	poller = spawn_thread((thread_entry)bitmap_thread,
						  "master sync",
						  B_NORMAL_PRIORITY+3,
						  (void*)this);
	resume_thread(poller);
}

long bitmap_thread(void *data) {
	int32			i, j, row_from, row_to;
	char			*from, *to;
	BRect			frame, src_rect;
	BView			*tmp_view;
	uint32			val;
	BBitmap			*tmp_buffer;
	BitmapSource    *s;

	s = (BitmapSource*)data;
	if (s->my_bitmap != NULL) {
		frame.Set(0.0, 0.0, 255.0, 255.0);
		tmp_buffer = new BBitmap(frame, B_RGB32_BIG, true);
		tmp_view = new BView(frame, "", 0, 0);
		tmp_buffer->AddChild(tmp_view);
		
		src_rect = s->my_bitmap->Bounds();
		if ((src_rect.right - src_rect.left) > 255.0) {
			src_rect.left = (float)((int)((src_rect.right - src_rect.left - 255.0)*0.5));
			src_rect.right = src_rect.left + 255.0;
		}
		if ((src_rect.bottom - src_rect.top) > 255.0) {
			src_rect.top = (float)((int)((src_rect.bottom - src_rect.top - 255.0)*0.5));
			src_rect.bottom = src_rect.top + 255.0;
		}
		
		tmp_buffer->Lock();
		tmp_view->DrawBitmap(s->my_bitmap, src_rect, frame);
		tmp_buffer->Unlock();
		
		from = (char*)tmp_buffer->Bits();
		to = (char*)s->buffer->Bits();
		row_from = tmp_buffer->BytesPerRow();
		row_to = s->buffer->BytesPerRow();
		for (i=0; i<256; i++) {
#if __POWERPC__
			for (j=0; j<256; j++) {
				val = ((uint32*)from)[j];
				val = (val>>24) | ((val>>8)&0xff00) | ((val&0xff00)<<8) | (val<<24);
				((uint32*)to)[j] = val;
			}
#else
			memcpy(to, from, 256*4);
#endif
			to += row_to;
			from += row_from;
		}
		
		delete tmp_buffer;
		delete s->my_bitmap;
		s->source_modified = true;
	}
	
	((Z3dApplication*)be_app)->map_ref_lock[s->channel_index] = FALSE;
	return 0L;
}

/*************************************************** kernel driver video in */
#if 0
RealTimeSource::RealTimeSource(entry_ref *file) : VideoSource(0, file) {
	char    name[10+B_FILE_NAME_LENGTH] = "/dev/";

	area = 0;
	video_area = create_area("3d_vid_area", &area, B_ANY_ADDRESS, 4 * 256 * 256 * 3,
							 B_FULL_LOCK | B_CONTIGUOUS, B_READ_AREA + B_WRITE_AREA);
	if (area) {
		strcat(name, file->name);
		
		driver_id = _kopen_(name, O_RDWR);
		if (driver_id >= 0) {
			init.src_frame.left = 0;
			init.src_frame.top = 0;
			init.src_frame.right = 255;
			init.src_frame.bottom = 239;
			init.dst_frame.left = 0;
			init.dst_frame.top = 0;
			init.dst_frame.right = 255;
			init.dst_frame.bottom = 239;
			init.row_bytes = 1024;
			init.format = VID_32;
			init.scale = FALSE;
			init.clip = FALSE;
			init.buffer_pool_count = 3;
			init.buffers[0] = area + (8 * init.row_bytes);
			init.buffers[1] = (void*)((ulong)init.buffers[0]+4*256*256);
			init.buffers[2] = (void*)((ulong)init.buffers[1]+4*256*256);
			_kioctl_ (driver_id, PLANB_START, &init);
		}
	}
	else
		driver_id = -1;
}

RealTimeSource::~RealTimeSource() {
	if (driver_id >= 0) {
		_kioctl_(driver_id, PLANB_STOP, 0L);
		_kclose_(driver_id);
		delete_area(video_area);	
	}
}

void RealTimeSource::GetFirstFrame() {
	if (driver_id >= 0)
		cur_buffer = (void*)(_kioctl_ (driver_id, PLANB_NEXT_BUFFER, (void *)TRUE) - (8 * 1024));
	else
		cur_buffer = NULL;
}

void RealTimeSource::GetNextFrame() {
	long         result;

	if (driver_id >= 0) {
		result = _kioctl_ (driver_id, PLANB_NEXT_BUFFER, FALSE);
		if (result != 0L)
			cur_buffer = (void*)(result - (8 * 1024));
	}
	else
		cur_buffer = NULL;
}
#endif

/************************************************** QuickTime player from IO */
QuickTimeSource::QuickTimeSource(entry_ref *movie) : VideoSource(2, movie) {
	mPlayer = NewBeQTPlayer(movie);	// May be null if this is not a good movie
	mTime = -1;
	buffers[0] = buffer->Bits();
	buffers[1] = (void*)((ulong)buffers[0]+4*256*256);
	curBuf = 0;
	cur_buffer = buffers[curBuf];
#if __POWERPC__
	Xmemset(buffers[0], 0x20, 4*256*256*2);
#else
	memset(buffers[0], 0x20, 4*256*256*2);
#endif
	sem = create_sem(0, "quick sem");
	in = create_sem(0, "quick sem");
	flag = FALSE;
}

QuickTimeSource::~QuickTimeSource() {
	long    exit;
	
	flag = TRUE;
	release_sem(sem);
	wait_for_thread(player, &exit);
	delete mPlayer;
	delete_sem(sem);
	delete_sem(in);
}

void QuickTimeSource::GetFirstFrame() {
	long     duration;
	BRect    bound;
	
	if (mPlayer == NULL)
		return;
	
	mTime = -1;	
	mPlayer->Step(false,&mTime,&duration);
	mBitmap = ((BeQTPlayer*)mPlayer)->GetBitmap(bound);
	row_byte = mBitmap->BytesPerRow();
	line_count = (int)bound.Height()+1;
	size = 4*((int)bound.Width()+1);
	base = mBitmap->Bits();
	offset = 0;
	
	if (line_count > 256) {
		base = (void*)((ulong)base+row_byte*((line_count-256)/2));
		line_count = 256;
	}
	else if (line_count < 256) {
		offset += 1024*((256-line_count)/2);
	}
	
	if (size > 1024) {
		base = (void*)((ulong)base+(((size-1024)/2)&0xfffc));
		size = 1024;
	}
	else if (size < 1024) {
		offset += ((1024-size)/2) & 0xfffc;
	}
	
	CopyBuffer();
	source_modified = true;
	
	player = spawn_thread((thread_entry)player_thread,
						  "Quicktime",
						  B_NORMAL_PRIORITY,
						  (void*)this);
	resume_thread(player);
}

void QuickTimeSource::GetNextFrame() {
	long     duration;

	if (mPlayer == NULL)
		return;
	acquire_sem(in);
	cur_buffer = buffers[curBuf];
	curBuf = 1-curBuf;
	release_sem(sem);
}

void QuickTimeSource::CopyBuffer() {
	int     i;
	uchar   *src, *dst;

	dst = (uchar*)buffers[curBuf] + offset;
	src = (uchar*)base;
	for (i=0; i<line_count; i++) {
		memcpy(dst, src, size);
		dst += 1024;
		src += row_byte;
	}
}

long player_thread(void *data) {
	int             i, count;
	long            duration;
	double          last, next, delay;
    QuickTimeSource *s;

	s = (QuickTimeSource*)data;
	while (TRUE) {
		release_sem(s->in);
		acquire_sem(s->sem);
		if (s->flag) break;
		s->mPlayer->Step(false,&s->mTime,&duration);
		s->CopyBuffer();
		s->source_modified = true;
	}
	return 0L;
}
