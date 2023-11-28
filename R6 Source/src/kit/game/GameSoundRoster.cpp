
#include <Autolock.h>
#include <Window.h>
#include <Button.h>
#include <ListView.h>
#include <ScrollBar.h>
#include <ListItem.h>
#include <StringView.h>
#include <Box.h>
#include <Path.h>
#include <Directory.h>
#include <Entry.h>
#include <OS.h>

#include <assert.h>
#include <stdio.h>

#include <list>
#include <vector>
#include <string>
#include <algorithm>
#include <new>

#include "GameSoundRoster.h"
#include "GameSoundDevice.h"



BGameSoundRoster * BGameSoundRoster::m_roster;

extern int _gs_debug;

struct _gd_info
{
	string addon;
	int32 addon_index;
	string device;
	string vendor;

	bool operator==(const _gd_info & other) const
		{
			return (addon == other.addon) && (addon_index == other.addon_index);
		}
};

struct _gd_loaded_addon;
static list<_gd_loaded_addon> _s_addons;
struct _gd_loaded_addon
{
	string path;
	image_id addon;
	int32 refcount;
	status_t (*info_getter)(int32 inIndex, char * outDevice, int32 inDeviceSize, char * outVendor, int32 inVendorSize);
	BGameSoundDevice * (*device_maker)(const char * inDevice);

	_gd_loaded_addon() :
		path(""),
		addon(-1),
		refcount(0),
		info_getter(0),
		device_maker(0)
		{
		}
	~_gd_loaded_addon()
		{
			refcount = 0;
			addon = -1;
		}
	status_t load(const char * ipath)
		{
			assert(refcount == 0);
			path = ipath;
			addon = load_add_on(path.c_str());
			status_t err = addon;
			if (addon >= 0)
			{
				err = get_image_symbol(addon, "get_device_info", B_SYMBOL_TYPE_TEXT, (void **)&info_getter);
				if (err == B_OK)
					err = get_image_symbol(addon, "make_device", B_SYMBOL_TYPE_TEXT, (void **)&device_maker);
			}
			if (err == B_OK)
				acquire();
			return err;
		}
	bool operator==(const _gd_loaded_addon & other) const
		{
			return addon == other.addon;
		}
	void acquire()
		{
			atomic_add(&refcount, 1);
		}
	void release()
		{
			if (atomic_add(&refcount, -1) == 1)
			{
				image_id to_unload = addon;
				//	warning: this call may delete the "this" object!
				_s_addons.erase(find(_s_addons.begin(), _s_addons.end(), *this));
				unload_add_on(to_unload);
			}
		}
};

static _gd_loaded_addon *
find_addon(
	const char * path)
{
	for (list<_gd_loaded_addon>::iterator ptr(_s_addons.begin());
		ptr != _s_addons.end(); ptr++)
	{
		if ((*ptr).path == path)
		{
			(*ptr).acquire();
			return &(*ptr);
		}
	}
	_gd_loaded_addon la;
	status_t err = la.load(path);
	if (err == B_OK)
	{
		_s_addons.push_back(la);
		list<_gd_loaded_addon>::iterator ptr(_s_addons.end());
		return &(*--ptr);
	}
	return NULL;
}

static vector<_gd_info> _s_devices;
BLocker BGameSoundRoster::_s_lock("GameSoundDevice Devices");

static void
init_directory(
	BDirectory * dir)
{
	BEntry entry;
	BPath path;

	if (_gs_debug > 0)
	{
		dir->GetEntry(&entry);
		entry.GetPath(&path);
		fprintf(stderr, "Scanning %s\n", path.Path());
	}
	while (dir->GetNextEntry(&entry, true) == B_OK)
	{
		if (entry.IsFile() && !entry.GetPath(&path))
		{
			_gd_loaded_addon * addon = find_addon(path.Path());
			if (addon != NULL)
			{
				_gd_info info;
				char device[64];
				char vendor[64];
				int32 index = 0;
				info.addon = path.Path();
				while (!(*addon->info_getter)(index, device, sizeof(device), vendor, sizeof(vendor)))
				{
					if (_gs_debug > 1) fprintf(stderr, "    index %ld: %s; %s\n", index, device, vendor);
					info.addon_index = index;
					info.device = device;
					info.vendor = vendor;
					_s_devices.push_back(info);
					index++;
				}
				addon->release();
			}
		}
	}
}

