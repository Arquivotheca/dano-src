
#if !defined(TVChannelControl_h)
#define TVChannelControl_h

#include <OptionControl.h>

class BTextView;
class BButton;
class BCheckBox;

#include <set>
#include <list>


namespace BPrivate {

class TVChannelControl : public BOptionControl
{
public:

		TVChannelControl(
							BRect frame,
							const char * name,
							const char * label,
							BMessage * model,
							uint32 resize,
							uint32 flags,
							const char * favourites_file = NULL);
virtual	~TVChannelControl();


virtual	void Draw(
							BRect area);
virtual	void MouseDown(
							BPoint where);
virtual	void AllAttached();
virtual	void MessageReceived(
							BMessage * message);
virtual	void GetPreferredSize(
							float * width,
							float * height);

virtual	void SetValue(
							int32 value);

virtual	status_t AddOptionAt(
							const char * name,
							int32 value,
							int32 index);
virtual	bool GetOptionAt(
							int32 index,
							const char ** out_name,
							int32 * out_value);
virtual	void RemoveOptionAt(
							int32 index);
virtual	int32				CountOptions() const;
virtual	int32				SelectedOption(
									const char ** outName = 0,
									int32 * outValue = 0) const;
private:

		struct channel_info {
			int32 channel;
			char name[64];
			bool favourite;
			bool operator<(const channel_info & other) const
				{
					return channel < other.channel;
				}
		};
		typedef std::list<channel_info> channel_set;
		channel_set m_channels;
		typedef std::set<channel_info> favourite_set;
		favourite_set m_favourites;
		int32 m_curValue;

		BButton * m_down;
		BButton * m_up;
		BButton * m_set;
		BTextControl * m_channel;
		BTextControl * m_name;
		BCheckBox * m_favourite;
		BCheckBox * m_only;
		char * m_favefile;

		void load_favourites(
							char * name);
		void save_favourites(
							char * name);

		void make_views();

};

}	//	namespace BPrivate

using namespace BPrivate;


#endif	//	TVChannelControl_h
