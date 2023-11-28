// Remote-proxy subclass of DNub

#ifndef DRemoteProxyNub_H
#define DRemoteProxyNub_H 1

#include "Dx86Nub.h"
#include <netinet/in.h>

class DTeam;
class TCPLooper;
class TCPMessenger;
class UDPLooper;
class UDPMessenger;

class DRemoteProxyNub : virtual public Dx86Nub
{
public:
	// Nub's should only be created and closed by a DTeam object
	DRemoteProxyNub(const char* remoteHostname, uint16 remotePort, DTeam* team);

	virtual void Start();
	virtual void Kill();
	virtual void Detach();
	virtual void DTeamClosed();

	// specific to DRemoteProxyNub, because of complex setup interaction
	void Attach();

	virtual void StopThread(thread_id thread);
	virtual void KillThread(thread_id thread);

	virtual void ReadData(ptr_t address, void *buffer, size_t size);
	virtual void WriteData(ptr_t address, const void *buffer, size_t size);

	virtual void SetBreakpoint(ptr_t address);
	virtual void ClearBreakpoint(ptr_t address);

	virtual void SetWatchpoint(ptr_t address);
	virtual void ClearWatchpoint(ptr_t address);

	void Run(thread_id tid, const DCpuState& cpu);
	void Step(thread_id tid, const DCpuState& cpu, ptr_t lowPC, ptr_t highPC);
	void StepOver(thread_id tid, const DCpuState& cpu, ptr_t lowPC, ptr_t highPC);
	void StepOut(thread_id tid, const DCpuState& cpu);

	virtual void GetThreadRegisters(thread_id thread, DCpuState*& outCpu);
	virtual void GetThreadList(team_id team, thread_map& outThreadList);
	virtual void GetThreadInfo(thread_id thread, thread_info& outThreadInfo);
	virtual void GetImageList(team_id team, image_list& outImageList);

protected:
	virtual ~DRemoteProxyNub();

private:

	void QuitLooper(void);

	friend class ProxyNubTCPLooper;
	friend class ProxyNubUDPLooper;

	DTeam* fTeam;									// we need a DTeam to manage the symbol information etc.
	team_id fRemoteTeamID;					// remote-host team_id of the target
	TCPLooper* fTCPLooper;					// thread that actually listens to the remote proxy
	TCPMessenger* fTCPMessenger;	// cached object for talking to the remote proxy
	UDPLooper* fUDPLooper;					// same as above if using UDP
	UDPMessenger* fUDPMessenger;
	struct sockaddr_in fRemoteAddress;	// IP address for the proxy for the remote debug target
	port_id fResponsePort;						// kernel port that responses will be written back through
	bool fUsingTCP;									// which transport are we using?

	// we cache all pages we've seen until the cache is invalidated
	struct CacheEntry
	{
		ptr_t base;						// base address of this page
		unsigned char* data;		// the data of this page
	};
	typedef std::vector<CacheEntry*> page_cache;
	page_cache fCache;

	CacheEntry* DataInCache(ptr_t addr, size_t size) const;
	void FlushPageCache();

	// messages that originate on the nub need to be handled in their own thread,
	// to avoid deadlock issues, so we pass them through this port to be handled
	// on the receiving end.
	port_id fNubMessagePort;

	static int32 nub_message_thread(void* arg);
	void HandleNubMessages();

	// remote to local image mapping
	void MapRemoteImageToLocal(const image_info& remoteImage, image_info &localImageInfo) const;
	const char* fRemoteBinRoot;
};

#endif
