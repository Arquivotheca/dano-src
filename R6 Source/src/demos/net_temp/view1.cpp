
#include "view1.h"
#include "disp_window.h"
#include "PersonMachine.h"

//--------------------------------------------------------------

#define	HBREAK	640

extern "C" char find_name(char *name);
extern "C" char	reject_last_name(char *name);

//--------------------------------------------------------------

tool_view::tool_view (BRect r)
  : BView (r, "Title view",
		   B_FOLLOW_NONE, B_WILL_DRAW)
{
}

//--------------------------------------------------------------

tool_view::~tool_view ()
{
}

//--------------------------------------------------------------
#define	TOOL_W	163
//--------------------------------------------------------------

char	*labels[] = {
					"Lookup Person",
					"Lookup Company",
					"Lookup City",
					"Lookup Phone",
					"Super Search",
					"FooBar"
					};

//--------------------------------------------------------------

void	tool_view::Draw(BRect ur)
{
	BRect	r;
	long	i;
	long	w;
	
	for (i = 0; i < 5; i++) {
		r.top = 0;
		r.left = i * TOOL_W;
		r.right = i * TOOL_W + TOOL_W - 1;
		r.bottom = 29;
		SetHighColor(250, 250, 250);
		StrokeRect(r);
		r.OffsetBy(-1, -1);
		SetHighColor(140, 140, 140);
		StrokeRect(r);
		SetHighColor(255, 0, 0);
		r.OffsetBy(1, 1);
		w = StringWidth(labels[i]);
		//SetFont(be_bold_font);
		SetFontSize(11);
		SetHighColor(0, 0, 0);
		MovePenTo(BPoint(r.left + (TOOL_W - w) / 2, r.bottom - 11));
		DrawString(labels[i]);
	}
}

//--------------------------------------------------------------

void	tool_view::MouseDown(BPoint where)
{
}

//--------------------------------------------------------------

main_view::main_view (BRect r)
  : BView (r, "Title view",
		   B_FOLLOW_NONE, B_WILL_DRAW)
{
	strcpy(status, "waiting for data");
	match_str[0] = 0;
	state = SMALL;
	full_text = 0;
	full_text_size = 0;
	dvp = 0;
	last_csum = -12345;
	dead_link = 0;
}

//--------------------------------------------------------------

void	main_view::set_dead()
{
	dead_link = 1;
}

//--------------------------------------------------------------

uchar	clow(uchar c)
{
	if ((c >= 'A') && (c <= 'Z')) {
		c = c + 0x020;
	}
	
	return c;
}

//--------------------------------------------------------------

void	ft_view::SmartDrawString(char *buf)
{
	char	tbuf[256];
	char	*c;
	char	a_char;
	char	buf_p;
	long	size;
	long	i;
		
	c = buf;
	
	buf_p = 0;
	
	while(a_char = *c++) {
		if (a_char != '|') {
			tbuf[buf_p] = a_char;
			buf_p++;
		}
		else {
			if (*(c) == 'H') {
				size = *(c+1);
				tbuf[buf_p] = 0;
				SetHighColor(0, 0, 0);
				DrawString(tbuf);
				c += 2;
				memcpy(tbuf, c, size);
				tbuf[size] = 0;
				SetHighColor(255, 0, 0);
				DrawString(tbuf);
				buf_p = 0;
				c += size;				
			}
			
			if (*(c) == 'P') {
				size = *(c+1);
				tbuf[buf_p] = 0;
				SetHighColor(0, 0, 0);
				DrawString(tbuf);
				c += 2;
				memcpy(tbuf, c, size);
				tbuf[size] = 0;
				SetHighColor(0, 130, 0);
				DrawString(tbuf);
				buf_p = 0;
				c += size;				
			}
		}
	}
	tbuf[buf_p] = 0;
	SetHighColor(0, 0, 0);
	DrawString(tbuf);
}


//--------------------------------------------------------------

long	ft_view::find(char *buf, char *str, long pos, long size)
{
	long	i;

	buf += pos;
	size -= pos;
	
	while(size > 0) {
		i = 0;
		
		while((*(str+i)) && (clow(*(str+i)) == (clow(*(buf+i))))) {
			i++;
		}
		if (*(str+i) == 0) {
			return pos;
		}
		
		size--;
		pos++;
		buf++;
	}
	return -1;
}

