#include <ScreenSaver.h>

class Message : public BScreenSaver
{
public:
	enum execStyle {
		execCommand,
		execEcho,
		execBuiltin,
		numExecStyles
	};

				Message(BMessage *message, image_id id);
	virtual void		StartConfig(BView *view);
	virtual status_t	StartSaver(BView *v, bool preview);
	virtual void		StopSaver();
	virtual status_t 	SaveState(BMessage *state) const;
	virtual void		Draw(BView *v, int32 frame);

	virtual void		SetSubtitles(bool on);
	virtual bool		GetSubtitles();
	virtual void		SetMessageString(char const * const newcmd);
	virtual char const * const	GetMessageString();
	virtual void		SetExecStyle(execStyle s);
	virtual execStyle	GetExecStyle();

protected:
	virtual void 		setup(BView *view);
	virtual void 		teardown();
	virtual char *		get_message();
	virtual void 		random_font(BView *view);
	virtual void 		step();
	virtual void 		draw(BView *view);

private:
	BView * 	m_bmview;
	BBitmap * 	m_bitmap;
	char * 		m_message;
	bool 		m_useSubtitles;
	BString 	m_messageString;
	execStyle	m_execStyle;
	// TODO dsandler: save the last fortune command and
	// text message separately
	//	BString		m_lastStrings[numExecStyles]; 
	uint32		m_sleep_msec;	// TODO: add a slider for this
};
