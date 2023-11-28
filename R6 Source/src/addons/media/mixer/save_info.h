//
// Data structures used for saving settings to disk
// 

#include <set>

struct save_info 
{
	char name[64];
	float gain[2];
	float pan;
	time_t time;
};

static inline bool operator<(const save_info& a, const save_info& b)
{
	return strncmp(a.name, b.name, 64) < 0;
}
static inline bool operator==(const save_info& a, const save_info& b)
{
	return strncmp(a.name, b.name, 64) == 0;
}

typedef std::set<save_info> save_map;
