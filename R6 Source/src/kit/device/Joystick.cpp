/* ++++++++++
	FILE:	Joystick.cpp
	REVS:	$Revision: 1.4 $
	NAME:	herold
	DATE:	Tue Jun  4 14:36:05 PDT 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <ctype.h>

#include <List.h>
#include <Debug.h>
#include <Entry.h>
#include <String.h>
#include <Joystick.h>
#include <Entry.h>
#include <Path.h>
#include <FindDirectory.h>

#include "JoystickTweaker.h"




#if __POWERPC__ // BeBox specific; tested for in Open()
const int adjust_point = 2047;
const int adjust_factor = 3;
#endif // __POWERPC__

static bool 
joy_debug()
{
	static bool isit, determined;
	if (!determined) {
		const char * str = getenv("JOYSTICK_DEBUG");
		if (str && atoi(str) > 0) isit = true;
		determined = true;
	}
	return isit;
}

/* ----------
	constructor
----- */

BJoystick::BJoystick() :
	timestamp(0),
	horizontal(0),
	vertical(0),
	button1(FALSE),
	button2(FALSE)

{
	ffd = -1;
	_fDevices = NULL;
	m_info = NULL;
	m_dev_name = NULL;
	_mBeBoxMode = false;
}

/* ----------
	destructor
----- */

BJoystick::~BJoystick()
{
	if (ffd >= 0)
		close(ffd);
	if (_fDevices != NULL) for (int ix=0; ix<_fDevices->CountItems(); ix++) {
		free(_fDevices->ItemAtFast(ix));
	}
	delete _fDevices;
	delete m_info;
	free(m_dev_name);
}


/* ----------
	open - actually open the device.
----- */

status_t
BJoystick::Open(const char *portName)
{
	return Open(portName, true);
}


status_t
BJoystick::Open(const char * portName, bool enter_enhanced)
{
	static struct {
		char	*object_name;	/* name for users of this object */
		char	*dev_name;		/* name for the driver */
	} names[4] = {
		{ "joystick1", "/dev/joystick/joystick_1" },
		{ "joystick2", "/dev/joystick/joystick_2" },
		{ "joystick3", "/dev/joystick/joystick_3" },
		{ "joystick4", "/dev/joystick/joystick_4" }
	};
	char	*dev = NULL;
	int		i;
	char	path[1024];
	struct stat	stbuf;

	// look for passed name, map to device name
	for (i = 0; !dev && i < sizeof (names) / sizeof (names[0]); i++)
		if (!strcmp (names[i].object_name, portName))
			dev = names[i].dev_name;
	
	if (!dev) {									// name not found?
		/* look for device name */
		if (strncmp(portName, "/dev/", 5)) {
			strcpy(path, "/dev/joystick/");
			strcat(path, portName);
		}
		else {
			strcpy(path, portName);
		}
		if (stat(path, &stbuf)) {
			return errno;
		}
		dev = path;
	}

	if (ffd >= 0)								// it's already open
		close(ffd);								// close to reopen

	_mBeBoxMode = false;
#if __POWERPC__
	const char * ptr = strrchr(dev, '/');
	if (ptr) {
		ptr++;
	}
	else {
		ptr = dev;
	}
	if (!strncmp(ptr, "joystick_", 9)) {
		_mBeBoxMode = true;
	}
#endif	//	__POWERPC__

	ffd = open(dev, O_RDONLY);					// open the driver
	free(m_dev_name);
	delete m_info;
	m_info = NULL;
	if (ffd >= 0) {
		m_dev_name = strdup(dev);
		if (enter_enhanced) EnterEnhancedMode();
	}
	else {
		m_dev_name = NULL;
	}
	return(ffd);
}

/* ----------
	Close - close the underlying device
----- */

void
BJoystick::Close()
{
	if (ffd >= 0)
		close(ffd);
	ffd = -1;
	delete m_info;
	free(m_dev_name);
	m_info = NULL;
	m_dev_name = NULL;
}

/* ----------
	Update - update the joystick position and button state
----- */

