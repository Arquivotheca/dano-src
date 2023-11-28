/*	Timecode.cpp	*/


#include <stdio.h>

#include "trinity_p.h"

#include "TimeCode.h"




static timecode_info s_timecode_array[] =
{
	{ B_TIMECODE_30_DROP_2, 2, 1800, 18000, 30, "NTSC (30 fps drop)", "%02dh:%02dm:%02ds.%02d", "" },
	{ B_TIMECODE_100, 0, 0, 0, 100, "100 fps", "%02dh:%02dm:%02d.%02ds", "" },
	{ B_TIMECODE_75, 0, 0, 0, 75, "CD (75 fps)", "%02dh:%02dm:%02ds.%02d", "" },
	{ B_TIMECODE_30, 0, 0, 0, 30, "MIDI (30 fps)", "%02dh:%02dm:%02ds.%02d", "" },
	{ B_TIMECODE_30_DROP_2, 2, 1800, 18000, 30, "NTSC (30 fps drop)", "%02dh:%02dm:%02ds.%02d", "" },
	{ B_TIMECODE_30_DROP_4, 4, 3600, 36000, 30, "M/PAL (30 fps drop)", "%02dh:%02dm:%02ds.%02d", "" },
	{ B_TIMECODE_25, 0, 0, 0, 25, "PAL (25 fps)", "%02dh:%02dm:%02ds.%02d", "" },
	{ B_TIMECODE_24, 0, 0, 0, 24, "Film (24 fps)", "%02dh:%02dm:%02ds.%02d", "" },
	{ B_TIMECODE_18, 0, 0, 0, 18, "Super8 (18 fps)", "%02dh:%02dm:%02ds.%02d", "" },
};


#define MICROS_PER_HOUR 3600000000LL
#define SECONDS_PER_HOUR 3600

status_t
us_to_timecode(
	bigtime_t micros,
	int * hours,
	int * minutes,
	int * seconds,
	int * frames,
	const timecode_info * code)
{
	if (code == NULL) {
		code = s_timecode_array;	/* default is first */
	}

	ASSERT(!code->drop_frames || (code->every_nth && code->except_nth));

/* every hour starts afresh */
	*hours = micros/MICROS_PER_HOUR;
	uint32 remainder = micros % MICROS_PER_HOUR;
	int32 frames_per_hour = code->fps_div * SECONDS_PER_HOUR; // unadjusted
	int32 fph_adjust = 0;
	if (code->drop_frames) {
		/* negative */
		fph_adjust = code->drop_frames * (frames_per_hour/code->except_nth -
			frames_per_hour/code->every_nth);
	}
	frames_per_hour += fph_adjust;
	/* frame within the hour */
	int32 hourframe = ((bigtime_t)frames_per_hour*(bigtime_t)remainder)/
		(bigtime_t)MICROS_PER_HOUR;
/* adjust hourframe based on drop to "virtual hour" time */
	if (code->drop_frames) {
		int32 cycle_drop = code->drop_frames*(code->except_nth/code->every_nth-1);
		fph_adjust = cycle_drop*(hourframe/(code->except_nth-cycle_drop));
		int32 integral = hourframe%(code->except_nth-cycle_drop);
		if (integral >= code->every_nth) {
			fph_adjust += code->drop_frames * (1 + (integral-code->every_nth)/
				(code->every_nth-code->drop_frames));
		}
		hourframe += fph_adjust;
	}
/* we now have a "virtual" frame time that we can spread out */
	*frames = hourframe % code->fps_div;
	hourframe /= code->fps_div;
	*seconds = hourframe % 60;
	hourframe /= 60;
	*minutes = hourframe;
	return B_OK;
}