//--------------------------------------------------------------

void	ft_view::sort_hits()
{
	long	step;
	long	i,j;
	long	tmp;
	Person	tmp_person;
	
	for (step = match_count / 2; step > 0; step /= 2) {
		for (i = step; i < match_count; i++) {
			for (j = i - step; j >= 0 && match_list[j] > match_list[j+step]; j -= step) {
				tmp = match_list[j];
				match_list[j] = match_list[j + step];
				match_list[j + step] = tmp;

				tmp = match_size[j];
				match_size[j] = match_size[j + step];
				match_size[j + step] = tmp;

				tmp = match_type[j];
				match_type[j] = match_type[j + step];
				match_type[j + step] = tmp;

				tmp_person = person_list[j];
				person_list[j] = person_list[j + step];
				person_list[j + step] = tmp_person;
			}
		}
	}
}
	
//--------------------------------------------------------------

char	is_lower(char c)
{
	if (c == '-')
		return 1;

	if (c >= 'a' && c <= 'z')
		return 1;
		
	return 0;
}

//--------------------------------------------------------------

char	is_upper(char c)
{
	if (c >= 'A' && c <= 'Z')
		return 1;
		
	return 0;
}
	
//--------------------------------------------------------------

char	is_separator(char c)
{
	if (c == ' ')
		return 1;
	if (c == ',')
		return 1;
	if (c == '.')
		return 1;
	if (c == '&')
		return 1;
		
	return 0;
}

//--------------------------------------------------------------

char	is_alpha(char c)
{
	if (c == '-')
		return 1;
		
	if (c >= 'a' && c <= 'z')
		return 1;
		
	if (c >= 'A' && c <= 'Z')
		return 1;
		
	return 0;
}
	
//--------------------------------------------------------------

void	debug_print(char *label, char *p, long n)
{
	long	i;

	
	printf("%s ", label);
	
	printf("<");
	for (i = 0; i < n; i++)
		printf("%c", p[i]);
	printf(">");
	
	printf("\n");
}
	
//--------------------------------------------------------------

void	ft_view::find_people(char *ft, long fts)
{
	long	i;
	char	first_name[256];
	char	last_name[256];
	char	middle[32];
	long	p;
	long	max;
	long	start;
	long	end;
	long	backup;
	

	i = -1;
	
		
	while(i < fts) {
		i++;
		backup = i;
		goto	skip;

failed:;

		i = backup + 1;
skip:;

		if ((!is_alpha(ft[i-1])) &&
			(is_upper(ft[i])     &&
			(is_lower(ft[i+1])))) {
			p = 0;
			first_name[0] = ft[i];
			first_name[1] = ft[i+1];
			first_name[2] = 0;
			i += 2;
			p = 2;
			start = i - 2;
			while(is_lower(ft[i]) && (i < fts)) {
				first_name[p] = ft[i];
				first_name[p+1] = 0;
				i++;
				p++;
			}
			if (ft[i] == ',') {
				goto failed;
			}
			if (is_upper(ft[i])) {
				goto failed;
			}
				
			i++;			// skip separator
			
			if (is_upper(ft[i]) && (ft[i+1] == '.')) {
				middle[0] = ft[i];
				middle[1] = 0;
				i += 2;
				max = 8;
				while ((!is_upper(ft[i]) && (max > 0))) {
					if (ft[i] == ',') {
						goto failed;
					}
					max--;
					i++;
				}
			}	
			p = 0;
			while (is_separator(ft[i])) {
				if (ft[i] == ',') {
						goto failed;
				}
				i++;
			}
		
			if (is_upper(ft[i]) && is_lower(ft[i+1])) {
				last_name[p] = ft[i];
				last_name[p+1] = 0;
				i++;
				p++;
				while(is_lower(ft[i]) && (i < fts)) {
					last_name[p] = ft[i];
					last_name[p+1] = 0;
					i++;
					p++;
				}
				end = i - 1;
				if (find_name(first_name)) {
					if (!reject_last_name(last_name)) {
						if (match_count < MAX_HIT) {
							match_list[match_count] = start;
							match_size[match_count] = end - start + 1;
							match_type[match_count] = PERSON_NAME;
							person_list[match_count].first_name = (char *)malloc(strlen(first_name) + 1);
							strcpy(person_list[match_count].first_name, first_name);
							person_list[match_count].last_name = (char *)malloc(strlen(last_name) + 1);
							strcpy(person_list[match_count].last_name, last_name);
							match_count++;
							person_count++;
						}
					}
				}
				else
					goto failed;
			}
			else {
				goto failed;
			}
		}
	}
}
	
