#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <vector>

class settingsvec {
public:
	settingsvec();
	~settingsvec();
	void push_back(void *elem);
	void *item(int index);	
	void erase();
	int size();
	void setitem(int index, void *elem);
private:
	void **m_items;
	int	m_size;
	int m_alloced;
};

settingsvec::settingsvec()
{
	m_items = 0;
	m_size = 0;
	m_alloced = 0;
}

settingsvec::~settingsvec()
{
	free(m_items);
}


void settingsvec::push_back(void *elem)
{
	void **tmp;
	
	if(m_size == m_alloced)
	{
		tmp = (void **) realloc(m_items, sizeof(void *) * (m_alloced + 100));
		if(tmp != 0)
		{
			m_items = tmp;
			m_alloced += 100;
		} else {
			return;
		}
	}
	
	m_items[m_size] = elem;
	m_size++;
	
	
}


void *settingsvec::item(int index)
{
	void *rc = 0;
	
	if(index < m_size)
		rc = m_items[index];
	
	return rc;
}

void settingsvec::setitem(int index, void *elem)
{
	if(index < m_size)
	{
		m_items[index] = elem;
	}
}


void settingsvec::erase()
{
	m_size = 0;
	m_alloced = 0;
	free(m_items);
	m_items = 0;
}


int settingsvec::size()
{
	return m_size;
}




struct SettingEntry 
{
	const char *name;
	const char *value;
};

static const char *basename = "/boot/home/config/settings/beia-";

class Settings
{
public:
	~Settings();
	
	status_t Load();
	status_t Save();
	
	int CountItems();
	
	status_t GetNthItem(int n, const char **name, const char **value);

	const char *GetItem(const char *name, const char *default_value = 0);
	void SetItem(const char *name, const char *value);	
		
	const char *Name() { return m_filename + strlen(basename); }
	
	static Settings *FindSettings(const char *domain);

	static void Lock() {acquire_sem(m_SettingsSem);}
	static void Unlock() {release_sem(m_SettingsSem);}
	
private:
	Settings(const char *name);
	 
	void Flush();
	static sem_id m_SettingsSem;
	
	const char *m_filename;
	settingsvec m_vec;
	static Settings *m_settings_list;

	Settings *m_next;
};


sem_id Settings::m_SettingsSem = B_BAD_SEM_ID;
Settings *Settings::m_settings_list = 0;



Settings *Settings::FindSettings(const char *domain)
{
	// Must be called with Lock held

	Settings *s = Settings::m_settings_list;
	
	while(s) {
		if(!strcmp(s->Name(), domain)) {
			break;
		}
		s = s->m_next;
	}
	if (! s)
		s = new Settings(domain);
	
	return s;
}

SettingEntry *MakeEntry(const char *name, const char *value)
{
	int len1 = strlen(name);
	int len2 = strlen(value);
	char *x = (char *) malloc(sizeof(SettingEntry) + len1 + len2 + 2);
					 
	SettingEntry *se = (SettingEntry *) x;
	
	x += sizeof(SettingEntry);
	memcpy(x, name, len1 + 1);
	se->name = x;
	
	x += len1 + 1;
	memcpy(x, value, len2 + 1);
	se->value = x;
	
	return se;
}

Settings::Settings(const char *name)
{
	// small race condition if twp of these are created
	// simultaneously;  may result in an extra sem
	if(m_SettingsSem == B_BAD_SEM_ID)
	{
		m_SettingsSem = create_sem(1, "settings sem");
		// The following code would work - but there's no find_sem !!
		// if m_SettingsSem != find_sem("settings sem") {
			// Whoops!  One too many sems.
			// delete_sem(m_SettingsSem);
			// m_SettingsSem = find_sem("settings sem");
		// }
		
	}
	
	m_filename = (char *) malloc(strlen(name) + strlen(basename) + 1);
	if(m_filename){
		sprintf((char*)m_filename,"%s%s",basename,name);
		Load();
	}
	
	m_next = Settings::m_settings_list;
	Settings::m_settings_list = this;
}

Settings::~Settings()
{
	Flush();
	free((char*)m_filename);
}