status_t
BJoystick::Update()
{
	status_t err = B_IO_ERROR;

	if (ffd < 0)
		return B_ERROR;

	if (!m_info) {
		joystick j;
		if (read (ffd, &j, sizeof(j)) >= 0) {
			timestamp = j.timestamp;
			horizontal = j.horizontal;
			vertical = j.vertical;
			button1 = j.button1;
			button2 = j.button2;
			err = B_OK;
#if __POWERPC__
			if (_mBeBoxMode) {	//	BeBox joystick range fix
				if (horizontal < adjust_point) {
					horizontal = adjust_point-(adjust_point-horizontal)*adjust_factor;
				}
				if (vertical < adjust_point) {
					vertical = adjust_point-(adjust_point-vertical)*adjust_factor;
				}
			}
#endif	//	__POWERPC__
		}
	}
	else {
		if (read(ffd, &m_info->data, sizeof(m_info->data)*m_info->module.num_sticks) > 0) {
			Calibrate(&m_info->data);
			/* we still update the legacy information to be nice */
			timestamp = m_info->data.timestamp;
			horizontal = 2047-(m_info->data.axes[0]>>4);
			vertical = 2047-(m_info->data.axes[1]>>4);
			button1 = !(m_info->data.buttons & 0x1);
			button2 = !(m_info->data.buttons & 0x2);
			err = B_OK;
		}
	}

	return B_OK;
}

status_t
BJoystick::SetMaxLatency(
	bigtime_t max_latency)
{
	if (ffd < 0) return B_ERROR;
	return ioctl(ffd, B_JOYSTICK_SET_MAX_LATENCY, &max_latency, sizeof(max_latency));
}

/* device roster -- tell user about available valid choices */

int32
BJoystick::CountDevices()
{
	if (_fDevices == NULL) {
		ScanDevices();
	}
	return _fDevices->CountItems();
}

status_t
BJoystick::GetDeviceName(
	int32 n,
	char * name,
	size_t bufSize)
{
	if (_fDevices == NULL) {
		ScanDevices();
	}
	if ((n<0) || (n>=_fDevices->CountItems())) {
		return B_BAD_INDEX;
	}
	if (strlen((char*)_fDevices->ItemAt(n)) >= bufSize) {
		return B_NAME_TOO_LONG;
	}
	strcpy(name, (char*)_fDevices->ItemAt(n));
	return B_OK;
}

static void
recursive_scan(
	const char * prefix,
	BList * dest,
	BList & forbidden)
{
	char * path = (char *)malloc(MAXPATHLEN);
	struct stat stbuf;
	struct dirent * dent;
	DIR * dir;

	if (!path) {
		return; /* out of memory */
	}
	if ((dir = opendir(prefix)) == NULL) {
		free(path);
		return;
	}
	while ((dent = readdir(dir)) != NULL) {
		if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..")) {
			continue;
		}
		strcpy(path, prefix);
		strcat(path, "/");
		strcat(path, dent->d_name);
		if (stat(path, &stbuf)) {
			continue;
		}
		if (S_ISDIR(stbuf.st_mode)) {
			recursive_scan(path, dest, forbidden);
		}
		else {
			/* remove the /dev/joystick/ prefix (14 chars) */
			ASSERT(!strncmp(path, "/dev/joystick/", 14));
			char * copy = NULL;
			for (int ix=0; ix<forbidden.CountItems(); ix++) {
				if (forbidden.ItemAt(ix) && !strcmp(path+14, (char *)forbidden.ItemAt(ix))) {
					goto not_this_one;
				}
			}
			copy = strdup(path+14);
		not_this_one:
			if (copy != NULL) {
				dest->AddItem(copy);
			}
		}
	}
	closedir(dir);
	free(path);
}

void
BJoystick::ScanDevices(
	bool use_disabled)
{
	if (_fDevices != NULL) for (int ix=0; ix<_fDevices->CountItems(); ix++) {
		free(_fDevices->ItemAt(ix));
	}
	else {
		_fDevices = new BList;
	}
	_fDevices->MakeEmpty();
	BList forbidden;
	BPath path;
	if (!use_disabled && !find_directory(B_COMMON_SETTINGS_DIRECTORY, &path)) {
		path.Append("disabled_joysticks");
		FILE * f = fopen(path.Path(), "r");
		if (f) {
			char line[300];
			char arg[300];
			while (true) {
				line[0] = 0;
				fgets(line, 299, f);
				if (!line[0]) break;
				char * ptr = line;
				while (*ptr && isspace(*ptr)) ptr++;
				if (!*ptr || *ptr == '#') {
					continue;
				}
				if (1 == sscanf(ptr, "disable = \"%299[^\"\n]\"", arg)) {
					forbidden.AddItem(strdup(arg));
				}
			}
			fclose(f);
		}
	}
	recursive_scan("/dev/joystick", _fDevices, forbidden);
	for (int ix=0; ix<forbidden.CountItems(); ix++) {
		free(forbidden.ItemAt(ix));
	}
}


