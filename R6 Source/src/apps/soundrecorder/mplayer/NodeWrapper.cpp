#include <MediaNode.h>
#include <TimeSource.h>
#include <Debug.h>
#include <MediaAddOn.h>
#include <string.h>
#include "NodeWrapper.h"

BMediaRoster* NodeWrapper::roster = 0;
BTimeSource* NodeWrapper::btimeSource = 0;
media_node NodeWrapper::timeSource;

struct ConnectionInfo {
	ConnectionInfo(media_node_id sourceID, media_node_id destID,
		media_source source, media_destination dest)
		:	fSourceID(sourceID),
			fDestID(destID),
			fSource(source),
			fDest(dest)
	{}

	media_node_id	 	fSourceID;
	media_node_id		fDestID;
	media_source		fSource;
	media_destination 	fDest; 
};

NodeWrapper::NodeWrapper(const media_node &node, bool release, bool stop)
	:	fNode(node),
		fIsStarted(false),
		fRelease(release),
		fStop(stop)
{
	if (roster == 0)
		roster = BMediaRoster::Roster();

	if (btimeSource == 0) {
		status_t err = roster->GetTimeSource(&timeSource);
		if (err != B_OK) {
			PRINT(("Couldn't get system time source\n"));
			return ;
		}
	}

	if (roster->SetTimeSourceFor(fNode.node, timeSource.node) != B_OK)
		PRINT(("Couldn't set time source for node.\n"));
		
	btimeSource = roster->MakeTimeSourceFor(fNode);
	if (btimeSource == 0)
		PRINT(("Couldn't get time source\n"));

	live_node_info info;
	roster->GetLiveNodeInfo(node, &info);
	strcpy(fName, info.name);  
}

NodeWrapper::~NodeWrapper()
{
	Disconnect();

	status_t err;
	if (fRelease) {
		PRINT(("Releasing node %s\n", Name()));
		err = roster->ReleaseNode(fNode);
		if (err != B_OK)
			PRINT(("Error releasing node %s\n", strerror(err)));		
	}
}

void 
NodeWrapper::Disconnect()
{
	status_t err;
	if (fStop)
		if (fIsStarted) {
			PRINT(("Stopping node %s\n", Name()));
			Stop();
		}

	ConnectionInfo *connection;		
	while ((connection = (ConnectionInfo*) fConnections.RemoveItem(0L)) != 0) {
		if (fNode.node == connection->fSourceID)
			PRINT(("Disconnecting output from node %s\n", Name()));
		else
			PRINT(("Disconnecting input to node %s\n", Name()));


		err = roster->Disconnect(connection->fSourceID, connection->fSource,
			connection->fDestID, connection->fDest);
		if (err != B_OK)
			PRINT(("Error disconnecting nodes %s\n", strerror(err)));
	
		delete connection;
	}
}



bool 
NodeWrapper::HasOutputType(media_type type)
{
	int32 numOutputs;
	media_output output;
	status_t err = roster->GetFreeOutputsFor(fNode, &output, 1,
		&numOutputs, type);
	if (err != B_OK) {
		PRINT(("Error searching for output type\n"));
		return false;
	}

	return (numOutputs > 0);
}

bool 
NodeWrapper::HasInputType(media_type type)
{
	int32 numInputs;
	media_input input;
	status_t err = roster->GetFreeInputsFor(fNode, &input, 1,
		&numInputs, type);
	if (err != B_OK) {
		PRINT(("Error searching for output type\n"));
		return false;
	}

	return (numInputs > 0);
}

status_t
NodeWrapper::GetOutputFormat(media_type type, media_format *format)
{
	int32 numOutputs;
	media_output output;
	status_t err = roster->GetFreeOutputsFor(fNode, &output, 1,
		&numOutputs, type);
	if (err != B_OK) {
		PRINT(("Error searching for output type\n"));
		return err;
	}

	if (numOutputs == 0) {
		PRINT(("Node returned no outputs\n"));
		return B_ERROR;
	}

	*format = output.format;
	return B_OK;
}




status_t
NodeWrapper::PlugNodes(NodeWrapper *source, NodeWrapper *dest, media_type type,
	media_format *format)
{
	int32 numOutputs;
	media_output output;
	status_t err = roster->GetFreeOutputsFor(source->fNode, &output, 1,
		&numOutputs, type);
	if (err != B_OK) {
		PRINT(("Couldn't get output\n"));
		return err;
	}

	if (numOutputs == 0) {
		PRINT(("source had no outputs\n"));
		return B_ERROR;
	}

	int32 numInputs;
	media_input input;
	err = roster->GetFreeInputsFor(dest->fNode, &input, 1,
		&numInputs, type);
	if (err != B_OK) {
		PRINT(("Couldn't get input\n"));
		return err;
	}
	
	if (numInputs == 0) {
		PRINT(("destination had no inputs\n"));
		return B_ERROR;
	}


	media_format use_format;
	if (format)
		use_format = *format;
	else
		use_format = output.format;

	char formatString[256];
	string_for_format(use_format, formatString, 255);
	PRINT(("Connecting nodes using format: %s\n", formatString));

		
	err = roster->Connect(output.source, input.destination, &use_format, &output, &input);
	if (err != B_OK) {
		PRINT(("connect failed: %s\n", strerror(err)));
		return err; 
	}
	
	fConnections.AddItem((void*) new ConnectionInfo(source->fNode.node,
		dest->fNode.node, input.source, output.destination));
	
	return B_OK;
}


status_t 
NodeWrapper::ConnectOutput(NodeWrapper *output, media_type type, media_format *format)
{
	return PlugNodes(this, output, type, format);
}