static void
init_game_devices()
{
	if (_s_devices.size() > 0) return;
	BDirectory dir;
	if (!dir.SetTo("/boot/home/config/add-ons/GameSoundDevice"))
		init_directory(&dir);
	if (!dir.SetTo("/system/add-ons/GameSoundDevice"))
		init_directory(&dir);
}


int32 
BGameSoundRoster::CountDevices()
{
	BAutolock lock(_s_lock);
	init_game_devices();
	return _s_devices.size();
}


status_t 
BGameSoundRoster::GetDeviceNameAt(int32 inIndex, char *ioDevice, size_t inDeviceSize, char *ioVendor, size_t inVendorSize)
{
	BAutolock lock(_s_lock);
	init_game_devices();
	if (inIndex < 0)
	{
		if (inIndex == B_DEFAULT_GAME_SOUND)
		{
			inIndex = 0;	//fixme	first is default until we get control panel going
			goto do_it;
		}
		return B_BAD_INDEX;
	}
do_it:
	if (inIndex >= (int32)_s_devices.size()) return B_BAD_INDEX;
	strncpy(ioDevice, _s_devices[inIndex].device.c_str(), inDeviceSize);
	ioDevice[inDeviceSize-1] = 0;
	strncpy(ioVendor, _s_devices[inIndex].vendor.c_str(), inVendorSize);
	ioVendor[inVendorSize-1] = 0;
	return B_OK;
}


status_t 
BGameSoundRoster::MakeGameSoundDevice(BGameSoundDevice **outSound, int32 inIndex)
{
	BAutolock lock(_s_lock);
	init_game_devices();
	if (inIndex == B_DEFAULT_GAME_SOUND)
		inIndex = 0;
	if (inIndex < 0 || inIndex >= (int32)_s_devices.size()) return B_BAD_INDEX;
	_gd_loaded_addon * addon = find_addon(_s_devices[inIndex].addon.c_str());
	if (addon == NULL) return ENOENT;
	*outSound = (*addon->device_maker)(_s_devices[inIndex].device.c_str());
	if (!*outSound)
	{
		addon->release();
		return ENOSYS;
	}
	(*outSound)->_fAddonInfo = addon;
	return B_OK;
}


#if _ADVANCED
status_t
BGameSoundRoster::RunDevicePanel(
	BGameSoundDevice ** outSound,
	int32 inToSelect)
{
	if
}


BGameSoundRoster *
BGameSoundRoster::Roster()
{
	BAutolock lock(_s_lock);
	if (!m_roster)
		m_roster = new BGameSoundRoster();
	return m_roster;
}

#endif


void
BGameSoundRoster::Done()
{
	delete this;
}


BGameSoundRoster::BGameSoundRoster()
{
	m_devicePanel = NULL;
	m_roster = this;
	m_sem_list = new list<sem_id>;
}


BGameSoundRoster::~BGameSoundRoster()
{
	BAutolock lock(_s_lock);

	//	un-wedge waiting modal dialogs
	list<sem_id> & ll(*reinterpret_cast<list<sem_id> *>(m_sem_list));
	for (list<sem_id>::iterator ptr(ll.begin()); ptr != ll.end(); ptr++)
		delete_sem(*(ptr++));
	delete reinterpret_cast<list<sem_id> *>(m_sem_list);

	//	remove us from view
	m_roster = NULL;
	if (m_devicePanel->Lock())
		m_devicePanel->Quit();
}


void
BGameSoundRoster::Unregister(
	BGameSoundDevice * device)
{
	BAutolock lock(BGameSoundRoster::_s_lock);
	if (device->_fAddonInfo) ((_gd_loaded_addon *)device->_fAddonInfo)->release();
}


#if _ADVANCED

status_t
BGameSoundRoster::RunDevicePanel(
	BGameSoundDevice ** outSound,
	int32 inToSelect)
{
	sem_id s = create_sem(0, "RunDevicePanel Done");
	BLooper * l = BLooper::LooperForThread(find_thread(NULL));
	BWindow * lw = dynamic_cast<BWindow *>(l);
	_s_lock.Lock();
	list<sem_id> & ll(*reinterpret_cast<list<sem_id> *>(m_sem_list));
	ll.push_back(s);
	BWindow * w = make_window(inToSelect, NULL, s, outSound);
	w->Show();
	_s_lock.Unlock();
	status_t err;
	while ((err = acquire_sem_etc(s, 1, B_TIMEOUT, (lw != NULL) ? 100000 : B_INFINITE_TIMEOUT))
			== B_TIMED_OUT)
	{
		if (lw != NULL)
			lw->UpdateIfNeeded();
	}
	_s_lock.Lock();
	//	did we die because of our destructor being called?
	if ((err != B_BAD_SEM_ID) && (m_roster == this))
	{
		ll.erase(ll.find(s));
		delete_sem(s);
	}
	_s_lock.Unlock();
	return err;
}


