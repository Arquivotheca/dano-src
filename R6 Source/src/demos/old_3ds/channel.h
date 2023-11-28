#include <KernelKit.h>

#ifndef	CHANNEL
#define	CHANNEL


#define	BACK_BUF			(1024*8)
#define	AVAIL_BUF			(1024*80)
#define	AVAIL_SAMPLE		(AVAIL_BUF/sizeof(short))
#define	TOTAL_SIZE			(BACK_BUF + AVAIL_BUF)
#define	PLAY_BUFFER_SIZE	(2048)		//was 4096
#define	PLAY_SAMPLE_COUNT	(PLAY_BUFFER_SIZE/(sizeof(short)*2))
#define	MIX_STEP			(AVAIL_SAMPLE/PLAY_SAMPLE_COUNT)


#define	FBS					16384
//-----------------------------------------------------------

#define	CMD_DONE		0x99
#define	CMD_READ		0x01
#define	CMD_WRITE		0x02

//-----------------------------------------------------------


class	Channel {
private:
	char			*buffer1;
	char			*buffer2;
	char			*cur_buffer;
	long			buffer_1_pos;
	long			buffer_2_pos;
	long			new_pos;
	long			the_offset;
	float			the_gain;

	thread_id		task;	
	sem_id			todo_sem;
	long			file_id;
	long			mini_file_id;
	int				error;
	int				command_type;
public:
	char			cname[256];
	float			left_weight;
	float			right_weight;
	float			rev_level;
	long			rev_dist;
	short			filter_raw[8];
	short			filter_clean[8];
	short			fast_buffer[FBS];
	long			fast_buffer_pos;
	
//-----------------------------------------------------------

					Channel(char *filename);
virtual				~Channel();
			long	GetSize();
			int		Error();
			char	*SetPos(long pos);
			int		Write(long pos, short *data, long count);
			void	PrivateTask();
			void	SetOffset(long offset);
			void	SetGain(float gain);
			float	GetGain();
			char	*GetBuffer();
			void	SetLeftRight(float v);
			float	GetReverbe_level();
			long	GetReverbe_distance();
			void	SetFilter(long index, long value);
			void	NormalizeFilter();
			void	get_sample(long p, long scale, short *vmax, short *vmin);
			void	handle_mini(short *p);
			
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

#endif
