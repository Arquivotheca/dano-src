#include <ScreenSaver.h>
#include <StringView.h>
#include <Bitmap.h>
#include <TranslationUtils.h>
#include <DataIO.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <OS.h>
#include <Locker.h>

// return 0->max-1
inline int32 rangerand(int32 max)
{
	return (int32)(((float)rand() / (float)RAND_MAX) * (float)max);
}

#define BUF_INCREMENT	65536

bool suckdoc(const char *host, int port, const char *uri, char **outdata, size_t *outsize);
bool suckdoc(const char *host, int port, const char *uri, char **outdata, size_t *outsize)
{
	char				*data = 0;
	struct hostent		*h;
	int					s;
	struct sockaddr_in	sin;
	char buf[512];

	if(host && (h = gethostbyname(host)) != 0)
	{
		memcpy(&sin.sin_addr, h->h_addr, sizeof(sin.sin_addr));
		if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0)
		{
			memset(&sin, 0, sizeof(sin));
			sin.sin_len = sizeof(sin);
			sin.sin_port = htons(port);
			sin.sin_family = AF_INET;
			sprintf(buf, "GET %s\n\r", uri);
			if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) >= 0 &&
				write(s, buf, strlen(buf)) >= 0)
			{
				data = (char *)malloc(BUF_INCREMENT);
				int32	offs = 0;
				int32	last;

				if(data)
				{
					// suck in the whole document
					while((last = read(s, data + offs, BUF_INCREMENT)) > 0)
					{
						if(last)
						{
							offs += last;
							char *newdata = (char *)realloc(data, offs + BUF_INCREMENT);
							if(newdata)
								data = newdata;
							else
							{
								free(data);
								data = 0;
								break;
							}
						}
					}
					if(data)
						*outsize = offs + last;
				}
			}
			close(s);
		}
	}

	*outdata = data;
	return data ? true : false;
}


class Dilbert : public BScreenSaver
{
public:
				Dilbert(BMessage *message, image_id id);
	void		StartConfig(BView *view);
	status_t	StartSaver(BView *v, bool preview);
	void		StopSaver();
	void		Draw(BView *v, int32 frame);

	BBitmap		*bmp;
	BLocker		lock;
	thread_id	tid;
	bigtime_t	last;
	int			loop;
	bool		previewmode;
};

// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Dilbert(message, image);
}

Dilbert::Dilbert(BMessage *message, image_id image)
 : BScreenSaver(message, image)
{
}

void Dilbert::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 24), B_EMPTY_STRING, "DilbertZone® browser"));
	view->AddChild(new BStringView(BRect(10, 25, 200, 39), B_EMPTY_STRING, "It's better than working"));
	view->AddChild(new BStringView(BRect(10, 110, 200, 135), B_EMPTY_STRING, "(You need a GIF translator installed)"));
}
int32 loaderthread(void *arg);

status_t Dilbert::StartSaver(BView *view, bool preview)
{
	previewmode = preview;
	if(! preview)
	{
		srand(time(0L));
		SetTickSize(15 * 1000000);	// 15 sec
		view->SetViewColor(0, 0, 0);
		loop = 0;
		bmp = 0;
		resume_thread(tid = spawn_thread(loaderthread, "loader", B_LOW_PRIORITY, (void *)this));
		return B_OK;
	}
	return B_ERROR;		// no preview
}

void Dilbert::StopSaver()
{
	if(! previewmode)
	{
		if(lock.Lock())
		{
			if(tid >= 0)
				kill_thread(tid);
			delete bmp;
			bmp = 0;
			lock.Unlock();
		}
	}
}

void Dilbert::Draw(BView *view, int32)
{
	if(lock.Lock())
	{
		if(bmp)
		{
			BRect r = BScreen().Frame();
			view->FillRect(r, B_SOLID_LOW);
			view->DrawBitmap(bmp,
				BPoint(rangerand((int)((r.right - r.left) - bmp->Bounds().Width())),
					rangerand((int)((r.bottom - r.top) - bmp->Bounds().Height()))));
		}
		if((loop++ % 6) == 0 && tid < 0)
			resume_thread(tid = spawn_thread(loaderthread, "loader", B_LOW_PRIORITY, (void *)this));
		lock.Unlock();
	}
}

int32 loaderthread(void *arg)
{
	Dilbert *that = (Dilbert *)arg;
	char	*data;
	size_t	size;
	char	selecteduri[128];
	*selecteduri = 0;

	if(suckdoc("www.dilbert.com", 80, "/comics/dilbert/archive/", &data, &size))
	{
		data[size] = 0;

#define MAXURI 50
#define PREFIX "/comics/dilbert/archive/dilbert"
#define IMGPREFIX "/comics/dilbert/archive/images/dilbert"
		int uricount = 0;
		char *uri[MAXURI];
		char *p = data;
		char *end;
		while((p = strstr(p, "HREF=\"")) != 0)
		{
			p += 6;
			if((end = strchr(p, '\"')) != 0)
			{
				*end = 0;
				if(strncmp(PREFIX, p, strlen(PREFIX)) == 0 &&
					strlen(p) + 1 < sizeof(selecteduri))
				{
					uri[uricount++] = p;
					if(uricount == MAXURI)
						break;
				}
				p = end + 1;
			}
		}

		if(uricount)
		{
			strcpy(selecteduri, uri[rangerand(uricount)]);
			free(data);

			if(suckdoc("www.dilbert.com", 80, selecteduri, &data, &size))
			{
				*selecteduri = 0;
				p = data;
				while((p = strstr(p, "IMG SRC=\"")) != 0)
				{
					p += 9;
					if((end = strchr(p, '\"')) != 0)
					{
						*end = 0;
						if(strncmp(IMGPREFIX, p, strlen(IMGPREFIX)) == 0 &&
							strcmp(p + strlen(p) - 4, ".gif") == 0 &&
							strlen(p) + 1 < sizeof(selecteduri))
						{
							strcpy(selecteduri, p);
							break;
						}
						p = end + 1;
					}
				}

				free(data);
			}
			else
				*selecteduri = 0;
		}
		else
			free(data);
	}

	BBitmap *bmp = 0;

	if(*selecteduri &&
		suckdoc("www.dilbert.com", 80, selecteduri, &data, &size))
	{
		BMemoryIO *mem = new BMemoryIO(data, size);
		bmp = BTranslationUtils::GetBitmap(mem);
		delete mem;
		free(data);
	}

	if(that->lock.Lock())
	{
		if(bmp)
		{
			delete that->bmp;
			that->bmp = bmp;
		}
		that->tid = B_ERROR;
		that->lock.Unlock();
	}

	return 0;
}