//--------------------------------------------------------------

ft_view::ft_view (BRect r, char *ft, long fts, TDispWindow *owner)
  : BView (r, "Title view",
		   B_FOLLOW_NONE, B_WILL_DRAW)
{
	char	search_word[256];
	char	*p;
	long	new_p;
	long	pos;
	long	result;
	long	sw_index;
	long	i;
	
	t0 = 0;
	full_text = ft;
	full_text_size = fts;
	
	match_count = 0;
	person_count = 0;
	keyword_count = 0;
	
	for (i = 0; i < MAX_HIT; i++) {
		person_list[i].first_name = 0;
		person_list[i].last_name = 0;
	}
	
	
	sw_index = 0;
	
	do {
		result = owner->GetSearchWord(sw_index, search_word);
		sw_index++;
		if (result >= 0) {
			
			p = full_text;
			
			pos = 0;
			do {
				new_p = find(p, search_word, pos, full_text_size);
				if (new_p != -1) {
					if (match_count == MAX_HIT)
						goto out;
					match_list[match_count] = new_p;
					match_size[match_count] = strlen(search_word);
					match_type[match_count] = KEYWORD;
					match_count++;
					keyword_count++;
					pos = new_p + 1;
				}
			} while(new_p >= 0);
		}
	} while(result >= 0);
out:;	

	find_people(ft, fts);
	
	sort_hits();
}

//--------------------------------------------------------------

ft_view::~ft_view()
{
	long	i;
	
	for (i = 0; i < MAX_HIT; i++) {
		free((char *)person_list[i].first_name);		//this is fine since we can free NIL
		free((char *)person_list[i].last_name);
	}
}

//--------------------------------------------------------------

void	main_view::SetFullText(char *ptr, long cnt)
{
	full_text = ptr;
	full_text_size = cnt;
	sub_box = new ft_view(BRect(5, 30, HBREAK-27, LARGE_SIZE - 12), full_text, full_text_size, (TDispWindow *)Window());

}

//--------------------------------------------------------------

main_view::~main_view()
{
	if (full_text)
		free((char *)full_text);
}
 
//--------------------------------------------------------------

typedef struct {
    char   cNum;
    char    *cStr;
    int		len;
} charInfo;

charInfo gCommonChars[] = {
    {'"',"quot", 4     },
    {'&',"amp", 3      },
    {'<',"lt", 2       },
    {'>',"gt", 2       },
    {'^',"copy", 4     },
    {'^',"reg", 3      },
    {'e',"eacute;", 7   },
    {'o',"ocirc;", 6   },
    {'a',"agrave;", 7   },
    {'e',"egrave;", 7   },
    {' ',"nbsp", 4   },
};
 
//--------------------------------------------------------------

void	main_view::RedrawFullText(long newv)
{
	dvp = newv;
	Draw(BRect(0,0,1000,1000));
}
 
//--------------------------------------------------------------

