#ifndef NODEWRAPPER_H
#define NODEWRAPPER_H

#include <MediaDefs.h>
#include <MediaRoster.h>

class NodeWrapper {
public:

						NodeWrapper(const media_node &node, bool release = false, bool stop = false);
						~NodeWrapper();
	
	status_t			InitCheck() const;
	void				Disconnect();
						
	bool				HasOutputType(media_type);
	bool				HasInputType(media_type);
	status_t			ConnectOutput(NodeWrapper*, media_type, media_format* = 0);
	status_t			ConnectInput(NodeWrapper*, media_type, media_format* = 0);
	status_t			Start(bigtime_t = 0);
	status_t			Stop(bigtime_t = 0);
	status_t			SyncStopNow();
	status_t			Sync(bigtime_t = 0);
	status_t			Seek(bigtime_t media_time, bigtime_t performance_time = 0);
	status_t			RollOnce(bigtime_t seekTime, bigtime_t startTime, bigtime_t duration);
	status_t			Preroll();
	status_t			GetOutputFormat(media_type, media_format*);
	int32				GetSourceConnectionID(int32 connectionNum);
	int32				GetDestConnectionID(int32 connectionNum);
	
	static NodeWrapper* InstantiateNode(const media_format &in, const media_format &out);
	static NodeWrapper* InstantiateNode(const media_format &in);
	bool				CanStop();
	
	const char			*Name();

private:

	status_t PlugNodes(NodeWrapper*,NodeWrapper*,media_type, media_format*);

	media_node fNode;
	bool fIsStarted;	
	BList fConnections;
	bool fRelease;
	bool fStop;
	static BTimeSource	*btimeSource;
	static BMediaRoster *roster;
	static media_node	timeSource;
	char fName[B_MEDIA_NAME_LENGTH];
	status_t fInitCheckVal;
};


#endif
