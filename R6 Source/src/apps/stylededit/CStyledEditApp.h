// ============================================================
//  CStyledEditApp.h	©1996 Hiroshi Lockheimer
// ============================================================



#include <Application.h>


//#define SOKYO_TUBWAY 1


#define MAX_BUFFER_LEN 40
const ulong	msg_WindowAdded 	= 'WAdd';
const ulong msg_WindowRemoved	= 'WRmv';
const uint32 kUTF8Conversion = 0xFFFF;


#ifdef SOKYO_TUBWAY
class TBottomlineView;
#endif


class CStyledEditApp : public BApplication {
public:
						CStyledEditApp();
	
	virtual void		AboutRequested();
	virtual void		ReadyToRun();
	virtual void		MessageReceived(BMessage *inMessage);
	virtual void		RefsReceived(BMessage *inMessage);
	virtual void		ArgvReceived(int32 argc, char **argv);
	
	bool				GetFindString(bool replace, BWindow *window);
	char				*SearchString(void);
	char				*ReplaceString(void);
	bool				SearchForward();
	bool				SearchWrap();
	bool				SearchSensitive();
	bool				ReplaceAll();


#ifdef SOKYO_TUBWAY
	static TBottomlineView*		sBottomline;
#endif


protected:
	void				MakeNewWindow();
	void				OpenWindow(entry_ref	*inRef,
								   uint32		encoding = kUTF8Conversion,
								   int32		line = 0,
								   int32 		selectionOffset = 0,
								   int32 		selectionLength = 0);

private:
	char				fSearchString[MAX_BUFFER_LEN + 1];
	char				fReplaceString[MAX_BUFFER_LEN + 1];
	bool				fSearchForward;
	bool				fSearchWrap;
	bool				fSearchSensitive;
	bool				fReplaceAll;
};	

inline	char *CStyledEditApp::SearchString(void)
	{ return fSearchString; };

inline	char *CStyledEditApp::ReplaceString(void)
	{ return fReplaceString; };

inline	bool CStyledEditApp::SearchForward(void)
	{ return fSearchForward; };

inline	bool CStyledEditApp::SearchWrap(void)
	{ return fSearchWrap; };

inline	bool CStyledEditApp::SearchSensitive(void)
	{ return fSearchSensitive; };

inline	bool CStyledEditApp::ReplaceAll(void)
	{ return fReplaceAll; };