void	main_view::do_large()
{
	long	vp;
	char	*p;
	char	end;
	long	cnt;
	long	pos;
	char	just_got_one;
	int		i;
	char	tmp[256];
	char	first = 1;
	BRect	r;
	long	cur;
	long	match_cnt;
	char	got_hit;
	BRect	bullet;
	long	cur_type;
	long	csum;
		
	r = sub_box->Bounds();
	r.bottom -= r.top;
	r.right -= r.left;
	r.top = 0;
	r.left = 0;
	
	r.OffsetBy(5, 30);
	r.top--;
	r.bottom++;
	r.right++;
	r.right++;
	SetHighColor(250, 250, 250);
	StrokeRect(r);
	r.top--;
	r.left--;
	r.bottom--;
	r.right--;
	SetHighColor(140, 140, 140);
	StrokeRect(r);
	SetHighColor(255, 0, 0);

	end = 0;
	vp = 40 - dvp;
	vp = 40;
	sub_box->SetFontSize(10);

	just_got_one = 1;
	
	full_text[full_text_size] = 0;
	
	r = sub_box->Bounds();
	
	p = full_text;
	pos = 0;
	

	match_cnt = 0;
	cur = sub_box->match_list[match_cnt];
	cur_type = sub_box->match_type[match_cnt];
	
	if (cur_type != PERSON_NAME)
	if ((cur == sub_box->match_list[match_cnt + 1]) && (sub_box->match_type[match_cnt + 1] == PERSON_NAME)) {
		match_cnt++;
		cur = sub_box->match_list[match_cnt];
		cur_type = sub_box->match_type[match_cnt];
	}
	
	got_hit = 0;
		
	context_match_count = 0;
	
	csum = 0;
	
	while(!end) {	
		cnt = 0;
		while(cnt < 115) {
again:;
			if (pos > cur) {
				match_cnt++;
				if (match_cnt > sub_box->match_count)
					cur = -1;
				else {
					cur = sub_box->match_list[match_cnt];
					cur_type = sub_box->match_type[match_cnt];
				}
			}
			
			if (pos == cur) {
				tmp[cnt] = '|';
				cnt++;
				if (cur_type == KEYWORD)
					tmp[cnt] = 'H';
				if (cur_type == PERSON_NAME) {
					tmp[cnt] = 'P';
					if ((vp > (r.top - 40)) && vp < (r.bottom + 40)) {
						context_match_array[context_match_count] = match_cnt;
						context_match_count++;
						csum = (csum << 2) ^ match_cnt + match_cnt ^ (csum>>16);
					}
				}

				cnt++;
				tmp[cnt] = sub_box->match_size[match_cnt];
				cnt++;
				match_cnt++;
				got_hit = 1;
				if (match_cnt > sub_box->match_count)
					cur = -1;
				else {
					cur = sub_box->match_list[match_cnt];
					cur_type = sub_box->match_type[match_cnt];
				}
				
				if (cur_type != PERSON_NAME)
				if ((cur == sub_box->match_list[match_cnt + 1]) && (sub_box->match_type[match_cnt + 1] == PERSON_NAME)) {
					match_cnt++;
					cur = sub_box->match_list[match_cnt];
					cur_type = sub_box->match_type[match_cnt];
				}
			}

			tmp[cnt] = *p;
			if (*p == 0) {
				end = 1;
				goto out;
			}
			
			if (*p == '&') {						//nasty stuff !
    			for (i = 0; i < 11; i++) {
        			if (!strncmp((p+1), gCommonChars[i].cStr, gCommonChars[i].len)) {
        				p += gCommonChars[i].len;
						tmp[cnt] = gCommonChars[i].cNum;
						pos += gCommonChars[i].len;
        				goto found;
        			}
         		}
 			}
			
			if ((*p == 10) || (*p == 13) || (*p == 9)) {
				if (just_got_one) {
					p++;
					pos++;
					goto again;
				}
				just_got_one = 1;
				tmp[cnt] = 0x20;
				p++;
				pos++;
				cnt++;
				goto out;
			}
			
			just_got_one = 0;
			
			if (*p < 0x20) {
				*p = 0x20;
			}
			
			if (cnt > 100) {
				if ((*p == '.') || (*p == ','))
					goto out; 
			}
			
			if (cnt > 100) {
				if (*p == ' ')
					goto out;
			}
			
found:;

			if (cnt > 115)
				goto out;
				
			cnt++;
			p++;
			pos++;
				
		}
	
out:;
		tmp[cnt] = 0;
		
		if ((vp > (r.top - 50)) && vp < (r.bottom + 100)) {
			sub_box->MovePenTo(BPoint(26, vp));
			if (first) {
				sub_box->SetFontSize(12);
			}
			sub_box->SmartDrawString(tmp);
			if (first) {
				sub_box->SetFontSize(10);
			}
			
			if (got_hit) {
				bullet.top = vp - 8;
				bullet.bottom = vp - 1;
				bullet.left = 4;
				bullet.right = 11;
				sub_box->SetHighColor(255, 0, 0);
				sub_box->FillEllipse(bullet);
			}
			
			got_hit = 0;
		}
		vp += 14;
			
		first = 0;
		if (vp > (r.bottom + 100))
			goto out1;;
	}
out1:;

	sub_box->SetHighColor(120, 120, 120);
	sub_box->MovePenTo(BPoint(18, 0));
	sub_box->StrokeLine(BPoint(18, r.bottom));
	if (csum != last_csum) {
		last_csum = csum;
		Invalidate();
	}
}

