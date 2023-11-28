#ifndef _VIEW_H
#include <View.h>
#endif
#include <ScrollBar.h>

#ifndef	MAIN_VIEW
#define	MAIN_VIEW


#define	SMALL	0
#define	LARGE	1


#define	LARGE_SIZE	(210+70)

//-----------------------------------------------------------------
#define	MAX_HIT	512
//-----------------------------------------------------------------
#define	PERSON_NAME	1
#define	KEYWORD		2
#define	MAX_CONTEXT	16
//-----------------------------------------------------------------

class	TDispWindow;
class	main_view;

//-----------------------------------------------------------------

typedef	struct {
	char	*first_name;
	char	*last_name;
} Person;

//-----------------------------------------------------------------

class ft_view : public BView {
friend		main_view;

public:
 	 				ft_view (BRect r, char *full_text, long full_text_size, TDispWindow *owner);
  					~ft_view();
virtual		void	MouseDown(BPoint where);
			long	find(char *buf, char *str, long pos, long size);
			void	SmartDrawString(char *buf);
			void	sort_hits();
			void	find_people(char *ft, long fts);

private:
			double		t0;
			char		*full_text;
			long		full_text_size;
			long		match_list[MAX_HIT];
			short		match_size[MAX_HIT];
			short		match_type[MAX_HIT];
			Person		person_list[MAX_HIT];
			long		match_count;
			long		person_count;
			long		keyword_count;
};

//-----------------------------------------------------------------

class main_view : public BView {

friend		ft_view;

public:
 	 				main_view (BRect r);
  					~main_view();
virtual  	void	Draw(BRect r);
			void	set_match_string(char *s);
			void	set_desc_string(char *s);
			void	set_status(char *s);
			void	draw_context();
			char	GetState();
			void	SetState(char s);
			void	SetFullText(char *ptr, long cnt);
			void	do_large();
			void	RedrawFullText(long newv);

			char	match_str[512];
			char	desc_str[512];
			char	status[128];
virtual		void	MouseDown(BPoint where);
			char	ContextClick(BPoint where);
			void	set_dead();
			long	get_vsize();

private:
			double		t0;
			char		state;
			char		*full_text;
			long		full_text_size;
			BScrollBar	*sb;
			long		dvp;
			ft_view		*sub_box;
			long		width;
			long		context_match_count;
			long		context_match_array[256];
			long		last_csum;
			long		name16[MAX_CONTEXT];
			char		dead_link;
			long		vsize;
};

//-----------------------------------------------------------------

class tool_view : public BView {

friend		ft_view;

public:
 	 				tool_view (BRect r);
  					~tool_view();
virtual  	void	Draw(BRect r);
virtual		void	MouseDown(BPoint where);
};


//-----------------------------------------------------------------



#endif