bool
BJoystick::EnterEnhancedMode(
	const entry_ref * ref)
{
	if (joy_debug()) fprintf(stderr, "EnterEnhancedMode(%s (%s))\n", ref ? ref->name : "(NULL)", 
		m_info ? "already there" : "will try");
	if (m_info) return true;	/* already in the mode */
	status_t err = gather_enhanced_info(ref);
	if (err < B_OK) {
		if (joy_debug()) fprintf(stderr, "gather_enhaced_info() returns error %x\n", err);
		return false;
	}
	err = ioctl(ffd, B_JOYSTICK_SET_DEVICE_MODULE, &m_info->module, sizeof(m_info->module));
	if (err < B_OK) {
		if (joy_debug()) fprintf(stderr, "ioctl(%d) returns error %x\n", ffd, err);
		delete m_info;
		m_info = NULL;
		return false;
	}
	int res = ioctl(ffd, B_JOYSTICK_GET_DEVICE_MODULE, &m_info->module, sizeof(m_info->module));
	timestamp = 0;
	Update();
	return true;
}

int32 
BJoystick::CountSticks()
{
	if (!m_info) return 1;
	return m_info->module.num_sticks;
}

int32
BJoystick::CountAxes()
{
	if (!m_info) return 2;
	return m_info->module.num_axes;
}

int32
BJoystick::CountHats()
{
	if (!m_info) return 0;
	return m_info->module.num_hats;
}

int32
BJoystick::CountButtons()
{
	if (!m_info) return 2;
	return m_info->module.num_buttons;
}

status_t
BJoystick::GetAxisValues(
	int16 * out_values,
	int32 for_stick)
{
	if (!m_info) {
		out_values[0] = (2047-horizontal)*16;	/* normalize to full range */
		out_values[1] = (2047-vertical)*16;
		return B_OK;
	}
	if (for_stick < 0 || for_stick >= m_info->module.num_sticks) return B_BAD_VALUE;
	memcpy(out_values, (&m_info->data)[for_stick].axes, sizeof(int16)*m_info->module.num_axes);
	return B_OK;
}

status_t
BJoystick::GetHatValues(
	uint8 * out_hats,
	int32 for_stick)
{
	if (!m_info) return B_BAD_VALUE;
	if (for_stick < 0 || for_stick >= m_info->module.num_sticks) return B_BAD_VALUE;
	memcpy(out_hats, (&m_info->data)[for_stick].hats, sizeof(uint8)*m_info->module.num_hats);
	return B_OK;
}

uint32
BJoystick::ButtonValues(
	int32 for_stick)
{
	if (!m_info) {
		return (button1 ? 0x00 : 0x01) | (button2 ? 0x00 : 0x02);
	}
	if (for_stick < 0 || for_stick >= m_info->module.num_sticks) return B_BAD_VALUE;
	return (&m_info->data)[for_stick].buttons;
}

status_t
BJoystick::GetAxisNameAt(
	int32 index,
	BString * out_name)
{
	if (!m_info) {
		static const char * s_names[] = {
			"left-right",
			"up-down"
		};
		if (index < 0 || index > 1) return B_BAD_INDEX;
		out_name->SetTo(s_names[index]);
		return B_OK;
	}
	if (index < 0 || index >= m_info->module.num_axes) return B_BAD_INDEX;
	out_name->SetTo(m_info->axis_names[index]);
	return B_OK;
}

status_t
BJoystick::GetHatNameAt(
	int32 index,
	BString * out_name)
{
	if (!m_info) return B_BAD_VALUE;
	if (index < 0 || index >= m_info->module.num_hats) return B_BAD_INDEX;
	out_name->SetTo(m_info->hat_names[index]);
	return B_OK;
}