//--------------------------------------------------------------

void	main_view::draw_context()
{
	char	tmp[256];
	long	i,j;
	long	n;
	BRect	r, r1;
	
	r = Bounds();
	
	r.left = HBREAK + 2;
	r.bottom -= 2;
	r.top += 1;

	if (strlen(match_str) == 0) {
		SetHighColor(216,216,216);
		FillRect(r);
		return;
	}
	SetHighColor(0, 0, 0);
	
	n = 0;
	
	if (state == LARGE) {
		for (i = 0; i < MAX_CONTEXT; i++)
			name16[i] = -1;
			
		for (i = 0; i < context_match_count; i++) {
			for (j = 0; j < i; j++) {
				if (strcmp(sub_box->person_list[context_match_array[i]].last_name,
						   sub_box->person_list[context_match_array[j]].last_name) == 0)
					if (strcmp(sub_box->person_list[context_match_array[i]].first_name,
							   sub_box->person_list[context_match_array[j]].first_name) == 0)
							   	goto skip;
			}
			name16[n] = context_match_array[i];
			r1.left = HBREAK+1;
			r1.right = r.right;
			r1.top = n*17;
			r1.bottom = n*17 + 17;
			
			SetHighColor(216,216,216);
			FillRect(r1);
			
			SetHighColor(250, 250, 250);
			MovePenTo(r1.LeftTop());
			StrokeLine(r1.RightTop());
			MovePenTo(r1.LeftBottom());
			StrokeLine(r1.RightBottom());
			r1.top--;
			r1.left--;
			r1.bottom--;
			r1.right--;
			SetHighColor(140, 140, 140);
			MovePenTo(r1.LeftTop());
			StrokeLine(r1.RightTop());
			MovePenTo(r1.LeftBottom());
			StrokeLine(r1.RightBottom());
			
			SetHighColor(0, 0, 0);
			MovePenTo(BPoint(668, n*17 + 12));
			DrawString(sub_box->person_list[context_match_array[i]].first_name);
			MovePenBy(6, 0);
			DrawString(sub_box->person_list[context_match_array[i]].last_name);
			
			n++;
			if (n == MAX_CONTEXT)
				goto out;
			
			skip:;
			
		}
out:;
		r1.left = HBREAK+1;
		r1.right = r.right;
		r1.top = n*17+1;
		r1.bottom = r.bottom;
			
		SetHighColor(216,216,216);
		FillRect(r1);
	}
	else {
		r = Bounds();
		r.left = HBREAK + 2;
		r.bottom -= 2;
		r.top += 1;
		SetHighColor(216,216,216);
		FillRect(r);
		
		MovePenTo(BPoint(660, 20));
		SetDrawingMode(B_OP_OVER);
		SetHighColor(0, 0, 0);
		DrawString(status);
		if (sub_box) {
			MovePenTo(BPoint(660, 35));
			sprintf(tmp, " %ld Keyword(s)", sub_box->keyword_count);
			DrawString(tmp);
			MovePenTo(BPoint(660, 50));
			if (sub_box->person_count == 0)
				strcpy(tmp, "No Person Name");
			else
				sprintf(tmp, " %ld Person's Name(s)", sub_box->person_count);
			DrawString(tmp);
		}
	}
}

//--------------------------------------------------------------

long	main_view::get_vsize()
{
	return vsize;
}

//--------------------------------------------------------------

