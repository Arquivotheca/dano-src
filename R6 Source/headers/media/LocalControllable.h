/*******************************************************************************
/
/ File:          LocalControllable.h
/
/ Description:   An alternative to using BControllable nodes to host parameter
/                webs.  A BLocalControllable implements a repository for
/                parameter data that is valid only within the team that
/                created it.
/
/	Copyright 2000, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_LOCAL_CONTROLLABLE_H_)
#define _LOCAL_CONTROLLABLE_H_

#include <SupportDefs.h>
#include <OS.h>

class BParameterWeb;
class BHandler;
class BList;
class BMessenger;
class BMessage;

class BLocalControllable
{
protected:
		virtual ~BLocalControllable();

public:
		status_t GetParameterWeb(
				BParameterWeb ** out_web);
				
		int32 ID() const;

		status_t ApplyParameterData(
				const void * value,
				size_t size);

		status_t MakeParameterData(
				const int32 * controls,
				int32 count,
				void * buf,
				size_t * ioSize);

		status_t MakeParameterData(
				void * buf,
				size_t * ioSize);

		// no base implementation

		virtual	status_t StartControlPanel(
				BMessenger * out_messenger);

protected:

		BLocalControllable();

		// required hooks:

		virtual	status_t GetParameterValue(
				int32 id,
				bigtime_t * last_change,
				void * value,
				size_t * ioSize) = 0;

		virtual	void SetParameterValue(
				int32 id,
				bigtime_t when,
				const void * value,
				size_t size) = 0;

		// you must clear the parameter web by calling
		// SetParameterWeb(NULL) before the BLocalControllable
		// destructor is called.
		
		status_t SetParameterWeb(
				BParameterWeb * web);

		// it is ONLY safe to call this method from your
		// GetParameterValue()/SetParameterValue() implementations.

		const BParameterWeb * Web() const;

		status_t BroadcastChangedParameter(
				int32 id);

		status_t BroadcastNewParameterValue(
				bigtime_t when, 				//	performance time
				int32 id,						//	parameter ID
				const void * newValue,
				size_t valueSize);

private:
		// unimplemented
		BLocalControllable(
				const BLocalControllable & clone);
		BLocalControllable & operator=(
				const BLocalControllable & clone);

		// stuffing
virtual		status_t _Reserved_LocalControllable_0(void *);
virtual		status_t _Reserved_LocalControllable_1(void *);
virtual		status_t _Reserved_LocalControllable_2(void *);
virtual		status_t _Reserved_LocalControllable_3(void *);
virtual		status_t _Reserved_LocalControllable_4(void *);
virtual		status_t _Reserved_LocalControllable_5(void *);
virtual		status_t _Reserved_LocalControllable_6(void *);
virtual		status_t _Reserved_LocalControllable_7(void *);
virtual		status_t _Reserved_LocalControllable_8(void *);
virtual		status_t _Reserved_LocalControllable_9(void *);
virtual		status_t _Reserved_LocalControllable_10(void *);
virtual		status_t _Reserved_LocalControllable_11(void *);
virtual		status_t _Reserved_LocalControllable_12(void *);
virtual		status_t _Reserved_LocalControllable_13(void *);
virtual		status_t _Reserved_LocalControllable_14(void *);
virtual		status_t _Reserved_LocalControllable_15(void *);

		class ObserverSet;
		friend class BLocalControllable::ObserverSet;

		friend class BParameter;
		friend class BParameterWeb;

		const int32       mID;
		BParameterWeb *   mWeb;
		const team_id     mTeam;
		uint32            mChangeCount;
		ObserverSet *     mObservers;
		volatile bool     mRegistered;
		uint32            _reserved_local_controllable[16];

		static status_t SetValue(
			BParameterWeb * inWeb,
			int32 parameterID,
			bigtime_t when,
			const void * newValue,
			size_t valueSize);

		static status_t GetValue(
			BParameterWeb * inWeb,
			int32 parameterID,
			bigtime_t * outLastChange,
			void * outValue,
			size_t * ioSize);
			
		static status_t AddObserver(
			BParameterWeb * ofWeb,
			const BMessenger& messenger,
			int32 notificationType);

		static status_t RemoveObserver(
			BParameterWeb * ofWeb,
			const BMessenger& messenger,
			int32 notificationType);
		
		void Broadcast(
			BMessage* message);
};

#endif // _LOCAL_CONTROLLABLE_H_