status_t 
NodeWrapper::ConnectInput(NodeWrapper *input, media_type type, media_format *format)
{
	return PlugNodes(input, this, type, format);
}

status_t 
NodeWrapper::Start(bigtime_t when)
{
	if (when == 0)
		when = btimeSource->Now() + 30000;

	PRINT(("Starting AT %Ld\n",when));
	status_t err = roster->StartNode(fNode, when);
	if (err != B_OK) {
		PRINT(("Couldn't start node: %s\n", strerror(err)));
		return err;	
	}

	fIsStarted = true;
	return B_OK;
}

status_t 
NodeWrapper::SyncStopNow()
{
	if (!fIsStarted) 
		return B_OK;
	
	status_t err = roster->StopNode(fNode, 0, true);
	if (err != B_OK)
		PRINT(("Couldn't sync stop node: %s\n", strerror(err)));
		
	fIsStarted = false;
	return B_OK;
}


status_t 
NodeWrapper::Stop(bigtime_t when)
{
	PRINT(("Stop node %s\n", Name()));
	if (!fIsStarted) {
		PRINT(("someone tried to stop stopped node\n"));
		return B_OK;
	}
		
	if (when == 0)
		when = btimeSource->Now();

	PRINT(("Stopping AT %Ld\n",when));
	status_t err = roster->StopNode(fNode, when);
	if (err != B_OK)
		PRINT(("Couldn't stop node: %s\n", strerror(err)));
		
	fIsStarted = false;
	return B_OK;
}

status_t 
NodeWrapper::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	status_t err;
	PRINT(("Seek with time of %Ld\n",performance_time));
	if (performance_time == 0)
		err = roster->SeekNode(fNode, media_time, btimeSource->Now());
	else
		err = roster->SeekNode(fNode, media_time, performance_time);
	
	if (err != B_OK)
		PRINT(("Couldn't seek node: %s\n", strerror(err)));

	return err;	
}

status_t 
NodeWrapper::Sync(bigtime_t when)
{
	status_t err;
	if (when == 0) when = btimeSource->Now();

	err = roster->SyncToNode(fNode, when);
	
	if (err != B_OK) PRINT(("Couldn't sync node: %s\n", strerror(err)));

	return err;	
}

status_t 
NodeWrapper::Preroll()
{
	status_t err = roster->PrerollNode(fNode);
	if (err != B_OK)
		PRINT(("Couldn't preroll node: %s\n", strerror(err)));

	return err;	
}

status_t 
NodeWrapper::RollOnce(bigtime_t seekTime, bigtime_t startTime, bigtime_t duration)
{
	status_t err = roster->RollNode(fNode,startTime,startTime+duration,seekTime);
	if (err != B_OK)
		PRINT(("Couldn't roll node: %s\n", strerror(err)));

	return err;	
}

NodeWrapper *
NodeWrapper::InstantiateNode(const media_format &in, const media_format &out)
{
	char inFormat[256];
	string_for_format(in, inFormat, 255);

	char outFormat[256];
	string_for_format(out, outFormat, 255);

	PRINT(("Finding node to translate from %s to %s\n", inFormat, outFormat));

	dormant_node_info info;
	int32 numNodes = 1;
	status_t err = roster->GetDormantNodes(&info, &numNodes, &in,
		&out, NULL, B_BUFFER_PRODUCER | B_BUFFER_CONSUMER);
	if (err != B_OK) {
		PRINT(("Error occured looking for node: %s\n", strerror(err)));
		return 0;
	}
	
	if (numNodes == 0) {
		PRINT(("Couldn't find node that handles this format\n"));
		return 0;
	}

	media_node node;
	err = roster->InstantiateDormantNode(info, &node);
	if (err != B_OK) {
		PRINT(("Couldn't instantiate node: %s\n", strerror(err)));
		return 0;
	}

	return new NodeWrapper(node, true, true);
}

NodeWrapper *
NodeWrapper::InstantiateNode(const media_format &in)
{
	char inFormat[256];
	string_for_format(in, inFormat, 255);
	PRINT(("Finding node to translate stream %s\n", inFormat));

	dormant_node_info info;
	int32 numNodes = 1;
	status_t err = roster->GetDormantNodes(&info, &numNodes, &in,
		NULL, NULL, B_BUFFER_PRODUCER | B_BUFFER_CONSUMER);
	if (err != B_OK) {
		PRINT(("Error occured looking for node: %s\n", strerror(err)));
		return 0;
	}
	
	if (numNodes == 0) {
		PRINT(("Couldn't find node that handles this format\n"));
		return 0;
	}

	media_node node;
	err = roster->InstantiateDormantNode(info, &node);
	if (err != B_OK) {
		PRINT(("Couldn't instantiate node: %s\n", strerror(err)));
		return 0;
	}

	return new NodeWrapper(node, true, true);
}



int32 
NodeWrapper::GetSourceConnectionID(int32 connectionNum)
{
	if (connectionNum < 0 || connectionNum > fConnections.CountItems())
		return 0;	

	return ((ConnectionInfo*)fConnections.ItemAt(connectionNum))->fSource.id;
}

int32 
NodeWrapper::GetDestConnectionID(int32 connectionNum)
{
	if (connectionNum < 0 || connectionNum > fConnections.CountItems())
		return 0;
		
	return ((ConnectionInfo*)fConnections.ItemAt(connectionNum))->fDest.id;
}

const char *
NodeWrapper::Name()
{
	return fName;
}

bool 
NodeWrapper::CanStop()
{
	return fStop;
}




