#include <KernelKit.h>
#include "track_view.h"

#ifndef	CHANNEL
#define	CHANNEL


#define	AVAIL_BUF			(1024*10)
#define	AVAIL_SAMPLE		(AVAIL_BUF/sizeof(short))
#define	TOTAL_SIZE			(AVAIL_BUF)
#define	PLAY_BUFFER_SIZE	(2048)
#define	PLAY_SAMPLE_COUNT	(PLAY_BUFFER_SIZE/(sizeof(short)*2))
#define	MIX_STEP			(AVAIL_SAMPLE/(PLAY_SAMPLE_COUNT))
#define	EXTRA				(512*3)		//was 512

//-----------------------------------------------------------

#define	CMD_DONE		0x99
#define	CMD_READ		0x01
#define	CMD_WRITE		0x02

//-----------------------------------------------------------


class	Channel {
private:
	TrackViewer		*viewer;
	short			*buffer1;
	char			*cur_buffer;
	float			the_gain;

	thread_id		task;	
	sem_id			todo_sem;
	long			file_id;
	int				error;
	int				command_type;
public:
	char			cname[384];
	float			left_weight;
	float			right_weight;
	float			rev_level;
	long			rev_dist;
	char			got_data;
	
//-----------------------------------------------------------

					Channel(const char *name, TrackViewer *the_viewer);
					~Channel();
			long	GetSize();
			int		Error();
			short	*SetPos(long pos);
			int		Write(long pos, short *data, long count);
			void	PrivateTask();
			void	SetOffset(long offset);
			void	SetGain(float gain);
			float	GetGain();
			short	*GetBuffer();
			void	SetLeftRight(float v);
			float	GetReverbe_level();
			long	GetReverbe_distance();
			void	SetFilter(long index, long value);
			void	NormalizeFilter();
			void	get_sample(long p, long scale, short *vmax, short *vmin);
			void	handle_mini(short *p);
			void	SetName(const char *);
private:
			long	PrivateRead(long pos, long count, char *p);
			long	PrivateWrite(long pos, long count, char *p);			
			void	HandleRequest();
			void	handle_read_request();
			void	handle_write_request();
			void	set_server(long pos);
			void	build_mini_file();

};
	
//-----------------------------------------------------------

class	MixChannel {
					MixChannel();
					~MixChannel();
			long	GetSize();
			int		Error();
			void	SetPos(long pos);
			void	PrivateTask();
			char	*GetBuffer();
private:
};
	
//-----------------------------------------------------------

#endif