void	main_view::Draw(BRect rr)
{
	BRect	r;
	long	vp;
	long	cnt;
	char	*p;
	char	end;
	long	i;
	char	tmp[120];
	
	r = Bounds();
	
	SetHighColor(216,216,216);
	r.right = HBREAK;
	FillRect(r);
	r = Bounds();
	
	SetHighColor(250, 250, 250);
	StrokeRect(r);
	r.top--;
	r.left--;
	r.bottom--;
	r.right--;
	SetHighColor(140, 140, 140);
	StrokeRect(r);
	
	
	SetHighColor(140, 140, 140);
	MovePenTo(BPoint(HBREAK, 1));
	StrokeLine(BPoint(HBREAK, r.bottom));
	
	SetHighColor(250, 250, 250);
	MovePenTo(BPoint(HBREAK+1, 1));
	StrokeLine(BPoint(HBREAK+1, r.bottom));
	
	
	
	SetHighColor(0, 0, 255);
	MovePenTo(BPoint(15, 18));
	SetFontSize(12);
	DrawString(match_str);
	
	if (width == 0) {
		width = StringWidth(match_str);
	}
	
	
	SetHighColor(50, 50, 180);
	MovePenTo(BPoint(13, 19));
	StrokeLine(BPoint(13 + width + 4, 19));
	
	SetHighColor(255, 0, 0);
	
	if (dead_link) {
		draw_context();
		return;
	}
	
	SetFontSize(10);
	
	
	end = 0;
	
	vp = 38;
	
	if (state == SMALL || full_text == 0) {
		p = desc_str;
		while(!end) {	
			cnt = 0;
			while(cnt < 115) {
				tmp[cnt] = *p;
				if (*p == 0) {
					end = 1;
					goto out;
				}
				if (*p == '&') {			//nasty stuff !
    				for (i = 0; i < 11; i++) {
        				if (!strncmp((p+1), gCommonChars[i].cStr, gCommonChars[i].len)) {
        					p += gCommonChars[i].len;
							tmp[cnt] = gCommonChars[i].cNum;
        					goto found;
        				}
         			}
 				}
				
				if (cnt > 100) {
					if ((*p == '.') || (*p == ','))
						goto out; 
				}
				
				if (cnt > 100) {
					if (*p == ' ')
						goto out;
				}
				
				if (cnt > 115)
					goto out;

found:;
				cnt++;
				p++;
					
			}
	
out:;
			tmp[cnt] = 0;
			
			MovePenTo(BPoint(15, vp));
							
			DrawString(tmp);
	
			vp += 14;
			if (vp > 60)
				goto out1;;
		}
out1:;
	}
	else
		if (state == LARGE && full_text != 0) {
			do_large();
		}
	draw_context();
}

//--------------------------------------------------------------

void	main_view::set_match_string(char *s)
{
	strcpy(match_str, s+1);
	width = 0;
}

//--------------------------------------------------------------

void	main_view::set_desc_string(char *s)
{
	strcpy(desc_str, s);
}

//--------------------------------------------------------------

void	main_view::set_status(char *s)
{
	strcpy(status, s);
}

//--------------------------------------------------------------

char	main_view::GetState()
{
	return state;
}

//--------------------------------------------------------------

void	main_view::SetState(char s)
{
	state = s;
}

//--------------------------------------------------------------

class	xScrollBar : public BScrollBar
{
public:

		xScrollBar(	BRect frame,
					const char *name,
					BView *target,
					float min,
					float max,
					orientation direction);
					
virtual				~xScrollBar();
virtual	void		ValueChanged(float newValue);
};

//--------------------------------------------------------------

		xScrollBar::
		xScrollBar(	BRect frame,
					const char *name,
					BView *target,
					float min,
					float max,
					orientation direction)
		:
		BScrollBar(frame, name, target, min, max, direction)
{
}

//--------------------------------------------------------------

	xScrollBar::~xScrollBar()
{
}

//--------------------------------------------------------------

void	xScrollBar::ValueChanged(float newValue)
{
	main_view	*parent;
	
	parent = (main_view*)Parent();
	
	parent->RedrawFullText(newValue);
}

//--------------------------------------------------------------

void	LaunchHtml(char *path)
{
	long	result;
	
	result = be_roster->Launch("text/html", 1, &path);
	
    if ((result != B_NO_ERROR) && (result != B_ALREADY_RUNNING)) {
           beep();
           (new BAlert("", "There is no installed handler for 'text/html'.","Sorry"))->Go();
    }
}