status_t 
Settings::Load()
{
	int fd;
	struct stat s;
	char *data, *name, *value;
	enum { NEWLINE, SKIPLINE, NAME, VALUE, QUOTE_START} state = NEWLINE;
	
	Flush();
	
	if(m_filename == NULL) goto oops;
	
	fd = open(m_filename, O_RDWR);
	if(fd < 0) goto oops;
	
	if(fstat(fd, &s)) goto oops;
	
	if((data = (char*) malloc(s.st_size + 1)) == NULL) goto oops;
	
	if(read(fd, data, s.st_size) != s.st_size) goto oops;
	
	data[s.st_size] = 0;
	
	while(*data){
		switch(state){
		case NEWLINE:
			if(*data == '#') {
				state = SKIPLINE;
			} else if(*data > ' ') {
				name = data;
				state = NAME;
			}
			break;
		
		case SKIPLINE:
			if(*data == '\n'){
				state = NEWLINE;
			}
			break;
			
		case NAME:
			if(*data == '='){
				state = QUOTE_START;
				*data = 0;
				data++;
				continue;
			}
			if(*data == '\n'){
				state = NEWLINE;
				break;
			}
			if(*data <= ' '){
				*data = 0;
				data++;
				continue;
			}
			break;
			
		case QUOTE_START:
			if(*data == '"'){
				state = VALUE;
				value = data + 1;
			}
			if(*data == '\n'){
				state = NEWLINE;
			}
			break;
		
		case VALUE:
			if(*data == '"'){
				*data = 0;
				data++;
				state = SKIPLINE;
				SettingEntry *se = MakeEntry(name,value /* ,1 */);
				
				if(se) m_vec.push_back(se);
				continue;
			}			
		}
		data++;
	}
	
	close(fd);
	return B_OK;
		
oops:
	if(fd >= 0) close(fd);
	return B_ERROR;
}

status_t 
Settings::Save()
{
	int i;
	int len = m_vec.size();
	SettingEntry *se;
	FILE *fp;
	
	if(m_filename == NULL) return B_ERROR;
	
	if((fp = fopen(m_filename,"w")) == NULL) return B_ERROR;
	
	for(i = 0; i < len; i++){
		se = (SettingEntry *) m_vec.item(i);
		fprintf(fp,"%s=\"%s\"\n",se->name,se->value);
	}
	
	fclose(fp);
	return B_OK;
}

int 
Settings::CountItems()
{
	return m_vec.size();
}

status_t
Settings::GetNthItem(int n, const char **name, const char **value)
{
	SettingEntry *se = (SettingEntry *) m_vec.item(n);
	if(se) {
		if(name) *name = (const char *) se->name;
		if(value) *value = (const char *) se->value;
		return B_OK;
	} else {
		return B_ERROR;
	}
}

const char *
Settings::GetItem(const char *name, const char *default_value)
{
	int i;
	SettingEntry *se;
	for(i = 0;(se = (SettingEntry *) m_vec.item(i)) != 0; i++){
		if(!strcmp(se->name,name)) return se->value;
	}
	return default_value;
}

void 
Settings::SetItem(const char *name, const char *value)
{
	int i;
	SettingEntry *se;
	SettingEntry *se2 = MakeEntry(name,value);
	
	if(se2 == NULL) return;
	for(i = 0;(se = (SettingEntry *) m_vec.item(i)) != 0; i++){
		if(!strcmp(se->name,name)) {
			m_vec.setitem(i, se2);
			delete se;
			return;
		}	
	}
	
	m_vec.push_back(se2);
}

void 
Settings::Flush()
{
	int i;
	int l = m_vec.size();
	for(i = 0; i < l; i++) {
		delete (SettingEntry *) m_vec.item(i);
	}
	m_vec.erase();
}


extern "C" void get_setting(const char *domain, const char *name, char *value, size_t max)
{
	max--;
	
	Settings::Lock();
	
	Settings *S = Settings::FindSettings(domain);
	const char *v = S->GetItem(name, "");
	strncpy(value, v, max);
	value[max] = 0;
	
	Settings::Unlock();
}

extern "C"  status_t get_nth_setting(const char *domain, size_t index, char *name, size_t namemax, char *value, size_t valmax)
{
	const char *nn, *nv;
	int rc = B_ERROR;
	
	valmax--;
	namemax--;
	
	Settings::Lock();

	Settings *S = Settings::FindSettings(domain);

	if(index < S->CountItems())
	{
		if(S->GetNthItem(index, &nn, &nv) == B_NO_ERROR)
		{
			strncpy(name, nn, namemax);
			strncpy(value, nv, valmax);
			value[valmax] = 0;
			name[namemax] = 0;
			rc = B_NO_ERROR;
		}
	}
	

	Settings::Unlock();
	return rc;
}


extern "C"  void set_setting(const char *domain, const char *name, const char *value)
{
	Settings::Lock();
	
	Settings *S = Settings::FindSettings(domain);
	S->SetItem(name, value);
	S->Save();

	Settings::Unlock();
}