status_t
BJoystick::GetButtonNameAt(
	int32 index,
	BString * out_name)
{
	if (!m_info) {
		static const char * s_names[] = {
			"top",
			"trigger"
		};
		if (index < 0 || index > 1) return B_BAD_INDEX;
		out_name->SetTo(s_names[index]);
		return B_OK;
	}
	if (index < 0 || index >= m_info->module.num_buttons) return B_BAD_INDEX;
	out_name->SetTo(m_info->button_names[index]);
	return B_OK;
}

status_t
BJoystick::GetControllerModule(
	BString * out_name)
{
	if (!m_info) {
		out_name->SetTo("Legacy");
		return B_OK;
	}
	out_name->SetTo(m_info->module.module_name);
	return B_OK;
}

status_t
BJoystick::GetControllerName(
	BString * out_name)
{
	if (!m_info) {
		out_name->SetTo("2-axis");
		return B_OK;
	}
	out_name->SetTo(m_info->module.device_name);
	return B_OK;
}


status_t
BJoystick::gather_enhanced_info(
	const entry_ref * ref)
{
	/* figure out config file path */
	BPath path;
	status_t err;
	char * n = "unknown device";
	if (!ref) {
		if (!m_dev_name) return ENODEV;
		n = m_dev_name;
		if (!strncmp(n, "/dev/", 5)) {
			n += 5;
		}
		if (!strncmp(n, "joystick/", 9)) {
			n += 9;
		}
		err = find_directory(B_COMMON_SETTINGS_DIRECTORY, &path);
		if (err < B_OK) return err;
		path.Append("joysticks");
		path.Append(n);
	}
	else {
		BEntry ent(ref);
		err = ent.GetPath(&path);
		if (err < B_OK) return err;
	}
	if (joy_debug()) fprintf(stderr, "trying '%s'\n", path.Path());
	FILE * f = fopen(path.Path(), "r");
	if (!f) {
		if (joy_debug()) fprintf(stderr, "BJoystick: no settings file for '%s', using legacy mode\n",  n);
		return ENOENT;
	}
	/* parse config file */
	char line[256];
	char arg[256];
	char * ptr;
	m_info = new _joystick_info;
	int l = 0;
	{
		BEntry ent(path.Path(), true);
		BPath fn_path;
		ent.GetPath(&fn_path);
		strncpy(m_info->file_name, fn_path.Leaf(), 255);
		m_info->file_name[255] = 0;
	}
	while (true) {
		line[0] = 0;
		if (ferror(f) || feof(f)) break;
		fgets(line, 256, f);
		if (!line[0]) break;
		l++;
		ptr = line;
		int val;
		int cal[6];
		while (*ptr && isspace(*ptr)) ptr++;
		if (!*ptr || *ptr == '#') continue;
//		if (joy_debug()) fprintf(stderr, "line: %s", ptr);
		if (1 == sscanf(ptr, "module = \"%255[^\"\n]\"", arg)) {
			if (!arg[0]) {
				goto file_err;
			}
			strncpy(m_info->module.module_name, arg, 64);
			m_info->module.module_name[63] = 0;
		}
		else if (1 == sscanf(ptr, "gadget = \"%255[^\"\n]\"", arg)) {
			if (!arg[0]) {
				goto file_err;
			}
			strncpy(m_info->module.device_name, arg, 64);
			m_info->module.device_name[63] = 0;
		}
		else if (1 == sscanf(ptr, "filename = \"%255[^\"\n]\"", arg)) {
			if (!arg[0]) {
				goto file_err;
			}
			strncpy(m_info->file_name, arg, 255);
			m_info->file_name[255] = 0;
		}
		else if (1 == sscanf(ptr, "num_axes = %d", &val)) {
			if (val < 0 || val > MAX_AXES) {
				goto file_err;
			}
			if (m_info->module.num_axes < val) {
				m_info->module.num_axes = val;
			}
		}
		else if (1 == sscanf(ptr, "num_hats = %d", &val)) {
			if (val < 0 || val > MAX_HATS) {
				goto file_err;
			}
			if (m_info->module.num_hats < val) {
				m_info->module.num_hats = val;
			}
		}
		else if (1 == sscanf(ptr, "num_buttons = %d", &val)) {
			if (val < 0 || val > MAX_BUTTONS) {
				goto file_err;
			}
			if (m_info->module.num_buttons < val) {
				m_info->module.num_buttons = val;
			}
		}
		else if (1 == sscanf(ptr, "num_sticks = %d", &val)) {
			if (val < 0 || val > MAX_STICKS) {
				goto file_err;
			}
			m_info->module.num_sticks = val;
		}
		else if (2 == sscanf(ptr, "axis %d = \"%255[^\"\n]\"", &val, arg)) {
			if (!arg[0]) {
				goto file_err;
			}
			if (val < 0 || val >= MAX_AXES) {
				goto file_err;
			}
			free(m_info->axis_names[val]);
			m_info->axis_names[val] = strdup(arg);
			if (m_info->module.num_axes < val+1) {
				m_info->module.num_axes = val+1;
			}
		}
		else if (2 == sscanf(ptr, "hat %d = \"%255[^\"\n]\"", &val, arg)) {
			if (!arg[0]) {
				goto file_err;
			}
			if (val < 0 || val >= MAX_HATS) {
				goto file_err;
			}
			free(m_info->hat_names[val]);
			m_info->hat_names[val] = strdup(arg);
			if (m_info->module.num_hats < val+1) {
				m_info->module.num_hats = val+1;
			}
		}
		else if (2 == sscanf(ptr, "button %d = \"%255[^\"\n]\"", &val, arg)) {
			if (!arg[0]) {
				goto file_err;
			}
			if (val < 0 || val > MAX_BUTTONS) {
				goto file_err;
			}
			free(m_info->button_names[val]);
			m_info->button_names[val] = strdup(arg);
			if (m_info->module.num_buttons < val+1) {
				m_info->module.num_buttons = val+1;
			}
		}
		else if (!strncmp(ptr, "config =", 8) || !strncmp(ptr, "config=", 7)) {
			int cnt = 0;
			int val;
			ptr += 7;
			if (*ptr == '=') ptr++;
			while (*ptr && isspace(*ptr)) ptr++;
			while (1 == sscanf(ptr, "%02x", &val)) {
				if (cnt >= MAX_CONFIG_SIZE) {
					goto file_err;
				}
				m_info->module.device_config[cnt++] = val;
				ptr += 2;
			}
			m_info->module.config_size = cnt;
		}
		else if (7 == sscanf(ptr, "calibrate %d = %d , %d , %d , %d , %d , %d",
			&val, cal, cal+1, cal+2, cal+3, cal+4, cal+5)) {
			if (val < 0 || val >= MAX_AXES) {
				goto file_err;
			}
			m_info->axis_calib[val].bottom = cal[0];
			m_info->axis_calib[val].start_dead = cal[1];
			m_info->axis_calib[val].end_dead = cal[2];
			m_info->axis_calib[val].top = cal[3];
			m_info->axis_calib[val].bottom_mul = cal[4];
			m_info->axis_calib[val].top_mul = cal[5];
		}
		else if (2 == sscanf(ptr, "autofire %d = %d\n", &val, cal)) {
			if (*cal) {
				m_info->button_autofire = m_info->button_autofire | (1 << val);
			}
			else {
				m_info->button_autofire = m_info->button_autofire & ~(1 << val);
			}
//			if (joy_debug()) fprintf(stderr, "autofire: %x\n", m_info->button_autofire);
		}
		else if (2 == sscanf(ptr, "latch %d = %d\n", &val, cal)) {
			if (*cal) {
				m_info->button_latch = m_info->button_latch | (1 << val);
			}
			else {
				m_info->button_latch = m_info->button_latch & ~(1 << val);
			}
//			if (joy_debug()) fprintf(stderr, "latch: %x\n", m_info->button_latch);
		}
		else {
	file_err:
			if (joy_debug()) fprintf(stderr, "BJoystick: error in '%s' line %d\n", n, l);
			if (joy_debug()) fprintf(stderr, "    the text was: %s\n", ptr);
		}
	}
	fclose(f);
	/* make sure we have gotten data we expect */
	if (!m_info->module.module_name[0] || !m_info->module.device_name[0]) {
		if (joy_debug()) fprintf(stderr, "BJoystick: missing module or gadget info in '%s'\n", n);
		delete m_info;
		m_info = NULL;
		return B_ERROR;
	}
	if (!m_info->module.num_axes || !m_info->module.num_buttons) {
		if (joy_debug()) fprintf(stderr, "BJoystick: missing button or axis in '%s'\n", n);
		delete m_info;
		m_info = NULL;
		return B_ERROR;
	}
	/* make sure we have names for everything */
	for (int ix=0; ix<m_info->module.num_axes; ix++) {
		if (!m_info->axis_names[ix]) {
			char str[20];
			sprintf(str, "axis %d", ix+1);
			m_info->axis_names[ix] = strdup(str);
		}
	}
	for (int ix=0; ix<m_info->module.num_hats; ix++) {
		if (!m_info->hat_names[ix]) {
			char str[20];
			sprintf(str, "hat %d", ix+1);
			m_info->hat_names[ix] = strdup(str);
		}
	}
	for (int ix=0; ix<m_info->module.num_buttons; ix++) {
		if (!m_info->button_names[ix]) {
			char str[20];
			sprintf(str, "button %d", ix+1);
			m_info->button_names[ix] = strdup(str);
		}
	}
	/* I suppose we're OK */
	return B_OK;
}