status_t
timecode_to_us(
	int hours, 
	int minutes, 
	int seconds, 
	int frames, 
	bigtime_t * micros, 
	const timecode_info * code)
{
	if (code == NULL) {
		code = s_timecode_array;	/* default is first */
	}
/* correct into linear frames */
	int32 linear = frames+code->fps_div*(seconds+60*minutes);
	int32 frames_per_hour = code->fps_div * SECONDS_PER_HOUR;
	if (code->drop_frames) {
		int32 cycle_drop = code->drop_frames*(code->except_nth/code->every_nth-1);
		int32 linear_adjust = -cycle_drop * (linear/code->except_nth);
		int32 integral = linear%code->except_nth;
		linear_adjust -= code->drop_frames * (integral/code->every_nth);
		linear += linear_adjust;
		int32 fph_adjust = code->drop_frames * (frames_per_hour/code->except_nth -
				frames_per_hour/code->every_nth);
		frames_per_hour += fph_adjust;
	}
/* so we finally know how long an hour is */
	/* compensate with frames_per_hour-1 to round correctly */
	*micros = (MICROS_PER_HOUR*(bigtime_t)linear+frames_per_hour-1)/frames_per_hour +
		MICROS_PER_HOUR * hours;
	return B_OK;
}


status_t
frames_to_timecode(
	int32 l_frames,
	int * hours,
	int * minutes,
	int * seconds,
	int * frames,
	const timecode_info * code)
{
	if (code == NULL) {
		code = s_timecode_array;	/* default is first */
	}

	if (code->drop_frames) {
		int32 cycle_drop = code->drop_frames*(code->except_nth/code->every_nth-1);
		int32 cycle_frames = code->except_nth-cycle_drop;
		int32 fr_adj = cycle_drop * (l_frames/cycle_frames);
		int32 remainder = l_frames % cycle_frames;
		if (remainder >= code->every_nth) {
			fr_adj += code->drop_frames*(1 + (remainder-code->every_nth)/
				(code->every_nth-code->drop_frames));
		}
		l_frames += fr_adj;
	}
	*frames = l_frames % code->fps_div;
	l_frames /= code->fps_div;
	*seconds = l_frames % 60;
	l_frames /= 60;
	*minutes = l_frames % 60;
	*hours = l_frames/60;
	return B_OK;
}


status_t
timecode_to_frames(
	int hours, 
	int minutes, 
	int seconds, 
	int frames, 
	int32 * l_frames, 
	const timecode_info * code)
{
	if (code == NULL) {
		code = s_timecode_array;	/* default is first */
	}
/* correct into linear frames */
	int32 linear = frames+code->fps_div*(seconds+60*minutes);
	int32 frames_per_hour = code->fps_div * SECONDS_PER_HOUR;
	if (code->drop_frames) {
		int32 cycle_drop = code->drop_frames*(code->except_nth/code->every_nth-1);
		int32 linear_adjust = -cycle_drop * (linear/code->except_nth);
		int32 integral = linear%code->except_nth;
		linear_adjust -= code->drop_frames * (integral/code->every_nth);
		linear += linear_adjust;
		int32 fph_adjust = code->drop_frames * (frames_per_hour/code->except_nth -
			frames_per_hour/code->every_nth);
		frames_per_hour += fph_adjust;
	}
	*l_frames = linear + frames_per_hour * hours;
	return B_OK;
}


status_t
get_timecode_description(
	timecode_type type,
	timecode_info * out_timecode)
{
	if (type < B_TIMECODE_DEFAULT ||
		type >= (int)(sizeof(s_timecode_array) / sizeof(timecode_info)))
	{
		return B_ERROR;
	}
	*out_timecode = s_timecode_array[type];
	return B_OK;
}


status_t
count_timecodes()
{
	return B_TIMECODE_18+1;
}

status_t
_set_default_timecode(
	const timecode_info * in_timecode)
{
	s_timecode_array[B_TIMECODE_DEFAULT] = *in_timecode;
	return B_OK;
}





BTimeCode::BTimeCode()
{
	m_hours = m_minutes = m_seconds = m_frames = 0;
	get_timecode_description(B_TIMECODE_DEFAULT, &m_info);
}

BTimeCode::BTimeCode(
	bigtime_t us,
	timecode_type type)
{
	m_hours = m_minutes = m_seconds = m_frames = 0;
	get_timecode_description(type, &m_info);
	SetMicroseconds(us);
}