//--------------------------------------------------------------

void	ft_view::MouseDown(BPoint where)
{

	BPoint		where0;
	ulong		but;
	BPoint		o0;
	long		dv;
	double		t1;
	double		dt;
	double		mod;
	main_view	*parent;
	
	
 
	parent = (main_view*)Parent();
	do {
		GetMouse(&where0, &but);
		dv = where0.y - where.y;
		if (dv != 0) {
			mod = fabs(dv);
			mod = sqrt(mod);
			if (mod > 2.5) mod = 2.5;
			if (but == 1) {
			parent->sb->SetValue(parent->sb->Value() + mod*-dv*2);
			}
			else {
				Parent()->Parent()->ScrollBy(0, mod*-dv*2);
			}
			Window()->UpdateIfNeeded();
			GetMouse(&where, &but);
		}
		else
			snooze(15000);
	} while(but);
	
	t1 = system_time();
	
	dt = t1 - t0;
	dt /= 1000.0;
	
	if (dt < 300) {
		((TDispWindow *)Window())->ToggleMyState(parent);
		if (parent->state == SMALL) {
			parent->RemoveChild(parent->sb);
			delete parent->sb;
			parent->RemoveChild(parent->sub_box);
			//delete parent->sub_box;
			//parent->sub_box = 0;
			parent->sb = 0;
			parent->dvp = 0;
		}
	}

	t0 = t1;	
}

//--------------------------------------------------------------

char	main_view::ContextClick(BPoint where)
{
	BRect			r;
	long			n;
	PersonMachine	*w;
	char			title[256];
	
	if (state != LARGE)
		return 0;
		
	n = where.y / 17;
	
	if (name16[n] >= 0) {
		sprintf(title, "Search <%s %s>", sub_box->person_list[name16[n]].first_name, sub_box->person_list[name16[n]].last_name);
		
		w = new PersonMachine(sub_box->person_list[name16[n]].first_name, sub_box->person_list[name16[n]].last_name, title);
		w->Lock();
		w->Show();
		w->Unlock();
		return 1;
	}

	return 0;
}

//--------------------------------------------------------------

void	main_view::MouseDown(BPoint where)
{
	BPoint	where0;
	ulong	but;
	BPoint	o0;
	long	dv;
	double	t1;
	double	dt;
	double	mod;
	float	v;
	float	d;
	
	do {
		GetMouse(&where0, &but);
		dv = where0.y - where.y;
		if (dv != 0) {
			mod = fabs(dv);
			mod = sqrt(mod);
			if (mod > 2.5) mod = 2.5;
			v = Parent()->Bounds().top + mod*-dv*2;
			d = 0;
			
			if (v > 1200)
				d = 1200-v;
			if (v < 0)
				d = -v;
			
			
			Parent()->ScrollBy(0, mod*-dv*2 + d);
			v = Parent()->Bounds().top;
			
			
			Window()->UpdateIfNeeded();
			GetMouse(&where, &but);
		}
		else
			snooze(15000);
	} while(but);
	
	t1 = system_time();
	
	dt = t1 - t0;
	dt /= 1000.0;
	
	if (dt < 300) {
		if (where.x > HBREAK)
			if (ContextClick(where))
				return;
			
		if (where.y < 23) {
			LaunchHtml(match_str);
			goto out;
		}
		
		if (state == SMALL && full_text == 0) 
			goto skip;
		((TDispWindow *)Window())->ToggleMyState(this);
		if (state == LARGE) {
					if (sub_box) {
						this->AddChild(sub_box);
						sub_box->SetViewColor(232, 232, 232);
					}
					sb = new BScrollBar(BRect(HBREAK-26, 28, HBREAK-12, LARGE_SIZE-10),
										"sb",
										sub_box,
										0,
										6000,
										B_VERTICAL);
					this->AddChild(sb);

		}
		if (state == SMALL) {
			RemoveChild(sb);
			delete sb;
			if (sub_box)
				RemoveChild(sub_box);
			sb = 0;
			dvp = 0;
		}
	}
skip:;
out:;
	t0 = t1;	
}