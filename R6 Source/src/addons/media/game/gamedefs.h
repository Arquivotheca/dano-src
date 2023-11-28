#if !defined(__GAME_DEFS_H__)
#define __GAME_DEFS_H__

namespace BPrivate {

// game_audio struct wrappers
template<class C> class G : public C {
public:
	G() { memset(this, 0, sizeof(*this)); info_size = sizeof(*this); }
};

template<class C> class H : public C {
public:
	H() { memset(this, 0, sizeof(*this)); }
};

// generic game-audio format description
struct game_format_spec
{
	uint32	frame_rate;
	float	cvsr;
	int16	channel_count;
	uint32	designations;
	uint32	format;
};

}; // BPrivate
#endif __GAME_DEFS_H__
