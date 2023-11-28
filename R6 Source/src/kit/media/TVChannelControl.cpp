#include <CheckBox.h>
#include <FindDirectory.h>
#include <Button.h>
#include <Path.h>
#include <TextControl.h>
#include <Window.h>

#include <stdio.h>

#include "TVChannelControl.h"


TVChannelControl::TVChannelControl(
	BRect frame,
	const char * name,
	const char * label,
	BMessage * model,
	uint32 resize,
	uint32 flags,
	const char * favourites_file) :
BOptionControl(frame, name, label, model, resize, flags | B_NAVIGABLE_JUMP)
{
	if (favourites_file) {
		m_favefile = strdup(favourites_file);
	}
	else {
		m_favefile = NULL;
	}
	if (m_favefile) {
		load_favourites(m_favefile);
	}
	m_curValue = 0;
	make_views();
}

TVChannelControl::~TVChannelControl()
{
	save_favourites(m_favefile);
	free(m_favefile);
}


void
TVChannelControl::Draw(
	BRect /*area*/)
{
	//	nothing
}


void
TVChannelControl::MouseDown(
	BPoint /*where*/)
{
	//	nothing
}


void
TVChannelControl::AllAttached()
{
	m_down->SetMessage(new BMessage('down'));
	m_down->SetTarget(this, Window());

	m_up->SetMessage(new BMessage('up  '));
	m_up->SetTarget(this, Window());

	m_set->SetMessage(new BMessage('set '));
	m_set->SetTarget(this, Window());

	m_channel->SetMessage(new BMessage('chan'));
	m_channel->SetTarget(this, Window());

	m_name->SetMessage(new BMessage('name'));
	m_name->SetTarget(this, Window());

	m_favourite->SetMessage(new BMessage('fave'));
	m_favourite->SetTarget(this, Window());

	m_only->SetMessage(new BMessage('only'));
	m_only->SetTarget(this, Window());
}


void
TVChannelControl::MessageReceived(
	BMessage * message)
{
	switch (message->what) {
	case 'down': {
			channel_set::iterator cur(m_channels.begin());
			while (cur != m_channels.end()) {
				if ((*cur).channel == m_curValue)
					break;
				cur++;
			}
			channel_set::iterator next(cur);
			if (next == m_channels.begin()) {
				next = m_channels.end();
			}
			next--;
			bool only = m_only->Value();
			while (next != cur) {
				if (!only || (*next).favourite) {
					break;
				}
				if (next == m_channels.begin()) {
					next = m_channels.end();
				}
				next--;
			}
			SetValue((*next).channel);
		}
		break;
	case 'up  ': {	//	add logic for favourites
			channel_set::iterator cur(m_channels.begin());
			while (cur != m_channels.end()) {
				if ((*cur).channel == m_curValue)
					break;
				cur++;
			}
			channel_set::iterator next(cur);
			next++;
			if (next == m_channels.end()) {
				next = m_channels.begin();
			}
			bool only = m_only->Value();
			while (next != cur) {
				if (!only || (*next).favourite) {
					break;
				}
				next++;
				if (next == m_channels.end()) {
					next = m_channels.begin();
				}
			}
			SetValue((*next).channel);
		}
		break;
	case 'name':
	case 'set ': {
			channel_set::iterator cur(m_channels.begin());
			while (cur != m_channels.end()) {
				if ((*cur).channel == m_curValue)
					break;
				cur++;
			}
			if (cur != m_channels.end()) {
				strncpy((*cur).name, m_name->Text(), 63);
				(*cur).name[63] = 0;
				favourite_set::iterator there(m_favourites.find(*cur));
				if (there != m_favourites.end()) {
					m_favourites.erase(there);
				}
				m_favourites.insert(*cur);
			}
		}
		break;
	case 'chan': {
			int id;
			if (1 == sscanf(m_channel->Text(), "%d", &id)) {
				SetValue(id);
			}
		}
		break;
	case 'fave': {
			channel_set::iterator cur(m_channels.begin());
			int encnt = 0;
			while (cur != m_channels.end()) {
				if ((*cur).channel == m_curValue)
					break;
				if ((*cur).favourite)
					encnt++;
				cur++;
			}
			if (cur != m_channels.end()) {
				(*cur).favourite = m_favourite->Value();
				favourite_set::iterator there(m_favourites.find(*cur));
				if (there != m_favourites.end()) {
					m_favourites.erase(there);
				}
				m_favourites.insert(*cur);
			}
			if (m_favourite->Value()) {
				m_only->SetEnabled(true);
			}
			else {
				if (encnt < 2) while (cur != m_channels.end()) {
					if ((*cur).favourite) {
						encnt++;
						if (encnt > 1) {
							break;
						}
					}
					cur++;
				}
				if (encnt > 1) {
					m_only->SetEnabled(true);
				}
				else {
					m_only->SetValue(0);
					m_only->SetEnabled(false);
				}
			}
		}
		break;
	case 'only':
		//	 nothing in particular
		break;
	default:
		BOptionControl::MessageReceived(message);
		break;
	}
}


