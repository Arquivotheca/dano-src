
#include "mixerinfo.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>


template<class C> class G : public C {
public:
	G() { memset(this, 0, sizeof(*this)); info_size = sizeof(*this); }
};

template<class C> class H : public C {
public:
	H() { memset(this, 0, sizeof(*this)); }
};



//#pragma mark --- mixerinfo ---

mixerinfo::mixerinfo() : m_mixer(0)
{
//puts(__PRETTY_FUNCTION__);
}



mixerinfo::mixerinfo(const mixerinfo &other) : m_mixer(0), m_values(other.m_values)
{
}

mixerinfo &
mixerinfo::operator=(const mixerinfo &other)
{
#if MIXERINFO
	m_mixer = other.m_mixer;
	m_values = other.m_values;
#endif
	return *this;
}

mixerinfo::~mixerinfo()
{
//puts(__PRETTY_FUNCTION__);
}

status_t 
mixerinfo::get(int fd, int16 mixer)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	status_t err = 0;
	m_mixer = 0;
	m_values.clear();
	G<game_get_mixer_infos> ggmis;
	H<game_mixer_info> gmi;
	ggmis.info = &gmi;
	ggmis.in_request_count = 1;
	gmi.mixer_id = mixer;
	if (ioctl(fd, GAME_GET_MIXER_INFOS, &ggmis) < 0) {
		err = errno;
		perror("error: ioctl(GAME_GET_MIXER_INFOS)");
		return err;
	}
	G<game_get_mixer_control_values> ggmcvs;
	ggmcvs.in_request_count = gmi.control_count;
	m_values.resize(ggmcvs.in_request_count);
	ggmcvs.values = &m_values[0];
	ggmcvs.mixer_id = mixer;
	G<game_get_mixer_controls> ggmcs;
	vector<game_mixer_control> gmc(gmi.control_count);
	ggmcs.in_request_count = gmi.control_count;
	ggmcs.controls = &gmc[0];
	if (ioctl(fd, GAME_GET_MIXER_CONTROLS, &ggmcs) < 0)
	{
		err = errno;
		perror("error: ioctl(GAME_GET_MIXER_CONTROLS)");
		return err;
	}
	for (int ix=0; ix<gmi.control_count; ix++)
	{
		m_values[ix].control_id = gmc[ix].control_id;
	}
	if (ioctl(fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcvs) < 0) {
		err = errno;
		perror("error: ioctl(GAME_GET_MIXER_CONTROL_VALUES)");
		return err;
	}
	m_mixer = mixer;
#endif
	return B_OK;
}

status_t 
mixerinfo::set(int fd)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	if (m_mixer <= 0) {
		return B_BAD_VALUE;
	}
	G<game_set_mixer_control_values> gsmcvs;
	gsmcvs.mixer_id = m_mixer;
	gsmcvs.in_request_count = m_values.size();
	gsmcvs.values = &m_values[0];
	status_t err = B_OK;
	if (ioctl(fd, GAME_SET_MIXER_CONTROL_VALUES, &gsmcvs) < 0) {
		err = errno;
		perror("error: ioctl(GAME_SET_MIXER_CONTROL_VALUES)");
		return err;
	}
#endif
	return B_OK;
}

ssize_t 
mixerinfo::save_size()
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	return sizeof(m_mixer)+sizeof(int)+sizeof(game_mixer_control_value)*m_values.size();
#else
	return 0;
#endif
}

ssize_t 
mixerinfo::save(void *data, size_t max_size)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	ssize_t t = save_size();
	if (t > max_size) return B_NO_MEMORY;
	char * s = (char *)data;
	memcpy(s, &m_mixer, sizeof(m_mixer));
	s += sizeof(m_mixer);
	int c = m_values.size();
	memcpy(s, &c, sizeof(c));
	s += sizeof(c);
	memcpy(s, &m_values[0], sizeof(game_mixer_control_value)*c);
	return t;
#else
	return 0;
#endif
}

ssize_t 
mixerinfo::load(const void *data, size_t size)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	if (size < sizeof(m_mixer)+sizeof(int)) return B_BAD_VALUE;
	int16 mix;
	int c;
	const char * l = (const char *)data;
	memcpy(&mix, l, sizeof(mix));
	l += sizeof(mix);
	memcpy(&c, l, sizeof(c));
	l += sizeof(c);
	if (size < sizeof(m_mixer)+sizeof(int)+sizeof(game_mixer_control_value)*c) {
		return B_BAD_VALUE;
	}
	m_values.resize(c);
	memcpy(&m_values[0], l, sizeof(game_mixer_control_value)*c);
	m_mixer = mix;
	return sizeof(m_mixer)+sizeof(int)+sizeof(game_mixer_control_value)*c;
#else
	return 0;
#endif
}


//#pragma mark --- mixerstate ---


mixerstate::mixerstate() : ok(false)
{
//puts(__PRETTY_FUNCTION__);
#if MIXERINFO
	m_mixers.resize(2);
#endif
}


mixerstate::~mixerstate()
{
//puts(__PRETTY_FUNCTION__);
}

status_t 
mixerstate::get(int fd)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	G<game_get_info> ggi;
	status_t err;
	if (ioctl(fd, GAME_GET_INFO, &ggi) < 0) {
		err = errno;
		perror("error: ioctl(GAME_GET_INFO)");
		return err;
	}
	m_mixers.resize(ggi.mixer_count);
	for (int ix=0; ix<ggi.mixer_count; ix++) {
		err = m_mixers[ix].get(fd, GAME_MAKE_MIXER_ID(ix));
		if (err < 0) {
			m_mixers.resize(0);
			return err;
		}
	}
	ok = true;
#endif
	return B_OK;
}

status_t 
mixerstate::set(int fd)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	if (!ok) return B_BAD_VALUE;
	status_t err;
	for (int ix=0; ix<m_mixers.size(); ix++) {
		err = m_mixers[ix].set(fd);
		if (err < 0) return err;
	}
#endif
	return B_OK;
}

ssize_t 
mixerstate::save_size()
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	ssize_t t = sizeof(int);
	if (!ok) return B_BAD_VALUE;
	for (int ix=0; ix<m_mixers.size(); ix++) {
		t += m_mixers[ix].save_size();
	}
	return t;
#else
	return 0;
#endif
}

ssize_t 
mixerstate::save(void *data, size_t max_size)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	ssize_t ss = save_size();
	if (ss > max_size) return B_NO_MEMORY;
	char * s = (char *)data;
	int c = m_mixers.size();
	memcpy(s, &c, sizeof(c));
	s += sizeof(c);
	status_t err;
	for (int ix=0; ix<m_mixers.size(); ix++) {
		ss = m_mixers[ix].save_size();
		err = m_mixers[ix].save(s, ss);
		if (err < 0) return err;
		s += err;
	}
	return ss;
#else
	return 0;
#endif
}

ssize_t 
mixerstate::load(const void *data, size_t size)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	ok = false;
	if (size < sizeof(int)) return B_BAD_VALUE;
	int c;
	const char * l = (const char *)data;
	const char * e = l+size;
	memcpy(&c, l, sizeof(int));
	l += sizeof(int);
	m_mixers.resize(c);
	status_t err;
	for (int ix=0; ix<c; ix++) {
		err = m_mixers[ix].load(l, e-l);
		if (err < 0) return err;
		l += err;
	}
	ok = true;
	return l - (const char *)data;
#else
	return 0;
#endif
}