bool
BJoystick::IsCalibrationEnabled()
{
	return m_info && m_info->calibrate;
}


status_t
BJoystick::EnableCalibration(
	bool value)
{
	if (!m_info) return B_NO_INIT;
	m_info->calibrate = value;
	return B_OK;
}


void
BJoystick::Calibrate(
	struct _extended_joystick * stick)
{
	if (!m_info || !m_info->calibrate) {
//		if (joy_debug()) fprintf(stderr, "no calibration\n");
		return;
	}
	for (int ix=0; ix<m_info->module.num_axes; ix++) {
		if (m_info->axis_calib[ix].bottom) {	/* calibrate axis at all? */
			int m;
			if (stick->axes[ix] < m_info->axis_calib[ix].start_dead) {
				m = (int)((stick->axes[ix]-m_info->axis_calib[ix].start_dead)
					* m_info->axis_calib[ix].bottom_mul)>>7;
			}
			else if (stick->axes[ix] > m_info->axis_calib[ix].end_dead) {
				m = (int)((stick->axes[ix]-m_info->axis_calib[ix].end_dead)
					* m_info->axis_calib[ix].top_mul)>>7;
			}
			else {
				m = 0;
			}
			if (m < -32768) {
				stick->axes[ix] = -32768;
			}
			else if (m > 32767) {
				stick->axes[ix] = 32767;
			}
			else {
				stick->axes[ix] = m;
			}
		}
	}
//	if (joy_debug()) fprintf(stderr, "latch %x autofire %x\n", m_info->button_latch, m_info->button_autofire);
	uint32 orig_btn = stick->buttons;
	uint32 changed = orig_btn ^ m_info->prev_read;
	if (changed & m_info->button_latch) {
		m_info->prev_latch ^= (orig_btn & m_info->button_latch);
	}
	uint32 ret = (orig_btn & ~m_info->button_latch) | m_info->prev_latch;
	m_info->prev_auto = (m_info->prev_auto ^ m_info->button_autofire) & ret;
	ret = (ret & ~m_info->button_autofire) | m_info->prev_auto;
	m_info->prev_read = orig_btn;
	stick->buttons = ret;
}


