#include "CGIF.H"


enum NetResult {
	NET_NO_ERROR			=  0,
	NET_ERROR				= -1,
	NET_NO_SOCKETS			= -2,
	NET_UNKNOWN_HOST		= -3,
	NET_CANT_CONNECT		= -4,
	NET_CANT_SEND			= -5,
	NET_CANT_RECV			= -6,
	NET_TIMEOUT				= -7,
	NET_IS_CLOSED			= -8,
	NET_ALREADY_CLOSED		= -9,
	NET_DONT_DOWNLOAD		= -10,

	NET_GET_URL				= 'nget',
//	HTTP Header Fields
	HTTP_ERROR				= -101
//	FTP  Results

};

//--------------------------------------------------------------------

#define	MAX_BUF	1768000

//--------------------------------------------------------------------

class	finder {
public:
			finder();
virtual		~finder();
	ulong 	LookupHost(const char *host);
	int		Connect(char *host);
	int 	Request(long msocket, char *usrstring, char *cpath);
	long 	FillBuffer(long msocket, char *buffer);
	long	find(char *buf, char *str, long pos, long size);
	
	//long	msocket;
};

//--------------------------------------------------------------------

class	site_getter : public finder {
public:
				site_getter();
virtual			~site_getter();
		int		doit(char *site_path);
		int		doit0();
		char	*output_text(long *ocnt);
		long	get_total_bytes();

		long	done_sem;
		char	full_path[512];
		char	done;
		char	*fbuffer;
		long	total;
		long	msocket;
};


//--------------------------------------------------------------------

class	gif_getter : public site_getter {
public:
				gif_getter();
virtual			~gif_getter();
		CGIF	*get_data();
		long	get_gif_size();
};

//--------------------------------------------------------------------

class	av_find : public finder {
public:
				av_find();
virtual			~av_find();
virtual	void	process(char *buffer, long size);
virtual	int		doit(char *str);
		char	*get_ind_match(long i);
		char	*get_ind_desc(long i);
		long	match_count();


		char	*match_list[128];
		char	*desc_list[128];
		long	cur_pos;

};

//--------------------------------------------------------------------

class	hb_find : public av_find {
public:
				hb_find();
virtual			~hb_find();
		void	process(char *buffer, long size);
		int		doit(char *str);
};
//--------------------------------------------------------------------

typedef	struct {
	char	last_name[64];
	char	first_name[64];
	char	city[64];
	char	zip[12];
	char	phone[16];
	char	address[128];
	char	state[8];
} full_person;

//--------------------------------------------------------------------

class	phone_find : public finder {
public:
					phone_find();
virtual				~phone_find();
		void		process(char *buffer, long size);
		int			doit(char *fn, char *ln);
		void 		add_full_person(char *first_name,
								 	char *last_name,
								 	char *city,
								 	char *zip,
								 	char *phone,
								 	char *address,
								 	char *state);
		
		long		person_count();
		full_person	*get_ind_person(long i);


//-----------

		full_person	*match_list[128];
		long		match_count;
};

//--------------------------------------------------------------------

typedef	struct {
	char	last_name[64];
	char	first_name[64];
	char	email[256];
} full_email;

//--------------------------------------------------------------------

class	email_find : public finder {
public:
					email_find();
virtual				~email_find();
		void		process(char *buffer, long size);
		int			doit(char *fn, char *ln);
		void 		add_email	   (char *first_name,
								 	char *last_name,
								 	char *email);
		long		email_count();
		full_email	*get_ind_email(long i);


//-----------

		full_email	*match_list[128];
		long		match_count;
};

//--------------------------------------------------------------------


typedef	struct {
	char	newsgroup[100];
	char	extra[64];
} usenet_match;

//--------------------------------------------------------------------

class	usenet_find : public finder {
public:
						usenet_find();
virtual					~usenet_find();
		void			process(char *buffer, long size);
		int				doit(char *string);
		void 			add_usenet	   (char *newsgroup,
								 		char *extra);
		long			newsgroup_count();
		usenet_match	*get_ind_newsgroup(long i);


//-----------

		usenet_match	*match_list[128];
		long			match_count;
};

//--------------------------------------------------------------------

class	CGIF;

class	map_getter : public finder {
public:
						map_getter();
virtual					~map_getter();
		int				doit(char *address, char *city, char *state, char *zip);
		BBitmap			*get_map();

		CGIF			*a_gif;
		BBitmap			*bits;
		
//-----------
};