BTimeCode::BTimeCode(
	const BTimeCode & clone)
{
	m_hours = clone.m_hours;
	m_minutes = clone.m_minutes;
	m_seconds = clone.m_seconds;
	m_frames = clone.m_frames;
	m_info = clone.m_info;
}

BTimeCode::BTimeCode(
	int hours,
	int minutes,
	int seconds,
	int frames,
	timecode_type type)
{
	m_hours = m_minutes = m_seconds = m_frames = 0;
	get_timecode_description(type, &m_info);
	SetData(hours, minutes, seconds, frames);
}

BTimeCode::~BTimeCode()
{
}

void 
BTimeCode::SetData(
	int hours,
	int minutes,
	int seconds,
	int frames)
{
	m_hours = hours;
	m_minutes = minutes;
	m_seconds = seconds;
	m_frames = frames;
}

status_t
BTimeCode::SetType(
	timecode_type type)
{
	if ((type < B_TIMECODE_DEFAULT) || (type >= (timecode_type)count_timecodes())) {
		return B_BAD_VALUE;
	}
	bigtime_t then = Microseconds();
	status_t err = get_timecode_description(type, &m_info);
	SetMicroseconds(then);
	return err;
}

void 
BTimeCode::SetMicroseconds(
	bigtime_t us)
{
	us_to_timecode(us, &m_hours, &m_minutes, &m_seconds, &m_frames, &m_info);
}

void 
BTimeCode::SetLinearFrames(
	int32 linear_frames)
{
	frames_to_timecode(linear_frames, &m_hours, &m_minutes, &m_seconds, &m_frames, &m_info);
}


BTimeCode & 
BTimeCode::operator=(
	const BTimeCode & clone)
{
	m_hours = clone.m_hours;
	m_minutes = clone.m_minutes;
	m_seconds = clone.m_seconds;
	m_frames = clone.m_frames;
	m_info = clone.m_info;
	return *this;
}

bool BTimeCode::operator==(
	const BTimeCode & other) const
{
	return Microseconds() == other.Microseconds();
}

bool BTimeCode::operator<(
	const BTimeCode & other) const
{
	return Microseconds() < other.Microseconds();
}

BTimeCode & BTimeCode::operator+=(
	const BTimeCode & other)
{
	SetMicroseconds(Microseconds() + other.Microseconds());
	return *this;
}

BTimeCode & BTimeCode::operator-=(
	const BTimeCode & other)
{
	SetMicroseconds(Microseconds() - other.Microseconds());
	return *this;
}

BTimeCode BTimeCode::operator+(
	const BTimeCode & other) const
{
	return BTimeCode(Microseconds() + other.Microseconds());
}

BTimeCode BTimeCode::operator-(
	const BTimeCode & other) const
{
	return BTimeCode(Microseconds() - other.Microseconds());
}

int 
BTimeCode::Hours() const
{
	return m_hours;
}

int 
BTimeCode::Minutes() const
{
	return m_minutes;
}

int 
BTimeCode::Seconds() const
{
	return m_seconds;
}

int 
BTimeCode::Frames() const
{
	return m_frames;
}

timecode_type 
BTimeCode::Type() const
{
	return m_info.type;
}

void 
BTimeCode::GetData(
	int * out_hours,
	int * out_minutes,
	int * out_seconds,
	int * out_frames,
	timecode_type * out_type) const
{
	if (out_hours) *out_hours = m_hours;
	if (out_minutes) *out_minutes = m_minutes;
	if (out_seconds) *out_seconds = m_seconds;
	if (out_frames) *out_frames = m_frames;
	if (out_type) *out_type = m_info.type;
}

bigtime_t 
BTimeCode::Microseconds() const
{
	bigtime_t micros;
	timecode_to_us(m_hours, m_minutes, m_seconds, m_frames, &micros, &m_info);
	return micros;
}

int32 
BTimeCode::LinearFrames() const
{
	int32 l_frames;
	timecode_to_frames(m_hours, m_minutes, m_seconds, m_frames, &l_frames, &m_info);
	return l_frames;
}

void 
BTimeCode::GetString(
	char * str) const	/* at least 24 bytes */
{
	sprintf(str, m_info.format, m_hours, m_minutes, m_seconds, m_frames);
}





