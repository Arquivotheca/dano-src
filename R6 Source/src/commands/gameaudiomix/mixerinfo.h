
#if !defined(mixerinfo_h)
#define mixerinfo_h

#include <vector>
#include <game_audio2.h>

class mixerinfo {
public:
		mixerinfo();
		mixerinfo(const mixerinfo & other);
		mixerinfo & operator=(const mixerinfo & other);
		~mixerinfo();
		status_t get(int fd, int16 mixer);
		status_t set(int fd);
		ssize_t save_size();
		ssize_t save(void * data, size_t max_size);
		ssize_t load(const void * data, size_t size);
private:
		int16 m_mixer;
		vector<game_mixer_control_value> m_values;
};

class mixerstate {
public:
	mixerstate();
	~mixerstate();
	status_t get(int fd);
	status_t set(int fd);
	ssize_t save_size();
	ssize_t save(void * data, size_t max_size);
	ssize_t load(const void * data, size_t size);
private:
	bool ok;
	vector<mixerinfo> m_mixers;
};


#endif	//	mixerinfo_h