static BRect & operator|=(BRect & a, const BRect & b)
{
	a.left = std::min(a.left, b.left);
	a.top = std::min(a.top, b.top);
	a.right = std::max(a.right, b.right);
	a.bottom = std::max(a.bottom, b.bottom);
	return a;
}

void
TVChannelControl::GetPreferredSize(
	float * width,
	float * height)
{
	BRect r = m_down->Frame();
	r |= m_up->Frame();
	r |= m_set->Frame();
	r |= m_channel->Frame();
	r |= m_name->Frame();
	r |= m_favourite->Frame();
	r |= m_only->Frame();
	r.right += r.left;
	r.bottom += r.top;
	r.left = r.top = 0;
	*width = r.right;
	*height = r.bottom;
}


void
TVChannelControl::SetValue(
	int32 value)
{
	channel_set::iterator iter(m_channels.begin());
	channel_set::iterator other(m_channels.end());
	while (iter != m_channels.end()) {
		if ((*iter).channel == m_curValue) {
			other = iter;
		}
		if ((*iter).channel == value) {
			break;
		}
		iter++;
	}
	if (iter == m_channels.end()) {
		if (other != m_channels.end()) {
			iter = other;
		}
		value = m_curValue;
	}
	else {
		m_curValue = value;
	}
	BOptionControl::SetValue(value);
	Invoke();
	char buf[24];
	sprintf(buf, "%ld", value);
	m_channel->SetText(buf);
	if (iter != m_channels.end()) {
		m_name->SetText((*iter).name);
		m_favourite->SetValue((*iter).favourite);
	}
}


status_t
TVChannelControl::AddOptionAt(
	const char * name,
	int32 value,
	int32)
{
	channel_info info;
	info.channel = value;
	strncpy(info.name, name, sizeof(info.name)-1);
	info.name[sizeof(info.name)-1] = 0;
	info.favourite = false;
	favourite_set::iterator ptr(m_favourites.find(info));
	if (ptr != m_favourites.end()) {
		info = *ptr;
	}
	if (m_channels.size() == 0) {
		m_curValue = info.channel;
	}
	m_channels.push_back(info);
	if (info.favourite) {
		m_only->SetEnabled(true);
	}
	return B_OK;
}


bool
TVChannelControl::GetOptionAt(
	int32 index,
	const char ** out_name,
	int32 * out_value)
{
	if ((index < 0) || (index >= (int32)m_channels.size())) {
		return false;
	}
	for (channel_set::iterator ptr(m_channels.begin()); ptr != m_channels.end(); ptr++) {
		if (index == 0) {
			*out_name = (*ptr).name;
			*out_value = (*ptr).channel;
			return true;
		}
		index--;
	}
	return false;
}


void
TVChannelControl::RemoveOptionAt(
	int32 index)
{
	if ((index < 0) || (index >= (int32)m_channels.size())) {
		return;
	}
	for (channel_set::iterator ptr(m_channels.begin()); ptr != m_channels.end(); ptr++) {
		if (index == 0) {
			m_channels.erase(ptr);
			return;
		}
		index--;
	}
}

int32
TVChannelControl::CountOptions() const
{
	return m_channels.size();
}

int32 
TVChannelControl::SelectedOption(const char **outName, int32 *outValue) const
{
	int32 value = Value();
	int ix = 0;
	for (channel_set::const_iterator ptr(m_channels.begin()); ptr != m_channels.end(); ptr++) {
		if ((*ptr).channel == value) {
			if (outValue) *outValue = value;
			if (outName) *outName = (*ptr).name;
			return ix;
		}
		ix++;
	}
	if (outName) *outName = 0;
	if (outValue) *outValue = 0;
	return -1;
}



void
TVChannelControl::load_favourites(
	char * name)
{
	BPath path;
	char * p;
	while ((p = strchr(name, '/')) != NULL)
		*p = '_';
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) < B_OK) {
		return;
	}
	path.Append("TV");
	mkdir(path.Path(), 0775);
	path.Append(name);
	FILE * f = fopen(path.Path(), "r");
	if (!f) return;
	fprintf(stderr, "load_favourites(%s)\n", path.Path());
	char buf[256];
	while (1) {
		buf[0] = 0;
		fgets(buf, 256, f);
		if (!buf[0]) break;
		int id;
		char name[64];
		int fave = 0;
		if (sscanf(buf, "%d '%63[^']' %d", &id, name, &fave) == 3) {
			channel_info info;
			info.channel = id;
			strcpy(info.name, name);
			info.favourite = fave;
			m_favourites.insert(info);
		}
	}
	fclose(f);
}