status_t
BGameSoundRoster::RunDevicePanel(
	BMessenger inToNotify,
	int32 inToSelect)
{
	BAutolock lock(_s_lock);
	if (m_devicePanel->Lock())
	{
		m_devicePanel->Show();
		m_devicePanel->Unlock();
		return B_OK;
	}
	m_devicePanel = make_window(inToSelcet, &inToNotify, -1, NULL);
	m_devicePanel->Show();
	return B_OK;
}


class _GameSoundRosterWindow : public BWindow
{
public:
static	BRect		get_frame()
					{
						BScreen scrn;
						BRect r = scrn.Frame();
						r.left += r.IntegerWidth()-150;
						r.top += r.IntegerHeight()-120;
						r.right = r.left+300;
						r.bottom = r.top+250;
						return r;
					}
static	const char *get_name()
					{
						return "Select a Sound Device";
					}

					enum {
						kOK = 'kOK ',
						kCancel
					};

					_GameSoundRosterWindow(
							int32 inToSelect,
							BMessenger * inToNotify,
							sem_id inSemaphore,
							BGameSoundDevice ** outSound) :
						BWindow(get_frame(), get_name(),
								(inToNotify != NULL) ? B_TITLED_WINDOW : B_MODAL_WINDOW,
								B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE),
						m_select(inToSelect),
						m_semaphore(inSemaphore),
						m_sound(outSound)
					{
						if (inToNotify != NULL)
							m_notify = *inToNotify;

						BBox * bg = new BBox(Bounds(), "box", B_FOLLOW_ALL,
								B_FRAME_EVENTS | B_WILL_DRAW, B_PLAIN_BORDER);
						AddChild(bg);
						bg->SetViewColor(216, 216, 216);

						//	add buttons
						BRect r(bg->Bounds());
						r.InsetBy(10, 10);
						BRect br(r);
						br.left = br.right-80;
						br.top = br.bottom-23;
						BButton * b = new BButton(br, "ok", "OK", new BMessage(kOK));
						b->SetViewColor(216, 216, 216);
						bg->AddChild(b);
						b->ResizeToPreferred();
						br.OffsetBy(-10-br.IntegerWidth(), 0);
						b = new BButton(br, "cancel", "Cancel", new BMessage(kCancel));
						b->SetViewColor(216, 216, 216);
						bg->AddChild(b);
						b->ResizeToPreferred();

						//	populate list view
						r.bottom = b->Frame().top-10;
						r.right -= B_V_SCROLL_BAR_WIDTH;
						r.InsetBy(2, 2);
						m_list = new BListView(r, "list", B_FOLLOW_ALL);

						BAutolock lock(_s_lock);
						init_game_devices();
						for (int ix=0; ix<_s_devices.size(); ix++)
						{
							BStringItem * si = new BStringItem(_s_devices[ix].vendor.c_str());
							m_list->AddItem(si);
						}

						BScrollView * sv = new BScrollView(m_list, "scroller",
							B_FOLLOW_ALL, B_WILL_DRAW, B_FANCY_BORDER);
						bg->AddChild(sv);
						sv->SetViewColor(216, 216, 216);

						//	set selection
						m_list->Select(m_select < 0 ? 0 : m_select);
					}

private:

		BListView *	m_list;
};


BWindow *
BGameSoundRoster::make_window(
	int32 inToSelect,
	BMessenger * inMessenger,
	sem_id inSemaphore,
	BGameSoundDevice ** outSound)
{
	return new _GameSoundRosterWindow(inToSelect, inMessenger, inSemaphore, outSound);
}

#endif


status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_1(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_2(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_3(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_4(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_5(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_6(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_7(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_8(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_9(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_10(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_11(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_12(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_13(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_14(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_15(void *, ...)
{
	return B_ERROR;
}

status_t 
BGameSoundRoster::_Reserved_GameSoundRoster_16(void *, ...)
{
	return B_ERROR;
}