status_t
BJoystick::save_config(
	const entry_ref * ref)
{
	if (!m_info) return B_NO_INIT;
	/* figure out config file path */
	BPath path;
	status_t err;
	if (!ref) {
		if (!m_dev_name) return ENODEV;
		char * n = m_dev_name;
		if (!strncmp(n, "/dev/", 5)) {
			n += 5;
		}
		if (!strncmp(n, "joystick/", 9)) {
			n += 9;
		}
		err = find_directory(B_COMMON_SETTINGS_DIRECTORY, &path);
		if (err < B_OK) return err;
		path.Append("joysticks");
		path.Append(n);
	}
	else {
		BEntry ent(ref);
		err = ent.GetPath(&path);
		if (err < B_OK) return err;
	}
	char str[1024];
	sprintf(str, "%s.%d", path.Path(), getpid());
	if (joy_debug()) fprintf(stderr, "trying to write '%s'\n", str);
	unlink(path.Path());	/* if there's a symlink or something */
	FILE * f = fopen(path.Path(), "w");
	if (!f) {
		return errno ? errno : EPERM;
	}
	fprintf(f, "# Settings file for %s %s (automatically generated).\n", 
		m_info->module.module_name, m_info->module.device_name);
	fprintf(f, "# Written by Joystick Preferences. Copyright 1998 Be Incorporated.\n");
	fprintf(f, "# This device was configured for '%s'.\n", m_dev_name ? m_dev_name : 
		"an unknown device");
	fprintf(f, "\n");
	fprintf(f, "module = \"%s\"\n", m_info->module.module_name);
	fprintf(f, "gadget = \"%s\"\n", m_info->module.device_name);
	fprintf(f, "num_axes = %d\n", m_info->module.num_axes);
	fprintf(f, "num_hats = %d\n", m_info->module.num_hats);
	fprintf(f, "num_buttons = %d\n", m_info->module.num_buttons);
	fprintf(f, "filename = \"%s\"\n", m_info->file_name);
	fprintf(f, "\n");
	for (int ix=0; ix<m_info->module.num_axes; ix++) {
		fprintf(f, "axis %d = \"%s\"\n", ix, m_info->axis_names[ix]);
	}
	for (int ix=0; ix<m_info->module.num_hats; ix++) {
		fprintf(f, "hat %d = \"%s\"\n", ix, m_info->hat_names[ix]);
	}
	for (int ix=0; ix<m_info->module.num_buttons; ix++) {
		fprintf(f, "button %d = \"%s\"\n", ix, m_info->button_names[ix]);
	}
	fprintf(f, "\n");
	if (m_info->module.config_size > 0) {
		fprintf(f, "config = ");
		for (int ix=0; ix<m_info->module.config_size; ix++) {
			fprintf(f, "%02x", (unsigned char)m_info->module.device_config[ix]);
		}
		fprintf(f, "\n\n");
	}
	for (int ix=0; ix<m_info->module.num_axes; ix++) {
		fprintf(f, "calibrate %d = %d, %d, %d, %d, %d, %d\n", ix, 
			m_info->axis_calib[ix].bottom, 
			m_info->axis_calib[ix].start_dead, 
			m_info->axis_calib[ix].end_dead, 
			m_info->axis_calib[ix].top, 
			m_info->axis_calib[ix].bottom_mul, 
			m_info->axis_calib[ix].top_mul);
	}
	fprintf(f, "\n");
	for (int ix=0; ix<m_info->module.num_buttons; ix++) {
		if (m_info->button_autofire & (1 << ix)) {
			fprintf(f, "autofire %d = 1\n", ix);
		}
		if (m_info->button_latch & (1 << ix)) {
			fprintf(f, "latch %d = 1\n", ix);
		}
	}
	fclose(f);
	return B_OK;
}