void
TVChannelControl::save_favourites(
	char * name)
{
	BPath path;
	char * p;
	while ((p = strchr(name, '/')) != NULL)
		*p = '_';
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) < B_OK) {
		return;
	}
	path.Append("TV");
	mkdir(path.Path(), 0775);
	path.Append(name);
	FILE * f = fopen(path.Path(), "w");
	if (!f) return;
	fprintf(stderr, "save_favourites(%s)\n", path.Path());
	for (favourite_set::iterator ptr(m_favourites.begin()); ptr != m_favourites.end(); ptr++) {
		fprintf(f, "%ld '%.63s' %d\n", (*ptr).channel, (*ptr).name, (*ptr).favourite ? 1 : 0);
	}
	fclose(f);
}


	static void
	resize(BView * view, float minx, float maxx, float miny, float maxy)
	{
		float width = floor((minx+maxx)/2), height = floor((minx+maxx)/2);
		view->GetPreferredSize(&width, &height);
		if (width < minx) width = minx;
		if (width > maxx) width = maxx;
		if (height < miny) height = miny;
		if (height > maxy) height = maxy;
		view->ResizeTo(width, height);
	}

void
TVChannelControl::make_views()
{
	//	create the controls
	m_down = new BButton(BRect(0,0,30,23), "down", "<", NULL);
	resize(m_down, 30, 30, 0, 35);
	m_channel = new BTextControl(BRect(0,0,30,17), "channel", "", "", NULL);
	m_channel->SetDivider(0);
	resize(m_channel, 30, 30, 0, 35);
	m_up = new BButton(BRect(0,0,30,23), "up", ">", NULL);
	resize(m_up, 30, 30, 0, 35);
	m_name = new BTextControl(BRect(0,0,80,17), "name", "", "", NULL);
	m_name->SetDivider(0);
	resize(m_name, 80, 80, 0, 35);
	m_set = new BButton(BRect(0,0,42,23), "set", "Set", NULL);
	resize(m_set, 42, 42, 0, 35);
	m_favourite = new BCheckBox(BRect(0,0,100,17), "favourite", "Favorite", NULL);
	m_favourite->ResizeToPreferred();
	m_only = new BCheckBox(BRect(0,0,70,17), "only", "Only", NULL);
	m_only->ResizeToPreferred();
	m_only->SetEnabled(false);

	//	and lay them out... we might want to spread them ?
	AddChild(m_down);
	m_channel->MoveTo(m_down->Frame().right+5,
			floor((m_down->Frame().bottom-m_channel->Frame().bottom)/2));
	AddChild(m_channel);
	m_up->MoveTo(m_channel->Frame().right+5, 0);
	AddChild(m_up);
	m_set->MoveTo(m_name->Frame().Width()+5, m_up->Frame().bottom+5);
	m_name->MoveTo(0, m_set->Frame().top+floor((m_set->Frame().Height()-m_name->Frame().Height())/2));
	AddChild(m_name);
	AddChild(m_set);
	m_favourite->MoveTo(0, m_set->Frame().bottom+5);
	AddChild(m_favourite);
	m_only->MoveTo(m_favourite->Frame().right+5, m_favourite->Frame().top);
	AddChild(m_only);

	//	center each row
	BRect r1 = m_down->Frame();
	r1 |= m_channel->Frame();
	r1 |= m_up->Frame();
	BRect r2 = m_name->Frame();
	r2 |= m_set->Frame();
	BRect r3 = m_favourite->Frame();
	r3 |= m_only->Frame();
	float max_r = std::max(r1.right,std::max(r2.right, r3.right));
	float d1 = floor((max_r-r1.right)/2);
	if (d1 > 0) {
		m_down->MoveBy(d1, 0);
		m_channel->MoveBy(d1, 0);
		m_up->MoveBy(d1, 0);
	}
	float d2 = floor((max_r-r2.right)/2);
	if (d2 > 0) {
		m_name->MoveBy(d2, 0);
		m_set->MoveBy(d2, 0);
	}
	float d3 = floor((max_r-r3.right)/2);
	if (d3 > 0) {
		m_favourite->MoveBy(d3, 0);
		m_only->MoveBy(d3, 0);
	}
}


