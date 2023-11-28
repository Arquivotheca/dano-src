#if !defined(MEDIA_LIST_VIEW_H)
#define MEDIA_LIST_VIEW_H

#include "MediaItem.h"

#include <View.h>
#include <ListView.h>
#include <MediaRoster.h>

class MediaListView : public BListView
{
public:
	MediaListView(BRect rect);
	~MediaListView();
	
	virtual void MessageReceived(BMessage *msg);
	virtual	void AttachedToWindow();

	void PrepareListForServerRestart();
	void PopulateList();
	
	void SetRoster(BMediaRoster *roster);
	
	void SetAudioView(BView *view);
	void SetVideoView(BView *view);
	
	status_t GetViewFor(int32 index, BView **out_view);
	const char *GetTitleFor(int32 index);

	bool WebChanged(media_node *node);

	void SetVideoInputView(const dormant_node_info &info);
	void SetVideoOutputView(const dormant_node_info &info);
	void SetAudioInputView(const dormant_node_info &info);
	void SetAudioOutputView(const dormant_node_info &info);	

private:
	void PopulateAudio();
	void PopulateVideo();
	
	MediaItem *mReleaseNextGet;
	BMediaRoster *mRoster;
	BView *mAudioView;
	BView *mVideoView;
	int32 mAudio;
	int32 mMixer;
	int32 mVideo;
	int32 mVideoInput;
	int32 mVideoOutput;
	int32 mAudioInput;
	int32 mAudioOutput;
	bool mPopulated;
};

#endif
