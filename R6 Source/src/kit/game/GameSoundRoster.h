//	GameSoundRoster.h

#if !defined(_GAME_SOUND_ROSTER_H)
#define _GAME_SOUND_ROSTER_H

class BGameSoundDevice;
class BWindow;


//	index for MakeGameSoundDevice()
#define B_DEFAULT_GAME_SOUND ((int32)-2)


class BGameSoundRoster
{
public:

static	BGameSoundRoster *
					Roster();

		void		Done();

		int32		CountDevices();
		status_t	GetDeviceNameAt(
							int32 inIndex,
							char * ioDevice,
							size_t inDeviceSize,
							char * ioVendor,
							size_t inVendorSize);
		status_t	MakeGameSoundDevice(
							BGameSoundDevice ** outSound,
							int32 inIndex = B_DEFAULT_GAME_SOUND);

#if _ADVANCED
		status_t	RunDevicePanel(
							BGameSoundDevice ** outSound,
							int32 inToSelect = B_DEFAULT_GAME_SOUND);
		status_t	RunDevicePanel(
							BMessenger toNotify,
							int32 inToSelect = B_DEFAULT_GAME_SOUND);
#endif

private:

	friend class BGameSoundDevice;

							BGameSoundRoster();
virtual						~BGameSoundRoster();

static	void				Unregister(
									BGameSoundDevice * device);

		BWindow *			m_devicePanel;
static	BLocker				_s_lock;
static	BGameSoundRoster *	m_roster;
		void *				m_sem_list;

		BWindow *	make_window(
							int32 inToSelect,
							BMessenger * inMessenger,
							sem_id inSemaphorem,
							BGameSoundDevice ** outSound);

virtual	status_t	_Reserved_GameSoundRoster_1(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_2(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_3(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_4(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_5(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_6(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_7(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_8(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_9(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_10(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_11(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_12(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_13(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_14(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_15(void *, ...);
virtual	status_t	_Reserved_GameSoundRoster_16(void *, ...);

		uint32		_reserved_GameSoundRoster_[32];
};

#endif	//	_GAME_SOUND_ROSTER_H