/* ----------
	fragile base class padding
----- */

void BJoystick::_ReservedJoystick1() { /* no longer virtual */ }
void BJoystick::_ReservedJoystick2() {}
void BJoystick::_ReservedJoystick3() {}

#if !_PR3_COMPATIBLE_

status_t BJoystick::_Reserved_Joystick_4(void *, ...) { return B_ERROR; }
status_t BJoystick::_Reserved_Joystick_5(void *, ...) { return B_ERROR; }
status_t BJoystick::_Reserved_Joystick_6(void *, ...) { return B_ERROR; }

#endif



_BJoystickTweaker::_BJoystickTweaker(
	BJoystick & stick) :
	m_stick(stick)
{
}

_BJoystickTweaker::~_BJoystickTweaker()
{
}

void
_BJoystickTweaker::scan_including_disabled()
{
	if (m_stick._fDevices) {
		for (int ix=0; ix<m_stick._fDevices->CountItems(); ix++) {
			free(m_stick._fDevices->ItemAtFast(ix));
		}
		delete m_stick._fDevices;
		m_stick._fDevices = NULL;
	}
	m_stick.ScanDevices(true);
}

status_t
_BJoystickTweaker::save_config(
	const entry_ref * ref)
{
	return m_stick.save_config(ref);
}

BJoystick::_joystick_info * 
_BJoystickTweaker::get_info()
{
	return m_stick.m_info;
}



