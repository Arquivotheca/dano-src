
#ifndef _SUPPORT2_LOOPER_H
#define _SUPPORT2_LOOPER_H

#include <OS.h>
#include <support2/Atom.h>
#include <support2/SupportDefs.h>
#include <support2/IBinder.h>
#include <support2/StreamPipe.h>
#include <support2/Team.h>
#include <support2/Vector.h>
#include <image.h>

namespace B {
namespace Support2 {

class BParcel;
class BImage;
struct handle_refs;

/*----------------------------------------------------------------*/

class BKernelOStrPipe : public BBufferedOutputPipe, public BStreamOutputPipe
{
	public:		BKernelOStrPipe(IByteOutput::arg stream, int32 bufferSize)
					: BStreamOutputPipe(stream,bufferSize) {};
};

class BKernelIStrPipe : public BBufferedInputPipe, public BStreamInputPipe
{
	public:		BKernelIStrPipe(IByteInput::arg stream, int32 bufferSize)
					: BStreamInputPipe(stream,bufferSize) {};
};

/*----------------------------------------------------------------*/
/*----- BLooper class --------------------------------------------*/

class BLooper {
	
	public:

		static	BLooper *				This();
		static	BLooper *				This(const atom_ptr<BTeam>& team);
		
		static	void					SetThreadPriority(int32 priority);
		
		static	status_t				Loop();
		
		//!	spawn_thread for binder debugging.
		/*!	This is just like spawn_thread(), except it takes care of propagating
			simulated team information when debugging the binder driver.
		*/
		static	thread_id				SpawnThread(	thread_func		function_name, 
														const char 		*thread_name, 
														int32			priority, 
														void			*arg);
		
		static	status_t				SpawnLooper();
		
		static	int32					Thread();
		static	atom_ptr<BTeam>			Team();
		static	int32					TeamID();

		static	status_t				SetRootObject(IBinder::ptr rootNode);
		static	IBinder::ptr			GetRootObject(thread_id id, team_id team=-1);

		static	IBinder::ptr			GetProxyForHandle(int32 handle);
		static	IBinder::ref			GetProxyRefForHandle(int32 handle);

				status_t				Transact(	int32 handle,
													uint32 code, const BParcel& data,
													BParcel* reply, uint32 flags);
		
		static	void					Sync();
		
		// Only called by the main thread during static initialization
		static	status_t				InitMain(const atom_ptr<BTeam>& team = atom_ptr<BTeam>());
		static	status_t				InitOther(const atom_ptr<BTeam>& team = atom_ptr<BTeam>());

		// For use only by IBinder proxy object
				status_t				IncrefsHandle(int32 handle);
				status_t				AcquireHandle(int32 handle);
				status_t				ReleaseHandle(int32 handle);
				status_t				DecrefsHandle(int32 handle);
				status_t				AttemptAcquireHandle(int32 handle);
		static	status_t				ExpungeHandle(int32 handle);

	private:
	
		friend	class BTeam;
		friend	class BImage;			// Accesses m_dyingAddons

										BLooper(const atom_ptr<BTeam>& team = atom_ptr<BTeam>());
										~BLooper();

		static	int32					TLS;
		static	void					_SetNextEventTime(bigtime_t when);
		static	bool					_ResumingScheduling();
		static	void					_ClearSchedulingResumed();
		static	int32					_ThreadEntry(void *info);
		static	int32					_DriverLoop(BLooper *parent);
		static	int32					_Loop(BLooper *parent);
		static	void					_DeleteSelf(void *self);
		static	status_t				_BufferReply(const BParcel& buffer, void* context);
		static	void					_BufferFree(const void* data, ssize_t len, void* context);
		
				IBinder::ptr			_GetRootObject(thread_id id, team_id team);
				void					_SetThreadPriority(int32 priority);
				status_t				_HandleCommand(int32 cmd);
				status_t				_WaitForCompletion(	BParcel *reply = NULL,
															status_t *acquireResult = NULL);
				status_t				_WriteTransaction(	int32 cmd, uint32 binderFlags,
															int32 handle, uint32 code,
															const BParcel& data);
				status_t				_Reply(uint32 flags, const BParcel& reply);

				int32					_Main();
				int32					_LoopSelf();

				atom_ptr<BTeam>			m_team;
				team_id					m_teamID;
				int32					m_thid;
				int32					m_priority;
				int32					m_binderDesc;
				BKernelIStrPipe			m_in;
				BKernelOStrPipe			m_out;
				int32					m_flags;
				
				BVector<image_id>		m_dyingAddons;
};

} } // namespace B::Support2

#endif /* _SUPPORT2_LOOPER_H */
